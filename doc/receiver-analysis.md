# Analyzing the receiver half of a channel

## Introductory notes

Mostly, receiving messages in Servo is handled by using a `match` statement with an arm for every possible message type. 
In LLVM IR, this then looks something like this:

```llvm
switch i64 %16, label %bb11 [
  i64 0, label %bb9
  i64 1, label %bb10
], !dbg !9468
```

_Note, that some type casting and unwrapping usually occurs beforehand._

The example above resembles the following code snippet:

```rust
match msg {
    Weather::Sunny   => println!("[{}] The sun is shining!", station.name),
    Weather::Cloudy  => println!("[{}] It's cloudy today.", station.name),
    Weather::Thunder => println!("[{}] Better seek shelter!", station.name),
}
```

One can observe that the last option (here: `Thunder`) is used in the LLVM IR as the fallback option and not explicitly specified in the switch options. This is most likely because often, the last statement in Rust would also be the fallback statement (`_`).  
In order to trace the relevant basic blocks, one could do the following:

- try to handle any unwrapping etc. on the way to the first switch statement
- when the value is used in a switch statement, branch the analysis and continue the search for every arm separately
  - for each arm, identify the basic blocks that follow after the `switch`
  - extract the relevant lines in the original source code, if possible _(this did not work too well last time, make sure Servo has this kind of debug information)_

I would also want to take the analysis one step deeper.
Because in case of the Constellation, the received message is a container type that encapsulates the various message types received.
Therefore, it would be helpful to have a look at the deeper workings of a received message
