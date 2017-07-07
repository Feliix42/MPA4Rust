#[macro_use]
extern crate serde_derive;

#[derive(Debug, Serialize, Deserialize)]
pub enum Weather {
    Sunny,
    Cloudy,
    Thunder,
}
