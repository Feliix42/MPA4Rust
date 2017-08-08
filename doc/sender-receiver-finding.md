# Deconstructing sent and received messages

## Message passing using [MPSC channels](https://doc.rust-lang.org/std/sync/mpsc/index.html)

### Sending Messages:

Example of a [`send`](https://doc.rust-lang.org/std/sync/mpsc/struct.Sender.html) instruction in the LLVM IR, generated from the simple `weatherstation` example:

```llvm
  %2 = invoke i16 @"_ZN41_$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$4send17h214a63b9db910dcfE"(%"std::sync::mpsc::Sender<weather::Weather>"* dereferenceable(16) %weather_sender, i8 %1)
          to label %bb4 unwind label %cleanup, !dbg !711
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN41_$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$4send17h214a63b9db910dcfE` |
| _Demangled_ | `_$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$::send::h214a63b9db910dcf` |
| Arguments | `%"std::sync::mpsc::Sender<weather::Weather>"* %0` <br /> `i8 %1` |


---

Another, slightly different, example a [`send`](https://doc.rust-lang.org/std/sync/mpsc/struct.Sender.html) instruction from the `callback_station` example:

```llvm
  invoke void @"_ZN41_$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$4send17h4a80dbc7d26f6fa3E"(%"core::result::Result<(), std::sync::mpsc::SendError<base_station::ControlMessage>>"* noalias nocapture sret dereferenceable(32) %_4, %"std::sync::mpsc::Sender<base_station::ControlMessage>"* dereferenceable(16) %weather_sender, %"base_station::ControlMessage"* noalias nocapture dereferenceable(24) %_6)
          to label %bb5 unwind label %cleanup, !dbg !922
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN41_$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$4send17h4a80dbc7d26f6fa3E` |
| _Demangled_ | `_$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$::send::h4a80dbc7d26f6fa3` |
| Arguments | `%"core::result::Result<(), std::sync::mpsc::SendError<base_station::ControlMessage>>"* %0` <br /> `%"std::sync::mpsc::Sender<base_station::ControlMessage>"* %1` <br /> `%"base_station::ControlMessage"* %2` |

**TODO:** Why is this different?

My assumption is, that the upper example hat the `Result` type inlined as integer and returns it immedieately, while the second example takes the `Result` type as argument and is marked as _void_. 

### Receiving Messages:

Example of a [**`recv`**](https://doc.rust-lang.org/std/sync/mpsc/struct.Receiver.html#method.recv) instruction in LLVM IR, generated from the `callback_station` example:

```llvm
  %22 = invoke i8 @"_ZN43_$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$4recv17hcd5ffbc1b3b64e91E"(%"std::sync::mpsc::Receiver<()>"* dereferenceable(16) %receiver)
          to label %bb13 unwind label %cleanup4, !dbg !929
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN43_$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$4recv17hcd5ffbc1b3b64e91E` |
| _Demangled_ | `_$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::recv::hcd5ffbc1b3b64e91` |
| Arguments | `%"std::sync::mpsc::Receiver<()>"* %0` |


---

Example of a [**`try_recv`**](https://doc.rust-lang.org/std/sync/mpsc/struct.Receiver.html#method.try_recv) instruction in LLVM IR, generated from the `callback_station` example:

```llvm
  invoke void @"_ZN43_$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$8try_recv17hdc4488b393eb5f29E"(%"core::result::Result<ControlMessage, std::sync::mpsc::TryRecvError>"* noalias nocapture sret dereferenceable(32) %_5, %"std::sync::mpsc::Receiver<ControlMessage>"* dereferenceable(16) %5)
          to label %bb4 unwind label %cleanup, !dbg !967
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN43_$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$8try_recv17hdc4488b393eb5f29E` |
| _Demangled_ | `_$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::try_recv::hdc4488b393eb5f29` |
| Arguments | `%"core::result::Result<ControlMessage, std::sync::mpsc::TryRecvError>"* %0` <br /> `%"std::sync::mpsc::Receiver<ControlMessage>"* %1` |

Again, although both versions of receive should return `Result` data types, the first example only takes one argument and returns an `i8` while the second example invokes the function with 2 arguments (one being the wanted `Result` type, but marks the return type as _void_.

## Message passing using [IPC channels](https://docs.rs/ipc-channel/)

### Sending Messages:

Example of a [`send`](https://docs.rs/ipc-channel/0.8.0/ipc_channel/ipc/struct.IpcSender.html#method.send) instruction in LLVM IR, generated from the `ipc_callback_station` example:

```llvm
  %11 = invoke i8* @"_ZN45_$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$4send17h02eb4d04f928232fE"(%"ipc_channel::ipc::IpcSender<base_station::ControlMessage>"* noalias readonly dereferenceable(4) %weather_sender, i64 %10)
          to label %bb5 unwind label %cleanup, !dbg !228
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN45_$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$4send17h02eb4d04f928232fE` |
| _Demangled_ | `_$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$::send::h02eb4d04f928232f` |
| Arguments | `%"ipc_channel::ipc::IpcSender<base_station::ControlMessage>"* %0` <br /> `i64 %1` |


### Receiving Messages: 

Example of a [**`recv`**](https://docs.rs/ipc-channel/0.8.0/ipc_channel/ipc/struct.IpcReceiver.html#method.recv) instruction in LLVM IR, generated from the `ipc_callback_station` example:

```llvm
  %34 = invoke i8* @"_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$4recv17h8019869a577223a5E"(%"ipc_channel::ipc::IpcReceiver<()>"* dereferenceable(4) %receiver)
          to label %bb14 unwind label %cleanup5, !dbg !235
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$4recv17h8019869a577223a5E` |
| _Demangled_ | `_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::recv::h8019869a577223a5` |
| Arguments | `%"ipc_channel::ipc::IpcReceiver<()>"* %0` |


---

Example of a [**`try_recv`**](https://docs.rs/ipc-channel/0.8.0/ipc_channel/ipc/struct.IpcReceiver.html#method.try_recv) instruction in LLVM IR, generated from the `ipc_callback_station` example:

```llvm
  invoke void @"_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$8try_recv17h418f0f08d4372e1cE"(%"core::result::Result<ControlMessage, alloc::boxed::Box<bincode::internal::ErrorKind>>"* noalias nocapture sret dereferenceable(16) %_5, %"ipc_channel::ipc::IpcReceiver<ControlMessage>"* dereferenceable(4) %5)
          to label %bb4 unwind label %cleanup, !dbg !300
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$8try_recv17h418f0f08d4372e1cE` |
| _Demangled_ | `_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::try_recv::h418f0f08d4372e1c` |
| Arguments | `%"core::result::Result<ControlMessage, alloc::boxed::Box<bincode::internal::ErrorKind>>"* %0` <br /> `%"ipc_channel::ipc::IpcReceiver<ControlMessage>"* %1` |


## Consequences

To find a send/recv, check the demangled function names and compare them to a substring of the demangled names displayed above. Remember that you might have to **handle `recv` and `try_recv` differently**.


As it seems, the `Sender` or `Receiver` is always passed as an argument to the function, but unfortunately there is no convention as to which position it has.

The most logical (and easy) thing to do seems to be to get the Pointer to the type from the _argument list_, dereference it, get the struct name and store it as "type information" in a string.
To find the correct argument, search all arguments until you find one matching a string like `ipc_channel::ipc::IpcReceiver<`.

