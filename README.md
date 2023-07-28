# ylc
simple language for learning purposes

using LLVM C API as a backend

# compile ylc
`mkdir -p build && make`

# run
- as repl: `./build/lang`
- with file input: `./build/lang test.ylc`

# compile .ylc to executable
use the ./ylcc script to compile an .ylc source file to executable

- compile source: `./ylcc <example>.ylc`
- ./ylcc script will compile a source file to LLVM IR bitcode,
emit an object file with llc and finally link with clang
- the executable will be named just '<example>'


# run tests
`make build/test_parser`

# DONE:
- passing callbacks from ylc to C functions
- closures
- custom type declarations / type aliases
- structs

# TODOs:
- [ ] assign result of function calls / instructions to global vars 
- [ ] type-checking / inference before codegen
- [ ] tuple dynamic index addressing
- [ ] arrays
- [ ] more sophisticated pattern-matching
- [ ] currying
