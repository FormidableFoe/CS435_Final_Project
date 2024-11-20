#include "mbed.h"
#include "Adafruit_NeoPixel.h"
#include "LCDi2c.h"

// Ultra sonic left
DigitalOut trigPinLeft(D13); // Trigger pin
DigitalIn echoPinLeft(D12);  // Echo pin

// Ultra sonic right
DigitalOut trigPinRight(D11); // Trigger pin
DigitalIn echoPinRight(D10);  // Echo pin

// Ultra sonic front
DigitalOut trigPinFront(D9); // Trigger pin
DigitalIn echoPinFront(D8);  // Echo pin

// LED Strip
#define LEDS_AMNT 30
Adafruit_NeoPixel ledStrip(LEDS_AMNT, D7, NEO_GRB + NEO_KHZ800);

// BufferedSerial replaces Serial in newer Mbed versions
BufferedSerial pc(USBTX, USBRX,
                  115200); // USB serial transmission at 115200 baud

// Timer to measure the pulse duration
Timer timer;

// Speed of sound in air (13503.9 inches/s)
const float SOUND_SPEED_INCHES_PER_SEC = 13503.9f;

#define I2C_SDA_PIN D14
#define I2C_SCL_PIN D15

// I2C i2c(I2C_SDA, I2C_SCL);  // Define I2C pins
// I2C i2c(I2C_SDA_PIN, I2C_SCL_PIN);
LCDi2c lcd(I2C_SDA_PIN, I2C_SCL_PIN, LCD20x4,
           0x27); // Try other addresses if needed

// gets the distance by sending out a pulse
// returns a float in inches
float send_pulse(DigitalOut trigPin, DigitalIn echoPin) {
  trigPin = 0;
  ThisThread::sleep_for(2ms); // Sleep for 2ms
  trigPin = 1;
  wait_us(10); // Send 10us pulse
  trigPin = 0;

  // Wait for echo signal to go high
  while (echoPin == 0)
    ;

  // Start timer when echo goes high
  timer.start();

  // Wait for echo signal to go low
  while (echoPin == 1)
    ;

  // Stop timer when echo goes low
  timer.stop();

  // Calculate the time of the pulse (in microseconds)
  auto pulse_duration_us =
      chrono::duration_cast<chrono::microseconds>(timer.elapsed_time()).count();
  timer.reset();

  float distance_in_inches =
      (pulse_duration_us / 2.0f) * (SOUND_SPEED_INCHES_PER_SEC / 1000000.0f);
  return distance_in_inches;
}

// Print function for serial output
void print(const char *label, float value) {
  printf("%s: %.2f\n", label, value);
}

void setStripColor(const int &red, const int &green, const int &blue) {
  for (int led = 0; led < LEDS_AMNT; led++) {
    ledStrip.setPixelColor(led, ledStrip.Color(red, green, blue));
  }
}

void setProximityColor(const float &front, const float &left,
                       const float &right) {
  const float PROXIMITY = max({front, left, right});
  const float CLOSE = 10.0;
  const float FAR = 30.0;

  ledStrip.clear();
  if (PROXIMITY < CLOSE) { // Too Close, red
    setStripColor(255, 0, 0);
  } else if (PROXIMITY > FAR) { // Too Far, blue
    setStripColor(0, 0, 255);
  } else { // Good, green
    setStripColor(0, 255, 0);
  }
  ledStrip.show();
}

int main() {
  printf("Before\n");
  parseI2C();
  ledStrip.begin();
  ledStrip.setBrightness(128);
  printf("After\n");
  // lcd.display(DISPLAY_OFF);
  // lcd.display(BACKLIGHT_OFF);
  // Initialize the LCD
  lcd.cls();                  // Clear the display
  lcd.locate(0, 0);           // Set cursor to the first line
  lcd.printf("Hello World!"); // Print "Hello World!" on the first line
  lcd.character(0, 1, 0);
  lcd.character(3, 1, 1);
  lcd.character(5, 1, 2);
  lcd.character(7, 1, 3);

  ThisThread::sleep_for(10s);
  while (true) {
    // Measure distances
    float front = send_pulse(trigPinFront, echoPinFront);
    float left = send_pulse(trigPinLeft, echoPinLeft);
    float right = send_pulse(trigPinRight, echoPinRight);

    // Set color strip
    setProximityColor(front, left, right);

    // Print distances to Serial
    print("Front", front);
    print("Left", left);
    print("Right", right);
    printf("\n");

    // Display distances on LCD
    lcd.locate(0, 1); // Second line
    lcd.printf("Front: %.2f in", front);

    lcd.locate(0, 2); // Third line
    lcd.printf("Left:  %.2f in", left);

    lcd.locate(0, 3); // Fourth line
    lcd.printf("Right: %.2f in", right);

    ThisThread::sleep_for(1s); // Sleep for 1 second before next measurement
  }
}
