/*
 * LibNC
 * 
 * Copyright (c) 2018-2023 Fabrice Bellard
 *
 */
#ifndef LIBNC_H
#define LIBNC_H

#include <inttypes.h>
#include "cutils.h"
#include "list.h"

/* profiling */

typedef enum {
    PROF_EVAL,
    PROF_GRAD,
    PROF_SGD,
    PROF_UPDATE,
    PROF_WRITE_SYM,
    PROF_PROBE,
    PROF_TOTAL,
    PROF_COUNT,
} ProfEnum;

#ifdef PROFILE

extern int64_t prof_cycles[PROF_COUNT];
extern int64_t prof_samples[PROF_COUNT];
extern int64_t prof_ops[PROF_COUNT];

static inline void prof_start(int idx)
{
    prof_cycles[idx] -= get_cycles();
}

static inline void prof_end(int idx)
{
    prof_cycles[idx] += get_cycles();
    prof_samples[idx]++;
}

static inline void prof_end_ops(int idx, int n_ops)
{
    prof_cycles[idx] += get_cycles();
    prof_ops[idx] += n_ops;
    prof_samples[idx]++;
}

#else

static inline void prof_start(int idx)
{
}

static inline void prof_end(int idx)
{
}

static inline void prof_end_ops(int idx, int n_ops)
{
}

#endif

void nc_prof_dump(void);

/* Automatic Differentiation Engine */

typedef struct NCContext NCContext;
typedef struct NCDevice NCDevice;
typedef struct NCTensor NCTensor;
typedef struct NCTensorBuffer NCTensorBuffer;
typedef struct NCNode NCNode;
typedef struct NCLazyTensor NCLazyTensor;
typedef struct NCRNDState NCRNDState;
typedef struct NCSGDOptState NCSGDOptState;

typedef enum {
    NC_TYPE_F32,
    NC_TYPE_BF16,
    NC_TYPE_F16,
    NC_TYPE_I8,
    NC_TYPE_I16,
    NC_TYPE_I32,
    NC_TYPE_U8,
    NC_TYPE_U16,
    NC_TYPE_U32,
    NC_TYPE_E4M3,
    NC_TYPE_E5M2,
    NC_TYPE_BF8,
    NC_TYPE_BF4,
    NC_TYPE_BF3,
    NC_TYPE_BF8L,
    NC_TYPE_BF8C, /* internal format */
    NC_TYPE_COUNT,
} NCTypeEnum;

extern size_t nc_type_size_table[NC_TYPE_COUNT];
extern int nc_type_size_log2_table[NC_TYPE_COUNT];
extern const char *nc_type_name_table[NC_TYPE_COUNT];

#define NC_N_DIMS_MAX 8 /* maximum number of axis for tensors */

typedef struct NCTensorData {
    NCTypeEnum item_type;
    size_t item_size;
    void *data;
    size_t stride; /* in elements */
    size_t n_strides; /* prod(j = 1 ... n_dims, dims[j]); */
    int n_dims;
    const size_t *dims; /* n_dims length */
    const size_t *strides; /* n_dims length, strides in bytes */
} NCTensorData;

void *nc_malloc(size_t size);
void *nc_mallocz(size_t size);
void nc_free(void *ptr);

/* return the number of available threads */
int nc_get_n_available_threads(void);
    
NCContext *nc_context_init(int nb_threads);
void nc_context_end(NCContext *m);

NCDevice *nc_new_cpu_device(NCContext *m);
NCDevice *nc_new_cuda_device(NCContext *m, int device_index);
NCDevice *nc_new_device(NCContext *m, const char *device_name);
void nc_synchronize(NCDevice *d);
/* must be called at the beginning of a new thread to initialize the
   device specific data */
void nc_thread_init(NCDevice *d);

