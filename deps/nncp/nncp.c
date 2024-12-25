/*
 * Lossless compression with Transformer
 * 
 * Copyright (c) 2018-2021 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <zlib.h>

#include "cutils.h"
#include "arith.h"
#include "libnc.h"
#include "cp_utils.h"
#include "preprocess.h"
#include "cmdopt.h"

/************************************************/
/* model description */

typedef struct NNCPModelState NNCPModelState;
typedef struct NNCPModelParams NNCPModelParams;

typedef struct {
    const char *name;
    uint8_t model_id;
    size_t instance_size;
    const CMDOptDesc *model_options;
    void (*model_parse_options)(NNCPModelParams *np, CMDOption *co);
    void (*model_write_params)(FILE *f, const NNCPModelParams *np);
    int (*model_read_params)(FILE *f, NNCPModelParams *np);
    void (*model_dump_params)(FILE *f, NNCPModelState *s,
                              const NNCPModelParams *np);

    void (*model_init)(NNCPModelState *s, const NNCPModelParams *params);
    void (*model_end)(NNCPModelState *s);
    NCTensor *(*model_eval)(NNCPModelState *s, int output_index, const NCTensor *input);
    void (*model_eval_end)(NNCPModelState *s);
    float (*model_eval_gradient)(NNCPModelState *s, const NCTensor *expected_output);
    void (*model_reset)(NNCPModelState *s);
    void (*model_update)(NNCPModelState *s);
    void (*model_set_retrain)(NNCPModelState *s, int enabled);
    void (*model_set_lr)(NNCPModelState *s, float lr);
} NNCPModelClass;

//#define DUMP_GRAD_NORM
//#define DUMP_HASH

#define LN_POST    (1 << 0)
#define LN_PRE     (1 << 1)
#define LN_FINAL   (1 << 2)
#define LN_COEF    (1 << 3)
#define LN_RMSNORM (1 << 4)

#define N_LAYER_MAX 32

typedef enum {
    FF_ACT_RELU,
    FF_ACT_GELU,
    FF_ACT_GEGLU,
    FF_ACT_GATED_SILU,
} FFActivationEnum;

typedef struct {
    int n_layer;
    int d_model;
    int n_head;
    int d_key;
    int d_value;
    int d_inner;
    int d_pos;
    int mem_len;
    int attn_len[N_LAYER_MAX];
    uint8_t tied_embed;
    uint8_t use_bias;
    uint8_t use_w_r;
    uint8_t tied_w_r;
    uint8_t tied_b_r;
    uint8_t query_bias;
    uint8_t rotary_pos_embed;
    uint8_t ln_flags;
    float init_range;
    float embed_mult;
    FFActivationEnum ff_act;
    SGDOptParams sgd_opt;
    
    float dropout_prob; /* using during retrain */
    float dropout_att_prob; /* using during retrain */
    BOOL use_sparse_grad;
} TransformerModelParams;

typedef enum {
    LSTM_TYPE_NORMAL,
    LSTM_TYPE_CLAMPED,
    LSTM_TYPE_TIED,
    LSTM_TYPE_GRU,
} LSTMTypeEnum;

typedef struct {
    int n_layers;
    int n_cells;
    int n_cells2;
    int n_embed_out;
    int n_states;
    LSTMTypeEnum lstm_type;
    BOOL use_layer_norm;
    BOOL full_connect;
    SGDOptParams sgd_opt;
    int retrain_start; /* in bytes, multiple of block_len */
    int retrain_factor; /* multiplied by 100 */
    float dropout_prob; /* using during retrain */
    float forget_bias;
    BOOL use_sparse_grad;
} LSTMParams;

struct NNCPModelParams {
    BOOL use_cuda;
    BOOL use_bf16;
    BOOL seq_eval;
    int batch_size;
    int seg_len;
    uint32_t seed;
    int n_symbols;
    
    InterpParams block_len;
    InterpParams lr;

    uint32_t retrain_period; /* in bytes, retrain happens between blocks */
    uint32_t retrain_len;
    BOOL has_retrain_lr;
    InterpParams retrain_lr;

    const NNCPModelClass *model_class;
    union {
        TransformerModelParams trf;
        LSTMParams lstm;
    } u;
};

typedef uint16_t DataSymbol;

struct NNCPModelState {
    const NNCPModelClass *model_class;
    
    NCRNDState *rnd_state;
    NCContext *model;
    NCDevice *device;
    NCDevice *cpu_device;
    
    int batch_size;
    int seg_len;
    uint32_t seed;
    int n_symbols;

    InterpParams block_len;
    InterpParams lr;

    int retrain_period;
    int retrain_buf_size; /* length of the retrain buffer */
    DataSymbol *retrain_buf;
    int retrain_pos; /* distance from the previous retrain */
    int retrain_buf_pos;
    int retrain_buf_len;
    BOOL has_retrain_lr;
    InterpParams retrain_lr;

    int64_t train_step;
    int64_t retrain_train_step;
};

/************************************************/
/* Transformer model */

typedef struct {
    NCTensor *w_q, *w_kv; /* self attention matrixes */
    NCTensor *b_q; /* query bias */
    NCTensor *w_o; /* linear output matrix */
    NCTensor *ff1, *ff2, *ff_bias1, *ff_bias2; /* feed forward layer */
    NCTensor *w_r;
    NCTensor *b_r; /* query bias for relative position */
    NCTensor *ln_g1, *ln_b1;
    NCTensor *ln_g2, *ln_b2;
    NCTensor *alpha; /* used with LN_COEF */
    NCTensor *mem_key; /* decoder only: storage for keys */
    NCTensor *mem_value; /* decoder only: storage for values */
    NCTensor *tmp_w_r; /* temporary use */
    NCTensor *tmp_b_r; /* temporary use */
    NCTensor *attn_mask; /* constant */
    NCNode **key_nodes; /* decoder only */
    NCNode **kq_nodes; /* decoder only */
    NCNode **value_nodes; /* decoder only */
    NCNode **va_nodes; /* decoder only */
} TransformerLayer;

typedef struct {
    NNCPModelState common;

    NCParamList param_list;
    int n_layer;
    int d_model;
    int n_head;
    int d_key;
    int d_value;
    int d_inner;
    int d_pos;
    int mem_len;
    int train_len;
    int n_symbols;
    uint8_t use_bias;
    uint8_t use_w_r;
    uint8_t tied_w_r;
    uint8_t tied_b_r;
    uint8_t query_bias;
    uint8_t rotary_pos_embed;
    uint8_t ln_flags;
    float embed_mult;
    TransformerLayer *layers;
    NCTensor *ln_g, *ln_b;
    NCTensor *embed, *embed_out, *out_bias, *rot_pos_embed;
    NCTensor **mem_h; /* n_layer */
    NCTensor **train_h; /* n_layer */
    NCTensor *input;
    NCTensor **outputs;
    
    int n_streams; /* rename */
    int n_states; /* same as train_len -> remove */
    NCSGDOptState *sgd_opt;
    BOOL seq_eval;
    FFActivationEnum ff_act;
    NCTypeEnum param_type; /* F32 or BF16 */
    
    BOOL dropout_enabled;
    float dropout_prob;
    float dropout_att_prob;
    BOOL use_sparse_grad;
} TransformerModel;

static void var_init(NCTensor *x, float range, NCRNDState *rnd_state)
{
    nc_tensor_set_rnd_unif(x, 0, range, rnd_state);
}

static void layer_norm_init(TransformerModel *s,
                            NCTensor **pg, NCTensor **pb,
                            size_t n, int name_idx)
{
    *pg = nc_new_tensor_1d(s->common.device, s->param_type, n);
    nc_new_param(&s->param_list, pg, "ln_g_%d", name_idx);
    nc_tensor_set_f32(*pg, 1.0f);
    *pb = nc_new_tensor_1d(s->common.device, s->param_type, n);
    nc_new_param(&s->param_list, pb, "ln_b_%d", name_idx);
}

static NCTensor *layer_norm(NCTensor *t0, NCTensor *g, NCTensor *b,
                            int flags)
{
    if (flags & LN_RMSNORM)
        t0 = nc_rms_norm(t0, 1e-5);
    else
        t0 = nc_layer_norm(t0, 1e-5);
    return nc_add(nc_mul(t0, nc_dup_tensor(g)),
                  nc_dup_tensor(b));
}

static NCTensor *dropout_mul(NCTensor *x, float prob, NCRNDState *rnd_state)
{
    NCTensor *x1;
    if (prob == 0)
        return x;
    x1 = nc_new_tensor_from_tensor_nz(x);
    nc_tensor_set_dropout(x1, prob, rnd_state);
    return nc_mul(x, x1);
}

static int nb_threads = 1;
static BOOL use_cuda;
static BOOL use_encode_only;

static void trf_init(NNCPModelState *s1, const NNCPModelParams *np)
{
    TransformerModel *s = (TransformerModel *)s1;
    const TransformerModelParams *p = &np->u.trf;
    NCContext *m = s1->model;
    NCDevice *d = s1->device;
    int layer_idx, n;
    TransformerLayer *tl;
    float init_val;
    
    s->n_layer = p->n_layer;
    s->d_model = p->d_model;
    s->n_head = p->n_head;
    s->d_key = p->d_key;
    s->d_value = p->d_value;
    s->d_inner = p->d_inner;
    s->d_pos = p->d_pos;
    s->mem_len = p->mem_len;
    s->train_len = np->seg_len;
    s->n_symbols = np->n_symbols;
    s->use_bias = p->use_bias;
    s->use_w_r = p->use_w_r;
    s->tied_w_r = p->tied_w_r;
    s->tied_b_r = p->tied_b_r;
    s->query_bias = p->query_bias;
    s->ln_flags = p->ln_flags;
    s->rotary_pos_embed = p->rotary_pos_embed;
    s->embed_mult = sqrtf(s->d_model) * p->embed_mult;
    s->ff_act = p->ff_act;
    
    s->n_streams = np->batch_size;
    s->n_states = np->seg_len;
    
    s->use_sparse_grad = p->use_sparse_grad;
    
    nc_param_list_init(m, &s->param_list);

    init_val = p->init_range / sqrtf(s->d_model);

    if (np->use_bf16)
        s->param_type = NC_TYPE_BF16;
    else
        s->param_type = NC_TYPE_F32;
        
    s->layers = nc_mallocz(sizeof(s->layers[0]) * s->n_layer);
    for(layer_idx = 0; layer_idx < s->n_layer; layer_idx++) {
        tl = &s->layers[layer_idx];

        tl->attn_mask = nc_new_tensor_2d(s->common.cpu_device, NC_TYPE_I8,
                                         s->train_len + s->mem_len,
                                         s->train_len);
        {
            int i, j, pos, v;
            for(i = 0; i < s->train_len; i++) {
                for(j = 0; j < s->mem_len + s->train_len; j++) {
                    pos = (i + s->mem_len) - j;
                    v = !(pos >= 0 && pos < p->attn_len[layer_idx]);
                    nc_set1_i32_2d(tl->attn_mask, j, i, v);
                }
            }
        }
        tl->attn_mask = nc_tensor_to_device(tl->attn_mask, d);
        
        if (!p->rotary_pos_embed) {
            if (layer_idx == 0 || !p->tied_b_r) {
                tl->b_r = nc_new_tensor_2d(d, s->param_type,
                                           s->train_len + s->mem_len, s->n_head);
                nc_new_param(&s->param_list, &tl->b_r, "b_r_%d", layer_idx);
            }
            if (p->use_w_r && (layer_idx == 0 || !p->tied_w_r)) {
                tl->w_r = nc_new_tensor_3d(d, s->param_type,
                                           s->d_key, s->d_pos, s->n_head);
                nc_new_param(&s->param_list, &tl->w_r, "w_r_%d", layer_idx);
                var_init(tl->w_r, init_val, s->common.rnd_state);
            }
        }
            
        tl->w_q = nc_new_tensor_2d(d, s->param_type, s->n_head * s->d_key,
                                   s->d_model);
        nc_new_param(&s->param_list, &tl->w_q, "w_q_%d", layer_idx);
        var_init(tl->w_q, init_val, s->common.rnd_state);

        if (s->query_bias) {
            tl->b_q = nc_new_tensor_1d(d, s->param_type, s->n_head * s->d_key);
            nc_new_param(&s->param_list, &tl->b_q, "b_q_%d", layer_idx);
        }
        
        tl->w_kv = nc_new_tensor_2d(d, s->param_type, s->n_head * s->d_key +
                                    s->n_head * s->d_value,
                                    s->d_model);
        nc_new_param(&s->param_list, &tl->w_kv, "w_kv_%d", layer_idx);
        var_init(tl->w_kv, init_val, s->common.rnd_state);

        if (s->d_value != s->d_model) {
            tl->w_o = nc_new_tensor_2d(d, s->param_type, s->d_model,
                                       s->n_head * s->d_value);
            nc_new_param(&s->param_list, &tl->w_o, "w_o_%d", layer_idx);
            var_init(tl->w_o, init_val, s->common.rnd_state);
        }
        
        if (s->ff_act == FF_ACT_GEGLU || s->ff_act == FF_ACT_GATED_SILU)
            n = s->d_inner * 2;
        else
            n = s->d_inner;
        tl->ff1 = nc_new_tensor_2d(d, s->param_type, n,
                                   s->d_model);
        nc_new_param(&s->param_list, &tl->ff1, "ff1_%d", layer_idx);
        var_init(tl->ff1, init_val, s->common.rnd_state);

        if (s->use_bias) {
            tl->ff_bias1 = nc_new_tensor_1d(d, s->param_type, n);
            nc_new_param(&s->param_list, &tl->ff_bias1, "ff_bias1_%d", layer_idx);
        }
        
        tl->ff2 = nc_new_tensor_2d(d, s->param_type, s->d_model,
                                   s->d_inner);
        nc_new_param(&s->param_list, &tl->ff2, "ff2_%d", layer_idx);
        var_init(tl->ff2, init_val *
                 sqrtf((float)s->d_model / (float)s->d_inner),
                 s->common.rnd_state);

        if (s->use_bias) {
            tl->ff_bias2 = nc_new_tensor_1d(d, s->param_type, s->d_model);
            nc_new_param(&s->param_list, &tl->ff_bias2, "ff_bias2_%d", layer_idx);
        }
        
        if (p->ln_flags & (LN_POST | LN_PRE)) {
            layer_norm_init(s, &tl->ln_g1, &tl->ln_b1, s->d_model, layer_idx * 2);
            layer_norm_init(s, &tl->ln_g2, &tl->ln_b2, s->d_model, layer_idx * 2 + 1);
        }
        if (p->ln_flags & LN_COEF) {
            tl->alpha = nc_new_scalar(d, NC_TYPE_F32);
            nc_new_param(&s->param_list, &tl->alpha, "alpha_%d", layer_idx);
            nc_tensor_set_f32(tl->alpha, 0);
        }
        if (np->seq_eval) {
            tl->mem_key =
                nc_new_tensor_4d(d, s->param_type, s->d_key,
                                 s->mem_len + s->train_len, s->n_head,
                                 s->n_streams);
            tl->mem_value =
                nc_new_tensor_4d(d, s->param_type, s->d_value,
                                 s->mem_len + s->train_len, s->n_head,
                                 s->n_streams);
            tl->key_nodes = nc_mallocz(sizeof(tl->key_nodes[0]) * (s->train_len + 1));
            tl->kq_nodes = nc_mallocz(sizeof(tl->kq_nodes[0]) * s->train_len);
            tl->value_nodes = nc_mallocz(sizeof(tl->value_nodes[0]) * (s->train_len + 1));
            tl->va_nodes = nc_mallocz(sizeof(tl->va_nodes[0]) * s->train_len);
        }
    }

    if (s->ln_flags & LN_FINAL) {
        layer_norm_init(s, &s->ln_g, &s->ln_b, s->d_model, s->n_layer * 2);
    }

    if (s->rotary_pos_embed) {
        int n, i, j;
        float th, *ptr;
        assert((s->d_key % 2) == 0);
        s->rot_pos_embed = nc_new_tensor_2d(s->common.cpu_device, NC_TYPE_F32, s->d_key,
                                            s->train_len + s->mem_len);
        n = s->d_key / 2;
        ptr = nc_tensor_get_ptr(s->rot_pos_embed, NULL);
        for(i = 0; i < s->d_key / 2; i++) {
            if (i >= n)
                th = 0;
            else
                th = 1.0f / pow(100000, (float)i / (float)n);
            //            printf("%d: th=%f\n", i, th);
            for(j = 0; j < s->train_len + s->mem_len; j++) {
                ptr[(2 * i) + j * s->d_key] = cos(th * j);
                ptr[(2 * i + 1) + j * s->d_key] = sin(th * j);
            }
        }
        s->rot_pos_embed = nc_convert(nc_tensor_to_device(s->rot_pos_embed, d), s->param_type);
    }
    
    s->embed = nc_new_tensor_2d(d, NC_TYPE_F32, s->d_model,
                                s->n_symbols);
    nc_new_param(&s->param_list, &s->embed, "embed");
    var_init(s->embed, init_val, s->common.rnd_state);

    n = s->d_model;
    if (p->tied_embed) {
        s->embed_out = NULL;
    } else {
        s->embed_out = nc_new_tensor_2d(d, s->param_type, s->n_symbols, n);
        nc_new_param(&s->param_list, &s->embed_out, "embed_out");
        var_init(s->embed_out, init_val, s->common.rnd_state);
    }
    
    if (s->use_bias) {
        s->out_bias = nc_new_tensor_1d(d, s->param_type, s->n_symbols);
        nc_new_param(&s->param_list, &s->out_bias, "out_bias");
    }
    
    s->mem_h = nc_mallocz(sizeof(s->mem_h[0]) * s->n_layer);
    s->train_h = nc_mallocz(sizeof(s->train_h[0]) * s->n_layer);
    for(layer_idx = 0; layer_idx < s->n_layer; layer_idx++) {
        s->mem_h[layer_idx] = 
            nc_new_tensor_3d(d, s->param_type, s->d_model, s->n_streams,
                             s->mem_len);
        s->train_h[layer_idx] = 
            nc_new_tensor_3d(d, s->param_type, s->d_model, s->n_streams,
                             s->train_len);
    }

    s->outputs = nc_mallocz(sizeof(s->outputs[0]) * s->train_len);

    s->sgd_opt = nc_sgd_opt_init(m, &p->sgd_opt);

    /* apply the SGD optimizer to all the parameters */
    nc_sgd_opt_set_all(&s->param_list, s->sgd_opt);

    s->dropout_prob = p->dropout_prob;
    s->dropout_att_prob = p->dropout_att_prob;
}

