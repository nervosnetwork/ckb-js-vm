/* example of JS module */

ckb.mount(2, ckb.SOURCE_CELL_DEP)

import('./fib_module.js')
    .then((module) => {
        console.log("fib(10)=", module.fib(10))
    })
    .catch((err) => {
        console.log(err)
    });