NCTensorBuffer *nc_new_tensor_buffer(NCDevice *d, size_t size);
NCTensorBuffer *nc_dup_tensor_buffer(const NCTensorBuffer *b);
void nc_free_tensor_buffer(NCTensorBuffer *b);
int64_t nc_tensor_buffer_get_size(NCTensorBuffer *b);
/* return the number of allocated bytes on the device for tensor buffers */
int64_t nc_get_allocated_memory_size(NCDevice *d);
int64_t nc_get_max_allocated_memory_size(NCDevice *d, BOOL reset);
int64_t nc_get_flop_count(NCContext *m);
/* enable/disable reproducible computation (same results regardless of
   the CPU/GPU model and threading configuration). Return the previous
   state. Some operations are slower. */
BOOL nc_set_reproducible(NCContext *m, BOOL enable);

NCTensor *nc_new_tensor_from_buffer(NCContext *m,
                                    NCTensorBuffer *buffer, NCTypeEnum type,
                                    int n_dims, const size_t *dims,
                                    size_t offset, const size_t *strides,
                                    float scale);
NCTensor *nc_new_tensor(NCDevice *d, NCTypeEnum type,
                        int n_dims, const size_t *dims);
NCTensor *nc_new_tensor_from_tensor(const NCTensor *x);
NCTensor *nc_new_scalar(NCDevice *d, NCTypeEnum type);
NCTensor *nc_new_tensor_1d(NCDevice *d, NCTypeEnum type, size_t len);
NCTensor *nc_new_tensor_2d(NCDevice *d, NCTypeEnum type, size_t n0, size_t n1);
NCTensor *nc_new_tensor_3d(NCDevice *d, NCTypeEnum type,
                           size_t n0, size_t n1, size_t n2);
NCTensor *nc_new_tensor_4d(NCDevice *d, NCTypeEnum type,
                           size_t n0, size_t n1, size_t n2, size_t n3);
NCTensor *nc_new_tensor_nz(NCDevice *d, NCTypeEnum type,
                           int n_dims, const size_t *dims);
NCTensor *nc_new_tensor_from_tensor_nz(const NCTensor *x);
NCTensor *nc_new_tensor_nz_1d(NCDevice *d, NCTypeEnum type, size_t len);
NCTensor *nc_new_tensor_nz_2d(NCDevice *d, NCTypeEnum type, size_t n0, size_t n1);
NCTensor *nc_new_tensor_nz_3d(NCDevice *d, NCTypeEnum type,
                              size_t n0, size_t n1, size_t n2);
NCTensor *nc_new_tensor_nz_4d(NCDevice *d, NCTypeEnum type,
                              size_t n0, size_t n1, size_t n2, size_t n3);
NCTensor *__attribute__((format(printf, 2, 3))) nc_tensor_set_name(NCTensor *x, const char *fmt, ...);
NCTensor *nc_dup_tensor(const NCTensor *x);
void nc_free_tensor(NCTensor *x);
void nc_dump_tensor(const char *name, const NCTensor *x, size_t n);
void nc_dump_tensor_strides(const char *name, const NCTensor *x);
uint32_t nc_tensor_get_hash(const NCTensor *x);
BOOL nc_tensor_isfinite(NCTensor *x);
void nc_dump_tensor_hash(const char *name, const NCTensor *x);
NCNode *nc_get_node(NCTensor *x);
NCTypeEnum nc_tensor_get_item_type(const NCTensor *x);
NCTensorData *nc_tensor_get_data(NCTensorData *sd, const NCTensor *x);
/* Return a pointer to the tensor data. If *pstride is non NULL,
   return the stride (in elements) of the first dimension. */
void *nc_tensor_get_ptr(const NCTensor *x, size_t *pstride);
int nc_tensor_get_n_dims(const NCTensor *x);
size_t nc_tensor_get_dim(const NCTensor *x, int axis);
size_t nc_tensor_get_stride(const NCTensor *x, int axis);
int nc_tensor_get_dims(const NCTensor *x, size_t *dims);
int64_t nc_tensor_get_buffer_size(NCTensor *x);
BOOL nc_same_shape(const NCTensor *x1, const NCTensor *x2);
void nc_tensor_set_zero(NCTensor *y);
void nc_tensor_set_u32(NCTensor *y, uint32_t val);
void nc_tensor_set_f32(NCTensor *y, float val);
NCRNDState *nc_rnd_init(NCDevice *d, uint32_t seed);
void nc_rnd_end(NCRNDState *s);
void nc_tensor_set_rnd_unif(NCTensor *y, float avg, float range,
                            NCRNDState *rnd_state);
