extern crate ipc_channel;
extern crate base_station;
extern crate weather;

use weather::Weather;
use base_station::{Weatherstation, ControlMessage};
use ipc_channel::ipc::channel;

fn main() {
    let weather_sender = Weatherstation::deploy("Test Station");

    let msg = ControlMessage::Record(Weather::Cloudy);
    let _ = weather_sender.send(msg);

    let (sender, receiver) = channel().unwrap();
    let _ = weather_sender.send(ControlMessage::Shutdown(sender));
    let _ = receiver.recv();
}