static void trf_end(NNCPModelState *s1)
{
    TransformerModel *s = (TransformerModel *)s1;
    int layer_idx;
    TransformerLayer *tl;

    for(layer_idx = 0; layer_idx < s->n_layer; layer_idx++) {
        tl = &s->layers[layer_idx];
        nc_free_tensor(s->mem_h[layer_idx]);
        nc_free_tensor(s->train_h[layer_idx]);
        nc_free_tensor(tl->mem_key);
        nc_free_tensor(tl->mem_value);
        nc_free_tensor(tl->attn_mask);
        nc_free(tl->key_nodes);
        nc_free(tl->kq_nodes);
        nc_free(tl->value_nodes);
        nc_free(tl->va_nodes);
    }
    nc_free_tensor(s->rot_pos_embed);
    nc_free(s->mem_h);
    nc_free(s->train_h);
    nc_free_tensor(s->input);
    nc_free(s->outputs);
    
    nc_sgd_opt_set_all(&s->param_list, NULL);
    nc_sgd_opt_end(s->sgd_opt);
    nc_param_list_end(&s->param_list);
    nc_free(s->layers);
}

/* [seg_len, batch_size, d_model] -> 
   [batch_size, n_head, seg_len, d_model/n_head] */
static NCTensor *split_head(NCTensor *x, int n_head)
{
    size_t dims[NC_N_DIMS_MAX];
    int n_dims;
    
    n_dims = nc_tensor_get_dims(x, dims);
    assert(n_dims == 3);
    assert((dims[0] % n_head) == 0);
    x = nc_reshape_4d(x, dims[0] / n_head, n_head, dims[1], dims[2]);
    /* [seg_len, batch_size, n_head, d_model/n_head] */
    return nc_permute_4d(x, 0, 3, 1, 2);
}

/* [batch_size, n_head, seg_len, d_value]
   -> [seg_len * batch_size, d_value * n_head] */
static NCTensor *concat_head(NCTensor *x)
{
    size_t dims[NC_N_DIMS_MAX];
    int n_dims;
    
    x = nc_permute_4d(x, 0, 2, 3, 1);
    n_dims = nc_tensor_get_dims(x, dims);
    assert(n_dims == 4);
    /* [seg_len, batch_size, n_head, d_value] */
    return nc_reshape_2d(x, dims[0] * dims[1], dims[2] * dims[3]);
}

static NCTensor *trf_eval(NNCPModelState *s1, int output_index, const NCTensor *input)
{
    TransformerModel *s = (TransformerModel *)s1;
    NCDevice *d = s->common.device;
    int layer_idx, seg_len;
    NCTensor *layer_input, **tab_tmp, *t0, *t1, *output;

#if defined(DUMP_HASH) && 0
    {
        NCParam *p;
        struct list_head *el;
        static int step;
        printf("step %d:\n", step);
        list_for_each(el, &s->param_list.param_list) {
            p = list_entry(el, NCParam, link);
            nc_dump_tensor_hash(p->name, *p->pval);
#if 0
            if (p->low_part)
                nc_dump_tensor_hash(" low_part", p->low_part);
#endif
        }
        step++;
    }
#endif
    
    prof_start(PROF_EVAL);

    tab_tmp = nc_mallocz(sizeof(tab_tmp[0]) *
                          max_int(max_int(2, s->train_len),
                                  max_int(s->n_head, s->n_layer)));

    if (output_index < 0) {
        t0 = nc_dup_tensor(input);
        s->seq_eval = FALSE;
        seg_len = s->train_len;
    } else {
        t0 = nc_slice_alias(input, 1, output_index, output_index + 1);
        s->seq_eval = TRUE;
        seg_len = 1;
    }
    t0 = nc_tensor_to_device(t0, d);
    
    t0 = nc_reshape_1d(t0, s->n_streams * seg_len);
    
    layer_input = nc_get_col(nc_dup_tensor(s->embed), t0);

    layer_input = nc_convert(layer_input, s->param_type);
    
    layer_input = nc_mul(layer_input, nc_convert(nc_new_f32(d, s->embed_mult),
                                                 s->param_type));
    if (s->dropout_enabled) {
        layer_input = dropout_mul(layer_input, s->dropout_prob, s->common.rnd_state);
    }
    
    for(layer_idx = 0; layer_idx < s->n_layer; layer_idx++) {
        TransformerLayer *tl = &s->layers[layer_idx];
        NCTensor *query, *key, *value, *layer_input1;
        NCTensor *ff_input, *rd;

        layer_input1 = nc_dup_tensor(layer_input);
        if (s->ln_flags & LN_PRE) {
            layer_input1 = layer_norm(layer_input1, tl->ln_g1, tl->ln_b1,
                                      s->ln_flags);
        }

        /* save the matrix input */
        if (output_index < 0) {
            t0 = nc_dup_tensor(s->train_h[layer_idx]);
        } else {
            t0 = nc_slice_alias(s->train_h[layer_idx], 2,
                                output_index, output_index + 1);
        }
        t1 = nc_reshape_3d(nc_dup_tensor(layer_input1), s->d_model,
                           s->n_streams, seg_len); 
        nc_tensor_copy(t0, t1);
        nc_free_tensor(t1);
        nc_free_tensor(t0);

        query = nc_matmul(nc_dup_tensor(tl->w_q), nc_dup_tensor(layer_input1));
        if (s->query_bias)
            query = nc_add(query, nc_dup_tensor(tl->b_q));
        t0 = nc_matmul(nc_dup_tensor(tl->w_kv), layer_input1);
        key = nc_slice(nc_dup_tensor(t0), 0, 0, s->n_head * s->d_key);
        value = nc_slice(t0, 0, s->n_head * s->d_key, s->n_head * s->d_key + s->n_head * s->d_value);

        query = nc_reshape_3d(query, s->d_key * s->n_head, s->n_streams,
                              seg_len);
        key = nc_reshape_3d(key, s->d_key * s->n_head, s->n_streams,
                            seg_len);
        value = nc_reshape_3d(value, s->d_value * s->n_head, s->n_streams,
                              seg_len);
        
        if (output_index < 0 || output_index == 0) {
            NCTensor *key0, *value0;
            /* also use the longer memory for key and value */
            t0 = nc_reshape_2d(nc_dup_tensor(s->mem_h[layer_idx]),
                               s->d_model, s->mem_len * s->n_streams);
            t0 = nc_matmul(nc_dup_tensor(tl->w_kv), t0);
            
            key0 = nc_slice(nc_dup_tensor(t0), 0, 0, s->n_head * s->d_key);
            value0 = nc_slice(t0, 0, s->n_head * s->d_key, s->n_head * s->d_key + s->n_head * s->d_value);

            key0 = nc_reshape_3d(key0, s->d_key * s->n_head, s->n_streams,
                                 s->mem_len);
            value0 = nc_reshape_3d(value0, s->d_value * s->n_head, s->n_streams,
                                   s->mem_len);

            if (output_index < 0) {
                tab_tmp[0] = key0;
                tab_tmp[1] = key;
                key = nc_concat(tab_tmp, 2, 2);
                
                tab_tmp[0] = value0;
                tab_tmp[1] = value;
                value = nc_concat(tab_tmp, 2, 2);
            } else {
                t0 = nc_slice_alias(tl->mem_key, 1, 0, s->mem_len);
                key0 = split_head(key0, s->n_head);
                tl->key_nodes[0] = nc_dup_node(nc_get_node(key0));
                nc_tensor_copy(t0, key0);
                nc_free_tensor(key0);
                nc_free_tensor(t0);

                t0 = nc_slice_alias(tl->mem_value, 1, 0, s->mem_len);
                value0 = split_head(value0, s->n_head);
                tl->value_nodes[0] = nc_dup_node(nc_get_node(value0));
                nc_tensor_copy(t0, value0);
                nc_free_tensor(value0);
                nc_free_tensor(t0);
            }
        }

        if (s->dropout_enabled) {
            query = dropout_mul(query, s->dropout_prob, s->common.rnd_state);
            /* no need to apply w_k on key */
            value = dropout_mul(value, s->dropout_prob, s->common.rnd_state);
        }
        /* split query, key and value for each head:
           [seg_len, batch_size, d_model] -> 
           [batch_size, n_head, seg_len, d_key] */
        
        key = split_head(key, s->n_head);

        query = split_head(query, s->n_head);
        
        value = split_head(value, s->n_head);
        
        if (output_index <= 0 && !s->rotary_pos_embed) {
            /* relative distance term */
            if (s->use_w_r) {
                if (layer_idx == 0 || !s->tied_w_r) {
                    NCTensor *w_r;
                    /* XXX: not efficient */
                    w_r = nc_dup_tensor(tl->w_r);
                    if (s->n_streams > 1) {
                        w_r = nc_repeat_1d(w_r, s->n_streams);
                    } else {
                        w_r = nc_reshape_4d(w_r, s->d_key, s->d_pos, s->n_head,
                                            s->n_streams);
                    }
                    tl->tmp_w_r = w_r;
                } else {
                    tl->tmp_w_r = nc_dup_tensor(s->layers[0].tmp_w_r);
                }
            }

            /* relative distance bias */
            if (layer_idx == 0 || !s->tied_b_r) {
                NCTensor *b_r;

                b_r = nc_mul(nc_dup_tensor(tl->b_r),
                             nc_convert(nc_new_f32(d, sqrtf(s->d_key * s->d_model)), s->param_type));
                b_r = nc_repeat_2d(b_r, s->train_len, s->n_streams);
                /* [batch_size, train_len, n_head, mem_len+train_len ] ->
                   [batch_size, n_head, train_len, mem_len+train_len ] 
                */
                tl->tmp_b_r = nc_permute_4d(b_r, 0, 2, 1, 3);
            } else {
                tl->tmp_b_r = nc_dup_tensor(s->layers[0].tmp_b_r);
            }
        }
        
        if (output_index < 0) {
            if (s->rotary_pos_embed) {
                NCTensor *t1;
                /* XXX: optimize */
                t1 = nc_slice_alias(s->rot_pos_embed, 1, s->mem_len, s->mem_len + s->train_len);
                query = nc_cmul(query, nc_repeat_2d(t1, s->n_head, s->n_streams), FALSE, FALSE);
                key = nc_cmul(key, nc_repeat_2d(nc_dup_tensor(s->rot_pos_embed), s->n_head, s->n_streams), FALSE, FALSE);
            }
            /* cross product term */
            t0 = nc_matmul_add(key, nc_dup_tensor(query), NULL, TRUE, FALSE, 1.0);
            if (!s->rotary_pos_embed) {
                if (s->use_w_r) {
                    rd = nc_matmul_add(nc_dup_tensor(tl->tmp_w_r),
                                       query, NULL, TRUE, FALSE, 1.0);
                    rd = nc_pad(rd, s->mem_len + s->train_len - s->d_pos,
                                NC_PAD_DUP, 0, NC_PAD_ZERO);
                    rd = nc_add(rd, nc_dup_tensor(tl->tmp_b_r));
                } else {
                    nc_free_tensor(query);
                    rd = nc_dup_tensor(tl->tmp_b_r);
                }
                rd = nc_rel_shift(rd, -(s->train_len - 1), 1);
                t0 = nc_add(rd, t0);
            } else {
                nc_free_tensor(query);
            }
            t0 = nc_mul(t0, nc_convert(nc_new_f32(d, 1.0f / sqrtf(s->d_key)),
                                       s->param_type));

            /* set the future cross products to -infinity so that they
               don't change the softmax result */
            t0 = nc_masked_fill(t0, nc_dup_tensor(tl->attn_mask), -INFINITY, 0);

            t0 = nc_soft_max(t0);
            if (s->dropout_enabled) {
                t0 = dropout_mul(t0, s->dropout_att_prob, s->common.rnd_state);
            }
            t0 = nc_matmul(value, t0);
        } else {
            int start, end;
            NCNode *node;

            /* decoder case: get all the previous keys and value_nodes */
            start = s->mem_len + output_index;
            end = start + 1;

            tl->key_nodes[output_index + 1] = nc_dup_node(nc_get_node(key));
            t0 = nc_slice_alias(tl->mem_key, 1, start, end);
            nc_tensor_copy(t0, key);
            nc_free_tensor(key);
            nc_free_tensor(t0);
            key = nc_dup_tensor(tl->mem_key);

            tl->value_nodes[output_index + 1] = nc_dup_node(nc_get_node(value));
            t0 = nc_slice_alias(tl->mem_value, 1, start, end);
            nc_tensor_copy(t0, value);
            nc_free_tensor(value);
            nc_free_tensor(t0);
            value = nc_dup_tensor(tl->mem_value);
            
            /* cross product term */
            t0 = nc_matmul_add(key, nc_dup_tensor(query), NULL, TRUE, FALSE, 1.0);
            node = nc_get_node(t0);
            /* force the storage of the argument  */
            nc_node_set_arg(node, 1, query);
            tl->kq_nodes[output_index] = node;
        
            /* relative distance term */
            /* XXX: rotary embedding are not supported yet with seq_eval */
            if (s->use_w_r) {
                rd = nc_matmul_add(nc_dup_tensor(tl->tmp_w_r), query, NULL,
                                   TRUE, FALSE, 1.0);
                rd = nc_pad(rd, s->mem_len + s->train_len - s->d_pos,
                            NC_PAD_DUP, 0, NC_PAD_ZERO);
                t1 = nc_slice(nc_dup_tensor(tl->tmp_b_r),
                              1, output_index, output_index + 1);
                rd = nc_add(rd, t1);
            } else {
                nc_free_tensor(query);
                rd = nc_slice(nc_dup_tensor(tl->tmp_b_r),
                              1, output_index, output_index + 1);
            }
            rd = nc_rel_shift(rd, -(s->train_len - 1 - output_index), 1);
            t0 = nc_add(rd, t0);
            t0 = nc_mul(t0, nc_convert(nc_new_f32(d, 1.0f / sqrtf(s->d_key)),
                                       s->param_type));

            /* set the future cross products to -infinity so that they
               don't change the softmax result */
            t1 = nc_slice_alias(tl->attn_mask,
                                1, output_index, output_index + 1);
            t0 = nc_masked_fill(t0, t1, -INFINITY, 0);
            
            t1 = nc_soft_max(t0);
            t0 = nc_matmul(value, nc_dup_tensor(t1));
            node = nc_get_node(t0);
            /* force the storage of the argument  */
            nc_node_set_arg(node, 1, t1);
            nc_free_tensor(t1);
            tl->va_nodes[output_index] = node;
        }
        
        if (tl->w_o) {
            /* merge all the heads:
               [batch_size, n_head, seg_len, d_value]
               -> [seg_len * batch_size, d_value*n_head] */
            t0 = concat_head(t0);
        
            /* linear layer */
            t0 = nc_matmul(nc_dup_tensor(tl->w_o), t0);
            if (s->dropout_enabled)
                t0 = dropout_mul(t0, s->dropout_prob, s->common.rnd_state);
        } else {
            int axis[4];
            /* just sum the output of the attention heads */
            /* [batch_size, n_head, seg_len, d_value] */
            axis[0] = 0;
            axis[1] = 3;
            axis[2] = 1;
            axis[3] = 2;
            t0 = nc_permute(t0, 4, axis);
            /* [n_head, seg_len, batch_size, d_value] */
            t0 = nc_reduce_sum(NULL, t0, 3);
            t0 = nc_mul(t0, nc_new_f32(d, 1.0 / sqrtf(s->n_head)));
            /* [seg_len, batch_size, d_value] */
            t0 = nc_reshape_2d(t0, s->d_value, s->train_len * s->n_streams);
            /* [seg_len * batch_size, d_value] */
        }

        if (s->ln_flags & LN_COEF) {
            t0 = nc_mul(t0, nc_dup_tensor(tl->alpha));
        }
        
        t0 = nc_add(t0, layer_input);
#if 0
        {
            char name[10];
            int i;
            NCTensor *t1;
            if (output_index < 0) {
                for(i = 0; i < s->train_len; i++) {
                    t1 = nc_slice_alias(t0, 1, i, i + 1);
                    snprintf(name, sizeof(name), "t%2d", i);
                    nc_dump_tensor_hash(name, t1);
                    nc_free_tensor(t1);
                }
            } else {
                snprintf(name, sizeof(name), "t%2d", output_index);
                nc_dump_tensor_hash(name, t0);
            }
        }
#endif       
#ifdef DUMP_HASH
        nc_dump_tensor_hash("attn_out_bl", t0);
#endif
        if (s->ln_flags & LN_POST) {
            t0 = layer_norm(t0, tl->ln_g1, tl->ln_b1, s->ln_flags);
        }
#ifdef DUMP_HASH
        nc_dump_tensor_hash("attn_out", t0);
#endif
        
        ff_input = nc_dup_tensor(t0);
        if (s->ln_flags & LN_PRE) {
            t0 = layer_norm(t0, tl->ln_g2, tl->ln_b2, s->ln_flags);
        }

        t0 = nc_matmul(nc_dup_tensor(tl->ff1), t0);
        if (tl->ff_bias1)
            t0 = nc_add(t0, nc_dup_tensor(tl->ff_bias1));
        if (s->dropout_enabled)
            t0 = dropout_mul(t0, s->dropout_prob, s->common.rnd_state);
#ifdef DUMP_HASH
        nc_dump_tensor_hash("ff1_out", t0);
#endif
        switch(s->ff_act) {
        case FF_ACT_RELU:
            t0 = nc_relu(t0);
            break;
        case FF_ACT_GELU:
            t0 = nc_gelu(t0);
            break;
        case FF_ACT_GEGLU:
            {
                NCTensor *tab2[2];
                nc_split(tab2, t0, 2, NULL, 0);
#if 1
                /* must keep for backward pass opt */
                t0 = nc_mul(nc_gelu(tab2[0]), tab2[1]);
#else
                t0 = nc_geglu(tab2[1], tab2[0]);
#endif
            }
            break;
        case FF_ACT_GATED_SILU:
            {
                NCTensor *tab2[2];
                nc_split(tab2, t0, 2, NULL, 0);
                t0 = nc_gated_swish(tab2[1], tab2[0], 1.0);
            }
            break;
        default:
            abort();
        }
#ifdef DUMP_HASH
        nc_dump_tensor_hash("ff2_in", t0);
#endif
        t0 = nc_matmul(nc_dup_tensor(tl->ff2), t0);
        if (tl->ff_bias2)
            t0 = nc_add(t0, nc_dup_tensor(tl->ff_bias2));
        if (s->dropout_enabled)
            t0 = dropout_mul(t0, s->dropout_prob, s->common.rnd_state);
        if (s->ln_flags & LN_COEF) {
            t0 = nc_mul(t0, nc_dup_tensor(tl->alpha));
        }
        
        t0 = nc_add(t0, ff_input);

#ifdef DUMP_HASH
        nc_dump_tensor_hash("ff_out_bl", t0);
#endif
        if (s->ln_flags & LN_POST) {
            t0 = layer_norm(t0, tl->ln_g2, tl->ln_b2, s->ln_flags);
        }
        layer_input = t0;
#ifdef DUMP_HASH
        nc_dump_tensor_hash("ff_out", t0);
#endif
    }
    
    if (s->ln_flags & LN_FINAL) {
        layer_input = layer_norm(layer_input, s->ln_g, s->ln_b, s->ln_flags);
    }

    if (s->dropout_enabled) {
        layer_input = dropout_mul(layer_input, s->dropout_prob, s->common.rnd_state);
    }

    t0 = layer_input;
    t0 = nc_matmul(nc_dup_tensor(s->embed_out), t0);
    if (s->out_bias)
        t0 = nc_add(t0, nc_dup_tensor(s->out_bias));
    t0 = nc_convert(t0, NC_TYPE_F32);
    
    t0 = nc_reshape_3d(t0, s->n_symbols, s->n_streams, seg_len);
    output = nc_soft_max(t0);
    s->outputs[max_int(output_index, 0)] = nc_dup_tensor(output);
#ifdef DUMP_HASH
    nc_dump_tensor_hash("output", s->outputs[max_int(output_index, 0)]);
#endif

    /* XXX: could free in trf_eval_gradient() */
    if (output_index < 0 || output_index == (s->train_len - 1)) {
        for(layer_idx = 0; layer_idx < s->n_layer; layer_idx++) {
            TransformerLayer *tl = &s->layers[layer_idx];
            nc_free_tensor(tl->tmp_w_r);
            tl->tmp_w_r = NULL;
            nc_free_tensor(tl->tmp_b_r);
            tl->tmp_b_r = NULL;
        }
    }

    nc_free(tab_tmp);

    prof_end(PROF_EVAL);
    return output;
}

