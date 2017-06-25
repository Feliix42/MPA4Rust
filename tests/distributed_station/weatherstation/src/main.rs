use std::io;

extern crate base_station;
extern crate weather;

use weather::Weather;
use base_station::Weatherstation;

fn main() {
    let weather_sender = Weatherstation::deploy("Test Station");

    let msg = Weather::Cloudy;
    let _ = weather_sender.send(msg);

    let mut input = String::new();
    let _ = io::stdin().read_line(&mut input);
}
