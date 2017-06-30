# Message-Passing Analyzer for Rust programs

This repository contains a small tool to analyze message passing in multi-threaded applications written in the Rust programming language.


## Building
You can build the project by simply running `make all` within the repository. 

_Please note:_ You need **LLVM 4.0** installed on your system and in your path for this to work, since the project uses many libraries provided by LLVM.


## Preparing the source code to be analyzed
To analyze the source code, you need to tell `cargo` to emit LLVM-IR during compilation.
You can do so by running 

```
$ RUSTFLAGS="--emit=llvm-ir" cargo build
```

Currently, this works only with Rust Nightly, since Stable Rust uses LLVM 3.9, but we require version 4.0. for the Intermediate Representations to be compatible.

