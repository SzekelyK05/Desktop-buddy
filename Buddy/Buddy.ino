#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//wifi
const char* ssid     = "yourssid";
const char* password = "yourpassword";

const char* ntpServer = "pool.ntp.org";

//buttons
const int up=4;
const int down=5;

//sound sensor
const int SENSOR_PIN=19;
bool detected=false;

//temp sensor
const int temp_sensor=23;
OneWire oneWire(temp_sensor);
DallasTemperature sensors(&oneWire);
String degree = "-- \xF8" "C";

//states
enum option {face, game_menu, game, timer};
option State;
bool isSleeping=false;
bool online;

//timer variables
const unsigned long INACTIVITY_TIMEOUT = 5000;
unsigned long lastActivityTime = 0;
unsigned long lastTickTime = 0;
int timerMinutes = 25;
int timeLeftSeconds = 0;
bool isCountingDown = false;
bool lastStateUp   = HIGH;
bool lastStateDown = HIGH;

//pong variables
int playerY=31;
int botY=32;
int ballX=63;
int ballY=31;
int vx=3;
int vy=3;
int playerScore=0;
int botScore=0;

//Face animation variables
unsigned long lastBlinkTime = 0;
int blinkInterval = 3000;
unsigned long blinkDuration = 150;
bool isBlinking = false;
unsigned long lastWakeTime = 0;
const unsigned long AWAKE_DURATION = 20000;

//Happiness variables
int happiness = 100;
unsigned long lastHappinessDrop = 0;
const unsigned long HAPPINESS_DROP_INTERVAL = 10000; // Loses 1 point every 10 seconds
unsigned int HAPPINESS_DROP_INTERVAL_ACTUAL;


void IRAM_ATTR onSoundHeard() {
  detected = true;
  lastWakeTime = millis();
}

void setup() {
  Serial.begin(115200);
  pinMode(up, INPUT_PULLUP);
  pinMode(down, INPUT_PULLUP);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), onSoundHeard, FALLING);
  Wire.begin(21, 22);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  updateDisplay("Buddy", "Starting...");

  Serial.printf("connecting to: %s ", ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // 10-second timeout
      delay(500);
      Serial.print(".");
      attempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
      Serial.println("Success!");
      configTzTime("EET-2EEST,M3.5.0/3,M10.5.0/4", ntpServer);
      updateDisplay("Connected!", "Hi");
      online=true;
  } else {
      Serial.println("WiFi Failed.");
      updateDisplay("Connection fail", "Offline mode");
      online=false;
  }
  delay(1000);
  
  // romania(utc+2/ +3 summer)
  configTzTime("EET-2EEST,M3.5.0/3,M10.5.0/4", ntpServer);

  pinMode(temp_sensor, INPUT_PULLUP);
  sensors.begin();
  Serial.print("Sensors found: ");
  Serial.println(sensors.getDeviceCount());
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  State=face;

}

void loop() {

  //wake up when interacting
  if (digitalRead(up) == LOW || digitalRead(down) == LOW) {
    isSleeping = false;
    lastWakeTime = millis();
  }

  //happiness decrease
  if(State!=face)
    HAPPINESS_DROP_INTERVAL_ACTUAL=HAPPINESS_DROP_INTERVAL;
  else
    HAPPINESS_DROP_INTERVAL_ACTUAL=HAPPINESS_DROP_INTERVAL*2;
  if (millis() - lastHappinessDrop >= HAPPINESS_DROP_INTERVAL_ACTUAL) {
    lastHappinessDrop = millis();
    if (happiness > 0) {
      happiness--;
    }
  }

  //temp
  static unsigned long lastTempUpdate = 0;
  if (millis() - lastTempUpdate >= 2000) {
    float tempC = sensors.getTempCByIndex(0);
    
    if(tempC == DEVICE_DISCONNECTED_C) {
      degree = "error";
    } else {
      degree = String(tempC, 1) + " \xF8" + "C"; 
    }
    
    sensors.requestTemperatures(); 
    lastTempUpdate = millis();
  }
  
  switch(State)
  {
    case face:  {
      static unsigned long lastFrameTime = 0;
  
      if (millis() - lastFrameTime >= 33) { 
        lastFrameTime = millis();
        updateFaceDisplay(getFormattedTime(), degree);
      }
  
      if (digitalRead(up) == LOW) {
        State = timer;
        lastStateUp = LOW; 
        delay(200); 
      }
      if (digitalRead(down) == LOW) {
        State = game_menu;
        delay(200); 
      }
      break;
    }

    case timer:
      handleTimer();
    break;

    case game_menu:
      handleMenu();
    break;

    case game:
      handleGame();
    break;

    default: break;
  }

}

