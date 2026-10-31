# IoE Smart Security System

An **Internet of Everything (IoE)-based security monitoring prototype** that combines sensor-based intrusion detection with a web dashboard. The project uses an ESP32 simulated in Wokwi to collect sensor readings, activate local alarms, and transmit telemetry to a React-based dashboard.

## Project Overview

The system monitors motion, object proximity, temperature, and humidity. When the system is armed, it activates a local alarm if motion is detected or an object is detected within the configured distance threshold.

Sensor readings and system status are transmitted to a web application, where the latest telemetry can be viewed through a dashboard.

## Features

- **Motion detection:** Uses a PIR sensor to detect movement.
- **Distance monitoring:** Uses an HC-SR04 ultrasonic sensor to measure proximity.
- **Environmental monitoring:** Uses a DHT22 sensor to measure temperature and humidity.
- **Local alarm system:** Controls a buzzer and red/green LEDs according to the alarm state.
- **ESP32 connectivity:** Sends sensor telemetry over Wi-Fi using HTTPS.
- **Web dashboard:** Displays incoming telemetry through a React and TypeScript interface.
- **Wokwi simulation:** Allows the virtual circuit and ESP32 firmware to be tested without physical hardware.

## Technologies Used

| Component | Technology |
|---|---|
| Embedded controller | ESP32 |
| Circuit simulation | Wokwi |
| Sensors | PIR, DHT22, HC-SR04 |
| Programming language | Arduino C++ |
| Frontend | React, TypeScript, CSS |
| Development server and API bridge | Vite |
| Communication format | JSON |
| Network communication | Wi-Fi and HTTPS |

## Project Structure

```text
IoE-Smart-Security-System/
├── dashboard/
│   ├── src/
│   ├── public/
│   ├── package.json
│   ├── vite.config.ts
│   └── ...
├── wokwi/
│   ├── sketch.ino
│   ├── diagram.json
│   ├── libraries.txt
│   └── wokwi-project.txt
└── README.md
```

*The structure above assumes the dashboard and Wokwi files are stored in separate folders as shown.*

## System Architecture

1. The ESP32 reads data from the DHT22, PIR, and HC-SR04 sensors.
2. The firmware evaluates the alarm condition when the system is armed.
3. The red LED and buzzer activate when the alarm condition is met; otherwise, the green LED indicates the non-alarm state.
4. The ESP32 packages sensor readings and system status into a JSON payload.
5. The payload is sent to the dashboard's telemetry API over HTTPS.
6. The dashboard retrieves the latest available telemetry and updates its interface.

**Alarm condition:** The system is armed and either motion is detected or the measured distance is less than 30 cm.

## Getting Started

### 1. Run the Wokwi Simulation

Open the Wokwi project:

**[Open the Wokwi Simulation](https://wokwi.com/projects/478881029364667393)**

The simulation contains the ESP32, sensors, LEDs, and buzzer.

The firmware's telemetry endpoint must point to a reachable API endpoint. If using a temporary Cloudflare Tunnel URL, update the URL in `sketch.ino` whenever the tunnel address changes.

### 2. Run the Web Dashboard

Ensure that Node.js and npm are installed.

Open a terminal in the `dashboard` directory and install the project dependencies:

```bash
npm install
```

Start the development server using the script defined in `package.json`. For example, if the project defines the `dev` script:

```bash
npm run dev
```

Open the local address printed by Vite in the terminal.

### 3. Connect the ESP32 Simulation to the Dashboard

The ESP32 firmware sends telemetry to:

```text
/api/wokwi/telemetry
```

The Vite development middleware handles incoming telemetry and provides the latest packet through:

```text
/api/wokwi/latest
```

For external access from Wokwi, configure the firmware to use the publicly reachable HTTPS URL of your running dashboard tunnel, followed by `/api/wokwi/telemetry`.

Keep the dashboard server and tunnel running while testing the connection.

## Important Notes

- The current hardware implementation is a **Wokwi simulation**, not a verified physical hardware deployment.
- The dashboard telemetry bridge stores the latest received packet in memory; it does not provide persistent historical storage.
- The external tunnel URL may change when a temporary tunnel is restarted.
- Verify optional dashboard functions, such as email notifications and camera integration, separately before relying on them.
- The firmware's TLS configuration should be reviewed before any production deployment.

## Future Improvements

- Add persistent storage for sensor readings and security events.
- Implement user authentication and access control.
- Improve alert handling and notification reliability.
- Deploy the dashboard and API to a persistent hosting environment.
- Test the system using physical sensors and hardware.
- Improve network error handling and telemetry security.

## Academic Project

Developed as part of the **Internet of Everything (IoE)** coursework at VIT Vellore.

### Team Members

- Hansitha Kakarla
- Aaditi Potey
- Alkha Susan Saju

---

*This project is an academic prototype intended for demonstration and educational purposes.*
