/* example of JS module */
import * as module from './fib_module.js';
import * as module_a from './a/fib_module.js';
import * as module_b from './b/fib_module.js';

console.log(`fib(10)=${module.fib(10)}!`);
console.log(`fib(10)=${module_a.fib(10)}!`);
console.log(`fib(10)=${module_b.fib(10)}!`);
