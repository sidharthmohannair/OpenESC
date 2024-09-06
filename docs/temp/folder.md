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
