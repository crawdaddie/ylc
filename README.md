# ylc
simple language for learning purposes, originally conceived as an interface for https://github.com/crawdaddie/yalce audio library
bootstrapped in C (self-hosting probably way out of scope)

uses LLVM C API as a backend to generate LLVM IR, and to implement a JIT for the REPL

# examples
```javascript
let m = fn (int val) int {
  match val -> int
  | 1 -> 1 
  | 2 -> 2
  | _ -> m(val - 2)
}

m(5)
```

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
- the executable will be named just '<example>'


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
- https://github.com/k-mrm/type-inference
- https://github.com/semahawk/type-inference
- https://cs3110.github.io/textbook/chapters/interp/inference.html
