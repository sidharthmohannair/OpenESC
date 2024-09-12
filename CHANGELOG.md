## CHANGELOG.md

## [v2.1] - [2024-08-28]
### Added
- **Potentiometer Control**: Introduced potentiometer input to control motor speed by adjusting the commutation delay. 
- **Commutation Delay Mapping**: The analog input from the potentiometer is mapped to a delay range between 500 to 2000 microseconds to control the speed of the motor.

### Fixed
- **Timing Consistency**: Ensured consistent delay between commutation steps for stable motor operation.

## [v2.0] - [2024-08-22]
### Added
- **Direct Port Manipulation**: Moved from `digitalWrite()` to direct port manipulation for faster switching in the commutation sequence. This significantly improves performance.
- **Dead Time**: Added a 2-microsecond dead time to prevent shoot-through during commutation.

## [v1.1] - [2024-08-22]
- Open Loop code simplified 
- Updated Commutation sequence 
- Verified using DSO

