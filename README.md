# Message-Passing Analyzer for Rust programs

This repository contains a small tool to analyze message passing in multi-threaded applications written in the Rust programming language.


## Building
You can build the project by simply running `make all` within the repository. 

### Requirements
- You need **LLVM 4.0** installed on your system and in your path for this to work, since the project uses many libraries provided by LLVM.
- The [`boost` Filesystem libraries](http://www.boost.org/doc/libs/1_64_0/libs/filesystem/doc/index.htm) are used for filesystem interaction. Make sure they are installed.


## Preparing the source code to be analyzed
To analyze the source code, you need to tell `cargo` to emit LLVM-IR during compilation.
You can do so by running 

```
$ RUSTFLAGS="--emit=llvm-ir" cargo build
```

This should work with stable Rust version 1.19.0 or greater, as they are using LLVM 4.0. If you are not sure whether your Rust version will work, check the LLVM version used by running `rustc -vV`.

