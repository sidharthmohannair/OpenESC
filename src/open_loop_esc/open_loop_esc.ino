/*
Step 1: High A (UP_A) and Low B (LOW_B) are turned on.
Step 2: High A (UP_A) and Low C (LOW_C) are turned on.
Step 3: High B (UP_B) and Low C (LOW_C) are turned on.
Step 4: High B (UP_B) and Low A (LOW_A) are turned on.
Step 5: High C (UP_C) and Low A (LOW_A) are turned on.
Step 6: High C (UP_C) and Low B (LOW_B) are turned on.
//code is created by Sidharth Mohan
*/

// Simple Open-Loop ESC Code for BLDC Motor (No Feedback)
// Commutation Sequence: 6-Step, Open-Loop

// Define PWM pins for High-Side MOSFETs
#define UP_A 9  // D9 - High A
#define UP_B 10 // D10 - High B
#define UP_C 11 // D11 - High C

// Define pins for Low-Side MOSFETs
#define LOW_A 4  // D4 - Low A
#define LOW_B 3  // D3 - Low B
#define LOW_C 2  // D2 - Low C

// Define commutation delay (in microseconds) for open-loop control
#define COMMUTATION_DELAY 500

// Define dead time (in microseconds) to prevent shoot-through
#define DEAD_TIME 0

void setup() {
  // Set PWM and Low-Side pins as outputs
  pinMode(UP_A, OUTPUT);
  pinMode(UP_B, OUTPUT);
  pinMode(UP_C, OUTPUT);

  pinMode(LOW_A, OUTPUT);
  pinMode(LOW_B, OUTPUT);
  pinMode(LOW_C, OUTPUT);

  // Initialize all outputs to LOW
  digitalWrite(UP_A, LOW);
  digitalWrite(UP_B, LOW);
  digitalWrite(UP_C, LOW);
  digitalWrite(LOW_A, LOW);
  digitalWrite(LOW_B, LOW);
  digitalWrite(LOW_C, LOW);
}

void loop() {
  // Commutation Sequence: 6-Step Open-Loop

  // Step 1: A+ B-

  digitalWrite(UP_A, HIGH);
  digitalWrite(UP_B, LOW);
  digitalWrite(UP_C, LOW);
  digitalWrite(LOW_A, LOW);
  digitalWrite(LOW_B, HIGH);
  digitalWrite(LOW_C, LOW);
delayMicroseconds(COMMUTATION_DELAY);
  // Step 2: A+ C-
 digitalWrite(UP_A, HIGH);
  digitalWrite(UP_B, LOW);
  digitalWrite(UP_C, LOW);
  digitalWrite(LOW_A, LOW);
  digitalWrite(LOW_B, LOW);
  digitalWrite(LOW_C, HIGH);
delayMicroseconds(COMMUTATION_DELAY);
  // Step 3: B+ C-
  digitalWrite(UP_A, LOW);
  digitalWrite(UP_B, HIGH);
  digitalWrite(UP_C, LOW);
  digitalWrite(LOW_A, LOW);
  digitalWrite(LOW_B, LOW);
  digitalWrite(LOW_C, HIGH);

delayMicroseconds(COMMUTATION_DELAY);
  // Step 4: B+ A-
  digitalWrite(UP_A, LOW);
  digitalWrite(UP_B, HIGH);
  digitalWrite(UP_C, LOW);
  digitalWrite(LOW_A, HIGH);
  digitalWrite(LOW_B, LOW);
  digitalWrite(LOW_C, LOW);
delayMicroseconds(COMMUTATION_DELAY);

  // Step 5: C+ A-
  digitalWrite(UP_A, LOW);
  digitalWrite(UP_B, LOW);
  digitalWrite(UP_C, HIGH);
  digitalWrite(LOW_A, HIGH);
  digitalWrite(LOW_B, LOW);
  digitalWrite(LOW_C, LOW);
delayMicroseconds(COMMUTATION_DELAY);

  // Step 6: C+ B-
  digitalWrite(UP_A, LOW);
  digitalWrite(UP_B, LOW);
  digitalWrite(UP_C, HIGH);
  digitalWrite(LOW_A, LOW);
  digitalWrite(LOW_B, HIGH);
  digitalWrite(LOW_C, LOW);
delayMicroseconds(COMMUTATION_DELAY);
  
  
//  digitalWrite(UP_C, LOW);    // Turn off previous high
//  digitalWrite(LOW_C, LOW);    // Turn off previous low
//  digitalWrite(UP_A, HIGH);   // High A on
//  //delayMicroseconds(DEAD_TIME); 
//  digitalWrite(LOW_B, HIGH);   // Low B on
//  delayMicroseconds(COMMUTATION_DELAY);
//
//  // Step 2: A+ C-
//  digitalWrite(LOW_B, LOW);    // Low B off
// // delayMicroseconds(DEAD_TIME); 
//  digitalWrite(LOW_C, HIGH);   // Low C on
//  delayMicroseconds(COMMUTATION_DELAY);
//
//  // Step 3: B+ C-
//  digitalWrite(UP_A, LOW);    // High A off
//  //delayMicroseconds(DEAD_TIME); 
//  digitalWrite(UP_B, HIGH);   // High B on
//  delayMicroseconds(COMMUTATION_DELAY);
//
//  // Step 4: B+ A-
//  digitalWrite(LOW_C, LOW);    // Low C off
// // delayMicroseconds(DEAD_TIME); 
//  digitalWrite(LOW_A, HIGH);   // Low A on
//  delayMicroseconds(COMMUTATION_DELAY);
//
//  // Step 5: C+ A-
//  digitalWrite(UP_B, LOW);    // High B off
// // delayMicroseconds(DEAD_TIME); 
//  digitalWrite(UP_C, HIGH);   // High C on
//  delayMicroseconds(COMMUTATION_DELAY);
//
//  // Step 6: C+ B-
//  digitalWrite(LOW_A, LOW);    // Low A off
// // delayMicroseconds(DEAD_TIME); 
//  digitalWrite(LOW_B, HIGH);   // Low B on
//  delayMicroseconds(COMMUTATION_DELAY);

  // Repeat the sequence continuously in the loop
}
