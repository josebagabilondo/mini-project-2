USE iotDB;

CREATE TABLE IF NOT EXISTS pressure_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pressure FLOAT,
    ts TIMESTAMP
);

CREATE TABLE IF NOT EXISTS temperature_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    temperature FLOAT,
    ts TIMESTAMP
);

CREATE TABLE IF NOT EXISTS light_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    light INT,
    ts TIMESTAMP
);