void nc_tensor_set_rnd_gaussian(NCTensor *y, float avg, float sigma,
                                NCRNDState *rnd_state);
void nc_tensor_set_dropout(NCTensor *y, float prob, NCRNDState *rnd_state);

void nc_set1_i32(NCTensor *y, int n_dims, const size_t *tab_indexes,
                 int32_t val);
void nc_set1_i32_1d(NCTensor *y, size_t i0, int32_t val);
void nc_set1_i32_2d(NCTensor *y, size_t i0, size_t i1, int32_t val);
void nc_set1_f32(NCTensor *y, int n_dims, const size_t *tab_indexes,
                 float val);
void nc_set1_f32_1d(NCTensor *y, size_t i0, float val);

int32_t nc_get1_i32(const NCTensor *x, int n_dims, const size_t *tab_indexes);
float nc_get1_f32(const NCTensor *x, int n_dims, const size_t *tab_indexes);
float nc_get1_f32_1d(const NCTensor *x, size_t i0);
float nc_get_scalar_f32(const NCTensor *x);
/* same as nc_get_scalar_f32() but free the tensor */
float nc_tensor_to_scalar_f32(NCTensor *x);

void nc_tensor_copy(NCTensor *dst, NCTensor *src);
void nc_tensor_copy_slice(NCTensor *dst, NCTensor *src, int axis,
                          size_t dst_start, size_t dst_end, size_t src_start);
void nc_tensor_convert(NCTensor *dst, NCTensor *src);

void nc_dump_dims(const char *str, const NCTensor *x);
size_t nc_get_heap_size(NCContext *m);
NCContext *nc_get_tensor_context(const NCTensor *x);
NCTensor *nc_tensor_to_device(NCTensor *x, NCDevice *d);
NCTensor *nc_tensor_to_cpu_device(NCTensor *x);
NCDevice *nc_get_tensor_device(const NCTensor *x);
                                 
/* element wise operations */
NCTensor *nc_convert(NCTensor *x, NCTypeEnum new_type);
NCTensor *nc_add(NCTensor *x1, NCTensor *x2);
NCTensor *nc_neg(NCTensor *x);
NCTensor *nc_sub(NCTensor *x1, NCTensor *x2);
NCTensor *nc_mul(NCTensor *x1, NCTensor *x2);
NCTensor *nc_div(NCTensor *x1, NCTensor *x2);
NCTensor *nc_recip(NCTensor *x);
NCTensor *nc_min(NCTensor *x1, NCTensor *x2);
NCTensor *nc_max(NCTensor *x1, NCTensor *x2);
/* select x1[i] if z[i] = 0 and x2[i] otherwise */
NCTensor *nc_select(NCTensor *z, NCTensor *x1, NCTensor *x2);
/* set y[i] = x1[i] if mask[i] = 0 and y[i] = c if mask[i] != 0. If
   mask_inv is TRUE, 'mask' is inverted */
NCTensor *nc_masked_fill(NCTensor *x, NCTensor *mask, float c, BOOL mask_inv);
NCTensor *nc_sigmoid(NCTensor *x);
NCTensor *nc_tanh(NCTensor *x);
NCTensor *nc_relu(NCTensor *x);
NCTensor *nc_sqr_relu(NCTensor *x);
NCTensor *nc_swish(NCTensor *x, float beta);
/* g * swish(x) */
NCTensor *nc_gated_swish(NCTensor *g, NCTensor *x, float beta);
NCTensor *nc_gelu(NCTensor *x);
/* g * gelu(x) */
NCTensor *nc_geglu(NCTensor *g, NCTensor *x);
void nc_geglu_bw(NCTensor **pgg, NCTensor **pxg, NCTensor **py,
                 NCTensor *yg, NCTensor *g, NCTensor *x);
/* x + sin(a*x)^2/(a + eps) */
NCTensor *nc_snake(NCTensor *x, NCTensor *a, float eps);
NCTensor *nc_log(NCTensor *x);
NCTensor *nc_exp(NCTensor *x);
/* return cp * fg + min(1 - fg, ig) * in */
NCTensor *nc_lstm_clamped(NCTensor *cp, NCTensor *in,
                          NCTensor *fg, NCTensor *ig);
