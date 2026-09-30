# Desktop-buddy

## Bill of Materials
The project needs an esp32 (ESP32-WROOM-32), 0.96 inch oled display,  lm393 sound detection sensor,  hw-506 temperature sensor, 2 tactile switch buttons, breadboard and jumper wires.
No soldering needed.

## User guide
Initially Buddy will greet you, who is showing the time, day and the temperature of your room.

By pressing the upper button the focus timer will come up. With the buttons the default 25 minutes of focus time can be incremented or decremented. If pressing both, timer mode will be canceled and Buddy will reappear.

By pressing the lower button, you can play a simple pong game. After the game, Buddy will reappear, and the score will be updated.

By playing the happines bar can be increased.

The happiness bar decreases over time (16 minutes, 32 if in game or using timer), and the expression of Buddy changes accordingly.

After 20 seconds of inactivity, Buddy will close his eyes and can be woken up by making a sound.

## Code features
Offline mode if unable to connect to wifi.
Use of IRAM_ATTR when detecting sound.
Bot slowly following the ball in Pong, providing a reallistic challange.
Updating smile based on happiness. 
Random blinking interval.

## Possible future updates
Deep sleep mode for better power consumption.
Touch sensor for petting Buddy, which would increase the happiness.
3D printing a custom case and including battery and charging mode.

## Wiring
Sound sensor:       OUT -> Pin 19
                    GND -> GND
                    +5V -> VIN

Temp sensor:        S -> Pin 23
                    - -> GND
                    + -> 3V3

Oled display:       SCL -> Pin 22
                    SDA -> Pin 21
                    GND -> GND
                    VCC -> 3V3

Up button:          Left leg -> Pin 4
                    Right leg -> GND

Down button:        Left leg -> Pin 5
                    Right leg -> GND

## Dependencies
Adafruit_GFX, Adafruit_SSD1306, Wire, time.h, WiFi, OneWire, DallasTemperature

## Installation

### Prerequisites
1. Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the ESP32 board package in the Arduino IDE (follow [Espressif's official guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) if you haven't done this before).

### Setup Steps
1. **Clone the repository:**
   ```bash
   git clone (https://github.com/SzekelyK05/Desktop-buddy.git)

(Or download the repository as a ZIP file and extract it).
2. Open the sketch: Open the src/main.ino file in the Arduino IDE.
3. Install dependencies: Go to Tools > Manage Libraries... in the Arduino IDE, search for and install the following:

        Adafruit GFX Library
        Adafruit SSD1306
        OneWire
        DallasTemperature

Configure your board:

Go to Tools > Board and select ESP32 Dev Module (or your specific ESP32 variant).

Go to Tools > Port and select the USB port your ESP32 is connected to.

Flash the firmware: Click the Upload button to compile and push the code to your board.
