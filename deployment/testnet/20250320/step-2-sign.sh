#!/bin/bash

ckb-cli deploy sign-txs \
    --from-account ckt1qzda0cr08m85hc8jlnfp3zer7xulejywt49kt2rr0vthywaa50xwsqvuyep8kue7r8mhfe6rzguzj3kmj4yqjwsg8lepn \
    --add-signatures \
    --info-file info.json