/* RWKV attention. 
   aa, bb, pp =(bs, d)
   k, v, result = (bs, s, d)
   u, w = (d)
   Warning: aa, bb and pp are modified in place 
*/
NCTensor *nc_rwkv_att(NCTensor *aa, NCTensor *bb, NCTensor *pp,
                      NCTensor *k, NCTensor *v, NCTensor *u, NCTensor *w);
/* return a * (1 - t) + b * t */
NCTensor *nc_lerp(NCTensor *a, NCTensor *b, NCTensor *t);
/* return y = clamp(x * mult + addend, v_min, v_max) with an integer d_type */
NCTensor *nc_convert_clamp(NCTensor *x, float mult, float addend, float v_min, float v_max,
                           NCTypeEnum d_type);
/* other operations */
NCTensor *nc_new_f32(NCDevice *d, float val);
/* use the same device and type as tensor 'x' */
NCTensor *nc_new_f32_from_tensor(const NCTensor *x, float val);
NCTensor *nc_reshape(NCTensor *x, int n_dims, const size_t *dims);
NCTensor *nc_reshape_1d(NCTensor *x, size_t n0);
NCTensor *nc_reshape_2d(NCTensor *x, size_t n0, size_t n1);
NCTensor *nc_reshape_3d(NCTensor *x, size_t n0, size_t n1, size_t n2);
NCTensor *nc_reshape_4d(NCTensor *x, size_t n0, size_t n1, size_t n2,
                        size_t n3);
NCTensor *nc_reshape_5d(NCTensor *x, size_t n0, size_t n1, size_t n2,
                        size_t n3, size_t n4);
/* duplicate the tensor by adding n_dims dimensions */
NCTensor *nc_repeat(NCTensor *x, int n_dims, const size_t *dims);
NCTensor *nc_repeat_1d(NCTensor *x, size_t n0);
NCTensor *nc_repeat_2d(NCTensor *x, size_t n0, size_t n1);
/* return y0 + sum over the dimensions > n_dims of 'x'. y0 = NULL
   is supported */
NCTensor *nc_reduce_sum(NCTensor *y0, NCTensor *x, int n_dims);
/* reduce sum on axis. The corresponding dimension is set to 1 */
NCTensor *nc_reduce_sum_axis(NCTensor *x, int axis);
/* sum all the elements of a tensor */
NCTensor *nc_sum(NCTensor *x);
/* sum of squares */
NCTensor *nc_reduce_sum_sqr(NCTensor *x);
/* return the maximum on axis 0 and the corresponding indices of the
   maximum element. 'pindices' can be NULL. */
NCTensor *nc_reduce_max(NCTensor **pindices, NCTensor *x);
NCTensor *nc_cumsum(NCTensor *x);
/* create an alias to tensor 'x1' */
NCTensor *nc_slice_alias(const NCTensor *x1, int axis, size_t start, size_t end);
NCTensor *nc_slice(NCTensor *x, int axis, size_t start, size_t end);
NCTensor *nc_slice_add(NCTensor *y0, NCTensor *x, int axis, size_t start);
/* concatenation along axis 'axis' */
NCTensor *nc_concat(NCTensor **inputs, int n_inputs, int axis);
/* shortcut with n_inputs = 2 */
NCTensor *nc_concat_2(NCTensor *x0, NCTensor *x1, int axis);
/* shortcut for axis = 0 */
NCTensor *nc_vconcat(NCTensor **inputs, int n_inputs);
/* shortcut for axis = 1 */
NCTensor *nc_hconcat(NCTensor **inputs, int n_inputs);
/* split along axis 'axis'. If tab_size = NULL, split equally. */
void nc_split(NCTensor **tab_y, NCTensor *x, int n_outputs,
              const size_t *tab_size, int axis);
/* specialized for n_outputs = 2 */
void nc_split_2(NCTensor **py0, NCTensor **py1, NCTensor *x,
                size_t size0, size_t size1, int axis);

