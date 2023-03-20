#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include <moonPhase.h>

// Pin definitions
#define SCLK 18
#define MOSI 23
#define CS   17
#define RST  4
#define DC   16

// Color definitions
#define RED       0xF800
#define GREEN     0x07E0
#define LESS_BRIGHT_CYAN 0x03FF
#define YELLOW    0xFFE0
#define BLACK     0x0000
#define PURPLE    0x780F

// Wi-Fi credentials
const char* ssid = "KooZoo";
const char* password = "katrinzrk";

// Objects and variables
Adafruit_SSD1331 display = Adafruit_SSD1331(CS, DC, MOSI, SCLK, RST);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 600000; // Sync interval (10 minutes)

bool ntpSynced = false;

String prevFormattedTime = "";
String prevFormattedDate = "";
String prevLastSync = "";
String prevMoonIllumination = "";
String prevNextFullMoon = "";

const char* daysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void initDisplay() {
  display.begin();
  display.fillScreen(BLACK);
  display.setTextSize(1);
  display.setTextWrap(false);
}

String formatTwoDigitNumber(int number) {
  if (number < 10) {
    return "0" + String(number);
  }
  return String(number);
}

void updateFormattedTime(struct tm* timeStruct) {
  String formattedTime = formatTwoDigitNumber(timeStruct->tm_hour) + ":" + formatTwoDigitNumber(timeStruct->tm_min) + ":" + formatTwoDigitNumber(timeStruct->tm_sec);
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
  String formattedDate = String(daysOfWeek[timeStruct->tm_wday]) + " " + formatTwoDigitNumber(timeStruct->tm_mday) + "/" + formatTwoDigitNumber(timeStruct->tm_mon + 1) + "/" + String(timeStruct->tm_year + 1900);
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
  String lastSync = "NTP sync: " + formatTwoDigitNumber(timeStruct->tm_hour) + ":" + formatTwoDigitNumber(timeStruct->tm_min);
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
  time_t currentTime = timeClient.getEpochTime(); // Get the current timestamp
  moonData_t moon = moonPhaseInstance.getPhase(currentTime); // Pass the timestamp to the getPhase function
  String moonIllumination = "Moon lit: " + String(moon.percentLit * 100, 2) + "%"; // Change the second argument to 2

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
  time_t nextFullMoonTimestamp = currentTime;

  while (true) {
    moonData_t moonData = moonPhaseInstance.getPhase(nextFullMoonTimestamp);
    if (moonData.percentLit >= 0.99) {
      break;
    }
    nextFullMoonTimestamp += 86400; // Add one day (86400 seconds)
  }

  struct tm* nextFullMoonStruct = localtime(&nextFullMoonTimestamp);
  String nextFullMoon = "Full moon: " + formatTwoDigitNumber(nextFullMoonStruct->tm_mday) + "/" + formatTwoDigitNumber(nextFullMoonStruct->tm_mon + 1);

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

/**
   Update the clock display including time, date, last NTP sync, moon illumination,
   and next full moon.
*/
void updateClock() {
  timeClient.update();

  time_t currentTime = timeClient.getEpochTime();
  struct tm* timeStruct = localtime(&currentTime);

  updateFormattedTime(timeStruct);
  updateFormattedDate(timeStruct);
  if (ntpSynced) { // Only update after NTP sync occurs
    updateLastNTPSync(timeStruct);
    ntpSynced = false; // Reset the flag
  }
  updateMoonIllumination();

  if (millis() - lastSyncTime > syncInterval) {
    timeClient.forceUpdate();
    currentTime = timeClient.getEpochTime();
    timeStruct = localtime(&currentTime);
    lastSyncTime = millis();
    ntpSynced = true; // Set the flag to true when NTP sync occurs
    updateNextFullMoon();
  }
}

/**
   Connect to Wi-Fi and set up initial state of the display.
*/
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
  initDisplay();

  // Initialize time client
  timeClient.begin();
  timeClient.setTimeOffset(2 * 60 * 60); // Set time offset to GMT+2

  // Force initial update and set prevLastSync
  timeClient.forceUpdate();
  time_t currentTime = timeClient.getEpochTime();
  struct tm* timeStruct = localtime(&currentTime);
  prevLastSync = "NTP sync: " + formatTwoDigitNumber(timeStruct->tm_hour) + ":" + formatTwoDigitNumber(timeStruct->tm_min);
  updateNextFullMoon();

  // Display the third line immediately after the first NTP sync
  display.setTextColor(LESS_BRIGHT_CYAN);
  display.setCursor(0, 24);
  display.print(prevLastSync);

  lastSyncTime = millis();
}

/**
   Updates the clock display at a regular 100ms interval.
*/
void loop() {
  updateClock();
  delay(100);
}
