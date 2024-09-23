# BFInterpreter
An interpreter for BF language.
## Build
Clone the reposity. If you want to clone brainfuck-benchmark as well, please use
```bash
git clone --recursive 
```
Under project root, create a new folder
```
mkdir build
```
Use CMake to generate the build system for your platform
```
cd build
cmake ..
```
## Usage
### Interpreter
```bash
bfi [-p] <filename> // -p enable profile
```
### Compiler
Work on MacOS with m1 chip(armv8-64bit)
```bash
bfc [-p] <filename> // generate output.s
```
Link generated assemly with c_main.c
```bash
gcc -O3 ../compiler/c_main.c output.s -o exe_name
```
Run executable
```bash
./exe_name
```
