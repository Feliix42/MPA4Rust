use std::sync::mpsc;
use std::thread;
use weather::Weather;

#[derive(Debug)]
pub struct Weatherstation {
    name: &'static str,
    receiver: mpsc::Receiver<Weather>,
}

impl Weatherstation {
    pub fn deploy(name: &'static str) -> mpsc::Sender<Weather> {
        let (chan, recv) = mpsc::channel();
        let station = Weatherstation {
            name: name,
            receiver: recv,
        };

        thread::spawn(move || loop {
                          match station.receiver.try_recv() {
                              Ok(msg) => {
                                  println!("[{}] The weather today: {:?}", station.name, msg)
                              }
                              Err(_) => (),
                          }
                      });
        chan
    }
}
