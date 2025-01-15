/* example of JS module */
import * as module from './fib_module.js';
import * as subdir_module from './subdir/fib_module.js';

console.log(`fib(10)=${module.fib(10)}!`);
console.log(`fib(10)=${subdir_module.fib(10)}!`);
