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

## 🧪 How It Works

- The PAS sensor generates pulses when you pedal. The system samples these pulses at regular intervals to estimate **RPM**.
- If RPM is above a configured **minimum RPM**, the assist becomes active and **ramps up** to a power level based on the selected assist level.
- The assist level can be **increased or decreased** using the **Up** and **Down** buttons. A higher level results in more power delivered to the motor (stronger assist).
- If **Walk Assist** is triggered (pedal is idle, level = 0, walk button pressed), assist power ramps to a fixed walk value.
- A **PWM signal** is generated on pin D9. The duty cycle maps to a voltage range (e.g., 1V–4.2V), simulating throttle output to the controller.
- ⚠️ **Setting the correct number of magnets in the PAS sensor** is crucial — wrong value will cause inaccurate RPM detection and poor assist behavior.

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
- **RC low-pass filter** on PWM output (explained below)

---

## 🔌 Pinout

| Signal            | Arduino Pin       |
|------------------|-------------------|
| PAS Input        | D2 (Interrupt)     |
| Assist Up        | D3                |
| Assist Down      | D4                |
| Set Button       | D5                |
| Walk Assist      | D6                |
| PWM Output       | D9 (analogWrite)  |
| I2C LCD (SDA/SCL)| A4/A5 (Uno/Nano)  |

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

### 📏 How to Check Throttle Voltage Range

To correctly simulate a throttle signal, you must match the **PWM voltage range** to what your e-bike controller expects (e.g., 1.0V to 4.2V). To find this range:

1. Disconnect the throttle from the controller.
2. Use a multimeter (in DC mode) to measure voltage:
   - **Ground** (black probe) → GND wire
   - **Positive** (red probe) → Signal wire from throttle
3. Turn throttle to **minimum** → note voltage (e.g., 1.0V)
4. Turn throttle to **maximum** → note voltage (e.g., 4.2V)
5. Enter these values in the PAS controller settings menu as **PWM Min/Max Voltage**

This ensures the e-bike controller interprets the PWM output correctly.

---

### 🔻 RC Low-Pass Filter Requirement

Most e-bike controllers expect a **smooth analog voltage** on the throttle input, not a fast-switching PWM signal. Therefore, it’s necessary to **add an RC low-pass filter** to the PWM output. This filter converts the PWM into a stable DC-like voltage proportional to the duty cycle.

**RC Filter Schematic:**

![RC Filter](attachment:file_000000001570620ab256241f3a9f1549)

This filter consists of:
- Resistor (R) in series with PWM output  
- Capacitor (C) to ground from the output side  
Together they form a **first-order low-pass filter**.

---

### ⚙️ How to Choose R and C

To select proper values for R and C, use the formula for the cutoff frequency:

```
f_c = 1 / (2πRC)
```

Where:
- `f_c` = filter cutoff frequency (Hz)  
- `R` = resistance in ohms (Ω), e.g. 1kΩ  
- `C` = capacitance in farads (F), e.g. 1µF

Example:
- `R = 1 kΩ`
- `C = 1 µF`
- Then:

```
f_c = 1 / (2 * π * 1000 * 0.000001) ≈ 159.15 Hz
```

This value is low enough to smooth a standard 490 Hz PWM from Arduino, while remaining responsive. You can increase **R** or **C** for better filtering (slower response), or reduce them for quicker voltage transitions (less filtering).

---

## 🛠️ Setup Instructions

1. Wire the system as shown in the pinout section.  
2. Upload the sketch to your Arduino.  
3. Power the system and observe LCD messages.  
4. Hold the **SET button** to enter the settings menu.  
5. Adjust parameters as needed.  
6. Connect PWM output (D9) to the **e-bike controller’s throttle input** through the **RC filter**.

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
