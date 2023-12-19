#include <Arduino.h>
#include <Wire.h>
#include <WiFiNINA.h>
#include <Arduino_MKRIoTCarrier.h>
#include <ThingSpeak.h>
#include "config.h"

#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;
MKRIoTCarrier carrier;
char ssid[] = WIFI_NAME;       // your network SSID (name)
char pass[] = WIFI_PASSWORD;   // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;

WiFiClient wifiClient;  // Use WiFiClient for HTTP

float lat = LAT;
float lng = LONG;
char weatherAPI[] = WEATHER_API;

// Global variables for inside temperature, humidity, and pressure
float inTemperature;
float inHumidity;
float inPressure;

// Global variables for outside temperature, humidity, and pressure
float outTemperature;
float outHumidity;
float outPressure;

HttpClient client = HttpClient(wifiClient, "api.openweathermap.org", 80);  // Use port 80 for HTTP

void setupWiFi() {
 // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(INTERVAL);
  }
  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
}

void setup() {

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  setupWiFi();
  ThingSpeak.begin(wifiClient);
  carrier.begin();

}

void displayButtonText(int text, int x, int y) {
  carrier.display.setCursor(x, y);
  carrier.display.setTextSize(2);
  carrier.display.setTextColor(0xFFFF); // White color, you can change it as needed
  carrier.display.print(text);
}

void outsideWeather() {
    // Construct the URL with variables
  String request = "/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lng) + "&units=metric&appid=" + String(weatherAPI);

  // Make a GET request to OpenWeatherMap API
  if (client.get(request) != 0) {
  Serial.println("Failed to connect to OpenWeatherMap API");
  return;
  }

  Serial.print("API Request URL: ");
  Serial.println(request);

  // Read the response
  Serial.println("HTTP Response Code: " + String(client.responseStatusCode()));
  String response = client.responseBody();

  // Parse JSON
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + 270;
  DynamicJsonDocument doc(capacity);
  Serial.println("Attempting JSON parsing...");
  deserializeJson(doc, response);
  Serial.println("JSON parsing complete.");

  Serial.println("API Response:");
  Serial.println(response);

  // Extract temperature, humidity, and pressure
  outTemperature = doc["main"]["temp"];
  outHumidity = doc["main"]["humidity"];
  outPressure = doc["main"]["pressure"];

  // Print the values
  Serial.print("Outdoor Temperature: ");
  Serial.println(outTemperature);
  Serial.print("Outdoor Humidity: ");
  Serial.println(outHumidity);
  Serial.print("Outdoor Pressure: ");
  Serial.println(outPressure);
}

void insideWeather() {

 // read the sensor values
  inTemperature = carrier.Env.readTemperature();
  inHumidity = carrier.Env.readHumidity();
  inPressure = carrier.Pressure.readPressure();

}


void loop() {

  outsideWeather();

  insideWeather();

  // set the fields with the values
  ThingSpeak.setField(1, inTemperature);
  ThingSpeak.setField(2, inHumidity);
  ThingSpeak.setField(3, inPressure);
  ThingSpeak.setField(4, outTemperature);
  ThingSpeak.setField(5, outHumidity);
  ThingSpeak.setField(6, outPressure);

 // repeat background for new inputs
  carrier.display.fillScreen(0x0000);

 // Display text near each button
  displayButtonText(inTemperature, 30, 70);
  displayButtonText(inHumidity, 160, 70);
  displayButtonText(inPressure, 160, 150);
  displayButtonText(0, 100, 200);
  displayButtonText(0, 30, 150);


  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
   if(x == 200){
    Serial.println("Channel update successful.");

  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

  delay(INTERVAL);
}