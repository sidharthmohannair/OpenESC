//This code is for testing commutation sequence of BLDC motor
// Motor Controller for BLDC using IR2101 and Arduino Uno
// Definitions for the connection of the IR2101 high and low side inputs
#define AHIN_PIN 9  // Arduino pin connected to IR2101 AHIN input for phase A high side
#define ALIN_PIN 4  // Arduino pin connected to IR2101 ALIN input for phase A low side
#define BHIN_PIN 10  // Arduino pin connected to IR2101 BHIN input for phase B high side
#define BLIN_PIN 3  // Arduino pin connected to IR2101 BLIN input for phase B low side
#define CHIN_PIN 11 // Arduino pin connected to IR2101 CHIN input for phase C high side
#define CLIN_PIN 2 // Arduino pin connected to IR2101 CLIN input for phase C low side
#define POT_PIN A0  // Analog pin connected to a potentiometer for speed control

// Constants for PWM configuration
#define PWM_FREQUENCY 20000 // PWM frequency in Hz (common in ESCs to reduce audible noise)
#define PWM_RESOLUTION 255  // PWM resolution, typical for 8-bit analogWrite function

// Comutation sequence table for a three-phase motor, defines which transistors are ON in each step
const byte commutationSequence[][6] = {
  {LOW, HIGH, LOW, HIGH, HIGH, LOW}, // Sequence 1: Phase U active
  {HIGH, LOW, LOW, HIGH, LOW, HIGH}, // Sequence 2: Phase V active
  {HIGH, LOW, HIGH, LOW, LOW, HIGH}, // Sequence 3: Phase W active
  {LOW, HIGH, HIGH, LOW, HIGH, LOW}, // Sequence 4: Phase U' active (reverse)
  {LOW, HIGH, LOW, HIGH, LOW, HIGH}, // Sequence 5: Phase V' active (reverse)
  {HIGH, LOW, LOW, HIGH, HIGH, LOW}  // Sequence 6: Phase W' active (reverse)
};

int currentPhase = 0; // Index of the current commutation sequence, managing the current step in the motor driving sequence

void setup() {
  Serial.begin(9600); // Begin serial communication for debugging
  
  // Setup all motor driver pins as outputs
  pinMode(AHIN_PIN, OUTPUT);
  pinMode(ALIN_PIN, OUTPUT);
  pinMode(BHIN_PIN, OUTPUT);
  pinMode(BLIN_PIN, OUTPUT);
  pinMode(CHIN_PIN, OUTPUT);
  pinMode(CLIN_PIN, OUTPUT);
}

void loop() {
  int potValue = analogRead(POT_PIN); // Read the potentiometer value
  int dutyCycle = map(potValue, 0, 1023, 0, PWM_RESOLUTION); // Map potentiometer value to PWM duty cycle

  // Set PWM duty cycle for each phase's high side using analogWrite (PWM output)
  analogWrite(AHIN_PIN, dutyCycle);
  analogWrite(BHIN_PIN, dutyCycle);
  analogWrite(CHIN_PIN, dutyCycle);

  // Execute the current commutation step by setting high and low driver inputs
  for (int i = 0; i < 6; i++) {
    digitalWrite(ALIN_PIN + i, commutationSequence[currentPhase][i]);
  }

  // Move to the next commutation step, wrap around after the last step
  currentPhase = (currentPhase + 1) % 6;

  // Print the current phase and duty cycle for debugging purposes
  Serial.print("Phase: ");
  Serial.print(currentPhase);
  Serial.print(", Duty Cycle: ");
  Serial.println(dutyCycle);

  // Delay to manage the speed of commutation, shorter delay for faster rotation
  delayMicroseconds(100); // Adjust this delay to control the rotation speed of the motor
}
