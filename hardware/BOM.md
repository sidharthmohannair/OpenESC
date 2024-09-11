# Bill of Materials (BOM)

| **Name**         | **Component Type**     | **Value/Part Number** |
|------------------|------------------------|-----------------------|
| **Capacitor**    | Ceramic Capacitor      | 100nF                 |
| **Capacitor**    | Electrolytic Capacitor | 22uF, 35V             |
| **Capacitor**    | Electrolytic Capacitor | 470uF, 35V            |
| **Resistor**     | Fixed Resistor         | 10Ω                   |
| **Resistor**     | Fixed Resistor         | 10KΩ                  |
| **Diode**        | Fast Recovery Diode    | UF4007                |
| **Driver**       | MOSFET Driver          | IR2110                |
| **MOSFET**       | N-channel MOSFET       | IRF3205               |
| **Microcontroller** | Arduino Nano        | -                     |
| **Potentiometer**| Variable Resistor      | 10KΩ                  |

## Notes:
- **Capacitors**: The 100nF ceramic capacitors are used for decoupling high-frequency noise, the 470uF electrolytic capacitor is used for input voltage filtering and back EMF suppression, and the 22uF electrolytic capacitors are used for bootstrap circuits or additional filtering purposes.
- **Resistors**: The 10Ω resistor might be used for current limiting or gate resistance, and the 10KΩ resistor is likely a pull-down for MOSFET gates or other logic-level control.
- **Diodes**: The UF4007 is a fast recovery diode, suitable for high-speed switching applications.
- **Driver and MOSFET**: The IR2110 MOSFET driver controls the IRF3205 N-channel MOSFETs, which are used for switching high currents in the ESC circuit.
- **Microcontroller**: The Arduino Nano is the main microcontroller, used for generating PWM signals and controlling the driver.
- **Potentiometer**: A 10KΩ potentiometer is used for manual control of motor speed in the open-loop ESC version.

---

For more details on how these components are used, refer to the [schematics](/hardware/schematic_diagram_v1.1.pdf).