typedef enum {
    NC_PAD_ZERO,
    NC_PAD_DUP, /* duplicate element */
    /* trim types, dual to padding */
    NC_TRIM_NORMAL = NC_PAD_ZERO,
    NC_TRIM_SUM, /* add trimmed elements to the edge */
} NCPadEnum;

/* pad (len > 0) or trim (len < 0) the axis 0 of 'x' */
NCTensor *nc_pad(NCTensor *x, ssize_t left_len, NCPadEnum left_op,
                 ssize_t right_len, NCPadEnum right_op);
/* shortcut to nc_pad() */
NCTensor *nc_resize(NCTensor *x, size_t n);
NCTensor *nc_resize_axis(NCTensor *x, int axis, size_t n);
/* can reduce the size or increase it if the corresponding dimension
   is 1. -1 indicates to keep the size. */
NCTensor *nc_resize_alias(NCTensor *x, int n_dims, const size_t *dims);
/* same as nc_reslize_alias but for one axis */
NCTensor *nc_resize_alias_1(NCTensor *x, int axis, size_t n);
NCTensor *nc_resize_alias_4d(NCTensor *x, size_t n0, size_t n1, size_t n2, size_t n3);

/* return a new tensor which is a copy of 'x' */
NCTensor *nc_clone(NCTensor *x);
/* return TRUE is the tensor is stored in contiguous storage elements */
BOOL nc_tensor_is_contiguous(const NCTensor *x);
/* if x is not contiguous then create a new contiguous tensor and copy
   x to it. Otherwise, return 'x'. */
NCTensor *nc_make_contiguous(NCTensor *x);
/* Return a new tensor sharing the same buffer as 'x' with the permuted
   dimensions. axis[i] is the corresponding axis in 'x' */
NCTensor *nc_permute_alias(NCTensor *x, int n_dims, const int *axis);
NCTensor *nc_permute_alias_4d(NCTensor *x, int a0, int a1, int a2, int a3);
/* same as nc_permute_alias but calls nc_make_contiguous after. */
NCTensor *nc_permute(NCTensor *x, int n_dims, const int *axis);
NCTensor *nc_permute_3d(NCTensor *x, int a0, int a1, int a2);
NCTensor *nc_permute_4d(NCTensor *x, int a0, int a1, int a2, int a3);
/* special case of nc_permute() */
NCTensor *nc_transpose(NCTensor *x);
/* enable reproducible results on CPU and GPU. Currently only works with the F32 type */
#define NC_MATMUL_FLAG_REPRODUCIBLE (1 << 0) 
/* return a*b*alpha + c. a and b can be optionally transposed. c can
   be NULL. */
NCTensor *nc_matmul_add2(NCTensor *a, NCTensor *b, NCTensor *c0,
                         BOOL a_trans, BOOL b_trans, float alpha, int flags);
/* same as nc_matmul_add2 with flags = 0 */
NCTensor *nc_matmul_add(NCTensor *w, NCTensor *x, NCTensor *c,
                        BOOL w_trans, BOOL x_trans, float alpha);
/* same as nc_matmul_add with c = NULL, a_trans = FALSE, b_trans = FALSE and alpha = 1 */
NCTensor *nc_matmul(NCTensor *w, NCTensor *x);
/* return a matrix where each column is the column x[i] of matrix 'w' */
NCTensor *nc_get_col(NCTensor *w, NCTensor *x);
/* add the vectors 'z' at column number 'x' in matrix 'w'. */
NCTensor *nc_add_col(NCTensor *z, NCTensor *x, NCTensor *w);
/* select the x-th element in each column of 'w' */
NCTensor *nc_get_element(NCTensor *w, NCTensor *x);
/* add z to the x-th element in each column of 'w' */
NCTensor *nc_add_element(NCTensor *z, NCTensor *x, NCTensor *w);
NCTensor *nc_soft_max(NCTensor *x);
/* max(soft_max[i] * mult, 1) converted to int32 */
NCTensor *nc_soft_max_int(NCTensor *x, float mult);
/* Equivalent to y = log(get_element(x, eout)). It is expected to be
   used as nc_index_log(nc_soft_max(x), eout) so that the gradient
   computation is optimized. */
