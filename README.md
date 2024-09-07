# OpenESC - Open Source Electronic Speed Controller

## Project Overview

**OpenESC** is an open-source Electronic Speed Controller (ESC) project for controlling Brushless DC (BLDC) motors. The goal is to provide a versatile and customizable ESC solution that is free for anyone to use and modify, making it a great alternative to proprietary ESC systems. This project will evolve from a basic open-loop controller to a more advanced closed-loop controller with Back-EMF (BEMF) feedback for improved performance.

## Key Features

- **Open-loop ESC**: Initial version based on open-loop control for BLDC motors.
- **Back-EMF Feedback**: Planned feature for future versions to implement closed-loop control with BEMF sensing.
- **MOSFET Control**: High-side and low-side MOSFET switching for precise motor control.
- **Dead Time and Commutation Delay**: Adjustable to prevent shoot-through and optimize commutation timing.
- **Modular Structure**: Designed for easy development and contributions.
- **Open-Source Hardware**: PCB design files and schematics are provided for building your own ESC.

## Roadmap

1. **v1.0 | Open-Loop ESC**:
   - Basic BLDC control using open-loop commutation.
   - Adjustable commutation timing.
   - No feedback control.

2. **v2.0 | Closed-Loop ESC with BEMF (ongoing work)**:
   - Implement BEMF sensing for feedback control.
   - Improve performance and efficiency under varying load conditions.

3. **Future Enhancements**:
   - Current sensing for protection and advanced control.
   - Field-Oriented Control (FOC) for smoother and more efficient motor control.
   - Parameter tuning for commutation timing, current limits, etc.

## Repository Structure

```text
OpenESC/
├── README.md                # Overview of the project.
├── LICENSE                  # Open-source license.
├── docs/                    # Documentation, schematics, and theory.
│   ├── ESC_Schematic.pdf    # Circuit diagrams and explanation.
│   └── ESC_Theory.md        # How the ESC works, BEMF concepts, etc.
├── src/                     # Source code directory.
│   ├── open_loop_esc/       # Open-loop ESC version.
│   │   └── open_loop_esc.ino
│   ├── bemf_esc/            # BEMF-based closed-loop ESC version (for future).
│   │   └── bemf_esc.ino
│   └── common/              # Common libraries or code used by both versions.
├── hardware/                # PCB design files, BOM, and hardware details.
    ├── gerbers/             # PCB Gerber files.
    └── BOM.md               # Bill of materials.

```
## Getting Started

### Prerequisites

- **Arduino IDE**: You can download the [Arduino IDE here](https://www.arduino.cc/en/software).
- **ATmega328P-based microcontroller** (e.g., Arduino Nano).
- **BLDC Motor**: Ensure that your motor is compatible with open-loop ESCs.
- **MOSFET Drivers**: This ESC requires an external driver such as the IR2110 for interfacing with the MOSFETs.

### Installing

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yourusername/OpenSourceESC.git
   ```

2. **Navigate to the Source Code**:
   ```bash
   cd OpenSourceESC/src/open_loop_esc/
   ```

3. **Open the Project in Arduino IDE**:
   - Open the `.ino` file in Arduino IDE.
   - Select the appropriate board and port in the IDE.

4. **Upload the Code**:
   - Upload the firmware to your Arduino-compatible microcontroller.

## Results

The open-loop ESC has been tested and verified to produce correct phase switching, stable operation, and is ready for use in various applications. Details of the tests can be found in the **docs/testing/** directory, along with oscilloscope captures and test logs.

## Hardware

- **PCB Design**: The PCB Gerber files and schematics can be found in the `hardware/` directory.
- **Bill of Materials (BOM)**: A complete list of components is available in the `BOM.md` file.

## Ongoing Work

The following features and improvements are currently being developed on separate branches:
- **BEMF-based Feedback Control**: Development of closed-loop ESC with back-EMF feedback is in progress on the `bemf_dev` branch.
- **Current Sensing**: Adding support for current sensors to protect the ESC from overcurrent conditions.
- **Advanced Field-Oriented Control (FOC)**: Future plans to implement FOC for smoother motor control.

To stay updated on these developments, check out the [development branches]().

## Contributing

We welcome contributions from the community! If you’d like to help, please check the open issues or submit a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.