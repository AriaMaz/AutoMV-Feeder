#include <Ultrasonic.h>  // Include Ultrasonic library for distance measurement
#include <Servo.h>       // Include Servo library to control servo motors
#include <HX711.h>       // Include HX711 library for the scale sensor

HX711 scale;  // Create an instance of the HX711 scale sensor

// Define pins for sensors and servo
#define TRIG_PIN 9
#define ECHO_PIN 10
#define SERVO_PIN 11
#define DOUT_PIN A1
#define SCK_PIN A2

// Define variables:
long duration;  // To store the duration of the ultrasonic pulse
int distance;   // To store the calculated distance

Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);  // Initialize the Ultrasonic sensor
Servo myServo;  // Create a Servo object to control the servo motor

// Constants for feeding times and portion sizes
const unsigned long pet1_feedingTimes[] = {1 * 36000000, 2000, 4000};  // Scheduled feeding times for pet 1
const unsigned long pet2_feedingTimes[] = {2 * 36000000, 2000, 4000};  // Scheduled feeding times for pet 2
const int numFeedingTimes1 = 3;  // Number of feeding times for pet 1
const int numFeedingTimes2 = 3;  // Number of feeding times for pet 2
const int thresholdDistance = 30;  // Threshold distance for pet detection in cm
const int pet_portion_sizes[] = {50, 60};  // Portion sizes for each pet in grams
float calibrationFactor = -7;  // Calibration factor for the scale
int fedpet1 = 0;  // Counter for the number of times pet 1 has been fed
int fedpet2 = 0;  // Counter for the number of times pet 2 has been fed

unsigned long startTime;  // To store the start time

void setup() {
  Serial.begin(9600);  // Start serial communication at 9600 baud
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  myServo.attach(SERVO_PIN);  // Attach the servo to its control pin
  scale.begin(DOUT_PIN, SCK_PIN);  // Initialize the scale
  scale.set_scale(calibrationFactor);  // Set the scale's calibration factor
  startTime = millis();  // Record the start time
}

bool isFeedingTime1(unsigned long currentMillis) {
  // Check if it is feeding time for pet 1
  if (fedpet1 < numFeedingTimes1 && currentMillis >= pet1_feedingTimes[fedpet1]) {
    return true;
  }
  return false;
}

bool isFeedingTime2(unsigned long currentMillis) {
  // Check if it is feeding time for pet 2
  if (fedpet2 < numFeedingTimes2 && currentMillis >= pet2_feedingTimes[fedpet2]) {
    return true;
  }
  return false;
}

void feedpet(int petNumber) {
  // Function to feed the specified pet
  int servoPosition;

  // Determine the servo position based on the pet number
  if (petNumber == 0) {
    servoPosition = 50;
    if (isFeedingTime1(millis())) {
      fedpet1++;
    }
  }
  if (petNumber == 1) {
    servoPosition = 127;
    if (isFeedingTime2(millis())) {
      fedpet2++;
    }
  }

  // Activate the servo to feed the pet
  int feed_start = millis();

  myServo.write(servoPosition);  // Move servo to the feeding position

  // Keep the servo in feeding position for 5 seconds
  while (millis() - feed_start < 1500 && scale.get_units() + 240 < pet_portion_sizes[petNumber]) {
    myServo.write(servoPosition + 8);
    delay(50);
    myServo.write(servoPosition - 8);
    delay(50);
  }

  myServo.write(90);  // Return servo to neutral position
  Serial.print("Fed pet");
  Serial.println(petNumber + 1);  // Print a message to the serial monitor
}

void loop() {
  unsigned long currentMillis = millis();  // Get the current time

  // Trigger the ultrasonic sensor to measure distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Calculate the distance based on the time of echo
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  // Send distance to the Python script via serial
  Serial.println(distance);

  // Check if it's time to feed any pet
  if (isFeedingTime1(currentMillis) || isFeedingTime2(currentMillis)) {
    if (Serial.available() > 0) {
      char received = Serial.read();  // Read the incoming data from Python script

      // Feed the appropriate pet based on the received character
      if (received == '1') {
        feedpet(0);  // Feed pet 1
      } else if (received == '2') {
        feedpet(1);  // Feed pet 2
      }

      // Clear the serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
  }

  delay(1000);  // Wait for a second before the next loop iteration
}