NCTensor *nc_indexed_log(NCTensor *x, NCTensor *eout);
NCTensor *nc_layer_norm(NCTensor *x, float eps);
NCTensor *nc_rms_norm(NCTensor *x, float eps);
/* compute layer_norm(x)*w+b. 'w' and 'b' must be both either != NULL
   or NULL. avg_flag = FALSE: rms_norm, avg_flag = TRUE:
   layer_norm. */
NCTensor *nc_fused_layer_norm(NCTensor *x, NCTensor *w, NCTensor *b,
                              float eps, BOOL avg_flag);
/* shift the column 'i' by 'pos + i * mult' elements and pad with with zeros */
NCTensor *nc_rel_shift(NCTensor *x, ssize_t pos, ssize_t mult);
/* complex multiplication of complex numbers. packed format: real part
   in even indices, imaginary part in odd indices. planar format: real
   part in first half of the vector, imaginary part in second half. x2
   is conjugated if 'is_conj' is true. */
NCTensor *nc_cmul(NCTensor *x1, NCTensor *x2, BOOL is_conj, BOOL is_planar);
NCTensor *nc_cmul_rotpos(NCTensor *x1, NCTensor *x2, BOOL is_conj);
/* x=(N, H, W, C) weight=(K, R, S, C), bias=(K) or NULL, result=(N, P, Q, K) */
NCTensor *nc_conv_2d(NCTensor *x, NCTensor *weight, NCTensor *bias,
                     int pad_left, int pad_right, int pad_top, int pad_bottom,
                     int stride_x, int stride_y, int dil_x, int dil_y);
/* x=(N, H, C) weight=(K, R, C), bias=(K), result=(N, P, K) */
NCTensor *nc_conv_1d(NCTensor *x, NCTensor *weight, NCTensor *bias,
                     int pad_left, int pad_right, int stride_x, int dil_x);
/* x=(N, P, Q, K) weight=(K, R, S, C), bias=(C), result=(N, H, W, C) */
NCTensor *nc_conv_transpose_2d(NCTensor *x, NCTensor *weight, NCTensor *bias,
                               int pad_left, int pad_right, int pad_top, int pad_bottom,
                               int stride_x, int stride_y, int dil_x, int dil_y,
                               int out_pad_x, int out_pad_y);
/* x=(N, P, K) weight=(K, R, C), bias=(C), result=(N, H, C) */
NCTensor *nc_conv_transpose_1d(NCTensor *x, NCTensor *weight, NCTensor *bias,
                               int pad_left, int pad_right,
                               int stride_x, int dil_x, int out_pad_x);

/* only for testing */
void nc_conv_force_im2col(NCContext *m, BOOL enable); 
NCTensor *nc_im2col(NCTensor *x, int P, int Q, int R, int S,
                    int pad_x, int pad_y, int stride_x, int stride_y,
                    int dil_x, int dil_y);
NCTensor *nc_col2im(NCTensor *x, int H, int W, int P, int Q, int R, int S,
                    int pad_x, int pad_y, int stride_x, int stride_y,
                    int dil_x, int dil_y);
/* x = (B, H, W, C) w, b = (C). C must be a multiple of num_groups */
NCTensor *nc_group_norm(NCTensor *x, NCTensor *w, NCTensor *b,
                        int num_groups, float eps);
/* upsample by duplicating the samples */
NCTensor *nc_upsample(NCTensor *x, int factor);
/* f8 conversion with automatic scaling */
typedef struct {
    NCTypeEnum type;
    int exp_margin;
    NCTensor *last_max;
    float cur_inv_scale; /* factor when converting to f8 */
    BOOL redo_flag;
    BOOL is_first;
    char *name;
} NCConvertToF8Context;

NCConvertToF8Context *nc_convert_to_f8_init(NCDevice *d, NCTypeEnum type, int exp_margin, BOOL redo_flag, const char *name);
void nc_convert_to_f8_update(NCConvertToF8Context *fcs);
void nc_convert_to_f8_end(NCConvertToF8Context *fcs);
NCTensor *nc_convert_to_f8(NCTensor *x, NCConvertToF8Context *fcs);

