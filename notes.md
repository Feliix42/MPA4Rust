# Notes

## Message passing using [MPSC channels](https://doc.rust-lang.org/std/sync/mpsc/index.html)

### Sending Messages:


### Receiving Messages:


## Message passing using [IPC channels](https://docs.rs/ipc-channel/)

### Sending Messages:

Example of a [`send`](https://docs.rs/ipc-channel/0.8.0/ipc_channel/ipc/struct.IpcSender.html#method.send) instruction in LLVM IR, generated from the `ipc_callback_station` example:

```
  %11 = invoke i8* @"_ZN45_$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$4send17h02eb4d04f928232fE"(%"ipc_channel::ipc::IpcSender<base_station::ControlMessage>"* noalias readonly dereferenceable(4) %weather_sender, i64 %10)
          to label %bb5 unwind label %cleanup, !dbg !228
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN45_$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$4send17h02eb4d04f928232fE` |
| _Demangled_ | `_$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$::send::h02eb4d04f928232f` |
| Arguments | `%"ipc_channel::ipc::IpcSender<base_station::ControlMessage>"* %0` <br /> `i64 %1` |


### Receiving Messages: 

Example of a [`recv`](https://docs.rs/ipc-channel/0.8.0/ipc_channel/ipc/struct.IpcReceiver.html#method.recv) instruction in LLVM IR, generated from the `ipc_callback_station` example:

```
  %34 = invoke i8* @"_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$4recv17h8019869a577223a5E"(%"ipc_channel::ipc::IpcReceiver<()>"* dereferenceable(4) %receiver)
          to label %bb14 unwind label %cleanup5, !dbg !235
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$4recv17h8019869a577223a5E` |
| _Demangled_ | `_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::recv::h8019869a577223a5` |
| Arguments | `%"ipc_channel::ipc::IpcReceiver<()>"* %0` |


---

Example of a [`try_recv`](https://docs.rs/ipc-channel/0.8.0/ipc_channel/ipc/struct.IpcReceiver.html#method.try_recv) instruction in LLVM IR, generated from the `ipc_callback_station` example:

```
  invoke void @"_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$8try_recv17h418f0f08d4372e1cE"(%"core::result::Result<ControlMessage, alloc::boxed::Box<bincode::internal::ErrorKind>>"* noalias nocapture sret dereferenceable(16) %_5, %"ipc_channel::ipc::IpcReceiver<ControlMessage>"* dereferenceable(4) %5)
          to label %bb4 unwind label %cleanup, !dbg !300
```

| Property | Value |
| -------- | ----- |
| Called Function | `_ZN47_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$8try_recv17h418f0f08d4372e1cE` |
| _Demangled_ | `_$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::try_recv::h418f0f08d4372e1c` |
| Arguments | `%"core::result::Result<ControlMessage, alloc::boxed::Box<bincode::internal::ErrorKind>>"* %0` <br /> `%"ipc_channel::ipc::IpcReceiver<ControlMessage>"* %1` |


