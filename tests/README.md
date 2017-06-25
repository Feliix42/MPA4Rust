# Test Cases

This folder contains a number of test cases, constructed to check whether the IR tracer is able to track down all the various ways how communication between two actors could be implemented. Currently existing test cases:

- everything in one binary, direct channel construction and message passing: `weatherstation`
- functionality distributed across many crates, direct channel construction and message passing: `distributed_station`
- functionality distributed across many crates, direct and indirect message passing (using callbacks): `callback_station`

## How to build the tests

To build the tests and extract the **LLVM IR**, run the following command in the _weatherstation_ folder of each example:

```
$ RUSTFLAGS="--emit=llvm-ir" cargo build
```

This will emit the LLVM IR for each crate during compile time, which can be found in the `target/debug/deps` folder.
