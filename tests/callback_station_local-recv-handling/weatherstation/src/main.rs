use std::sync::mpsc::channel;

extern crate base_station;
extern crate weather;

use weather::Weather;
use base_station::{Weatherstation, ControlMessage};

fn main() {
    let weather_sender = Weatherstation::deploy("Test Station");

    let msg = ControlMessage::Record(Weather::Cloudy);
    let _ = weather_sender.send(msg);

    let (sender, receiver) = channel();
    let _ = weather_sender.send(ControlMessage::Shutdown(sender));
    let _ = receiver.recv();
}
