/*
  Version: v2.1
  Author: Sidharth Mohan

  This version of the open-loop ESC code adds potentiometer-based speed control.
  The potentiometer input adjusts the commutation delay dynamically, allowing
  real-time speed control of the BLDC motor.

  Connections:
  - D9 (PB1) -> High-Side MOSFET A (UP_A)
  - D10 (PB2) -> High-Side MOSFET B (UP_B)
  - D11 (PB3) -> High-Side MOSFET C (UP_C)
  - D4 (PD4) -> Low-Side MOSFET A (LOW_A)
  - D3 (PD3) -> Low-Side MOSFET B (LOW_B)
  - D2 (PD2) -> Low-Side MOSFET C (LOW_C)
  - Potentiometer connected to A1

  Note: The potentiometer value adjusts the speed of the motor by changing
  the delay between commutation steps.
*/

// Define the analog pin for the potentiometer
#define pot A1
#define DEAD_TIME 2

void setup() {
  // Set PWM and Low-Side pins as outputs
  DDRB |= (1 << DDB1) | (1 << DDB2) | (1 << DDB3); // Set PB1 (D9), PB2 (D10), PB3 (D11) as outputs
  DDRD |= (1 << DDD2) | (1 << DDD3) | (1 << DDD4); // Set PD2 (D2), PD3 (D3), PD4 (D4) as outputs

  // Initialize all outputs to LOW
  PORTB &= ~((1 << PORTB1) | (1 << PORTB2) | (1 << PORTB3)); // Set PB1, PB2, PB3 (D9, D10, D11) LOW
  PORTD &= ~((1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4)); // Set PD4, PD3, PD2 (D4, D3, D2) LOW
}

void loop() {
  int v_delay = analogRead(pot); // Read the potentiometer value
  v_delay = map(v_delay, 0, 1023, 50, 2000); // Map the potentiometer value to the desired delay range

  // Step 1: A+ B-
  PORTB &= ~(1 << PORTB3);            // Ensure UP_C (PB3, Pin 11) is LOW
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTB |= (1 << PORTB1);             // Set UP_A (PB1, Pin 9) HIGH
  PORTD |= (1 << PORTD3);             // Set LOW_B (PD3, Pin 3) HIGH
  delayMicroseconds(v_delay);

  // Step 2: A+ C-
  PORTD &= ~(1 << PORTD3);            // Set LOW_B (PD3, Pin 3) LOW
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTD |= (1 << PORTD2);             // Set LOW_C (PD2, Pin 2) HIGH
  delayMicroseconds(v_delay);

  // Step 3: B+ C-
  PORTB &= ~(1 << PORTB1);            // Set UP_A (PB1, Pin 9) LOW
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTB |= (1 << PORTB2);             // Set UP_B (PB2, Pin 10) HIGH
  delayMicroseconds(v_delay);

  // Step 4: B+ A-
  PORTD &= ~(1 << PORTD2);            // Set LOW_C (PD2, Pin 2) LOW
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTD |= (1 << PORTD4);             // Set LOW_A (PD4, Pin 4) HIGH
  delayMicroseconds(v_delay);

  // Step 5: C+ A-
  PORTB &= ~(1 << PORTB2);            // Set UP_B (PB2, Pin 10) LOW
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTB |= (1 << PORTB3);             // Set UP_C (PB3, Pin 11) HIGH
  delayMicroseconds(v_delay);

  // Step 6: C+ B-
  PORTD &= ~(1 << PORTD4);            // Set LOW_A (PD4, Pin 4) LOW
  delayMicroseconds(DEAD_TIME);       // Add dead time
  PORTD |= (1 << PORTD3);             // Set LOW_B (PD3, Pin 3) HIGH
  delayMicroseconds(v_delay);

  // Repeat the sequence continuously in the loop
}
