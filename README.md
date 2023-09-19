# capuchin-comp

Capuchin is a compiler for the Monkey programming language, written entirely in C. The implementation is based on Thorsten Ball's followup book ['Writing A Compiler In Go'](https://compilerbook.com/).

The lexer and parser implementations have been reused from ['capuchin-interp'](https://github.com/ConstantinNicula/capuchin-interp).

### How does  it work?
TO DO: 

## Building
A `Makefile` is provided in the root directory of the repo. It relies on gcc for compiling and has only been tested on Ubuntu so your milage may vary if you intend to build for another target. 

The following make commands are provided: 
- `make clean` 
- `make test` - run test cases and produce report 
- `make repl` - build the REPL