/* called to free resources of trf_eval() if no gradient is computed */
static void trf_eval_end(NNCPModelState *s1)
{
    TransformerModel *s = (TransformerModel *)s1;
    TransformerLayer *tl;
    int layer_idx, i;
    
    for(layer_idx = 0; layer_idx < s->n_layer; layer_idx++) {
        tl = &s->layers[layer_idx];

        nc_free_tensor(tl->tmp_w_r);
        tl->tmp_w_r = NULL;
        nc_free_tensor(tl->tmp_b_r);
        tl->tmp_b_r = NULL;
        if (s->seq_eval) {
            for(i = 0; i < s->train_len + 1; i++) {
                nc_free_node(tl->key_nodes[i]);
                tl->key_nodes[i] = NULL;
            }
            for(i = 0; i < s->train_len + 1; i++) {
                nc_free_node(tl->value_nodes[i]);
                tl->value_nodes[i] = NULL;
            }
        }
    }
    for(i = 0; i < s->train_len; i++) {
        nc_free_tensor(s->outputs[i]);
        s->outputs[i] = NULL;
    }
}

static __unused void dump_param_hash(TransformerModel *s)
{
    struct list_head *el;
    static int count;
    printf("Step %d:\n", count++);
    list_for_each(el, &s->param_list.param_list) {
        NCParam *p = list_entry(el, NCParam, link);
        printf("  %12s %08x\n", p->name, nc_tensor_get_hash(*p->pval));
    }
}

#ifdef DUMP_GRAD_NORM
static double grad_sum;
static int grad_count;
#endif

static void backward_cb(void *opaque, NCTensor *yg, NCTensor *get_col_index)
{
#ifdef DUMP_GRAD_NORM
    {
        NCTensor *t0;
        int n_dims, i, n;
        const size_t *dims;
        dims = nc_tensor_get_dims(yg, &n_dims);
        n = 1;
        for(i = 0; i < n_dims; i++)
            n *= dims[i];
        grad_count += n;
        t0 = nc_sum(nc_mul(nc_dup_tensor(yg), nc_dup_tensor(yg)));
        grad_sum += nc_get_scalar_f32(t0);
        nc_free_tensor(t0);
    }
#endif
    sgd_opt_update_var(opaque, yg, get_col_index);
}

/* evaluate the gradient and return the loss (not averaged) */
static float trf_eval_gradient(NNCPModelState *s1, const NCTensor *expected_output1)
{
    TransformerModel *s = (TransformerModel *)s1;
    NCContext *m = s->common.model;
    int i, layer_idx;
    TransformerLayer *tl;
    NCTensor *loss, *expected_output, *output;
    float ret_loss;
    
    prof_start(PROF_GRAD);

    expected_output = nc_tensor_to_device(nc_dup_tensor(expected_output1), s->common.device);
    if (s->seq_eval) {
        NCNode *n, *n1, *tab_n[2], **tab_nodes;
        size_t *tab_size, tab_size2[2];
        int node_count;
        
        tab_nodes = nc_malloc(sizeof(tab_nodes[0]) * (s->n_layer * 2 + 1));
        node_count = 0;
        for(layer_idx = 0; layer_idx < s->n_layer; layer_idx++) {
            tl = &s->layers[layer_idx];

            tab_size = nc_malloc(sizeof(tab_size[0]) * s->train_len);
            for(i = 0; i < s->train_len; i++)
                tab_size[i] = 1;
            tab_size2[0] = s->mem_len;
            tab_size2[1] = s->train_len;
            
            /* factorize the 'key*t(query)' matrix multiplication */
            n = nc_concat_node(m, tl->key_nodes + 1, s->train_len, 1, tab_size);
            tab_n[0] = tl->key_nodes[0];
            tab_n[1] = n;
            n1 = nc_concat_node(m, tab_n, 2, 1, tab_size2);
            for(i = 0; i < s->train_len + 1; i++) {
                nc_free_node(tl->key_nodes[i]);
                tl->key_nodes[i] = NULL;
            }
            for(i = 0; i < s->train_len; i++)
                nc_node_set_parent(tl->kq_nodes[i], 0, n1);
            nc_free_node(n);
            nc_free_node(n1);
            tab_nodes[node_count++] = n;
            
            /* factorize the 'value*att' matrix multiplication */
            n = nc_concat_node(m, tl->value_nodes + 1, s->train_len, 1, tab_size);
            tab_n[0] = tl->value_nodes[0];
            tab_n[1] = n;
            n1 = nc_concat_node(m, tab_n, 2, 1, tab_size2);
            for(i = 0; i < s->train_len + 1; i++) {
                nc_free_node(tl->value_nodes[i]);
                tl->value_nodes[i] = NULL;
            }
            for(i = 0; i < s->train_len; i++)
                nc_node_set_parent(tl->va_nodes[i], 0, n1);
            nc_free_node(n);
            nc_free_node(n1);
            tab_nodes[node_count++] = n;

            nc_free(tab_size);
        }
        
        output = nc_concat(s->outputs, s->train_len, 2);
        for(i = 0; i < s->train_len; i++)
            s->outputs[i] = NULL; /* fail safe */
        tab_nodes[node_count++] = nc_get_node(output);
        nc_concat_optimization(m, tab_nodes, node_count);
        nc_free(tab_nodes);
    } else {
        output = s->outputs[0];
        s->outputs[0] = NULL; /* fail safe */
    }
    loss = nc_indexed_log(output, nc_dup_tensor(expected_output));
    nc_free_tensor(expected_output);
    
    loss = nc_sum(loss);
    ret_loss = nc_get_scalar_f32(loss);
    loss = nc_mul(loss, nc_new_f32(nc_get_tensor_device(loss), -1.0f / (s->train_len * s->n_streams)));

    //    nc_dump_graph(loss); exit(1);

#ifdef DUMP_GRAD_NORM
    grad_sum = 0;
    grad_count = 0;
#endif
    nc_backward(loss, nc_new_f32(s->common.device, 1.0), backward_cb,
                s->use_sparse_grad ? NC_BW_SPARSE_GRAD : 0);
#ifdef DUMP_GRAD_NORM
    printf("grad: norm=%0.2e RMS=%0.2e\n",
           sqrt(grad_sum), sqrt(grad_sum / grad_count));
#endif
    nc_free_tensor(loss);
    
    prof_end(PROF_GRAD);
    return ret_loss;
}

static void trf_reset(NNCPModelState *s1)
{
    TransformerModel *s = (TransformerModel *)s1;
    int i;
    
    for(i = 0; i < s->n_layer; i++) {
        nc_tensor_set_zero(s->mem_h[i]);
    }
}

static void mem_update(TransformerModel *s)
{
    NCTensor **mem = s->mem_h, **train = s->train_h;
    size_t i;

    for(i = 0; i < s->n_layer; i++) {
        if (s->mem_len > s->train_len) {
            nc_tensor_copy_slice(mem[i], mem[i], 2,
                                 0, s->mem_len - s->train_len, s->train_len);
            nc_tensor_copy_slice(mem[i], train[i], 2,
                                 s->mem_len - s->train_len, s->mem_len, 0);
        } else {
            nc_tensor_copy_slice(mem[i], train[i], 2,
                                 0, s->mem_len, s->train_len - s->mem_len);
        }
    }
}

/* update with coefficients */
static void trf_update(NNCPModelState *s1)
{
    TransformerModel *s = (TransformerModel *)s1;
    prof_start(PROF_UPDATE);

    nc_sgd_opt_update(s->sgd_opt);

    /* shift the memory */
    mem_update(s);
    
    prof_end(PROF_UPDATE);
}

static void trf_set_retrain(NNCPModelState *s1, BOOL enabled)
{
    TransformerModel *s = (TransformerModel *)s1;
    s->dropout_enabled = (enabled && s->dropout_prob != 0);
}

static void trf_set_lr(NNCPModelState *s1, float lr)
{
    TransformerModel *s = (TransformerModel *)s1;
    nc_sgd_opt_set_lr(s->sgd_opt, lr);
}

static void trf_dump_params(FILE *f, NNCPModelState *s1,
                            const NNCPModelParams *np)
{
    const TransformerModelParams *p = &np->u.trf;
    TransformerModel *s = (TransformerModel *)s1;
    
    char buf1[32], buf3[32];
    uint64_t n_params, n_params_nie;
    int i;
    
    fprintf(f, "n_layer=%d d_model=%d n_head=%d d_key=%d d_value=%d mem_len=%d d_pos=%d d_inner=%d tied_embed=%d init_range=%g use_bias=%d use_w_r=%d tied_w_r=%d tied_b_r=%d query_bias=%d rot_pos=%d ln_flags=%d ff_act=%d",
            p->n_layer,
            p->d_model, 
            p->n_head, 
            p->d_key, 
            p->d_value, 
            p->mem_len, 
            p->d_pos,
            p->d_inner,
            p->tied_embed,
            p->init_range,
            p->use_bias, p->use_w_r, p->tied_w_r, p->tied_b_r, p->query_bias,
            p->rotary_pos_embed,
            p->ln_flags, p->ff_act);

    fprintf(f, " attn_len=");
    for(i = 0; i < p->n_layer; i++) {
        if (i != 0)
            fprintf(f, ",");
        fprintf(f, "%d", p->attn_len[i]);
    }

    fprintf(f, " dropout=%g dropout_att=%g",
                p->dropout_prob, p->dropout_att_prob);

    dump_sgd_opt_params(f, &p->sgd_opt);

    n_params = nc_get_param_count(&s->param_list);
    /* without input embeddings */
    n_params_nie = n_params;
    if (!p->tied_embed)
        n_params_nie -= p->d_model * s->n_symbols;

    fprintf(f, " n_params=%s n_params_nie=%s\n",
            get_si_prefix(buf1, sizeof(buf1), n_params),
            get_si_prefix(buf3, sizeof(buf3), n_params_nie));
}

static void fput_interp_params(FILE *f, const InterpParams *p)
{
    int i;
    fput_u8(f, p->n_steps);
    for(i = 0; i <= p->n_steps; i++)
        fput_f32(f, p->val[i]);
    for(i = 0; i < p->n_steps; i++)
        fput_be32(f, p->pos[i]);
    fput_f32(f, p->decay_power);
}

static int fget_interp_params(FILE *f, InterpParams *p)
{
    uint8_t v8;
    uint32_t v32;
    int i;
    
    if (fget_u8(f, &v8))
        return -1;
    if (v8 > INTERP_MAX_STEPS)
        return -1;
    p->n_steps = v8;
    for(i = 0; i <= p->n_steps; i++) {
        if (fget_f32(f, &p->val[i]))
            return -1;
    }
    for(i = 0; i < p->n_steps; i++) {
        if (fget_be32(f, &v32))
            return -1;
        p->pos[i] = v32;
    }
    if (fget_f32(f, &p->decay_power))
        return -1;
    return 0;
}

static void trf_write_params(FILE *f, const NNCPModelParams *np)
{
    const TransformerModelParams *p = &np->u.trf;
    int i;
    
    fput_u8(f, p->n_layer);
    fput_u8(f, p->n_head);
    fput_be16(f, p->d_key);
    fput_be16(f, p->d_value);
    fput_be16(f, p->d_inner);
    fput_be16(f, p->d_pos);
    fput_be16(f, p->mem_len);
    for(i = 0; i < p->n_layer; i++) {
        fput_be16(f, p->attn_len[i]);
    }
    fput_u8(f, p->tied_embed);
    fput_u8(f, p->use_bias);
    fput_u8(f, p->use_w_r);
    fput_u8(f, p->tied_w_r);
    fput_u8(f, p->tied_b_r);
    fput_u8(f, p->query_bias);
    fput_u8(f, p->rotary_pos_embed);
    fput_u8(f, p->ln_flags);
    fput_f32(f, p->init_range);
    fput_f32(f, p->embed_mult);
    fput_u8(f, p->ff_act);
    fput_sgd_opt(f, &p->sgd_opt);
    if (np->retrain_period != 0) {
        fput_f32(f, p->dropout_prob);
        fput_f32(f, p->dropout_att_prob);
    }
    fput_u8(f, p->use_sparse_grad);
}

