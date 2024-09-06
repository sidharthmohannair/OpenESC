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
