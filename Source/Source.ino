#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"
#include "MAX30100.h"

// Constants for display refresh and serial report timing
#define REPORTING_PERIOD_MS 100

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define LINE_SIZE1 12
#define LINE_SIZE2 16

// Data packet size and header for serial communication
#define TAMANO 4
#define ENCABEZADO 0xFACE

// MAX30100 sensor settings
#define SAMPLING_RATE MAX30100_SAMPRATE_100HZ
#define IR_LED_CURRENT MAX30100_LED_CURR_50MA
#define RED_LED_CURRENT MAX30100_LED_CURR_27_1MA
#define PULSE_WIDTH MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE true

// Heart icon bitmap to display on OLED when a beat is detected
const unsigned char Heart_Icon[] PROGMEM = {
    0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc, 0xff, 0xfe, 0xff, 0xfe,
    0xee, 0xee, 0xd5, 0x56, 0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0,
    0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00};

// Initialize OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Timer variables for display update
unsigned long tv_ant = 0;
const long tv_int = 200;
unsigned long tv_act = 0;

// Pulse oximeter object (based on MAX30100)
PulseOximeter pox;
uint32_t tsLastReport = 0;

// Union to format and send IR data via serial
union dato2
{
  struct
  {
    word encabezado = ENCABEZADO;
    int infrarojo;
  };
  byte bufferDatos[TAMANO];
} tx2;

// Timer variables for serial data transmission (not used here but reserved)
unsigned long tt_ant = 0;
const long tt_int = 500;
unsigned long tt_act = 0;

// Raw MAX30100 sensor instance for low-level access
MAX30100 sensor;

// Callback function triggered when a heartbeat is detected
void onBeatDetected()
{
  Serial.println("Beat!");
  display.setTextSize(1);
  display.setCursor(0, 0 * LINE_SIZE1);
  display.print(F("                Beat!"));
  display.setCursor(0, 1 * LINE_SIZE2);
  display.setTextSize(2);
  display.drawBitmap(100, 16, Heart_Icon, 16, 16, WHITE);
  display.display();
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("CAZ"));

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ; // Halt if display fails
  }
  display.setRotation(0);
  delay(10);

  // Initialize pulse oximeter
  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin())
  {
    Serial.println("FAILED");
    while (true)
      ;
  }
  else
  {
    Serial.println("SUCCESS");
  }

  // Initialize raw MAX30100 sensor
  Serial.print("Initializing MAX30100..");
  if (!sensor.begin())
  {
    Serial.println("FAILED");
    while (true)
      ;
  }
  else
  {
    Serial.println("SUCCESS");
  }

  // Configure MAX30100 sensor
  sensor.setMode(MAX30100_MODE_SPO2_HR);
  sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
  sensor.setLedsPulseWidth(PULSE_WIDTH);
  sensor.setSamplingRate(SAMPLING_RATE);
  sensor.setHighresModeEnabled(HIGHRES_MODE);

  // Register heartbeat detection callback
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop()
{
  uint16_t ir, red;

  // Update sensor readings
  pox.update();    // For BPM and beat detection
  sensor.update(); // For raw IR/Red values

  tv_act = millis();

  // Update OLED display every tv_int milliseconds
  if (tv_act - tv_ant >= tv_int)
  {
    tv_ant = tv_act;
    int hr = (int)pox.getHeartRate();

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0 * LINE_SIZE1);
    display.print(F("Ritmo Cardiaco"));
    display.setTextSize(2);
    display.setCursor(0, 1 * LINE_SIZE2);
    display.print(String(hr) + " BPM");
    display.display();
  }

  // Send raw IR data via serial every REPORTING_PERIOD_MS milliseconds
  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {
    sensor.getRawValues(&ir, &red);        // Read raw IR and red light values
    tx2.infrarojo = ir * -10;              // Send inverted IR value for visualization
    Serial.write(tx2.bufferDatos, TAMANO); // Send data as byte array
    delay(10);                             // Small delay to stabilize serial output
    tsLastReport = millis();
  }
}