static int trf_read_params(FILE *f, NNCPModelParams *np)
{
    TransformerModelParams *p = &np->u.trf;
    uint16_t v16;
    uint8_t v8;
    int i;
    
    if (fget_u8(f, &v8))
        return -1;
    p->n_layer = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->n_head = v8;
    if (fget_be16(f, &v16))
        return -1;
    p->d_key = v16;
    p->d_model = p->d_key * p->n_head;
    if (fget_be16(f, &v16))
        return -1;
    p->d_value = v16;
    if (fget_be16(f, &v16))
        return -1;
    p->d_inner = v16;
    if (fget_be16(f, &v16))
        return -1;
    p->d_pos = v16;
    if (fget_be16(f, &v16))
        return -1;
    p->mem_len = v16;
    for(i = 0; i < p->n_layer; i++) {
        if (fget_be16(f, &v16))
            return -1;
        p->attn_len[i] = v16;
    }
    if (fget_u8(f, &v8))
        return -1;
    p->tied_embed = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->use_bias = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->use_w_r = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->tied_w_r = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->tied_b_r = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->query_bias = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->rotary_pos_embed = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->ln_flags = v8;
    if (fget_f32(f, &p->init_range))
        return -1;
    if (fget_f32(f, &p->embed_mult))
        return -1;
    if (fget_u8(f, &v8))
        return -1;
    p->ff_act = v8;

    if (fget_sgd_opt(f, &p->sgd_opt))
        return -1;

    if (np->retrain_period != 0) {
        fget_f32(f, &p->dropout_prob);
        fget_f32(f, &p->dropout_att_prob);
    }

    if (fget_u8(f, &v8))
        return -1;
    p->use_sparse_grad = v8;
    return 0;
}

static const CMDOptDesc trf_options[] = {
    { "n_layer", CMD_HAS_ARG, "number of layers" },
    { "d_model", CMD_HAS_ARG, "model dimension" },
    { "n_head", CMD_HAS_ARG, "number of attention heads" },
    { "d_key", CMD_HAS_ARG, "set the attention key dimension" },
    { "d_value", CMD_HAS_ARG, "set the attention value dimension" },
    { "mem_len", CMD_HAS_ARG, "recurrent memory length" },
    { "d_pos", CMD_HAS_ARG, "number of relative positions" },
    { "d_inner", CMD_HAS_ARG, "dimension of the feed forward layer" },
    { "query_bias", CMD_HAS_ARG, "add a query bias" },
    { "rot_pos", CMD_HAS_ARG, "rotary position embedding" },

    { "init_range", CMD_HAS_ARG, "initial range" },
    { "tied_embed", CMD_HAS_ARG, "use tied embedding" },
    { "use_bias", CMD_HAS_ARG, "use bias in matmul" },
    { "use_w_r", CMD_HAS_ARG, "use relative pos dot products" },
    { "tied_w_r", CMD_HAS_ARG, "use tied relative pos encodings" },
    { "tied_b_r", CMD_HAS_ARG, "use tied relative pos bias" },
    { "ln_flags", CMD_HAS_ARG, "layer normalisation flags" },
    { "gradient_clip", CMD_HAS_ARG, "per parameter gradient clip value" },
    { "attn_len", CMD_HAS_ARG, "per layer attention length" },
    { "embed_mult", CMD_HAS_ARG, "embedding multiplier" },
    { "retrain_dropout", CMD_HAS_ARG, "retrain dropout" },
    { "retrain_dropout_att", CMD_HAS_ARG, "retrain dropout for the attention" },

    { "ff_act", CMD_HAS_ARG, "feed forward activation: 0=RELU, 1=GELU, 2=GEGLU" },
    { "sparse_grad", CMD_HAS_ARG, "use sparse gradient update" },
    { NULL },
};

static void trf_parse_options(NNCPModelParams *np, CMDOption *co)
{
    TransformerModelParams *p = &np->u.trf;
    const char *r;
    int i;
    
    p->n_layer = cmdopt_get_int(co, "n_layer", p->n_layer);

    p->d_model = cmdopt_get_int(co, "d_model", p->d_model);
    p->n_head = cmdopt_get_int(co, "n_head", p->n_head);
    p->d_key = p->d_model / p->n_head;
    p->d_value = p->d_key;
    
    p->d_key = cmdopt_get_int(co, "d_key", p->d_key);
    p->d_value = cmdopt_get_int(co, "d_value", p->d_value);
    
    p->d_inner = cmdopt_get_int(co, "d_inner", p->d_inner);

    p->mem_len = cmdopt_get_int(co, "mem_len", p->mem_len);
    p->d_pos = cmdopt_get_int(co, "d_pos", p->d_pos);
    
    for(i = 0; i < p->n_layer; i++)
        p->attn_len[i] = p->mem_len + np->seg_len;

    p->init_range = cmdopt_get_float(co, "init_range", p->init_range);

    p->tied_embed = cmdopt_get_int(co, "tied_embed", p->tied_embed);

    p->use_bias = (cmdopt_get_int(co, "use_bias", p->use_bias) != 0);
    p->use_w_r = (cmdopt_get_int(co, "use_w_r", p->use_w_r) != 0);
    p->tied_w_r = (cmdopt_get_int(co, "tied_w_r", p->tied_w_r) != 0);
    p->tied_b_r = (cmdopt_get_int(co, "tied_b_r", p->tied_b_r) != 0);

    p->ln_flags = cmdopt_get_int(co, "ln_flags", p->ln_flags);

    p->sgd_opt.u.adam.gradient_clip =
        cmdopt_get_float(co, "gradient_clip", p->sgd_opt.u.adam.gradient_clip);
    p->embed_mult = cmdopt_get_float(co, "embed_mult", p->embed_mult);

    r = cmdopt_get(co, "attn_len");
    if (r) {
        p->attn_len[0] = strtoul(r, (char **)&r, 0);
        if (*r == '\0') {
            for(i = 1; i < p->n_layer; i++)
                p->attn_len[i] = p->attn_len[0];
        } else {
            for(i = 1; i < p->n_layer; i++) {
                skip_c(&r, ',');
                p->attn_len[i] = strtoul(r, (char **)&r, 0);
            }
        }
    }

    p->dropout_prob = cmdopt_get_float(co, "retrain_dropout",
                                       p->dropout_prob);
    p->dropout_att_prob = cmdopt_get_float(co, "retrain_dropout_att",
                                           p->dropout_att_prob);

    p->query_bias = (cmdopt_get_int(co, "query_bias", p->query_bias) != 0);

    p->rotary_pos_embed = (cmdopt_get_int(co, "rot_pos", p->rotary_pos_embed) != 0);

    p->ff_act = cmdopt_get_int(co, "ff_act", p->ff_act);

    p->use_sparse_grad = cmdopt_get_int(co, "sparse_grad", 0);
}

static NNCPModelClass trf_model = {
    "trf",
    0,
    sizeof(TransformerModel),
    trf_options,
    trf_parse_options,
    trf_write_params,
    trf_read_params,
    trf_dump_params,
    trf_init,
    trf_end,
    trf_eval,
    trf_eval_end,
    trf_eval_gradient,
    trf_reset,
    trf_update,
    trf_set_retrain,
    trf_set_lr,
};

/*********************************************************/
/* LSTM model */

typedef uint16_t DataSymbol;

#define LSTM_MAT_COUNT_MAX 4

#define LSTM_FORGET_GATE 0
#define LSTM_INPUT_NODE  1
#define LSTM_OUTPUT_GATE 2
#define LSTM_INPUT_GATE  3

#define GRU_UPDATE_GATE 0
#define GRU_RESET_GATE  1
#define GRU_OUTPUT_NODE 2

typedef struct {
    BOOL use_layer_norm;
    int mat_count;
    NCTensor *u;
    NCTensor *w;
    NCTensor *ws;
    NCTensor *b[LSTM_MAT_COUNT_MAX];
    NCTensor *g[LSTM_MAT_COUNT_MAX]; /* only for layer norm */
    NCTensor *p; /* only used for projection */
    NCTensor *c0; /* initial memory state */
    NCTensor *h0; /* initial output */
    NCTensor *c; /* current memory state */
    NCTensor *h; /* current output */
    NCNode **w_nodes;
    NCNode **ws_nodes;
} LSTMCell;

typedef struct {
    NNCPModelState common;

    BOOL seq_eval;
    int n_layers;
    int n_inputs;
    int n_outputs;
    int n_cells;
    int n_streams;
    int n_states;
    int seg_len;
    BOOL full_connect;
    int n_cells2;
    int n_embed_out;
    LSTMTypeEnum lstm_type;
    LSTMCell *lstm_layers;
    NCTensor *fc_b, *fc_w;
    NCTensor **outputs;
    NCSGDOptState *sgd_opt;
    int retrain_start; /* in symbols, multiple of block_len */
    int retrain_factor; /* multiplied by 100 */
    NCParamList param_list;
    float dropout_prob0;
    float dropout_prob;
    BOOL use_sparse_grad;
    NCTypeEnum param_type; /* F32 or BF16 */
} LSTM;

static void lstm_reset(NNCPModelState *s);
static void lstm_set_batch_size(LSTM *s, int batch_size);

static NCTensor *concat_add(NCTensor **tab, int n_in, int n_out)
{
    NCTensor **tab1, *out;
    int q, r, i, n, j, k;
    assert(n_out <= n_in);
    tab1 = nc_mallocz(sizeof(tab1[0]) * n_out);
    q = n_in / n_out;
    r = n_in % n_out;
    /* if r != 0, we add less high order inputs */
    k = 0;
    for(i = 0; i < n_out; i++) {
        n = q + (i < r);
        tab1[i] = tab[k++];
        for(j = 1; j < n; j++) {
            tab1[i] = nc_add(tab1[i], tab[k++]);
        }
    }
    assert(k == n_in);
    out = nc_vconcat(tab1, n_out);
    nc_free(tab1);
    return out;
}

static void var_init_rnd(NCTensor *x, float range, NCRNDState *rnd_state)
{
    nc_tensor_set_rnd_unif(x, 0, range, rnd_state);
}

static NCTensor *dropout2_init(LSTM *s,
                               float prob, int n_dims, const size_t *dims)
{
#if 0
    NCTensor *mult;
    NCTensorData xbuf, *x;
    size_t i, j;
    int v;
    float m;

    mult = nc_new_tensor(s->device, NC_TYPE_F32, n_dims, dims);
    x = nc_tensor_get_data(&xbuf, mult);
    m = 1.0f / (1.0f - prob);
    for(i = 0; i < x->n_strides; i++) {
        for(j = 0; j < x->dims[0]; j++) {
            v = (rnd_unif(s->rnd_state) >= prob);
            ((float *)x->data)[i * x->stride + j] = (float)v * m;
        }
    }
    return mult;
#else
    return NULL;
#endif
}

static NCTensor *dropout2_init_2d(LSTM *s,
                                  float prob, size_t d0, size_t d1)
{
    size_t dims[2];
    dims[0] = d0;
    dims[1] = d1;
    return dropout2_init(s, prob, 2, dims);
}

static NCTensor *dropout2_mul(LSTM *s, NCTensor *x, float prob)
{
    NCTensor *d;
    size_t dims[NC_N_DIMS_MAX];
    int n_dims;
    
    n_dims = nc_tensor_get_dims(x, dims);
    d = dropout2_init(s, prob, n_dims, dims);
    return nc_mul(x, d);
}

static void lstm_init(NNCPModelState *s1, const NNCPModelParams *np)
{
    LSTM *s = (LSTM *)s1;
    const LSTMParams *p = &np->u.lstm;
    int layer_idx;
    NCContext *m = s1->model;
    NCDevice *d = s1->device;
    int j;
    
    assert(np->seg_len <= p->n_states);

    s->n_states = p->n_states;
    s->seg_len = np->seg_len;
    s->n_layers = p->n_layers;
    s->n_inputs = np->n_symbols;
    s->n_outputs = np->n_symbols;
    s->n_cells = p->n_cells;
    s->lstm_type = p->lstm_type;
    s->full_connect = p->full_connect;
    s->n_embed_out = p->n_embed_out;
    s->n_cells2 = p->n_cells2;
    
    s->use_sparse_grad = p->use_sparse_grad;
    
    nc_param_list_init(m, &s->param_list);

    if (np->use_bf16)
        s->param_type = NC_TYPE_BF16;
    else
        s->param_type = NC_TYPE_F32;

    s->lstm_layers = nc_mallocz(sizeof(s->lstm_layers[0]) * s->n_layers);
    for(layer_idx = 0; layer_idx < s->n_layers; layer_idx++) {
        LSTMCell *lc;
        int n_inputs, n_sparse_inputs;
        lc = &s->lstm_layers[layer_idx];
        if (p->full_connect) {
            /* input from all previous layers */
            n_inputs = s->n_cells * layer_idx;
        } else {
            /* input from the previous layer */
            n_inputs = layer_idx != 0 ? s->n_cells : 0;
        }
        n_sparse_inputs = s->n_inputs;
        
        lc->use_layer_norm = p->use_layer_norm;
        assert(p->n_cells2 >= s->n_cells);
        if (p->lstm_type == LSTM_TYPE_TIED || p->lstm_type == LSTM_TYPE_GRU) {
            lc->mat_count = 3;
        } else {
            lc->mat_count = 4;
        }
        lc->u = nc_new_tensor_2d(d, s->param_type, p->n_cells2 * lc->mat_count,
                                 p->n_cells);
        nc_new_param(&s->param_list, &lc->u, "u%d", layer_idx);
        var_init_rnd(lc->u, 1.0f / sqrtf(p->n_cells), s->common.rnd_state);
        if (n_inputs != 0) {
            lc->w = nc_new_tensor_2d(d, s->param_type, p->n_cells2 * lc->mat_count,
                                 n_inputs);
            nc_new_param(&s->param_list, &lc->w, "w%d", layer_idx);
            var_init_rnd(lc->w, 1.0f / sqrtf(n_inputs), s->common.rnd_state);
            lc->w_nodes = nc_mallocz(sizeof(lc->w_nodes[0]) * s->seg_len);
        }
        if (n_sparse_inputs != 0) {
            lc->ws = nc_new_tensor_2d(d, NC_TYPE_F32, p->n_cells2 * lc->mat_count,
                                  n_sparse_inputs);
            nc_new_param(&s->param_list, &lc->ws, "ws%d", layer_idx);
            var_init_rnd(lc->ws, 0.75f, s->common.rnd_state);
            lc->ws_nodes = nc_mallocz(sizeof(lc->w_nodes[0]) * s->seg_len);
        }
        for(j = 0; j < lc->mat_count; j++) {
            lc->b[j] = nc_new_tensor_1d(d, s->param_type, p->n_cells2);
            nc_new_param(&s->param_list, &lc->b[j], "b%d_%d", layer_idx, j);
            if (lc->use_layer_norm) {
                lc->g[j] = nc_new_scalar(d, s->param_type);
                nc_new_param(&s->param_list, &lc->g[j], "g%d_%d", layer_idx, j);
                nc_tensor_set_f32(lc->g[j], 1.0f);
            }
        }
        nc_tensor_set_f32(lc->b[LSTM_FORGET_GATE], p->forget_bias);
        
        if (p->n_cells2 != p->n_cells) {
            lc->p = nc_new_tensor_2d(d, s->param_type, p->n_cells, p->n_cells2);
            nc_new_param(&s->param_list, &lc->p, "p%d", layer_idx);
            var_init_rnd(lc->p, sqrtf(1.0f / p->n_cells2), s->common.rnd_state);
        }
    }
    
    s->fc_b = nc_new_tensor_1d(d, s->param_type, s->n_outputs);
    nc_new_param(&s->param_list, &s->fc_b, "fc_b");
    s->fc_w = nc_new_tensor_2d(d, s->param_type, s->n_outputs, s->n_cells * p->n_embed_out);
    nc_new_param(&s->param_list, &s->fc_w, "fc_w");
    var_init_rnd(s->fc_w, sqrtf(12.0f / (s->n_cells * s->n_layers)),
                 s->common.rnd_state);
    
    s->outputs = nc_mallocz(sizeof(s->outputs[0]) * s->n_states);
        
    lstm_set_batch_size(s, np->batch_size);
    
    /* apply the SGD optimizer to all the parameters */
    s->sgd_opt = nc_sgd_opt_init(m, &p->sgd_opt);

    nc_sgd_opt_set_all(&s->param_list, s->sgd_opt);

    lstm_reset(s1);

    /* retrain parameters */
    s->retrain_start = p->retrain_start;
    s->retrain_factor = p->retrain_factor;
    s->dropout_prob0 = p->dropout_prob;
    s->dropout_prob = 0;
}

/* change the batch size */
static void lstm_set_batch_size(LSTM *s, int batch_size)
{
    NCDevice *d = s->common.device;
    int i;
    
    s->n_streams = batch_size;

    /* XXX: keep the data */
    for(i = 0; i < s->n_layers; i++) {
        LSTMCell *lc = &s->lstm_layers[i];
        nc_free_tensor(lc->h0);
        nc_free_tensor(lc->c0);
        lc->h0 = nc_new_tensor_2d(d, s->param_type, s->n_cells, s->n_streams);
        if (s->lstm_type != LSTM_TYPE_GRU) {
            lc->c0 = nc_new_tensor_2d(d, s->param_type, s->n_cells2, s->n_streams);
        }
    }
}

static void lstm_end(NNCPModelState *s1)
{
    LSTM *s = (LSTM *)s1;
    int i;

    nc_sgd_opt_set_all(&s->param_list, NULL);
    nc_sgd_opt_end(s->sgd_opt);

    nc_param_list_end(&s->param_list);

    nc_free(s->outputs);

    for(i = 0; i < s->n_layers; i++) {
        LSTMCell *lc = &s->lstm_layers[i];
        nc_free_tensor(lc->c0);
        nc_free_tensor(lc->h0);
        nc_free_tensor(lc->c);
        nc_free_tensor(lc->h);
        nc_free(lc->w_nodes);
        nc_free(lc->ws_nodes);
    }

    nc_free(s->lstm_layers);
}

