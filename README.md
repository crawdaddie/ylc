# ylc
simple language for learning purposes, originally conceived as an interface for https://github.com/crawdaddie/yalce audio library

bootstrapped in C (self-hosting way out of scope for now)

uses the LLVM C API to compile to native code via a JIT compiler


# examples
```javascript
# functions / pattern matching
let m = fn (int val) int {
  match val -> int
  | 1 -> 1 
  | 2 -> 2
  | _ -> -1
}

m(5)

# extern fn declaration matches printf from <stdio.h>
let printf = extern fn (str input) int

# randint randfloat declared in .so library loaded at runtime
import "libffi.so"
let randint = extern fn (int max) int
let randfloat = extern fn (double max) double

# recursion (LLVM optimisation pass converts generated code to tail-calls where possible)
let fib = fn (int n) int {
  match n
  | 0 -> 0
  | 1 -> 1
  | _ -> fib(n - 1) + fib(n - 2)
}

fib(10)

# type declaration
type Point = struct (
  double x,
  double y,
)

let Point x = (
  x = 2.0,
  y = 1.0,
)


```
# dependencies
- LLVM (after installing llvm you may need to add the llvm headers & libraries to your CPATH & LIBRARY_PATH)

# compile ylc
`mkdir -p build && make`

# run
- as repl: `./build/lang`
- with file input: `./build/lang test.ylc`
- with both: `./build/lang test.ylc -r` (runs the input file before starting the repl in the same environment)

# compile .ylc to executable
use the ./ylcc script to compile an .ylc source file to executable

- compile source: `./ylcc <example>.ylc`
- ./ylcc script will compile a source file to LLVM IR bitcode,
emit an object file with llc and finally link with clang
- the resulting executable will be named just `<example>`


# run tests
`make build/test_parser`

`make build/test_codegen`


# DONE:
- REPL (`./build/lang -r`)
- C ffi capability using `extern` keyword
- passing callbacks from ylc to C functions
- closures
- custom type declarations / type aliases
- structs
- import statements (no namespaces yet, symbols are dumped into the calling module)

# TODOs:
- [ ] assign result of function calls / instructions to global vars 
- [ ] type-checking / inference before codegen
- [ ] tuple dynamic index addressing
- [ ] arrays
- [ ] more sophisticated pattern-matching
- [ ] currying
- [ ] namespaced modules

# REFERENCES
- https://craftinginterpreters.com/a-bytecode-virtual-machine.html
- http://lucacardelli.name/Papers/BasicTypechecking.pdf
- https://github.com/k-mrm/type-inference
- https://cs3110.github.io/textbook/chapters/interp/inference.html
