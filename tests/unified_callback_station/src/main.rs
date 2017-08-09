use std::sync::mpsc::channel;

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


mod weather {
    #[derive(Debug)]
    pub enum Weather {
        Sunny,
        Cloudy,
        Thunder,
    }
}


mod base_station {
    use std::sync::mpsc;
    use std::thread;

    use weather::Weather;

    #[derive(Debug)]
    pub struct Weatherstation {
        name: &'static str,
        receiver: mpsc::Receiver<ControlMessage>,
    }

    pub enum ControlMessage {
        Record(Weather),
        Shutdown(mpsc::Sender<()>),
    }

    impl Weatherstation {
        pub fn deploy(name: &'static str) -> mpsc::Sender<ControlMessage> {
            let (chan, recv) = mpsc::channel();
            let station = Weatherstation {
                name: name,
                receiver: recv,
            };

            thread::spawn(move || {
                let mut cont = true;
                loop {
                    match station.receiver.try_recv() {
                        Ok(msg) => cont = Weatherstation::handle_message(msg, station.name),
                        Err(_) => (),
                    }

                    // stop the thread after receiving the shutdown signal
                    if !cont {
                        break;
                    }
                }
            });
            chan
        }

        pub fn handle_message(msg: ControlMessage, name: &str) -> bool {
            match msg {
                ControlMessage::Record(w) => {
                    println!("[{}] The weather today: {:?}", name, w);
                    true
                }
                ControlMessage::Shutdown(sender) => {
                    println!("[{}] Shutting down.", name);
                    let _ = sender.send(());
                    false
                }
            }
        }
    }
}