/* auto differentiation */

/* get_col_index is non NULL in the sparse gradient case */
typedef void NCParamUpdateFunc(void *opaque, NCTensor *grad,
                               NCTensor *get_col_index);

/* add a 'parameter' graph node to 'x' and return 'x'. */
NCTensor *nc_set_param(NCTensor *x, void *opaque);
/* return a new tensor with its graph removed */
NCTensor *nc_stop_grad(NCTensor *x);

/* manipulation of graph nodes */
NCNode *nc_dup_node(const NCNode *n);
void nc_free_node(NCNode *n);
void nc_combine_nodes(NCContext *m, NCNode **tab_op1, int count,
                      int axis, int elem_size, const size_t *tab_elem_size);
NCNode *nc_concat_node(NCContext *m, NCNode **inputs, int count,
                       int axis, const size_t *tab_size);
void nc_concat_optimization(NCContext *m, NCNode **concat_nodes, int count);
void nc_node_set_parent(NCNode *n, int arg_index, const NCNode *n1);
void nc_node_set_arg(NCNode *n, int arg_index, const NCTensor *x);
/* '...' represents n_args 'const NCTensor *' */
void nc_node_set_args(NCNode *n, int n_args, ...);

#define NC_BW_KEEP_GRAD_GRAPH (1 << 0)
/* optimize the nc_get_col() gradient */
#define NC_BW_SPARSE_GRAD     (1 << 1)
/* keep the saved arguments so that nc_bacward() can be called again */
#define NC_BW_KEEP_ARGS       (1 << 2)

void nc_backward(const NCTensor *x, NCTensor *grad,
                 NCParamUpdateFunc *param_update_func, int flags);
void nc_dump_graph(NCTensor *x);

/* tensor rematerialization */

/* return a new tensor with evaluation graph recording enabled. The
   operations generating values from this value are recorded so that
   they can be executed again to get the same value e.g. to save
   memory for back propagation. Not all operations support this
   feature. */
NCTensor *nc_tensor_enable_eval_graph(NCTensor *x);
/* disable the evaluation graph recording */
NCTensor *nc_tensor_stop_eval_graph(NCTensor *x);
NCLazyTensor *nc_lazy_tensor_from_tensor(const NCTensor *x);
NCTensor *nc_lazy_tensor_eval2(NCLazyTensor *lt, BOOL dump);
NCTensor *nc_lazy_tensor_eval(NCLazyTensor *lt);
void nc_free_lazy_tensor(NCLazyTensor *lt);

/* utilities for function parameters */

typedef struct {
    struct list_head link;
    NCTensor **pval; /* pointer to the tensor location */
    char *name; /* parameter name */
    NCTensor *saved_grad; /* debug */
    /* SGD opt data */
    struct SGDOptVarState *sgd_opt;
    float lr_mult; /* the global learning rate is multiplied by lr_mult */
    NCConvertToF8Context *fcs; /* only used with FP8 */
} NCParam;

typedef struct {
    NCContext *ctx;
    struct list_head param_list;
    BOOL add_graph;
} NCParamList;

void nc_param_list_init(NCContext *m, NCParamList *pl);
void nc_param_list_set_graph(NCParamList *pl, BOOL add_graph);
NCParam *nc_new_param_str(NCParamList *pl, NCTensor **pval, const char *str);
__attribute__((format(printf, 3, 4))) NCParam *nc_new_param(NCParamList *pl, NCTensor **pval, const char *fmt, ...);
void nc_param_list_end(NCParamList *pl);

NCParam *nc_find_param(NCParamList *pl, const char *name);
size_t nc_get_param_count(NCParamList *pl);

/* parameter file handling */
void nc_save_param_header(FILE *f, const char *config);
void nc_save_param(FILE *f, const NCTensor *v1, const char *name);
void nc_save_coefs(NCParamList *pl, const char *filename);
/* return a pointer to the allocated configuration or NULL if
   error. Use nc_free() to free the configuration data. */
