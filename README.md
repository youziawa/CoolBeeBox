# CoolBeeBox

CoolBeeBox is an ESP32-S3 smart beehive monitoring system. It combines
environment sensing, on-device bee-audio inference, MQTT telemetry, and a Vue
dashboard for remote monitoring.

## Repository layout

- `firmware/environment-monitor/` — ESP-IDF firmware for BME680 and SCD40
  sensing, Wi-Fi connectivity, MQTT telemetry, and temperature control.
- `firmware/edge-ai/` — ESP-IDF firmware for INMP441 audio capture and
  Edge Impulse inference. Its generated inference SDK is included in
  `coolbeebox-cpp-mcu-v1-impulse-#1/` because the firmware CMake project uses it.
- `frontend/` — Vue 3 + Vite dashboard with real-time MQTT data, history, and
  AI-analysis views.
- `backend/` — backend placeholder; the supplied API directory had no source.

## Security configuration

No working credentials are stored in this repository. Configure the firmware at
build time with `MY_WIFI_SSID`, `MY_WIFI_PASS`, `COOLBEE_MQTT_URI`,
`COOLBEE_MQTT_USERNAME`, and `COOLBEE_MQTT_PASSWORD`. The frontend's MQTT
password starts blank and can be set through its configuration UI.

## Build notes

Both firmware projects target ESP-IDF. Before building, install ESP-IDF and set
the required Wi-Fi and MQTT compiler definitions for your environment. The
`managed_components/` directories are intentionally excluded; ESP-IDF restores
them from each project's `dependencies.lock` file.

The frontend can be run from `frontend/` with the normal Node.js workflow:
`npm install` followed by `npm run dev`.
