
# OpenESC - Open Source Electronic Speed Controller

## Project Overview

**OpenESC** is an open-source Electronic Speed Controller (ESC) project for controlling Brushless DC (BLDC) motors. The goal is to provide a versatile and customizable ESC solution that is free for anyone to use and modify, making it a great alternative to proprietary ESC systems. This project will evolve from a basic open-loop controller to a more advanced closed-loop controller with Back-EMF (BEMF) feedback for improved performance.

## Key Features

- **Open-loop ESC**: Initial version based on open-loop control for BLDC motors.
- **Back-EMF Feedback**: Planned feature for future versions to implement closed-loop control with BEMF sensing.
- **MOSFET Control**: High-side and low-side MOSFET switching for precise motor control.
- **Dead Time and Commutation Delay**: Adjustable to prevent shoot-through and optimize commutation timing.
- **Modular Structure**: Designed for easy development and contributions.

## Roadmap

1. **v1.0 (Open-Loop ESC)**:
   - Basic BLDC control using open-loop commutation.
   - Adjustable commutation timing.
   - No feedback control.

2. **v2.0 (Closed-Loop ESC with BEMF)**:
   - Implement BEMF sensing for feedback control.
   - Improve performance and efficiency under varying load conditions.

3. **Future Enhancements**:
   - Current sensing for protection and advanced control.
   - Field-Oriented Control (FOC) for smoother and more efficient motor control.
   - Parameter tuning for commutation timing, current limits, etc.

## Repository Structure

```text
OpenESC/
├── README.md                   # Overview of the project.
├── LICENSE                     # Open-source license (e.g., MIT, GPL, etc.).
├── docs/                       # Documentation, schematics, tutorials, and theory.
│   ├── ESC_Schematic.pdf       # Circuit diagrams and explanation.
│   ├── ESC_Theory.md           # How the ESC works, BEMF concepts, etc.
│   ├── Installation.md         # Instructions on how to install and use the ESC.
│   ├── troubleshooting/        # Troubleshooting and common issue fixes.
│   │   └── Troubleshooting.md  # Common troubleshooting steps.
│   └── testing/                # Test logs, results, DSO captures.
│       ├── test_log_01.md      # Specific test session logs, captures.
│       └── test_log_02.md
├── src/                        # Source code directory for stable releases.
│   ├── v1.0_open_loop_esc/     # Open-loop ESC version (v1.0).
│   │   └── open_loop_esc_v1.ino
│   ├── v2.0_bemf_esc/          # BEMF-based ESC version (v2.0).
│   │   └── bemf_esc_v2.ino
│   └── common/                 # Common libraries or utility code shared by multiple versions.
│       ├── pwm_lib.h
│       └── utilities.h
├── test_firmware/              # Test-specific firmware or code for debugging/troubleshooting.
│   ├── test_pwm_tuning.ino     # Test code to tune PWM.
│   ├── test_signal_stability.ino
│   └── troubleshooting_code_01.ino
├── hardware/                   # PCB design files, BOM, hardware details.
│   ├── gerbers/                # PCB Gerber files.
│   ├── schematics/             # Hardware schematics.
│   └── BOM.md                  # Bill of materials.
├── releases/                   # Release artifacts, changelogs, and documentation for stable versions.
│   ├── v1.0/                   # Release files for v1.0.
│   │   ├── firmware_v1.hex
│   │   ├── changelog_v1.md
│   │   └── release_notes_v1.md
│   ├── v2.0/                   # Release files for v2.0.
│   │   ├── firmware_v2.hex
│   │   ├── changelog_v2.md
│   │   └── release_notes_v2.md
└── .gitignore                  # Ignore files generated during builds (object files, binaries, etc.).

```

## How to Use

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/sidharthmohannair/OpenESC.git
   cd OpenESC
   ```

2. **Open the Source Code**:
   - Navigate to the `src/` directory.
   - The `open_loop_esc/` folder contains the basic open-loop control code.

3. **Compile and Upload**:
   - Use the Arduino IDE to compile and upload the `open_loop_esc.ino` to your microcontroller (e.g., Arduino Nano or similar).

4. **Connect Hardware**:
   - Follow the hardware schematic (located in `docs/ESC_Schematic.pdf`) for wiring the MOSFET drivers, power supply, and motor.

## Contributions

We welcome contributions from the community! To contribute:
1. Fork the repository.
2. Create a new branch for your feature or bug fix.
3. Submit a pull request with detailed descriptions of your changes.

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

Special thanks to all contributors and the open-source community for making this project possible.

---

**Let's build a robust, open-source ESC for everyone!**