static void lstm_reset(NNCPModelState *s1)
{
    LSTM *s = (LSTM *)s1;
    int i;
    for(i = 0; i < s->n_layers; i++) {
        LSTMCell *lc = &s->lstm_layers[i];
        nc_tensor_set_f32(lc->h0, 0);
        if (lc->c) {
            nc_tensor_set_f32(lc->c0, 0);
        }
    }
}

static NCTensor *lstm_eval(NNCPModelState *s1,
                           int output_index, const NCTensor *input)
{
    LSTM *s = (LSTM *)s1;
    NCDevice *d = s->common.device;
    int state_start, overlap, state_end, layer_idx;
    int state_start_out, state_idx, i, j, len;
    NCTensor *t0, *c, *h, **layer_output, **tab_tmp, **tab_tmp1, *t1;
    NCTensor *h_dropout, *output;
    prof_start(PROF_EVAL);
    
    overlap = s->n_states - s->seg_len;
    if (output_index < 0) {
        s->seq_eval = FALSE;
        state_start = 0;
        state_start_out = overlap;
        state_end = s->n_states;
    } else {
        s->seq_eval = TRUE;
        if (output_index == 0)
            state_start = 0;
        else
            state_start = overlap + output_index;
        state_start_out = overlap + output_index;
        state_end = state_start_out + 1;
        assert(s->dropout_prob == 0); /* dropout is not supported in decoder */
    }
    
    layer_output = nc_mallocz(sizeof(layer_output[0]) *
                              s->n_states * s->n_layers);
    tab_tmp = nc_mallocz(sizeof(tab_tmp[0]) *
                         max_int(LSTM_MAT_COUNT_MAX * s->n_states,
                                 s->n_layers));
    tab_tmp1 = nc_mallocz(sizeof(tab_tmp1[0]) * s->n_states);
    
    for(layer_idx = 0; layer_idx < s->n_layers; layer_idx++) {
        LSTMCell *lc = &s->lstm_layers[layer_idx];
        
        /* handle the inputs first for cache locality */
        t0 = nc_slice_alias(input, 1, state_start, state_end);
        t0 = nc_tensor_to_device(t0, d);
        t0 = nc_reshape_1d(t0, (state_end - state_start) * s->n_streams);
        t1 = nc_get_col(nc_dup_tensor(lc->ws), t0);
        if (output_index >= 0)
            lc->ws_nodes[output_index] = nc_get_node(t1);
        t1 = nc_convert(t1, s->param_type);
        if (s->dropout_prob != 0) {
            t1 = dropout2_mul(s, t1, s->dropout_prob);
        }
        
        if (layer_idx != 0) {
            for(state_idx = state_start; state_idx < state_end; state_idx++) {
                if (!s->full_connect || layer_idx == 1) {
                    t0 = nc_dup_tensor(layer_output[state_idx * s->n_layers + layer_idx - 1]);
                } else {
                    for(i = 0; i < layer_idx; i++)
                        tab_tmp[i] = nc_dup_tensor(layer_output[state_idx * s->n_layers + i]);
                    t0 = nc_vconcat(tab_tmp, layer_idx);
                }
                tab_tmp1[state_idx] = t0;
            }
            t0 = nc_hconcat(tab_tmp1 + state_start, state_end - state_start);
            if (s->dropout_prob != 0) {
                t0 = dropout2_mul(s, t0, s->dropout_prob);
            }
            t0 = nc_matmul(nc_dup_tensor(lc->w), t0);
            if (output_index >= 0)
                lc->w_nodes[output_index] = nc_get_node(t0);
            t1 = nc_add(t1, t0);
        }
        nc_split(tab_tmp + state_start, t1, state_end - state_start, NULL, 1);
            
        if (state_start == 0) {
            nc_free_tensor(lc->h);
            lc->h = nc_new_tensor_2d(d, s->param_type, s->n_cells, s->n_streams);
            nc_tensor_copy(lc->h, lc->h0);
            if (lc->c0) {
                nc_free_tensor(lc->c);
                lc->c = nc_new_tensor_2d(d, s->param_type, s->n_cells2, s->n_streams);
                nc_tensor_copy(lc->c, lc->c0);
            }
        }
        h = lc->h;
        c = lc->c;
        h_dropout = NULL;
        for(state_idx = state_start; state_idx < state_end; state_idx++) {
            NCTensor *tab[LSTM_MAT_COUNT_MAX];
            if (s->dropout_prob != 0) {
                if (!h_dropout) {
                    h_dropout = dropout2_init_2d(s, s->dropout_prob,
                                                 s->n_cells,
                                                 s->n_streams);
                }
                h = nc_mul(h, nc_dup_tensor(h_dropout));
            }
            /* XXX: GRU is no longer supported */
            t0 = nc_matmul(nc_dup_tensor(lc->u), h);
            t0 = nc_add(t0, tab_tmp[state_idx]);
            nc_split(tab, t0, lc->mat_count, NULL, 0);
            
            for(j = 0; j < lc->mat_count; j++) {
                t0 = tab[j];
                if (lc->use_layer_norm) {
                    t0 = nc_mul(nc_rms_norm(t0, 1e-5), nc_dup_tensor(lc->g[j]));
                }
                t0 = nc_add(t0, nc_dup_tensor(lc->b[j]));
                tab[j] = t0;
            }

            if (s->lstm_type == LSTM_TYPE_GRU) {
                h = nc_lerp(nc_tanh(tab[GRU_OUTPUT_NODE]), h,
                              nc_sigmoid(tab[GRU_UPDATE_GATE]));
            } else {
                NCTensor *fg, *ig, *og, *in;
                fg = nc_sigmoid(tab[LSTM_FORGET_GATE]);
                og = nc_sigmoid(tab[LSTM_OUTPUT_GATE]);
                in = nc_tanh(tab[LSTM_INPUT_NODE]);
                switch(s->lstm_type) {
                case LSTM_TYPE_NORMAL:
                    ig = nc_sigmoid(tab[LSTM_INPUT_GATE]);
                    c = nc_add(nc_mul(c, fg), nc_mul(in, ig));
                    h = nc_mul(og, nc_tanh(nc_dup_tensor(c)));
                    break;
                case LSTM_TYPE_CLAMPED:
                    ig = nc_sigmoid(tab[LSTM_INPUT_GATE]);
                    c = nc_lstm_clamped(c, in, fg, ig);
                    h = nc_mul(og, nc_dup_tensor(c));
                    break;
                case LSTM_TYPE_TIED:
                    c = nc_lerp(in, c, fg);
                    h = nc_mul(og, nc_dup_tensor(c));
                    break;
                default:
                    abort();
                }
            }
            /* optional projection */
            if (lc->p)
                h = nc_matmul(nc_dup_tensor(lc->p), h);

            nc_tensor_set_name(h, "h%d_%d", layer_idx, state_idx);
            layer_output[state_idx * s->n_layers + layer_idx] =
                nc_dup_tensor(h);
            if (state_idx == (s->seg_len - 1)) {
                nc_tensor_copy(lc->h0, h);
                if (lc->c0)
                    nc_tensor_copy(lc->c0, c);
            }
        }
        nc_free_tensor(h_dropout);
        lc->h = h;
        lc->c = c;
    }

    /* free the unused layer output */
    for(state_idx = state_start; state_idx < state_start_out; state_idx++) {
        for(layer_idx = 0; layer_idx < s->n_layers; layer_idx++) {
            nc_free_tensor(layer_output[state_idx * s->n_layers + layer_idx]);
        }
    }

    /* for efficiency, do the matrix multiplication on all the states
       at once */
    for(state_idx = state_start_out; state_idx < state_end; state_idx++) {
        t0 = concat_add(layer_output + state_idx * s->n_layers,
                        s->n_layers, s->n_embed_out);
        tab_tmp1[state_idx - state_start_out] = t0;
    }
    len = state_end - state_start_out;
    t0 = nc_hconcat(tab_tmp1, len);
    if (s->dropout_prob != 0) {
        t0 = dropout2_mul(s, t0, s->dropout_prob);
    }
    t0 = nc_matmul(nc_dup_tensor(s->fc_w), t0);
    t0 = nc_add(t0, nc_dup_tensor(s->fc_b));
    t0 = nc_convert(t0, NC_TYPE_F32);
    t0 = nc_reshape_3d(t0, s->n_outputs, s->n_streams, len);
    output = nc_soft_max(t0);
    s->outputs[state_start_out - overlap] = nc_dup_tensor(output);

    nc_free(layer_output);
    nc_free(tab_tmp);
    nc_free(tab_tmp1);
    
    prof_end(PROF_EVAL);
    return output;
}

static void lstm_eval_end(NNCPModelState *s1)
{
    LSTM *s = (LSTM *)s1;
    int i;
    for(i = 0; i < s->seg_len; i++) {
        nc_free_tensor(s->outputs[i]);
        s->outputs[i] = NULL;
    }
}

static float lstm_eval_gradient(NNCPModelState *s1, const NCTensor *expected_output)
{
    LSTM *s = (LSTM *)s1;
    NCContext *m = s->common.model;
    NCDevice *d = s->common.device;
    int layer_idx, i;
    NCTensor *loss, *output;
    float ret_loss;
    
    prof_start(PROF_GRAD);

    /* compute the loss */
    if (s->seq_eval) {
        NCNode *output_node;
        output = nc_concat(s->outputs, s->seg_len, 2);
        for(i = 0; i < s->seg_len; i++)
            s->outputs[i] = NULL;
        output_node = nc_get_node(output);
        nc_concat_optimization(m, &output_node, 1);
    } else {
        output = s->outputs[0];
        s->outputs[0] = NULL;
    }

    loss = nc_indexed_log(output, nc_tensor_to_device(nc_dup_tensor(expected_output), d));
    loss = nc_sum(loss);
    ret_loss = nc_get_scalar_f32(loss);
    loss = nc_mul(loss, nc_new_f32(d, -1.0f / (s->seg_len * s->n_streams)));

    /* in the decoder, combine the matmul and additions so that the
       result of the backward pass is the same as in the encoder. It
       also reduces the computation cost */
    if (s->seq_eval) {
        size_t *tab;
        tab = nc_malloc(sizeof(tab[0]) * s->seg_len);
        tab[0] = (s->n_states - s->seg_len + 1) * s->n_streams;
        for(i = 1; i < s->seg_len; i++)
            tab[i] = s->n_streams;
        for(layer_idx = 0; layer_idx < s->n_layers; layer_idx++) {
            LSTMCell *lc = &s->lstm_layers[layer_idx];
            if (lc->w_nodes) {
                nc_combine_nodes(m, lc->w_nodes, s->seg_len, 1, 0,
                                 tab);
            }
            nc_combine_nodes(m, lc->ws_nodes, s->seg_len, 1, 0, tab);
        }
        nc_free(tab);
    }
    //    nc_dump_graph(loss); exit(1);
    
#ifdef DUMP_GRAD_NORM
    grad_sum = 0;
    grad_count = 0;
#endif
    nc_backward(loss, nc_new_f32(d, 1.0), backward_cb,
                s->use_sparse_grad ? NC_BW_SPARSE_GRAD : 0);
    nc_free_tensor(loss);
#ifdef DUMP_GRAD_NORM
    printf("grad: norm=%0.2e RMS=%0.2e\n",
           sqrt(grad_sum), sqrt(grad_sum / grad_count));
#endif
    prof_end(PROF_GRAD);
    return ret_loss;
}

/* update with coefficients */
static void lstm_update(NNCPModelState *s1)
{
    LSTM *s = (LSTM *)s1;
    prof_start(PROF_UPDATE);
    nc_sgd_opt_update(s->sgd_opt);
    prof_end(PROF_UPDATE);
}

static void lstm_dump_params(FILE *f, NNCPModelState *s1,
                             const NNCPModelParams *np)
{
    LSTM *s = (LSTM *)s1;
    const LSTMParams *p = &np->u.lstm;
    char buf1[32], buf3[32];
    uint64_t n_params, n_params_nie;
    int mat_count;
    
    fprintf(f, "cell=");
    switch(p->lstm_type) {
    case LSTM_TYPE_CLAMPED:
        fprintf(f, "LSTM-C");
        break;
    case LSTM_TYPE_TIED:
        fprintf(f, "LSTM-T");
        break;
    case LSTM_TYPE_NORMAL:
        fprintf(f, "LSTM");
        break;
    case LSTM_TYPE_GRU:
        fprintf(f, "GRU");
        break;
    default:
        abort();
    }
    fprintf(f, " n_layer=%d hidden_size=%d time_steps=%d ln=%d fc=%d forget_bias=%0.1f",
            p->n_layers,
            p->n_cells, p->n_states, 
            p->use_layer_norm, p->full_connect, p->forget_bias);
    if (p->n_cells2 != p->n_cells)
        fprintf(f, " proj=%d", p->n_cells2);
    if (p->n_embed_out != p->n_layers)
        fprintf(f, " n_embed_out=%d", p->n_embed_out);
    if (p->retrain_start != 0) {
        fprintf(f, " dropout=%0.3g",
                p->dropout_prob);
    }
    dump_sgd_opt_params(f, &p->sgd_opt);

    n_params = nc_get_param_count(&s->param_list);
    /* without input embeddings */
    n_params_nie = n_params;
    if (p->lstm_type == LSTM_TYPE_TIED || p->lstm_type == LSTM_TYPE_GRU)
        mat_count = 3;
    else
        mat_count = 4;
    n_params_nie -= p->n_layers * mat_count * p->n_cells2 * np->n_symbols;

    fprintf(f, " n_params=%s n_params_nie=%s\n",
            get_si_prefix(buf1, sizeof(buf1), n_params),
            get_si_prefix(buf3, sizeof(buf3), n_params_nie));
}

static void lstm_write_params(FILE *f, const NNCPModelParams *np)
{
    const LSTMParams *p = &np->u.lstm;
    fput_u8(f, p->lstm_type);
    fput_u8(f, p->n_layers);
    fput_u8(f, p->n_embed_out);
    fput_be16(f, p->n_cells);
    fput_be16(f, p->n_states);
    fput_u8(f, p->use_layer_norm | (p->full_connect << 1));
    fput_sgd_opt(f, &p->sgd_opt);
    fput_f32(f, p->dropout_prob);
    fput_f32(f, p->forget_bias);
    fput_u8(f, p->use_sparse_grad);
}

static int lstm_read_params(FILE *f, NNCPModelParams *np)
{
    LSTMParams *p = &np->u.lstm;
    uint16_t v16;
    uint8_t v8;

    if (fget_u8(f, &v8))
        return -1;
    p->lstm_type = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->n_layers = v8;
    if (fget_u8(f, &v8))
        return -1;
    p->n_embed_out = v8;
    if (fget_be16(f, &v16))
        return -1;
    p->n_cells = v16;
    p->n_cells2 = p->n_cells;
    if (fget_be16(f, &v16))
        return -1;
    p->n_states = v16;
    if (fget_u8(f, &v8))
        return -1;
    p->use_layer_norm = v8 & 1;
    p->full_connect = (v8 >> 1) & 1;
    if (fget_sgd_opt(f, &p->sgd_opt))
        return -1;
    if (fget_f32(f, &p->dropout_prob))
        return -1;
    if (fget_f32(f, &p->forget_bias))
        return -1;
    if (fget_u8(f, &v8))
        return -1;
    p->use_sparse_grad = v8;
    return 0;
}

static void lstm_set_lr(NNCPModelState *s1, float lr)
{
    LSTM *s = (LSTM *)s1;
    nc_sgd_opt_set_lr(s->sgd_opt, lr);
}

static const CMDOptDesc lstm_options[] = {
    { "n_layer", CMD_HAS_ARG, "number of layers" },
    { "hidden_size", CMD_HAS_ARG, "number of LSTM hidden states", "n" },
    { "cell", CMD_HAS_ARG, "LSTM cell variant", "[lstm|lstmc|lstmt]" },
    { "full_connect", CMD_HAS_ARG, "fully connect all the layers", "[0|1]" },
    { "n_embed_out", CMD_HAS_ARG, "number of layers in output embedding", "n" },
    { "layer_norm", CMD_HAS_ARG, "enable layer normalization", "[0|1]" },
    { "adam_beta1", CMD_HAS_ARG, "ADAM beta1 parameter" },
    { "adam_beta2", CMD_HAS_ARG, "ADAM beta2 parameter" },
    { "adam_eps", CMD_HAS_ARG, "ADAM epsilon parameter" },
    { "sparse_grad", CMD_HAS_ARG, "use sparse gradient update" },
    { NULL },
};

