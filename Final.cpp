#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

// Define pins for ultrasonic sensors
#define TRIG_LEFT 8
#define ECHO_LEFT 9
#define TRIG_FRONT 10
#define ECHO_FRONT 11
#define TRIG_RIGHT 12
#define ECHO_RIGHT 13

// Define LED strip pins and settings
#define LED_PIN 6
#define NUM_LEDS 150 // Total number of LEDs
#define START_ACTIVE 23 // Starting index for active LEDs
#define END_ACTIVE 65   // Ending index for active LEDs
#define NUM_SEGMENTS 3  // Number of segments (left, front, right)

#define SOUND_SPEED_INCHES 13503.9f
#define GREEN_COLOR 0x00FF00
#define YELLOW_COLOR 0xFFFF00
#define RED_COLOR 0xFF0000
#define OFF_COLOR 0x000000

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 20, 4);

bool backlightOn = true;

// Calculate the size of each segment
const int segmentSize = (END_ACTIVE - START_ACTIVE + 1) / NUM_SEGMENTS;
const int separatorIndex1 = START_ACTIVE + segmentSize;       // First separator index
const int separatorIndex2 = START_ACTIVE + 2 * segmentSize;   // Second separator index

// Warning distances for color changes
const float RED_WARNING_DISTANCE = 2; 
const float YELLOW_WARNING_DISTANCE = 3;
const float FRONT_ACTIVATE_DISTANCE = 10;

// Averaging parameters
const int NUM_SAMPLES = 20; // Number of samples to average
float leftSamples[NUM_SAMPLES] = {0};
float frontSamples[NUM_SAMPLES] = {0};
float rightSamples[NUM_SAMPLES] = {0};
int sampleIndex = 0; // Circular buffer index




// Function prototypes
float getDistance(int trigPin, int echoPin);
void clearLEDs();
uint32_t getLEDColor(float distance);
float getAverage(float samples[]);
void updateLEDSegment(int start, int end, float distance);
void updateLEDSegments(float left, float front, float right);
void serialOutput(float left, float front, float right);
void lcdOutput(float left, float front, float right);




// Function to determine LED color based on distance
uint32_t getLEDColor(float distance) 
{
  if (distance < RED_WARNING_DISTANCE) 
  {
    return RED_COLOR; // Red for critical distance
  } else if (distance < YELLOW_WARNING_DISTANCE) 
  {
    return YELLOW_COLOR; // Yellow for warning distance
  } else 
  {
    return GREEN_COLOR; // Green for safe distance
  }
}

void setup() 
{
  Serial.begin(115200);

  // Initialize ultrasonic sensors
  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);
  pinMode(TRIG_FRONT, OUTPUT);
  pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);

  // Initialize LED strip
  strip.begin();
  //strip.setBrightness(25); // Set brightness to 10%
  strip.show();

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Setup complete");
  Serial.println("Setup complete");
  
  delay(2000);
}

// Function to calculate distance
float getDistance(int trigPin, int echoPin) 
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long timeout = micros() + 30000; // Timeout to avoid hanging

  while (digitalRead(echoPin) == LOW) 
  {
    if (micros() > timeout) return -1;
  }

  unsigned long startTime = micros();
  while (digitalRead(echoPin) == HIGH) 
  {
    if (micros() > timeout) return -1;
  }
  unsigned long duration = micros() - startTime;

   // Duration = there and back time so div by 2
   // div speed of sound by 1,000,000 since duration is in microseconds
  float distance = (duration / 2.0) * (SOUND_SPEED_INCHES / 1000000.0);

  return distance;
}

// Function to calculate the average of a sample array
float getAverage(float samples[]) 
{
  float sum = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) 
  {
    sum += samples[i];
  }

  return sum / NUM_SAMPLES;
}

void clearLEDs()
{
  // Clear all LEDs
  for (int i = 0; i < NUM_LEDS; i++) 
  {
    strip.setPixelColor(i, OFF_COLOR);
  }

}

// Function to update LED segment based on distance
void updateLEDSegment(int start, int end, float distance) 
{
  uint32_t color = getLEDColor(distance);

  for (int i = start; i <= end; i++) 
  {
    strip.setPixelColor(i, color);
  }
}

void updateLEDSegments(float left, float front, float right)
{
  clearLEDs();

  // Update active LED segments if front sensor detects within activation distance
  if (front < FRONT_ACTIVATE_DISTANCE) 
  {
    updateLEDSegment(START_ACTIVE, separatorIndex1 - 1, right);   // Right LEDs
    updateLEDSegment(separatorIndex1 + 1, separatorIndex2 - 1, front); // Front LEDs
    updateLEDSegment(separatorIndex2 + 1, END_ACTIVE, left);  // Left LEDs
  }

  // Set separator LEDs to OFF
  strip.setPixelColor(separatorIndex1, OFF_COLOR); // Between left and front
  strip.setPixelColor(separatorIndex2, OFF_COLOR); // Between front and right
  
  strip.show(); // Apply changes to the LED strip
}

void serialOutput(float left, float front, float right)
{
  Serial.print("Left: ");
  Serial.print(left);
  Serial.print(" in, Front: ");
  Serial.print(front);
  Serial.print(" in, Right: ");
  Serial.print(right);
  Serial.println(" in");
}

// Display distances on LCD
void lcdOutput(float left, float front, float right)
{
  if (front < FRONT_ACTIVATE_DISTANCE) 
  {
    if (!backlightOn)
    {
      lcd.backlight();
      backlightOn = true;

      //Line 1
      lcd.setCursor(0, 0);
      lcd.print("Distance Sensors");
    }

    //Line 2
    lcd.setCursor(0, 1);
    lcd.print("Left:  ");
    lcd.print(left, 1); // rounds to 1 decimal
    lcd.print(" in    ");

    //Line 3
    lcd.setCursor(0, 2);
    lcd.print("Front: ");
    lcd.print(front, 1); // rounds to 1 decimal
    lcd.print(" in    ");

    //Line 4
    lcd.setCursor(0, 3);
    lcd.print("Right: ");
    lcd.print(right, 1); // rounds to 1 decimal
    lcd.print(" in    ");
  }
  else if (backlightOn) // only turn off if 
  {
    lcd.noBacklight();
    lcd.clear();
    backlightOn = false;
  }
}

void loop() 
{
  frontSamples[sampleIndex] = getDistance(TRIG_FRONT, ECHO_FRONT); // Read current distance
  float frontDistance = getAverage(frontSamples); // Calculate averages to smooth outliers

  leftSamples[sampleIndex] = getDistance(TRIG_LEFT, ECHO_LEFT);
  float leftDistance = getAverage(leftSamples);

  rightSamples[sampleIndex] = getDistance(TRIG_RIGHT, ECHO_RIGHT);
  float rightDistance = getAverage(rightSamples);

  // Move to the next sample index
  sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;  

  // Print distances to Serial Monitor
  serialOutput(leftDistance, frontDistance, rightDistance);

  // Print distances to LCD
  lcdOutput(leftDistance, frontDistance, rightDistance);

  // Display proper LED's
  updateLEDSegments(leftDistance, frontDistance, rightDistance);

  delay(50);
}
