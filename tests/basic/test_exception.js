import * as misc from "@ckb-js-std/bindings";

function main_1() {
    console.log("main_1() function");
    main_2();
}
function main_2() {
    console.log("main_2() function");
    main_3();
}
function main_3() {
    console.log("main_3() function");
    misc.throw_exception("Exception in main_3() function");
}

function main() {
    console.log("main() function");
    main_1();
}

main();
