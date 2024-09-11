/*
  Version: v2.0
  Author: Sidharth Mohan

  This is the open-loop ESC code developed for BLDC motor control. It implements a 6-step commutation sequence
  for the control of the high and low-side MOSFETs in an H-bridge configuration. 

  Key Features:
  - Open-loop control (no feedback).
  - Dead time to avoid shoot-through (simultaneous ON state of high and low MOSFETs).
  - Direct port manipulation for faster switching.

  Connections:
  - D9 (PB1) -> High-Side MOSFET A (UP_A)
  - D10 (PB2) -> High-Side MOSFET B (UP_B)
  - D11 (PB3) -> High-Side MOSFET C (UP_C)
  - D4 (PD4) -> Low-Side MOSFET A (LOW_A)
  - D3 (PD3) -> Low-Side MOSFET B (LOW_B)
  - D2 (PD2) -> Low-Side MOSFET C (LOW_C)

  Note: 
  - COMMUTATION_DELAY defines the delay between each commutation step.
  - DEAD_TIME defines the dead time to prevent simultaneous activation of high and low-side MOSFETs.
*/

// Define bit masks for High-Side MOSFETs
#define UP_A_MASK (1 << PORTB1) // D9 - High A (PB1)
#define UP_B_MASK (1 << PORTB2) // D10 - High B (PB2)
#define UP_C_MASK (1 << PORTB3) // D11 - High C (PB3)

// Define bit masks for Low-Side MOSFETs
#define LOW_A_MASK (1 << PORTD4) // D4 - Low A (PD4)
#define LOW_B_MASK (1 << PORTD3) // D3 - Low B (PD3)
#define LOW_C_MASK (1 << PORTD2) // D2 - Low C (PD2)

// Define commutation delay (in microseconds) for open-loop control
#define COMMUTATION_DELAY 500

// Define dead time (in microseconds) to prevent shoot-through
#define DEAD_TIME 2

void setup() {
  // Set PWM and Low-Side pins as outputs
  DDRB |= UP_A_MASK | UP_B_MASK | UP_C_MASK; // Set PB1, PB2, PB3 as outputs (D9, D10, D11)
  DDRD |= LOW_A_MASK | LOW_B_MASK | LOW_C_MASK; // Set PD4, PD3, PD2 as outputs (D4, D3, D2)

  // Initialize all outputs to LOW
  PORTB &= ~(UP_A_MASK | UP_B_MASK | UP_C_MASK); // Set PB1, PB2, PB3 (D9, D10, D11) LOW
  PORTD &= ~(LOW_A_MASK | LOW_B_MASK | LOW_C_MASK); // Set PD4, PD3, PD2 (D4, D3, D2) LOW
}

void loop() {
  // Commutation Sequence: 6-Step Open-Loop
  
  // Step 1: A+ B-
  PORTB &= ~UP_B_MASK;                // Turn off High B (D10)
  PORTD &= ~LOW_C_MASK;               // Turn off Low C (D2)
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTB |= UP_A_MASK;                 // Turn on High A (D9)
  PORTD |= LOW_B_MASK;                // Turn on Low B (D3)
  delayMicroseconds(COMMUTATION_DELAY);

  // Step 2: A+ C-
  PORTD &= ~LOW_B_MASK;               // Turn off Low B (D3)
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTD |= LOW_C_MASK;                // Turn on Low C (D2)
  delayMicroseconds(COMMUTATION_DELAY);

  // Step 3: B+ C-
  PORTB &= ~UP_A_MASK;                // Turn off High A (D9)
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTB |= UP_B_MASK;                 // Turn on High B (D10)
  delayMicroseconds(COMMUTATION_DELAY);

  // Step 4: B+ A-
  PORTD &= ~LOW_C_MASK;               // Turn off Low C (D2)
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTD |= LOW_A_MASK;                // Turn on Low A (D4)
  delayMicroseconds(COMMUTATION_DELAY);

  // Step 5: C+ A-
  PORTB &= ~UP_B_MASK;                // Turn off High B (D10)
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTB |= UP_C_MASK;                 // Turn on High C (D11)
  delayMicroseconds(COMMUTATION_DELAY);

  // Step 6: C+ B-
  PORTD &= ~LOW_A_MASK;               // Turn off Low A (D4)
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTD |= LOW_B_MASK;                // Turn on Low B (D3)
  delayMicroseconds(COMMUTATION_DELAY);

  // Repeat the sequence continuously in the loop
}
