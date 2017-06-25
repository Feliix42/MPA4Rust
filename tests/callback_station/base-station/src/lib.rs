use std::sync::mpsc;
use std::thread;

extern crate weather;
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
                    Err(_)  => (),
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
