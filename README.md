# ESP32 Integration Calculator

A calculator built on the ESP32 microcontroller with a physical keypad and LCD display, capable of performing mathematical calculations, specifically focusing on integration using an external API.

## 🔧 Features

- **Physical Interface:** Input via a 4x4 matrix keypad and output on a 16x2 LCD screen.
- **WiFi Connectivity:** Connects to your local WiFi network to access online API services.
- **Integration Calculation:** Sends mathematical expressions to the Derivative API ([https://newton.vercel.app](https://newton.vercel.app)) for integration.
- **Shift Functionality:** Access additional mathematical functions (trigonometric, logarithmic, etc.) via a shift key.
- **Mode Switching:** Cycle through different function modes (Normal, Trig, Hyperbolic, Inverse, Log).
- **Scrolling Display:** Navigate through long input expressions and results using dedicated scroll buttons.
- **Basic Operations:** Includes standard calculator operations (+, -, *, /) via extra buttons.

![WhatsApp Image 2025-05-21 at 00 41 48_1ea3e302](https://github.com/user-attachments/assets/4408e119-05b5-4643-9b9b-6e2247a6ef35)

## 🧰 Hardware Requirements

- ESP32 Development Board (30-pin version used in development)
- 4x4 Matrix Keypad
- 16x2 LCD Display (with I2C adapter preferred)
- 2 x Momentary Push Buttons (for scrolling)
- 4 x Push Buttons for extra operations (+, -, *, /)
- Connecting Wires
- Breadboard (optional)

## 📌 Pin Configuration

### LCD (Parallel Connection in Code)

| LCD Pin | ESP32 GPIO |
|---------|------------|
| RS      | 15         |
| E       | 2          |
| D4      | 21         |
| D5      | 19         |
| D6      | 18         |
| D7      | 5          |

> If you’re using an I2C LCD instead, replace with `LiquidCrystal_I2C` and connect SDA/SCL accordingly.

### Keypad (4x4 Matrix)

| Keypad Pin | ESP32 GPIO |
|------------|------------|
| R1         | 13         |
| R2         | 12         |
| R3         | 14         |
| R4         | 27         |
| C1         | 26         |
| C2         | 25         |
| C3         | 33         |
| C4         | 32         |

### Extra Operation Buttons

| Operation | ESP32 GPIO |
|-----------|------------|
| '+'       | 22         |
| '-'       | 23         |
| '*'       | 16         |
| '/'       | 17         |

### Scroll Buttons

| Function | ESP32 GPIO |
|----------|------------|
| Left     | 4          |
| Right    | 35         |

> Scroll buttons use internal pull-ups; connect one side to the GPIO and the other to GND.

## 💻 Software Requirements

- Arduino IDE
- ESP32 Board Package (via Board Manager)
- Required Libraries:
  - `WiFi.h`
  - `HTTPClient.h`
  - `LiquidCrystal.h` (or `LiquidCrystal_I2C.h`)
  - `Keypad.h`

## 🚀 Getting Started

1. Clone this repository.
2. Open `esp32.ino` in Arduino IDE.
3. Install required libraries.
4. Update your WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