static void lstm_parse_options(NNCPModelParams *np, CMDOption *co)
{
    LSTMParams *p = &np->u.lstm;
    const char *r;
    
    p->n_layers = cmdopt_get_int(co, "n_layer", p->n_layers);

    p->n_cells = cmdopt_get_int(co, "hidden_size", p->n_cells);
    p->n_cells2 = p->n_cells;
    
    r = cmdopt_get(co, "cell");
    if (r) {
        if (!strcmp(r, "lstm"))
            p->lstm_type = LSTM_TYPE_NORMAL;
        else if (!strcmp(r, "lstmc"))
            p->lstm_type = LSTM_TYPE_CLAMPED;
        else if (!strcmp(r, "lstmt"))
            p->lstm_type = LSTM_TYPE_TIED;
#if 0
        /* GRU is no longer supported */
        else if (!strcmp(r, "gru"))
            p->lstm_type = LSTM_TYPE_GRU;
#endif
        else {
            cmd_error("unknown cell type: %s", r);
        }
    }

    p->full_connect =
        (cmdopt_get_int(co, "full_connect", p->full_connect) != 0);
    p->use_layer_norm =
        (cmdopt_get_int(co, "layer_norm", p->use_layer_norm) != 0);
    p->sgd_opt.u.adam.beta1 =
        cmdopt_get_float(co, "adam_beta1", p->sgd_opt.u.adam.beta1);
    p->sgd_opt.u.adam.beta2 =
        cmdopt_get_float(co, "adam_beta2", p->sgd_opt.u.adam.beta2);
    p->sgd_opt.u.adam.eps =
        cmdopt_get_float(co, "adam_eps", p->sgd_opt.u.adam.eps);
    p->use_sparse_grad = cmdopt_get_int(co, "sparse_grad", 0);
}

static NNCPModelClass lstm_model = {
    "lstm",
    1,
    sizeof(LSTM),
    lstm_options,
    lstm_parse_options,
    lstm_write_params,
    lstm_read_params,
    lstm_dump_params,
    lstm_init,
    lstm_end,
    lstm_eval,
    lstm_eval_end,
    lstm_eval_gradient,
    lstm_reset,
    lstm_update,
    NULL,
    lstm_set_lr,
};

/*********************************************************/

static int stats_dump_interval = 100000;

typedef struct {
    char debug_dir[1024];
    FILE *log_file;
    FILE *plot_file;
    int64_t n_input_bytes;
    int64_t last_n_input_bytes;
    int64_t last_n_output_bytes;
    int64_t last_time;
    int64_t start_time;
    BOOL header_output;
    BOOL debug_output;
    float last_lr;
} LogState;

typedef struct {
    int64_t max_size; /* truncate (preprocessed) input to max_size symbols */
    BOOL preprocess_flag;
    int n_words;
    int min_word_freq;
    const char *dict_filename; /* if != NULL, force a dictionary and
                                  assume the input is already
                                  preprocessed */
    NNCPModelParams model_params;
} EncodeParams;

static const NNCPModelClass *nncp_models[] = {
    &trf_model,
    &lstm_model,
};

static void log_init(LogState *st, const char *debug_dir, const char *prog_name)
{
    char filename[1024];
    
    memset(st, 0, sizeof(*st));
    if (debug_dir) {
        st->debug_output = TRUE;
        create_debug_dir(st->debug_dir, sizeof(st->debug_dir),
                         debug_dir, prog_name);
        printf("[Outputing logs to '%s']\n", st->debug_dir);
        snprintf(filename, sizeof(filename), "%s/log.txt", st->debug_dir);
        st->log_file = fopen(filename, "wb");
        if (!st->log_file) {
            fprintf(stderr, "could not create '%s'\n", filename);
            exit(1);
        }
    }
    st->start_time = st->last_time = get_time_ms();
}

static void __attribute__((format(printf, 2, 3))) log_printf(LogState *st, const char *fmt, ...)
{
    va_list ap;
    char buf[4096];
    size_t len, i;
    
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    len = strlen(buf);
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
    if (st->log_file) {
        for(i = 0; i < len; i++) {
            if (buf[i] == '\r')
                buf[i] = '\n';
        }
        fwrite(buf, 1, len, st->log_file);
        fflush(st->log_file);
    }
}

static void log_end(LogState *st)
{
    if (st->log_file)
        fclose(st->log_file);
    if (st->plot_file)
        fclose(st->plot_file);
}

static void log_dump(LogState *st, int64_t n_input_bytes,
                     int64_t n_output_bytes, int64_t train_step, float lr,
                     BOOL is_end)
{
    int64_t ti;
    double bps, speed;
    int n_input_bytes1, n_output_bytes1;
    
    if (is_end) {
        /* last line is averaged over the whole file */
        ti = get_time_ms() - st->start_time;
        bps = (double)(n_output_bytes * 8) / n_input_bytes;
        speed = (double)n_input_bytes / ti;
    } else {
        n_input_bytes1 = n_input_bytes - st->last_n_input_bytes;
        n_output_bytes1 = n_output_bytes - st->last_n_output_bytes;
        ti = get_time_ms() - st->last_time;
        if (ti <= 0)
            ti = 1;
        
        bps = (double)(n_output_bytes1 * 8) / n_input_bytes1;
        speed = (double)n_input_bytes1 / ti;
    }

    if (!st->header_output) {
        st->header_output = TRUE;
        log_printf(st, "%1s %8s %10s %10s %6s %6s %8s\n",
                   "M", "STEP", "SIZE", "CSIZE", "BPS", "kS/s", "LR");
    }
    log_printf(st, "  %8" PRIu64 " %10" PRIu64 " %10" PRIu64 " %6.3f %6.2f %8.2e%c",
               train_step,
               n_input_bytes,
               n_output_bytes,
               bps,
               speed,
               lr,
               is_end ? '\n' : '\r');
    if (st->plot_file) {
        fprintf(st->plot_file, "  %10" PRIu64 " %10" PRIu64 "\n",
                n_input_bytes,
                n_output_bytes);
        fflush(st->plot_file);
    }
    st->last_time = get_time_ms();
    st->last_n_output_bytes = n_output_bytes;
    st->last_n_input_bytes = n_input_bytes;
}

static void nncp_init_params(NNCPModelParams *np, const char *profile)
{
    int i;
    
    memset(np, 0, sizeof(*np));
    np->seed = 123;

    if (!strcmp(profile, "default")) {
        TransformerModelParams *p;

        np->block_len.n_steps = 0;
        np->block_len.val[0] = 500000;
        
        np->batch_size = 16;
        np->seg_len = 32;
        np->n_symbols = 256;
        
        np->lr.n_steps = 1;
        np->lr.val[0] = 3e-4;
        np->lr.val[1] = 1e-4;
        np->lr.pos[0] = 5e6 / 32;
        
        np->model_class = &trf_model;
        p = &np->u.trf;
        
        p->n_layer = 4;
        p->d_model = 256;
        p->n_head = 8;
        p->d_key = p->d_model / p->n_head;
        p->d_value = p->d_key;
        p->d_inner = p->d_model * 2;
        p->d_pos = 32;
        p->mem_len = 32;
        for(i = 0; i < p->n_layer; i++)
            p->attn_len[i] = p->mem_len + np->seg_len;
        p->tied_embed = 0;
        p->init_range = 1.0;
        p->use_bias = 1;
        p->use_w_r = 1;
        p->tied_w_r = 1;
        p->tied_b_r = 1;
        p->ln_flags = LN_POST;
        p->embed_mult = 1.0;
        p->ff_act = FF_ACT_GELU;
        
        p->sgd_opt.algo = SGD_OPT_ADAM;
        p->sgd_opt.u.adam.beta1 = 0;
        p->sgd_opt.u.adam.beta2 = 0.9999;
        p->sgd_opt.u.adam.eps = 1e-8;
        p->sgd_opt.u.adam.gradient_clip = 0.1;

    } else if (!strcmp(profile, "enwik9") ||
               !strcmp(profile, "enwik8")) {
        TransformerModelParams *p;
        BOOL is_enwik8 = !strcmp(profile, "enwik8");
        
        np->batch_size = 32;
        np->seg_len = 64;
        np->n_symbols = 256; /* will be modified by the preprocessor */
        
        if (is_enwik8) {
            parse_interp_param(&np->block_len, "100000,500000,100000,500000,500000");
        } else {
            parse_interp_param(&np->block_len, "500000");
        }
        parse_interp_param(&np->lr, "1.6e-4,10000,1.0e-4,p0.5");
        
        if (is_enwik8) {
            np->retrain_len = 15000000;
        } else {
            np->retrain_len = 7500000;
        }
        np->retrain_period = 1;
        
        np->has_retrain_lr = TRUE;
        parse_interp_param(&np->retrain_lr, "1.6e-4,10000,1.0e-4,p0.5");
        
        np->use_bf16 = 1;
        
        np->model_class = &trf_model;
        p = &np->u.trf;
        
        p->n_layer = 20;
        p->d_model = 1024;
        p->n_head = 8;
        p->d_key = p->d_model / p->n_head;
        p->d_value = p->d_key;
        p->d_inner = 3072;
        p->ff_act = FF_ACT_GEGLU;
        p->d_pos = 320;
        p->mem_len = 256;
        for(i = 0; i < p->n_layer; i++)
            p->attn_len[i] = p->mem_len + np->seg_len;
        p->tied_embed = 0;
        p->init_range = 0.79;
        p->use_bias = 1;
        p->use_w_r = 1;
        p->tied_w_r = 0;
        p->tied_b_r = 1;
        p->ln_flags = LN_PRE | LN_FINAL | LN_RMSNORM;
        p->embed_mult = 1.0;
        p->dropout_prob = 0.19;
        p->dropout_att_prob = 0.19;
        
        p->sgd_opt.algo = SGD_OPT_ADAM;
        p->sgd_opt.u.adam.beta1 = 0;
        p->sgd_opt.u.adam.beta2 = 0.9999;
        p->sgd_opt.u.adam.eps = 1e-8;
        p->sgd_opt.u.adam.gradient_clip = 0.05;

    } else if (!strcmp(profile, "lstm")) {
        LSTMParams *p;

        np->block_len.n_steps = 0;
        np->block_len.val[0] = 500000;
        
        np->batch_size = 32;
        np->seg_len = 20;
        np->n_symbols = 256;

        np->lr.n_steps = 0;
        np->lr.val[0] = 4e-3;

        np->model_class = &lstm_model;
        p = &np->u.lstm;
        p->use_layer_norm = TRUE;
        p->full_connect = TRUE;
        p->lstm_type = LSTM_TYPE_CLAMPED;
        p->n_cells = 352;
        p->n_layers = 4;
        p->n_embed_out = p->n_layers;
        p->n_states = np->seg_len;
        p->n_cells2 = p->n_cells;
        p->forget_bias = 0;
    
        p->sgd_opt.algo = SGD_OPT_ADAM;
        p->sgd_opt.u.adam.beta1 = 0;
        p->sgd_opt.u.adam.beta2 = 0.9999;
        p->sgd_opt.u.adam.eps = 1e-10;
        p->sgd_opt.u.adam.gradient_clip = 0;
        
    } else if (!strcmp(profile, "lstm_fast")) {
        LSTMParams *p;

        np->block_len.n_steps = 0;
        np->block_len.val[0] = 100000000;
        
        np->batch_size = 256;
        np->seg_len = 20;
        np->n_symbols = 256;

        np->lr.n_steps = 0;
        np->lr.val[0] = 1e-2;

        np->model_class = &lstm_model;
        p = &np->u.lstm;
        p->use_layer_norm = TRUE;
        p->full_connect = TRUE;
        p->lstm_type = LSTM_TYPE_CLAMPED;
        p->n_cells = 512;
        p->n_layers = 4;
        p->n_embed_out = p->n_layers;
        p->n_states = np->seg_len;
        p->n_cells2 = p->n_cells;
        p->forget_bias = 0;
    
        p->sgd_opt.algo = SGD_OPT_ADAM;
        p->sgd_opt.u.adam.beta1 = 0;
        p->sgd_opt.u.adam.beta2 = 0.9999;
        p->sgd_opt.u.adam.eps = 1e-10;
        p->sgd_opt.u.adam.gradient_clip = 0;
        
    } else {
        cmd_error("unknown profile: %s\n", profile);
    }
}

static void nncp_dump_params(FILE *f, NNCPModelState *s, const NNCPModelParams *p)
{
    fprintf(f, "model=%s bf16=%d batch_size=%d seg_len=%d n_symb=%d",
            p->model_class->name, p->use_bf16,
            p->batch_size, p->seg_len, p->n_symbols);
    fprintf(f, " block_len=");
    dump_interp_param(f, &p->block_len);
    fprintf(f, " lr=");
    dump_interp_param(f, &p->lr);
    if (p->retrain_period != 0) {
        fprintf(f, " retrain_period=%d retrain_len=%d",
                p->retrain_period, p->retrain_len);
        if (p->has_retrain_lr) {
            fprintf(f, " retrain_lr=");
            dump_interp_param(f, &p->retrain_lr);
        }
    }
    fprintf(f, " ");
    p->model_class->model_dump_params(f, s, p);
}

#define NNCP_FILE_MAGIC   0xb727ac58
#define NNCP_FILE_VERSION 1

static void nncp_write_file_header(FILE *f, const NNCPModelParams *p)
{
    fput_be32(f, NNCP_FILE_MAGIC);
    fput_be16(f, NNCP_FILE_VERSION);
    fput_u8(f, p->use_cuda);
    fput_u8(f, p->use_bf16);
    
    fput_be16(f, p->batch_size);
    fput_be16(f, p->seg_len);
    fput_be16(f, p->n_symbols);
    fput_be32(f, p->seed);

    fput_interp_params(f, &p->lr);
    fput_interp_params(f, &p->block_len);
    fput_be32(f, p->retrain_period);
    if (p->retrain_period != 0) {
        fput_be32(f, p->retrain_len);
        fput_u8(f, p->has_retrain_lr);
        if (p->has_retrain_lr) {
            fput_interp_params(f, &p->retrain_lr);
        }
    }

    fput_u8(f, p->model_class->model_id);
    p->model_class->model_write_params(f, p);
}

static int nncp_read_file_header(FILE *f, NNCPModelParams *p)
{
    uint32_t v32;
    uint16_t v16;
    uint8_t v8;
    int i;
    
    if (fget_be32(f, &v32))
        return -1;
    if (v32 != NNCP_FILE_MAGIC)
        return -1;
    if (fget_be16(f, &v16))
        return -1;
    if (v16 != NNCP_FILE_VERSION)
        return -1;
    if (fget_u8(f, &v8))
        return -1;
    p->use_cuda = v8;

    if (fget_u8(f, &v8))
        return -1;
    p->use_bf16 = v8;

    if (fget_be16(f, &v16))
        return -1;
    p->batch_size = v16;

    if (fget_be16(f, &v16))
        return -1;
    p->seg_len = v16;
    
    if (fget_be16(f, &v16))
        return -1;
    p->n_symbols = v16;

    if (fget_be32(f, &p->seed))
        return -1;

    if (fget_interp_params(f, &p->lr))
        return -1;
    
    if (fget_interp_params(f, &p->block_len))
        return -1;

    fget_be32(f, &p->retrain_period);
    if (p->retrain_period != 0) {
        fget_be32(f, &p->retrain_len);
        if (fget_u8(f, &v8))
            return -1;
        p->has_retrain_lr = v8;
        if (p->has_retrain_lr) {
            if (fget_interp_params(f, &p->retrain_lr))
                return -1;
        }
    }

    /* model_id */
    if (fget_u8(f, &v8))
        return -1;
    for(i = 0; i < countof(nncp_models); i++) {
        if (nncp_models[i]->model_id == v8)
            break;
    }
    if (i == countof(nncp_models))
        return -1;
    p->model_class = nncp_models[i];
    
    return p->model_class->model_read_params(f, p);
}

static NNCPModelState *nncp_init(NNCPModelParams *p)
{
    NNCPModelState *s;
    NCContext *m;
    NCDevice *d;
    
    s = nc_mallocz(p->model_class->instance_size);

    m = nc_context_init(nb_threads);
    s->model = m;
    s->cpu_device = nc_new_cpu_device(m);
    if (p->use_cuda)
        d = nc_new_cuda_device(m, 0);
    else
        d = s->cpu_device;
    s->device = d;

    s->rnd_state = nc_rnd_init(s->device, p->seed);

    s->model_class = p->model_class;
    s->batch_size = p->batch_size;
    s->seg_len = p->seg_len;
    s->seed = p->seed;
    s->n_symbols = p->n_symbols;
    
    s->lr = p->lr;
    s->retrain_period = p->retrain_period;
    s->retrain_buf_size = p->retrain_len;

    s->has_retrain_lr = p->has_retrain_lr;
    s->retrain_lr = p->retrain_lr;
    s->retrain_buf = nc_mallocz(sizeof(s->retrain_buf[0]) * s->retrain_buf_size);
    s->retrain_buf_pos = 0;
    s->retrain_buf_len = 0;
    s->retrain_pos = 0;

    s->model_class->model_init(s, p);
    return s;
}

static void nncp_end(NNCPModelState *s)
{
    s->model_class->model_end(s);
    nc_rnd_end(s->rnd_state);
    nc_context_end(s->model);
    nc_free(s->retrain_buf);
    nc_free(s);
}

static int get_symb_fifo(const DataSymbol *block_buf, int rpos, int size,
                         int idx)
{
    if (idx < 0) {
        return 0;
    } else {
        return block_buf[(rpos + idx) % size];
    }
}