void handleTimer()
{
  bool currentUp   = digitalRead(up);
  bool currentDown = digitalRead(down);
  bool anyButtonPressed = false;

  //increase min
  if (currentUp == LOW && lastStateUp == HIGH) {
    delay(30); 
    if (digitalRead(up) == LOW) {
      timerMinutes++;
      anyButtonPressed = true;
    }
  }

  //decrease min
  if (currentDown == LOW && lastStateDown == HIGH) {
    delay(30); 
    if (digitalRead(down) == LOW) {
      if (timerMinutes > 1) { 
        timerMinutes--;
      }
      anyButtonPressed = true;
    }
  }

  //exit
  if(currentDown == LOW && currentUp == LOW)
  {
    anyButtonPressed = true;
    isCountingDown = false;      
    lastActivityTime = millis(); 
    delay(300);
    State=face;
  }

  lastStateUp   = currentUp;
  lastStateDown = currentDown;

  if (anyButtonPressed) {
    lastActivityTime = millis();
    
    if (isCountingDown) {
      isCountingDown = false;
    }
    
    updateDisplay("Set the time: ", String(timerMinutes) + " minutes");
  }

  //start timer
  if (!isCountingDown && (millis() - lastActivityTime >= INACTIVITY_TIMEOUT)) {
    isCountingDown = true;
    timeLeftSeconds = timerMinutes * 60; 
    lastTickTime = millis();             
    
    updateDisplay("Starting...", "Done!");
  }

  //timer logic
  if (isCountingDown) {
    if (millis() - lastTickTime >= 1000) {
      lastTickTime += 1000; 
      
      if (timeLeftSeconds > 0) {
        timeLeftSeconds--;
        
        int m = timeLeftSeconds / 60;
        int s = timeLeftSeconds % 60;
        
        // Format MM:SS with leading zeros
        char timeStr[6];
        sprintf(timeStr, "%02d:%02d", m, s);
        
        updateDisplay("Time left:", String(timeStr));
      } else {
        isCountingDown = false;      
        lastActivityTime = millis(); 
        updateDisplay("DONE!", "00:00");
        delay(1000);
        State=face;
      }
    }
  }
}

void handleMenu()
{
    //Clear Display
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(40, 0);
    display.print("PONG");
    display.drawLine(0, 18, SCREEN_WIDTH, 18, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(14, 25);
    display.print("Press any button!");
    
    display.setCursor(20, 45);
    display.print("Score: ");
    display.print(playerScore);
    display.print("-");
    display.print(botScore);
    display.display();

    if(digitalRead(up)==LOW || digitalRead(down)==LOW)
    {
      playerY = 27;
      botY = 27;
      ballX = 62;
      ballY = 30;
      
      vx = (random(0, 2) == 0) ? -2 : 2;
      vy = (random(0, 2) == 0) ? -2 : 2;
      
      State = game;
      delay(200);
    }
}


void handleGame()
{
 //player movement
 if (digitalRead(up) == LOW) {
    playerY -= 3;
  }
  if (digitalRead(down) == LOW) {
    playerY += 3;
  }
  
  if(playerY < 16) playerY = 16;
  if(playerY > 54) playerY = 54;

  //bot movement
  static int frameCounter = 0;
  frameCounter++;

  int currentBotSpeed = (frameCounter % 2 == 0) ? 2 : 1;

  if (botY + 5 < ballY) {
    botY += currentBotSpeed; 
  } else if (botY + 5 > ballY) {
    botY -= currentBotSpeed;
  }
  
  
  if(botY < 16) botY = 16;
  if(botY > 54) botY = 54;

  //ball movement
  ballX+=vx;
  ballY+=vy;

  if(ballY >= 60) { ballY = 60; vy = -vy; }
  if(ballY <= 16) { ballY = 16; vy = -vy; }

  if (ballX <= 7 && ballX >= 5 && ballY + 4 >= playerY && ballY <= playerY + 10) {
    vx = -vx;
    ballX = 7;
  }
  
  if (ballX + 4 >= 123 && ballX <= 125 && ballY + 4 >= botY && ballY <= botY + 10) {
    vx = -vx;
    ballX = 119;
  }

  if (ballX < 0) {
    botScore++;
    happiness = min(100, happiness + 25);
    State = face;
    delay(650);
    return;
  } else if (ballX > 128) {
    playerScore++;
    happiness = min(100, happiness + 25);
    State = face;
    delay(750);
    return;
  }

  //Clear Display
  display.clearDisplay();
  
  //score
  display.setCursor(55, 3);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print(playerScore);
  display.print("|");
  display.print(botScore);
  
  //Draw Player and Bot
  display.fillRect(5, playerY, 2, 10, SSD1306_WHITE);
  display.fillRect(123, botY, 2, 10, SSD1306_WHITE);

  //Draw horizontal line
  display.drawLine(0, 15, SCREEN_WIDTH, 15, SSD1306_WHITE);

  //Draw vertical line
  for(int i = 16; i < 64; i += 4) {
     display.drawPixel(64, i, SSD1306_WHITE);
  }

  //draw ball
  display.fillRect(ballX, ballY, 4, 4, SSD1306_WHITE);

  
  display.display();
  delay(30);

}

String getFormattedTime() {
  if (!online) {
    return "Offline"; 
  }

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo, 10)){
    return "--:--";
  }
  
  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  
  char timeBuffer[10]; 
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
  
  return String(days[timeinfo.tm_wday]) + " " + String(timeBuffer); 
}