char *nc_load_param_header(FILE *f);
/* return NULL if EOF. 'name' is filled with the saved parameter name. */
NCTensor *nc_load_param(NCContext *m, FILE *f, char *name, size_t name_size);
void nc_load_coefs(NCParamList *pl, const char *filename);

/* SGD optimizer */

typedef enum {
    SGD_OPT_BASIC,
    SGD_OPT_ADAM,
    SGD_OPT_TEST,
} SGDOptAlgoEnum;

typedef struct {
    SGDOptAlgoEnum algo;
    union {
        struct {
            float beta1;
            float beta2;
            float eps;
            float gradient_clip; /* if != 0, per parameter gradient clipping */
            float weight_decay;
            float loss_scale;
        } adam;
    } u;
    float lr;
} SGDOptParams;

NCSGDOptState *nc_sgd_opt_init(NCContext *m, const SGDOptParams *p);
void nc_sgd_opt_end(NCSGDOptState *s);
void sgd_opt_update_var(void *opaque, NCTensor *yg, NCTensor *get_col_index);

/* set the SGD optimizer 's' to all parameters of the model */
void nc_sgd_opt_set_all(NCParamList *param_list, NCSGDOptState *s);

/* set the SGD optimizer 's' to the variable 'x'. Remove it if s = NULL */
void nc_sgd_opt_set(NCParam *x, NCSGDOptState *s);
void nc_sgd_opt_update(NCSGDOptState *s);
/* force the learning rate */
void nc_sgd_opt_set_lr(NCSGDOptState *s, float lr);
float nc_sgd_opt_get_lr(NCSGDOptState *s);
void nc_sgd_opt_set_step(NCSGDOptState *s, int64_t step);

/* for SGD_OPT_TEST */
NCTensor *nc_sgd_opt_get_grad(NCParam *p);

void nc_save_param_opt(FILE *f, const NCParam *p);
void nc_update_param(NCParamList *pl, const char *name, const NCTensor *v);

/* misc utilities (to be removed) */

typedef struct {
    uint32_t x, y, z, w, v, d;
    /* used by Gaussian generator */
    int idx;
    float y1;
} RNDState;

void rnd_init(RNDState *s, uint32_t seed);
uint32_t rnd_unif_u32(RNDState *s);
float rnd_unif(RNDState *s);
float rnd_gaussian(RNDState *s);
void rnd_unif_vec(float *tab, size_t n, float mu, float range,
                  RNDState *s);
void rnd_unif_mat(float *tab, size_t stride, size_t h, size_t w,
                  float mu, float sigma, RNDState *s);

float vec_sum_f32(const float *tab, size_t n);
float vec_max_f32(const float *tab, size_t n);
float vec_logsumexp_f32(const float *tab, int n);

typedef struct  {
    float val;
    uint32_t idx;
} NCTopKEntry;

/* Return the k largest values among prob[0...n_symb-1]. The floating
   point numbers are sorted according to their binary
   representation. 'tab' must be freed with nc_free(). */
int nc_topk(NCTopKEntry **ptab, const float *prob, size_t n_symb, int topk);
/* same as nc_topk() but compare against 'dist' and stop returning
   values with their sum is larger or equal to 'topp' */
int nc_topkp(NCTopKEntry **pout_tab, double *psum,
             const float *tab, const float *dist, size_t n_symb,
             int topk, float topp);


/* allocate the whole device memory for this task */
#define NC_DEVICE_PARAM_FULL_MEM 0 
/* set device specific options */
int nc_set_device_param(NCDevice *d, int param, int64_t val);
/* use it before nc_new_cuda_device() to make cublasLt optional */
void nc_set_cublasLt_required(BOOL val);

/* apply a forward function with custom backward function */

typedef NCTensor *NCForwardFunction(NCTensor **inputs, int n_inputs,
                                    NCNode *n, void *opaque);
typedef void NCBackwardFunction(NCTensor **grad_outputs, int n_outputs,
                                NCTensor *grad, NCTensor **args, int n_args,
                                void *opaque);
NCTensor *nc_apply(NCTensor **inputs, int n_inputs,
                   NCForwardFunction *func, NCBackwardFunction *bw_func,
                   void *opaque);

#endif /* LIBNC_H */
