# esp32-cam-mjpeg
A simple MJPEG streaming server for ESP32-CAM

## Features

- Streams MJPEG video at `/stream`
- Health check at `/` just returns `OK`

## Prerequisites
- ESP32-CAM
- ESP32-CAM-MB(CH340)
- OV2640/OV3660 or else

## Getting Started

### 0. Clone repository
```bash
git clone https://github.com/stevejkang/esp32-cam-mjpeg ESP32CamMJPEG
```

### 1. Configure WiFi

Copy `secrets.h.example` to `secrets.h` and fill in your WiFi credentials:

```bash
cp secrets.h.example secrets.h
```

Set your `WIFI_SSID` and `WIFI_PASSWORD` in `secrets.h`.

### 2. Usage

- On boot, serial monitor will print the stream URL (like `http://192.168.x.x/stream`)
- Open the stream link in VLC, Chrome, or any MJPEG-compatible viewer.

## License

MIT
