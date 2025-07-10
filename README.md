# Arduino Pedal Assist System (PAS) Controller

This project is a custom **Pedal Assist System (PAS) controller** for an electric bike, based on an **Arduino-compatible microcontroller** and **I2C LCD display**. It reads pedal movement using a PAS sensor, allows level-based assist adjustment, and outputs a PWM signal to simulate a throttle signal (e.g., to an e-bike controller).

The controller also includes **walk-assist mode**, **configuration menu**, and **EEPROM storage** for user settings.

---

## 🔧 Features

- Adjustable **assist levels** (0 to N).
- Configurable **min RPM** to activate assist.
- Smooth **ramp-up logic** for assist power.
- **Walk assist mode** with delay and ramp.
- Adjustable **PWM output voltage range** (min/max voltage simulation).
- EEPROM storage for all settings.
- **LCD interface with buttons** for configuration.

---

## 🧰 Hardware Requirements

- **Arduino Uno / Nano** or compatible board
- **I2C 16x2 LCD display** (e.g. based on PCF8574)
- **PAS sensor** (pedal cadence sensor)
- 5x push-buttons:
  - Assist Up
  - Assist Down
  - Walk Assist
  - PAS signal input (connected to PAS sensor output)
  - Set button (long press to enter settings)
- Optional: pull-up resistors (if not using `INPUT_PULLUP`)
- **PWM Output** (D9 by default) connected to the **e-bike controller’s throttle input**

---

## 🔌 Pinout

| Signal            | Arduino Pin |
|------------------|-------------|
| PAS Input        | D2 (Interrupt) |
| Assist Up        | D3          |
| Assist Down      | D4          |
| Set Button       | D5          |
| Walk Assist      | D6          |
| PWM Output       | D9 (analogWrite) |
| I2C LCD (SDA/SCL)| A4/A5 (Uno/Nano default) |

---

## ⚙️ Configuration Menu

Enter settings by **holding the SET button for ~1.5s**.

You can configure the following options using the **Up/Down buttons**, and go to the next setting using the **Set button**:

1. Number of PAS magnets
2. Walk assist power (%)
3. Walk assist delay (ms)
4. Minimum RPM for assist activation
5. Ramp time for assist (ms)
6. Number of assist levels
7. RPM sampling interval (ms)
8. **PWM minimum voltage**
9. **PWM maximum voltage**

All settings are saved automatically to **EEPROM** when exiting the settings menu.

---

## 🧪 How It Works

- The PAS sensor generates pulses when you pedal. The system samples these pulses at regular intervals to estimate **RPM**.
- If RPM is above a configured **minimum RPM**, the assist becomes active and **ramps up** to a power level based on the selected assist level.
- If **Walk Assist** is triggered (pedal is idle, level = 0, walk button pressed), assist power ramps to a fixed walk value.
- A **PWM signal** is generated on pin D9. The duty cycle maps to a voltage range (e.g., 1V–4.2V), simulating throttle output.

---

## 💾 EEPROM Addresses (for reference)

| Setting            | EEPROM Address |
|-------------------|----------------|
| PAS magnets        | 0              |
| Walk assist power  | 2              |
| Walk delay         | 3              |
| Min RPM            | 7              |
| Ramp time          | 9              |
| Assist levels      | 13             |
| Sample interval    | 14             |
| PWM Min voltage ×100 | 16           |
| PWM Max voltage ×100 | 18           |

---

## 🛠️ Setup Instructions

1. Wire the system as shown in the pinout section.
2. Upload the sketch to your Arduino.
3. Power the system and observe LCD messages.
4. Hold the **SET button** to enter the settings menu.
5. Adjust parameters as needed.
6. Connect PWM output (D9) to the **e-bike controller’s throttle input**.

---

## 📷 Example Wiring Diagram (not included yet)

Coming soon: schematic and image of real-world connection.

---

## ⚠️ Disclaimer

- This is an open-source hobby project. Use at your own risk.
- Be cautious when interfacing with e-bike controllers — improper PWM voltage may damage components.

---

## 📄 License

MIT License – free to use, modify, and distribute.
