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
const unsigned long dog1_feedingTimes[] = {0 * 36000000, 2000, 4000};  // Scheduled feeding times for Dog 1
const unsigned long dog2_feedingTimes[] = {0 * 36000000, 2000, 4000};  // Scheduled feeding times for Dog 2
const int numFeedingTimes1 = 3;  // Number of feeding times for Dog 1
const int numFeedingTimes2 = 3;  // Number of feeding times for Dog 2
const int thresholdDistance = 30;  // Threshold distance for pet detection in cm
const int dog_portion_sizes[] = {50, 60};  // Portion sizes for each dog
float calibrationFactor = -7;  // Calibration factor for the scale
int fedDog1 = 0;  // Counter for the number of times Dog 1 has been fed
int fedDog2 = 0;  // Counter for the number of times Dog 2 has been fed

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
  // Check if it is feeding time for Dog 1
  if (fedDog1 < numFeedingTimes1 && currentMillis >= dog1_feedingTimes[fedDog1]) {
    return true;
  }
  return false;
}

bool isFeedingTime2(unsigned long currentMillis) {
  // Check if it is feeding time for Dog 2
  if (fedDog2 < numFeedingTimes2 && currentMillis >= dog2_feedingTimes[fedDog2]) {
    return true;
  }
  return false;
}

void feedDog(int dogNumber) {
  // Function to feed the specified dog
  int servoPosition;

  // Determine the servo position based on the dog number
  if (dogNumber == 0) {
    servoPosition = 50;
    if (isFeedingTime1(millis())) {
      fedDog1++;
    }
  }
  if (dogNumber == 1) {
    servoPosition = 127;
    if (isFeedingTime2(millis())) {
      fedDog2++;
    }
  }

  // Activate the servo to feed the dog
  int feed_start = millis();

  myServo.write(servoPosition);  // Move servo to the feeding position

  // Keep the servo in feeding position for 5 seconds
  while (millis() - feed_start < 1500) {
    myServo.write(servoPosition + 8);
    delay(50);
    myServo.write(servoPosition - 8);
    delay(50);
  }

  myServo.write(90);  // Return servo to neutral position
  Serial.print("Fed Dog ");
  Serial.println(dogNumber + 1);  // Print a message to the serial monitor
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

  // Check if it's time to feed any dog
  if (isFeedingTime1(currentMillis) || isFeedingTime2(currentMillis)) {
    if (Serial.available() > 0) {
      char received = Serial.read();  // Read the incoming data from Python script

      // Feed the appropriate dog based on the received character
      if (received == '1') {
        feedDog(0);  // Feed Dog 1
      } else if (received == '2') {
        feedDog(1);  // Feed Dog 2
      }

      // Clear the serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
  }

  delay(1000);  // Wait for a second before the next loop iteration
}
