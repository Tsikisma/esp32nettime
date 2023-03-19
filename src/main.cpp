#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include <moonPhase.h>

#define SCLK 18
#define MOSI 23
#define CS   17
#define RST  4
#define DC   16

// Color definitions
const uint16_t RED                = 0xF800;
const uint16_t GREEN              = 0x07E0;
const uint16_t LESS_BRIGHT_CYAN   = 0x03FF;
const uint16_t YELLOW             = 0xFFE0;
const uint16_t BLACK              = 0x0000;
const uint16_t PURPLE             = 0x780F;

Adafruit_SSD1331 display(CS, DC, MOSI, SCLK, RST);

// WiFi network information
const char* ssid = "KooZoo";
const char* password = "katrinzrk";

// Time sync via NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
unsigned long lastSyncTime = 0;
const unsigned long SYNC_INTERVAL = 10 * 60 * 1000; // Sync interval (10 minutes)

// Display strings for clock, date, etc.
String prevFormattedTime          = "";
String prevFormattedDate          = "";
String prevLastSync               = "";
String prevMoonIllumination       = "";
String prevNextFullMoon           = "";

// Array of days of the week
const char* DAYS_OF_WEEK[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void initializeDisplay() {
  display.begin();
  display.fillScreen(BLACK);
  display.setTextSize(1);
  display.setTextWrap(false);
}

String convertTwoDigitNumberToString(int number) {
  return (number < 10 ? "0" : "") + String(number);
}

void updateFormattedTime(struct tm* timeStruct) {
  String formattedTime = convertTwoDigitNumberToString(timeStruct->tm_hour) + ":" +
                          convertTwoDigitNumberToString(timeStruct->tm_min) + ":" +
                          convertTwoDigitNumberToString(timeStruct->tm_sec);
  if (formattedTime != prevFormattedTime) {
    display.setTextColor(BLACK);
    display.setCursor(0, 0);
    display.print(prevFormattedTime);

    display.setTextColor(RED);
    display.setCursor(0, 0);
    display.print(formattedTime);
    prevFormattedTime = formattedTime;
  }
}

void updateFormattedDate(struct tm* timeStruct) {
  String formattedDate = String(DAYS_OF_WEEK[timeStruct->tm_wday]) + " " +
                          convertTwoDigitNumberToString(timeStruct->tm_mday) + "/" +
                          convertTwoDigitNumberToString(timeStruct->tm_mon + 1) + "/" +
                          String(timeStruct->tm_year + 1900);
  if (formattedDate != prevFormattedDate) {
    display.setTextColor(BLACK);
    display.setCursor(0, 12);
    display.print(prevFormattedDate);

    display.setTextColor(GREEN);
    display.setCursor(0, 12);
    display.print(formattedDate);
    prevFormattedDate = formattedDate;
  }
}

void updateLastNTPSync(struct tm* timeStruct) {
  String lastSync = "NTP sync: " + convertTwoDigitNumberToString(timeStruct->tm_hour) + ":" +
                     convertTwoDigitNumberToString(timeStruct->tm_min);
  if (lastSync != prevLastSync) {
    display.setTextColor(BLACK);
    display.setCursor(0, 24);
    display.print(prevLastSync);

    display.setTextColor(LESS_BRIGHT_CYAN);
    display.setCursor(0, 24);
    display.print(lastSync);
    prevLastSync = lastSync;
  }
}

void updateMoonIllumination() {
  moonPhase moonPhaseInstance;
  moonData_t moon = moonPhaseInstance.getPhase();
  String moonIllumination = "Moon lit: " + String(moon.percentLit * 100, 1) + "%";

  if (moonIllumination != prevMoonIllumination) {
    display.setTextColor(BLACK);
    display.setCursor(0, 36);
    display.print(prevMoonIllumination);

    display.setTextColor(YELLOW);
    display.setCursor(0, 36);
    display.print(moonIllumination);
    prevMoonIllumination = moonIllumination;
  }
}

void updateNextFullMoon() {
  moonPhase moonPhaseInstance;
  time_t currentTime = timeClient.getEpochTime();

  // Search for next Full Moon iteratively
  time_t nextFullMoonTimestamp = currentTime;
  while (true) {
    moonData_t moonData = moonPhaseInstance.getPhase(nextFullMoonTimestamp);
    if (moonData.percentLit >= 0.99) {
      break;
    }
    nextFullMoonTimestamp += 86400; // Add one day in seconds (86400 seconds = 1 day)
  }

  struct tm* nextFullMoonStruct = localtime(&nextFullMoonTimestamp);
  String nextFullMoon = "Full moon: " + convertTwoDigitNumberToString(nextFullMoonStruct->tm_mday) + "/" +
                         convertTwoDigitNumberToString(nextFullMoonStruct->tm_mon + 1);

  if (nextFullMoon != prevNextFullMoon) {
    display.setTextColor(BLACK);
    display.setCursor(0, 48);
    display.print(prevNextFullMoon);

    display.setTextColor(PURPLE);
    display.setCursor(0, 48);
    display.print(nextFullMoon);
    prevNextFullMoon = nextFullMoon;
  }
}

void updateClock() {
  // Update UTC time using NTP server
  timeClient.update();
  time_t currentTime = timeClient.getEpochTime();
  struct tm* timeStruct = localtime(&currentTime);

  // Update LCD screen with current time, date, and other info
  updateFormattedTime(timeStruct);
  updateFormattedDate(timeStruct);
  updateLastNTPSync(timeStruct);
  updateMoonIllumination();

  // Force NTP update at sync interval and update full moon status
  if (millis() - lastSyncTime > SYNC_INTERVAL) {
    timeClient.forceUpdate();
    currentTime = timeClient.getEpochTime();
    timeStruct = localtime(&currentTime);
    lastSyncTime = millis();
    updateNextFullMoon(); // Add this line
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Initialize display
  initializeDisplay();

  // Initialize time client
  timeClient.begin();
  timeClient.setTimeOffset(2 * 60 * 60); // Set time offset to GMT+2
  
  // Force initial update and set prevLastSync
  timeClient.forceUpdate();
  time_t currentTime = timeClient.getEpochTime();
  struct tm* timeStruct = localtime(&currentTime);
  prevLastSync = "NTP sync: " + convertTwoDigitNumberToString(timeStruct->tm_hour) + ":" +
                  convertTwoDigitNumberToString(timeStruct->tm_min);
  updateNextFullMoon();

  // Display NTP sync immediately after initialization
  display.setTextColor(LESS_BRIGHT_CYAN);
  display.setCursor(0, 24);
  display.print(prevLastSync);

  lastSyncTime = millis();
}

void loop() {
  // Loop through updates
  updateClock();
  delay(100);
}
