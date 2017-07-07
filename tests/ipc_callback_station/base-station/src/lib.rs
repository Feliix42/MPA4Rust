use std::thread;

#[macro_use]
extern crate serde_derive;
extern crate ipc_channel;
extern crate weather;
use ipc_channel::ipc;
use weather::Weather;

#[derive(Debug)]
pub struct Weatherstation {
    name: &'static str,
    receiver: ipc::IpcReceiver<ControlMessage>,
}

#[derive(Debug,Serialize,Deserialize)]
pub enum ControlMessage {
    Record(Weather),
    Shutdown(ipc::IpcSender<()>),
}

impl Weatherstation {
    pub fn deploy(name: &'static str) -> ipc::IpcSender<ControlMessage> {
        let (chan, recv) = ipc::channel().unwrap();
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
