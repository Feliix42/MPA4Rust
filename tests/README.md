# Test Cases

This folder contains a number of test cases, constructed to check whether the IR tracer is able to track down all the various ways how communication between two actors could be implemented. Currently existing test cases:

| example | # crates | message passing | channel type |
| --- | --- | --- | --- |
| `weatherstation` | 1 | direct | [`mpsc`](https://doc.rust-lang.org/stable/std/sync/mpsc/index.html) |
| `distributed_station` | many | direct | [`mpsc`](https://doc.rust-lang.org/stable/std/sync/mpsc/index.html) |
| `callback_station` | many | indirect | [`mpsc`](https://doc.rust-lang.org/stable/std/sync/mpsc/index.html) |
| `callback_station_local-recv-handling` | many | indirect | [`mpsc`](https://doc.rust-lang.org/stable/std/sync/mpsc/index.html) |
| `unified_callback_station` | 1 | indirect | [`mpsc`](https://doc.rust-lang.org/stable/std/sync/mpsc/index.html) |
| `ipc_callback_station` | many | indirect | [`ipc`](https://docs.rs/ipc-channel/) |

_Direct_ message passing means, that all channels are allocated statically and the sender/receiver halves are part of the threads' internal state.
_Indirect_ message passing refers to the use of ad-hoc allocated channels where one of the halves is sent via message to another thread which uses the received half to interact with the first thread (usually to communicate the result of a computation or acknowledge the receive of a certain message). "indirect" indicates in the table above the use of both channel types.

## How to build the tests

To build the tests and extract the **LLVM IR**, run the following command in the _weatherstation_ folder of each example:

```
$ RUSTFLAGS="--emit=llvm-ir" cargo build
```

This will emit the LLVM IR for each crate during compile time, which can be found in the `target/debug/deps` folder.