static void retrain_block(NNCPModelState *s,
                          const DataSymbol *block_buf, int block_len,
                          int64_t file_pos, LogState *st)
{
    int l, pos, cur_state, c, stream_idx;
    int n_streams, n_states, seg_len, overlap, block_stride, block_idx;
    int64_t train_bytes, last_train_bytes;
    NCTensor *expected_output, *input, *output;
    double n_output_bits, last_n_output_bits;
    int64_t last_time;
    float lr, inv_log2;
    
    if (s->retrain_period == 0)
        return; /* no retrain */
    
    /* add the block to the retrain buffer */
    pos = 0;
    while (pos < block_len) {
        l = min_int(s->retrain_buf_size - s->retrain_buf_pos, block_len - pos);
        memcpy(s->retrain_buf + s->retrain_buf_pos, block_buf + pos,
               sizeof(block_buf[0]) * l);
        s->retrain_buf_pos += l;
        if (s->retrain_buf_pos == s->retrain_buf_size)
            s->retrain_buf_pos = 0;
        pos += l;
    }
    s->retrain_buf_len = min_int(s->retrain_buf_len + block_len,
                                 s->retrain_buf_size);
    s->retrain_pos += block_len;
    if (s->retrain_pos < s->retrain_period)
        return;
    s->retrain_pos = 0;
    
    n_streams = s->batch_size;
    n_states = s->seg_len;
    seg_len = s->seg_len;
    overlap = 0;
    block_stride = s->retrain_buf_len / n_streams;
    if (block_stride == 0)
        return;
    
    /* statistics */
    train_bytes = file_pos - s->retrain_buf_len;
    last_train_bytes = train_bytes;
    n_output_bits = 0;
    last_n_output_bits = 0;
    inv_log2 = 1.0 / log(2);
    last_time = get_time_ms();
    
    block_idx = 0;
    
    s->model_class->model_set_retrain(s, TRUE);

    input = nc_new_tensor_2d(s->cpu_device, NC_TYPE_I32,
                             n_streams, n_states);
    expected_output = nc_new_tensor_2d(s->cpu_device, NC_TYPE_I32,
                                       n_streams, n_states);
    s->model_class->model_reset(s);
    
    while (s->retrain_buf_len >= ((seg_len + block_idx) * n_streams)) {
        prof_start(PROF_TOTAL);
        for(cur_state = 0; cur_state < n_states; cur_state++) {
            for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                c = get_symb_fifo(s->retrain_buf, s->retrain_buf_pos,
                                  s->retrain_buf_len,
                                  block_stride * stream_idx + 
                                  block_idx + cur_state - 1 - overlap);
                nc_set1_i32_2d(input, stream_idx, cur_state, c);
                if (cur_state >= overlap) {
                    c = get_symb_fifo(s->retrain_buf, s->retrain_buf_pos,
                                      s->retrain_buf_len,
                                      block_stride * stream_idx + 
                                      block_idx + cur_state - overlap);
                    nc_set1_i32_2d(expected_output, stream_idx,
                                   cur_state - overlap, c);
                }
            }
        }
        output = s->model_class->model_eval(s, -1, input);
        nc_free_tensor(output);
        
        if (s->has_retrain_lr) {
            lr = get_interp_param(&s->retrain_lr, s->retrain_train_step);
        } else {
            lr = get_interp_param(&s->lr, s->train_step);
        }
        s->model_class->model_set_lr(s, lr);

        n_output_bits -= s->model_class->model_eval_gradient(s, expected_output) * inv_log2;
        s->model_class->model_update(s);
        if (s->has_retrain_lr) {
            s->retrain_train_step++;
        } else {
            s->train_step++;
        }
        block_idx += seg_len;
        prof_end(PROF_TOTAL);
        train_bytes += seg_len * n_streams;
        if ((train_bytes - last_train_bytes) >= stats_dump_interval) {
            double bps;
            int64_t cur_time;
            bps = (n_output_bits - last_n_output_bits) /
                (train_bytes - last_train_bytes);
            cur_time = get_time_ms();
            log_printf(st, "R %8" PRIu64 " %10" PRIu64 " %10s %6.3f %6.2f %8.2e\r",
                       s->has_retrain_lr ? s->retrain_train_step : s->train_step,
                       train_bytes,
                       "-",
                       bps, (double)(train_bytes - last_train_bytes) /
                       (double)(cur_time - last_time),
                       lr);

            last_train_bytes = train_bytes;
            last_time = cur_time;
            last_n_output_bits = n_output_bits;
        }
    }
    nc_free_tensor(expected_output);
    nc_free_tensor(input);
    s->model_class->model_set_retrain(s, FALSE);
}

/* the first rem streams have (stride + 1) symbols, the next ones have
   stride symbols */
static int get_symb(const DataSymbol *buf, int stride, int rem,
                    int stream_idx, int pos)
{
    int e;
    
    e = (stream_idx < rem);
    if (pos < 0 || pos >= (stride + e))
        return 0;
    return buf[stride * stream_idx + min_int(rem, stream_idx) + pos];
}

static void put_symb(DataSymbol *buf, int stride, int rem,
                    int stream_idx, int pos, int c)
{
    int e;
    
    e = (stream_idx < rem);
    if (pos < 0 || pos >= (stride + e))
        abort();
    buf[stride * stream_idx + min_int(rem, stream_idx) + pos] = c;
}

/* compress block_len bytes divided in n_streams */
static void process_block(NNCPModelState *s, FILE *fo,
                          PutBitState *pb, GetBitState *gb,
                          DataSymbol *block_buf, int block_len, LogState *st,
                          BOOL is_decode)
{
    int block_idx, c, stream_idx, cur_state, n_states;
    int block_stride, n_streams, block_rem, seg_len1, seg_len2;
    float *output, lr;
    size_t stride;
    NCTensor *output_host, *expected_output, *input;
    
    n_streams = s->batch_size;
    n_states = s->seg_len;
    block_stride = block_len / n_streams;
    block_rem = block_len % n_streams;

    input = nc_new_tensor_2d(s->cpu_device, NC_TYPE_I32,
                             n_streams, n_states);
    expected_output = nc_new_tensor_2d(s->cpu_device, NC_TYPE_I32,
                                       n_streams, n_states);
    s->model_class->model_reset(s);
    
    /* normal batches */
    lr = 0;
    block_idx = 0;
    while ((block_idx + n_states) <= block_stride) {
        prof_start(PROF_TOTAL);
        if (!use_encode_only) {
            for(cur_state = 0; cur_state < n_states; cur_state++) {
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    nc_set1_i32_2d(input, stream_idx, cur_state, 
                                   get_symb(block_buf, block_stride, block_rem,
                                            stream_idx, block_idx + cur_state - 1));
                }
                
                output_host = s->model_class->model_eval(s, cur_state, input);
                output_host = nc_tensor_to_cpu_device(output_host);
                output = nc_tensor_get_ptr(output_host, &stride);
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    prof_start(PROF_WRITE_SYM);
                    if (!is_decode) {
                        c = get_symb(block_buf, block_stride, block_rem,
                                     stream_idx, block_idx + cur_state);
                        write_sym(pb, output + stream_idx * stride, s->n_symbols, c);
                    } else {
                        c = read_sym(gb, output + stream_idx * stride,
                                     s->n_symbols);
                        put_symb(block_buf, block_stride, block_rem,
                                 stream_idx, block_idx + cur_state, c);
                    }
                    prof_end(PROF_WRITE_SYM);
                    nc_set1_i32_2d(expected_output, stream_idx, cur_state, c);
                }
                nc_free_tensor(output_host);
            }
        } else {
            for(cur_state = 0; cur_state < n_states; cur_state++) {
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    nc_set1_i32_2d(input, stream_idx, cur_state, 
                                   get_symb(block_buf, block_stride, block_rem,
                                            stream_idx, block_idx + cur_state - 1));
                }
            }
            output_host = s->model_class->model_eval(s, -1, input);

            output_host = nc_tensor_to_cpu_device(output_host);
            output = nc_tensor_get_ptr(output_host, &stride);
            for(cur_state = 0; cur_state < n_states; cur_state++) {
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    c = get_symb(block_buf, block_stride, block_rem,
                                 stream_idx, block_idx + cur_state);
                    prof_start(PROF_WRITE_SYM);
                    write_sym(pb, output + (cur_state * n_streams + stream_idx) * stride, s->n_symbols, c);
                    prof_end(PROF_WRITE_SYM);
                    nc_set1_i32_2d(expected_output, stream_idx, cur_state, c);
                }
            }
            nc_free_tensor(output_host);
        }

        lr = get_interp_param(&s->lr, s->train_step);
        st->last_lr = lr;
        s->model_class->model_set_lr(s, lr);
        
        s->model_class->model_eval_gradient(s, expected_output);
        s->model_class->model_update(s);
        s->train_step++;
        block_idx += n_states;
        prof_end(PROF_TOTAL);

        /* statistics */
        {
            int64_t n_output_bytes;
            
            st->n_input_bytes += n_states * n_streams;
            if ((st->n_input_bytes - st->last_n_input_bytes) >=
                stats_dump_interval) {
                if (!is_decode) {
                    n_output_bytes = put_bit_get_bit_count(pb) / 8;
                } else {
                    n_output_bytes = get_bit_get_bit_count(gb) / 8;
                }
                log_dump(st, st->n_input_bytes, n_output_bytes,
                         s->train_step, st->last_lr, FALSE);
            }
        }
    }
    
    seg_len2 = block_stride - block_idx;
    seg_len1 = seg_len2 + (block_rem != 0);
    if (seg_len1 > 0) {
        st->n_input_bytes += seg_len2 * n_streams + block_rem;
        /* no training for the last batch */
        for(cur_state = 0; cur_state < n_states; cur_state++) {
            for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                nc_set1_i32_2d(input, stream_idx, cur_state, 0);
            }
        }
        if (!use_encode_only) {
            for(cur_state = 0; cur_state < seg_len1; cur_state++) {
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    c = get_symb(block_buf, block_stride, block_rem,
                                 stream_idx, block_idx + cur_state - 1);
                    nc_set1_i32_2d(input, stream_idx, cur_state, c);
                }

                output_host = s->model_class->model_eval(s, cur_state, input);

                output_host = nc_tensor_to_cpu_device(output_host);
                output = nc_tensor_get_ptr(output_host, &stride);
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    if (cur_state < seg_len2 ||
                        (cur_state == seg_len2 && stream_idx < block_rem)) {
                        if (!is_decode) {
                            c = get_symb(block_buf, block_stride, block_rem,
                                         stream_idx, block_idx + cur_state);
                            write_sym(pb, output + stream_idx * stride, s->n_symbols, c);
                        } else {
                            c = read_sym(gb, output + stream_idx * stride,
                                         s->n_symbols);
                            put_symb(block_buf, block_stride, block_rem,
                                     stream_idx, block_idx + cur_state, c);
                        }
                    }
                }
                nc_free_tensor(output_host);
            }
        } else {
            for(cur_state = 0; cur_state < seg_len1; cur_state++) {
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    c = get_symb(block_buf, block_stride, block_rem,
                                 stream_idx, block_idx + cur_state - 1);
                    nc_set1_i32_2d(input, stream_idx, cur_state, c);
                }
            }

            output_host = s->model_class->model_eval(s, -1, input);

            output_host = nc_tensor_to_cpu_device(output_host);
            output = nc_tensor_get_ptr(output_host, &stride);
            for(cur_state = 0; cur_state < seg_len1; cur_state++) {
                for(stream_idx = 0; stream_idx < n_streams; stream_idx++) {
                    if (cur_state < seg_len2 ||
                        (cur_state == seg_len2 && stream_idx < block_rem)) {
                        c = get_symb(block_buf, block_stride, block_rem,
                                     stream_idx, block_idx + cur_state);
                        write_sym(pb, output +
                                  (cur_state * n_streams + stream_idx) *
                                  stride, s->n_symbols, c);
                    }
                }
            }
            nc_free_tensor(output_host);
        }
        s->model_class->model_eval_end(s);
    }

    nc_free_tensor(expected_output);
    nc_free_tensor(input);
}

static const char *load_coefs_filename;
static BOOL quiet_flag;
static const char *plot_filename;

static void read_block(FILE *f, DataSymbol *block_buf, int len, int symb_shift,
                      int n_symbols)
{
    int i, c;
    for(i = 0; i < len; i++) {
        if (symb_shift == 0) {
            c = fgetc(f);
            if (c < 0)
                break;
        } else {
            uint16_t v16;
            if (fget_be16(f, &v16))
                break;
            c = v16;
        }
        if (c >= n_symbols)
            fatal_error("Invalid symbol: %d\n", c);
        block_buf[i] = c;
    }
}

static void write_block(FILE *f, DataSymbol *block_buf, int len, int symb_shift)
{
    int i;
    for(i = 0; i < len; i++) {
        if (symb_shift == 0)
            fputc(block_buf[i], f);
        else
            fput_be16(f, block_buf[i]);
    }
}

#define ARITH_BUF_LEN 65536

static ssize_t arith_read_buf(void *opaque, uint8_t *buf, size_t buf_size)
{
    FILE *f = opaque;
    return fread(buf, 1, buf_size, f);
}

static void arith_write_buf(void *opaque, const uint8_t *buf, size_t buf_size)
{
    FILE *f = opaque;
    fwrite(buf, 1, buf_size, f);
}

static int64_t get_file_size(FILE *f)
{
    int64_t last_pos, file_size;
    last_pos = ftello(f);
    fseek(f, 0, SEEK_END);
    file_size = ftello(f);
    fseek(f, last_pos, SEEK_SET);
    return file_size;
}

static int64_t get_file_size2(const char *filename)
{
    FILE *f;
    int64_t file_size;
    f = fopen(filename, "rb");
    if (!f)
        return -1;
    file_size = get_file_size(f);
    fclose(f);
    return file_size;
}

/* write a small compressed file using zlib */
static void write_compressed_file(FILE *fo, const char *filename)
{
    FILE *f;
    int file_size, out_buf_size;
    uint8_t *buf, *out_buf;
    unsigned long csize;
    
    f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        exit(1);
    }
    file_size = get_file_size(f);
    if (file_size < 0)
        goto read_error;
    buf = malloc(file_size);
    if (fread(buf, 1, file_size, f) != file_size) {
    read_error:
        fatal_error("%s: read error", filename);
    }
    fclose(f);
    
    out_buf_size = compressBound(file_size);
    out_buf = malloc(out_buf_size);
    csize = out_buf_size;
    if (compress2(out_buf, &csize, buf, file_size,
                  Z_BEST_COMPRESSION) != Z_OK) {
        fatal_error("zlib compress2");
    }

    if (!quiet_flag) {
        printf("Compressed dictionary size=%d bytes\n", (int)csize);
    }
    
    fput_be32(fo, file_size);
    fput_be32(fo, csize);

    if (fwrite(out_buf, 1, csize, fo) != csize) {
        fatal_error("write_compressed_file: write_error");
    }
    
    free(buf);
    free(out_buf);
}

/* read a small compressed file using zlib */
static void read_compressed_file(FILE *f, const char *filename)
{
    uint32_t file_size, csize;
    uint8_t *buf, *out_buf;
    unsigned long dsize;
    FILE *fo;

    if (fget_be32(f, &file_size))
        goto read_error;
    if (fget_be32(f, &csize))
        goto read_error;
    
    buf = malloc(csize);
    out_buf = malloc(file_size);

    if (fread(buf, 1, csize, f) != csize) {
    read_error:
        fatal_error("read_compressed_file: read_error");
    }
    
    dsize = file_size;
    if (uncompress(out_buf, &dsize, buf, csize) != Z_OK) {
        fatal_error("zlib uncompress");
    }
    if (dsize != file_size) {
        fatal_error("invalid zlib decoded size");
    }
    
    fo = fopen(filename, "wb");
    if (!fo) {
        fatal_error("%s: write error", filename);
    }
    fwrite(out_buf, 1, file_size, fo);
    fclose(fo);
    
    free(out_buf);
    free(buf);
}