void updateDisplay(String top, String bottom) {
  
    display.clearDisplay();     
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(top);
    //display.drawLine(0, 10, 128, 10, WHITE);
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.println(bottom);
    display.display();          
  
}

void updateFaceDisplay(String time, String degrees) {
  unsigned long currentMillis = millis();

  if (detected) {
    isSleeping = false;
    detected = false;
  }

  if (!isSleeping && (currentMillis - lastWakeTime >= AWAKE_DURATION)) {
    isSleeping = true;
  }

  display.clearDisplay();     
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(time);
  display.setCursor(85, 0);
  display.print(degrees);
  
  if (!isBlinking && (currentMillis - lastBlinkTime > blinkInterval)) {
    isBlinking = true;
    lastBlinkTime = currentMillis;
  } else if (isBlinking && (currentMillis - lastBlinkTime > blinkDuration)) {
    isBlinking = false;
    lastBlinkTime = currentMillis;
    blinkInterval = random(2000, 5500);
  }

  int eyeY = 22;
  int eyeWidth = 16;
  int eyeHeight = 20;
  
  int leftEyeX = 32;  
  int rightEyeX = 80; 

  // Draw Eyes
  if(isSleeping) {
    display.fillRect(leftEyeX, eyeY + (eyeHeight/4), eyeWidth, 4, WHITE);
    display.fillRect(rightEyeX, eyeY + (eyeHeight/4), eyeWidth, 4, WHITE);
  }
  else if (isBlinking) {
    display.fillRect(leftEyeX, eyeY + (eyeHeight/2), eyeWidth, 4, WHITE);
    display.fillRect(rightEyeX, eyeY + (eyeHeight/2), eyeWidth, 4, WHITE);
  } else {
    display.fillRoundRect(leftEyeX, eyeY, eyeWidth, eyeHeight, 4, WHITE);
    display.fillRoundRect(rightEyeX, eyeY, eyeWidth, eyeHeight, 4, WHITE);
  }

  // Draw Mouth
  if(happiness >= 66) {
    display.fillRect(57, 45, 4, 3, WHITE);
    display.fillRect(67, 45, 4, 3, WHITE);
  }
  else if(happiness <= 33) {
    display.fillRect(57, 49, 4, 3, WHITE);
    display.fillRect(67, 49, 4, 3, WHITE);
  }
  display.fillRect(60, 47, 8, 3, WHITE);

  // Draw Happiness Bar
  display.drawRect(14, 61, 100, 3, WHITE);
  
  int fillWidth = map(happiness, 0, 100, 0, 96);
  if (fillWidth > 0) {
    display.fillRect(16, 61, fillWidth, 2, WHITE);
  }

  display.display();  
}