void encode_file(const char *in_filename, const char *out_filename,
                 const char *debug_dir, EncodeParams *ep, int argc, const char **argv)
{
    FILE *f, *fo;
    NNCPModelParams *p;
    NNCPModelState *pred;
    LogState st_s, *st = &st_s;
    DataSymbol *block_buf;
    int symb_shift, block_len, block_buf_size;
    int64_t n_output_bytes, file_length, file_pos, block_len1;
    int64_t org_file_length;
    char out_filename_buf[1024], dict_filename[1024], tmp_filename[1024];
    PutBitState pb;
    uint8_t *arith_buf;
    
    p = &ep->model_params;
    
    log_init(st, debug_dir, "nncp-log");

    if (st->log_file) {
        int i;
        fprintf(st->log_file, "cmd_line='");
        for(i = 0; i < argc; i++) {
            if (i != 0)
                fprintf(st->log_file, " ");
            fprintf(st->log_file, "%s", argv[i]);
        }
        fprintf(st->log_file, "'\n");
    }
    
    if (plot_filename || st->debug_output) {
        char filename[1024];
        if (plot_filename) {
            snprintf(filename, sizeof(filename), "%s", plot_filename);
        } else {
            snprintf(filename, sizeof(filename), "%s/plot.txt", st->debug_dir);
        }
        st->plot_file = fopen(filename, "wb");
        if (!st->plot_file) {
            fatal_error("could not create '%s'", filename);
        }
    }

    if (!out_filename) {
        snprintf(out_filename_buf, sizeof(out_filename_buf), "%s/%s",
                 st->debug_dir, "out.bin");
        out_filename = out_filename_buf;
    }

    org_file_length = -1;
    if (ep->preprocess_flag) {
        int n_words;
        int64_t pp_time;

        snprintf(tmp_filename, sizeof(tmp_filename),
                 "%s.pp", out_filename);
        if (ep->dict_filename) {
            /* force dictionary and use preprocessed input */
            snprintf(dict_filename, sizeof(dict_filename), "%s",
                     ep->dict_filename);
        } else {
            snprintf(dict_filename, sizeof(dict_filename),
                     "%s.voc", out_filename);

            org_file_length = get_file_size2(in_filename);

            pp_time = get_time_ms();
            n_words = word_encode(in_filename, tmp_filename, dict_filename,
                                  ep->n_words, ep->min_word_freq,
                                  NULL, FALSE, !quiet_flag);
            if (!quiet_flag) {
                log_printf(st, "Preprocessing time=%0.3f s\n", 
                           (get_time_ms() - pp_time) / 1000.0);
            }
            /* multiple of 8 to accelerate computations */
            p->n_symbols = (n_words + 7) & ~7;
            in_filename = tmp_filename;
        }
        symb_shift = 1;
    } else {
        if (p->n_symbols <= 256)
            symb_shift = 0;
        else
            symb_shift = 1;
    }

    f = fopen(in_filename, "rb");
    if (!f) {
        perror(in_filename);
        exit(1);
    }
        
    fo = fopen(out_filename, "wb");
    if (!fo) {
        perror(out_filename);
        exit(1);
    }

    nncp_write_file_header(fo, p);
    
    /* preprocessing info */
    if (ep->preprocess_flag) {
        fput_u8(fo, 1);
        write_compressed_file(fo, dict_filename);
    } else {
        fput_u8(fo, 0);
    }
    
    file_length = get_file_size(f);
    if (file_length < 0)
        fatal_error("could not get input file size");
    
    file_length >>= symb_shift;
    if (ep->max_size >= 0 && file_length > ep->max_size)
        file_length = ep->max_size;
    
    fput_be32(fo, file_length);
    
    arith_buf = nc_malloc(ARITH_BUF_LEN);
    put_bit_init(&pb, arith_buf, ARITH_BUF_LEN, arith_write_buf, fo);

    p->seq_eval = !use_encode_only;
    
    pred = nncp_init(p);
    if (!quiet_flag) {
        nncp_dump_params(stdout, pred, p);
    }
    if (st->log_file) {
        nncp_dump_params(st->log_file, pred, p);
        fflush(st->log_file);
    }

#if 0
    if (load_coefs_filename)
        nc_load_coefs(&pred->param_list, load_coefs_filename);
#endif


    block_buf = NULL;
    block_buf_size = 0;
    file_pos = 0;
    st->last_time = get_time_ms(); /* exclude the init time */
    for(;;) {
        block_len1 = file_length - file_pos;
        if (block_len1 == 0)
            break;
        block_len = lrintf(get_interp_param(&p->block_len, file_pos));
        block_len = max_int(block_len / (p->seg_len * p->batch_size), 1) *
            (p->seg_len * p->batch_size);
        if (block_len > block_len1)
            block_len = block_len1;
        
        if (block_len > block_buf_size) {
            nc_free(block_buf);
            block_buf = nc_malloc(block_len * sizeof(block_buf[0]));
            block_buf_size = block_len;
        }
        
        read_block(f, block_buf, block_len, symb_shift, p->n_symbols);
        
        process_block(pred, fo, &pb, NULL, block_buf, block_len,
                      st, FALSE);

        file_pos += block_len;
        if (file_pos >= file_length)
            break;

        retrain_block(pred, block_buf, block_len, file_pos, st);
    }
    nc_free(block_buf);
    
    put_bit_flush(&pb);
    nc_free(arith_buf);
    n_output_bytes = ftell(fo);
    fclose(fo);
    fclose(f);

    if (ep->preprocess_flag && !ep->dict_filename) {
        unlink(dict_filename);
        unlink(tmp_filename);
    }
    
    log_dump(st, st->n_input_bytes, n_output_bytes,
             pred->train_step, st->last_lr, TRUE);
    if (!quiet_flag) {
        int64_t ti;
        ti = get_time_ms() - st->start_time;
        log_printf(st, "Total time=%0.3f s", ti / 1000.0);
        if (org_file_length > 0) {
            log_printf(st, " (%0.2f kB/s)",
                       (double)org_file_length / (double)ti);
        }
        log_printf(st, "\n");
    }
    log_end(st);

    nncp_end(pred);

    nc_prof_dump();
}

void decode_file(const char *in_filename, const char *out_filename,
                 const char *debug_dir)
{
    FILE *f, *fo;
    NNCPModelParams p_s, *p = &p_s;
    NNCPModelState *pred;
    LogState st_s, *st = &st_s;
    DataSymbol *block_buf;
    int symb_shift, block_len, block_buf_size;
    int64_t n_output_bytes, file_length, file_pos, block_len1;
    char out_filename_buf[1024], dict_filename[1024], tmp_filename[1024];
    GetBitState gb;
    uint8_t *arith_buf;
    BOOL preprocess_flag;
    uint32_t v32;
    uint8_t v8;
    
    log_init(st, debug_dir, "nncp-log");

    if (!out_filename) {
        snprintf(out_filename_buf, sizeof(out_filename_buf), "%s/%s",
                 st->debug_dir, "out.bin");
        out_filename = out_filename_buf;
    }

    f = fopen(in_filename, "rb");
    if (!f) {
        perror(in_filename);
        exit(1);
    }
        
    if (nncp_read_file_header(f, p) < 0) {
        fatal_error("invalid file header");
    }

    if (p->use_cuda != use_cuda) {
        if (p->use_cuda) {
            fprintf(stderr, "Warning: enabling CUDA as the file was encoded with it\n");
        } else {
            fprintf(stderr, "Warning: disabling CUDA as the file was encoded without it\n");
        }
    }
    p->use_cuda = use_cuda;
    
    if (fget_u8(f, &v8) < 0) {
        fatal_error("read error");
    }
    preprocess_flag = v8 & 1;

    if (preprocess_flag) {
        snprintf(tmp_filename, sizeof(tmp_filename),
                 "%s.pp", out_filename);
        snprintf(dict_filename, sizeof(dict_filename),
                 "%s.voc", out_filename);

        read_compressed_file(f, dict_filename);

        fo = fopen(tmp_filename, "wb");
        if (!fo) {
            perror(tmp_filename);
            exit(1);
        }
        symb_shift = 1;
    } else {
        fo = fopen(out_filename, "wb");
        if (!fo) {
            perror(out_filename);
            exit(1);
        }
        
        if (p->n_symbols <= 256)
            symb_shift = 0;
        else
            symb_shift = 1;
    }

    if (fget_be32(f, &v32) < 0) {
        fprintf(stderr, "Read error\n");
        exit(1);
    }
    file_length = v32;
           
    arith_buf = nc_malloc(ARITH_BUF_LEN);
    get_bit_init(&gb, arith_buf, ARITH_BUF_LEN, arith_read_buf, f);

    p->seq_eval = TRUE;
    
    pred = nncp_init(p);
    if (!quiet_flag) {
        nncp_dump_params(stdout, pred, p);
    }
    if (st->log_file) {
        nncp_dump_params(st->log_file, pred, p);
        fflush(st->log_file);
    }

#if 0
    if (load_coefs_filename)
        nc_load_coefs(&pred->param_list, load_coefs_filename);
#endif
    
    block_buf = NULL;
    block_buf_size = 0;
    file_pos = 0;
    for(;;) {
        block_len1 = file_length - file_pos;
        if (block_len1 == 0)
            break;
        block_len = lrintf(get_interp_param(&p->block_len, file_pos));
        block_len = max_int(block_len / (p->seg_len * p->batch_size), 1) *
            (p->seg_len * p->batch_size);
        if (block_len > block_len1)
            block_len = block_len1;
        
        if (block_len > block_buf_size) {
            nc_free(block_buf);
            block_buf = nc_malloc(block_len * sizeof(block_buf[0]));
            block_buf_size = block_len;
        }

        process_block(pred, fo, NULL, &gb, block_buf, block_len,
                      st, TRUE);
        
        write_block(fo, block_buf, block_len, symb_shift);
        fflush(fo);

        file_pos += block_len;
        if (file_pos >= file_length)
            break;

        retrain_block(pred, block_buf, block_len, file_pos, st);
    }
    nc_free(block_buf);
    
    n_output_bytes = ftell(f);

    fclose(fo);
    fclose(f);
    nc_free(arith_buf);

    log_dump(st, st->n_input_bytes, n_output_bytes,
             pred->train_step, st->last_lr, TRUE);
    
    nncp_end(pred);

    if (preprocess_flag) {
        word_decode(tmp_filename, out_filename, dict_filename);

        unlink(dict_filename);
        unlink(tmp_filename);
    }

    if (!quiet_flag) {
        int64_t ti, org_file_length;

        ti = get_time_ms() - st->start_time;

        log_printf(st, "Total time=%0.3f s", ti / 1000.0);
        if (preprocess_flag) {
            org_file_length = get_file_size2(out_filename);
            log_printf(st, " (%0.2f kB/s)",
                       (double)org_file_length / (double)ti);
        }
        log_printf(st, "\n");
    }

    log_end(st);
    
    nc_prof_dump();
}


static const CMDOptDesc nncp_options[] = {
    { "h,help", 0, "show the help" },
    { "d", CMD_HAS_ARG, "set the debug directory", "dir" },
    { "q", 0, "enable quiet mode" },
    { "T", CMD_HAS_ARG, "number of CPU threads" },
    { "p,profile", CMD_HAS_ARG, "set the encoding profile: default, enwik8, enwik9, lstm, lstm_fast." },
    { "max_size", CMD_HAS_ARG, "truncate the input to N symbols", "N" },
    { "plot", CMD_HAS_ARG, "set the plot filename" },
    { "load_coefs", CMD_HAS_ARG, "load the model coefficients from file" },
    { "dump_interval", CMD_HAS_ARG, "dump interval of statistics" },
    { "cuda", 0, "enable CUDA support" },
    { "bf16", CMD_HAS_ARG, "enable bf16 processing", "[0|1]" },
    { "encode_only", 0, "faster encode only mode (output cannot be decompressed)" },

    { "batch_size", CMD_HAS_ARG, "batch size" },
    { "seed", CMD_HAS_ARG, "random number seed" },
    { "block_len", CMD_HAS_ARG, "set the encoding block length" },
    { "train_len", CMD_HAS_ARG, "training segment length" },
    { "lr", CMD_HAS_ARG, "learning rate", "lr0[,step0,lr1]..." },
    { "retrain_period", CMD_HAS_ARG, "retrain period in symbols, 0 to disable retrain" },
    { "retrain_len", CMD_HAS_ARG, "retrain length" },
    { "retrain_lr", CMD_HAS_ARG, "retrain learning rate" },
    { "n_symb", CMD_HAS_ARG, "vocabulary size (2 to 65535)" },

    { "preprocess", CMD_HAS_ARG, "enable text preprocessing", "n_words,min_word_freq" },
    { "dict", CMD_HAS_ARG, "set the dictionary filename (pc, pd, and c commands)", "filename" },
    { NULL },
};

static void help(void)
{
    int i;
    const NNCPModelClass *mc;
    
    printf("NNCP version " CONFIG_VERSION", Copyright (c) 2018-2021 Fabrice Bellard\n"
           "Lossless data compression with Neural Networks\n"
           "usage: nncp [options] cmd args...\n"
           "\n"
           "Commands:\n"
           "c infile outfile         compress 'infile' to 'outfile'\n"
           "d infile outfile         decompres 'infile' to 'outfile'\n"
           "pc infile outfile        preprocessor-only encoding\n"
           "pd infile outfile        preprocessor-only decoding\n"
           "\n"
           "General options:\n");

    cmdopt_show_desc(nncp_options);

    for(i = 0; i < countof(nncp_models); i++) {
        mc = nncp_models[i];
        printf("\nOptions for the %s model:\n", mc->name);
        cmdopt_show_desc(mc->model_options);
    }

    exit(1);
}

int main(int argc, const char **argv)
{
    int optind, i;
    const char *debug_dir, *cmd, *r;
    CMDOption *co;

    debug_dir = NULL;

    co = cmdopt_init("nncp");
    cmdopt_add_desc(co, nncp_options);
    for(i = 0; i < countof(nncp_models); i++)
        cmdopt_add_desc(co, nncp_models[i]->model_options);
    optind = cmdopt_parse(co, argc, argv);

    if (optind >= argc) 
        help();

    cmd = argv[optind];
    
    if (cmdopt_has(co, "help"))
        help();

    plot_filename = cmdopt_get(co, "plot");

    load_coefs_filename = cmdopt_get(co, "load_coefs");
    
    stats_dump_interval = cmdopt_get_int(co, "dump_interval",
                                         stats_dump_interval);
    
    use_cuda = cmdopt_has(co, "cuda");

    use_encode_only = cmdopt_has(co, "encode_only");
    
    quiet_flag = cmdopt_has(co, "q");

    nb_threads = cmdopt_get_int(co, "T", nb_threads);
    
    debug_dir = cmdopt_get(co, "d");

    
    if (!strcmp(cmd, "c")) {
        const char *out_filename;
        EncodeParams ep_s, *ep = &ep_s;
        NNCPModelParams *p = &ep->model_params;
        
        memset(ep, 0, sizeof(*ep));
        
        if (optind + 1 >= argc) 
            help();
        if ((optind + 2) < argc) {
            out_filename = argv[optind + 2];
        } else {
            out_filename = NULL;
            if (!debug_dir)
                help();
        }

        r = cmdopt_get(co, "profile");
        if (!r)
            r = "default";
        
        nncp_init_params(p, r);

        p->use_cuda = use_cuda;
        
        ep->max_size = cmdopt_get_int(co, "max_size", -1);

        r = cmdopt_get(co, "preprocess");
        if (r) {
            ep->preprocess_flag = TRUE;
            ep->n_words = strtoul(r, (char **)&r, 0);
            if (*r != ',')
                cmd_error("comma expected");
            r++;
            ep->min_word_freq = strtoul(r, (char **)&r, 0);
            if (*r != '\0')
                cmd_error("unexpected chars");
        }
        
        r = cmdopt_get(co, "dict");
        if (r) {
            ep->preprocess_flag = TRUE;
            ep->dict_filename = r;
        }


        r = cmdopt_get(co, "lr");
        if (r) 
            parse_interp_param(&p->lr, r);
        
        p->retrain_period = cmdopt_get_int(co, "retrain_period",
                                           p->retrain_period);
        p->retrain_len = cmdopt_get_int(co, "retrain_len",
                                        p->retrain_len);

        r = cmdopt_get(co, "retrain_lr");
        if (r) {
            parse_interp_param(&p->retrain_lr, r);
            p->has_retrain_lr = TRUE;
        }
        
        p->batch_size = cmdopt_get_int(co, "batch_size",
                                       p->batch_size);
        p->seg_len = cmdopt_get_int(co, "train_len", p->seg_len);
        p->seed = cmdopt_get_int(co, "seed", p->seed);
        p->n_symbols= cmdopt_get_int(co, "n_symb", p->n_symbols);
        if (p->n_symbols < 2 || p->n_symbols > 65536)
            cmd_error("invalid number of symbols");
        
        r = cmdopt_get(co, "block_len");
        if (r) {
            parse_interp_param(&p->block_len, r);
        }

        p->use_bf16 = cmdopt_get_int(co, "bf16", p->use_bf16);
        
        ep->model_params.model_class->model_parse_options(p, co);
        
        encode_file(argv[optind + 1], out_filename, debug_dir, ep, argc, argv);
    } else if (!strcmp(cmd, "d")) {
        if (optind + 2 >= argc) 
            help();
        decode_file(argv[optind + 1], argv[optind + 2], debug_dir);
    } else if (!strcmp(cmd, "pc")) {
        const char *in_filename;
        const char *out_filename;
        const char *dict_filename;
        int n_words, min_word_freq;
        
        if (optind + 2 >= argc) 
            help();

        r = cmdopt_get(co, "preprocess");
        if (!r)
            cmd_error("--preprocess option missing");

        n_words = strtoul(r, (char **)&r, 0);
        if (*r != ',')
            cmd_error("comma expected");
        r++;
        min_word_freq = strtoul(r, (char **)&r, 0);
        if (*r != '\0')
            cmd_error("unexpected chars");

        dict_filename = cmdopt_get(co, "dict");
        if (!dict_filename)
            cmd_error("--dict option missing");
        
        in_filename = argv[optind + 1];
        out_filename = argv[optind + 2];
        word_encode(in_filename, out_filename, dict_filename, n_words,
                    min_word_freq, NULL, FALSE, !quiet_flag);
    } else if (!strcmp(cmd, "pd")) {
        const char *in_filename;
        const char *out_filename;
        const char *dict_filename;
        
        if (optind + 2 >= argc) 
            help();
        dict_filename = cmdopt_get(co, "dict");
        if (!dict_filename)
            cmd_error("--dict option missing");
        in_filename = argv[optind + 1];
        out_filename = argv[optind + 2];
        word_decode(in_filename, out_filename, dict_filename);
    } else {
        help();
    }
    cmdopt_free(co);
    return 0;
}
