
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2);
#define ONE_WIRE_BUS 14
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <FirebaseArduino.h>
// Set these to run example.
#define FIREBASE_HOST "wastewaterproj-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "uwC1tOurFYhLjuAQXmz3yW6JsB4vs30Eb2gxeQ3r"
// Replace with your WiFi credentials
const char* ssid = "dhanush";
const char* password = "dhanush200322";
// Replace with your ThingsBoard server details
const char* tbServer = "demo.thingsboard.io";
const int tbPort = 1883;  // MQTT port
// Replace with your ThingsBoard access token
const char* tbAccessToken = "r5Dwyl1S1zaRmlNDflUp";
const int turbidityPin = A0;  // Analog pin connected to the turbidity sensor
// Calibration curve parameters (adjust these based on your calibration data)
float calibrationSlope = 1.0;  // Slope of the calibration curve
float calibrationIntercept = 0.0;  // Intercept of the calibration curve
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;
void setup(void)
{
  pinMode(13,OUTPUT);
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  lcd.init();                       // Initialize the LCD
  lcd.backlight();                  // Turn on the backlight
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  mqttClient.setServer(tbServer, tbPort);
  lcd.clear();
  Serial.print("Locating devices...");
  sensors.begin();
  delay(1000);
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  sensors.setResolution(insideThermometer, 9);
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.println();
}
void printTemperature(DeviceAddress deviceAddress)
{
  
  float tempC = sensors.getTempC(deviceAddress);
  
  if (!mqttClient.connected()) {
    connectToThingsBoard();
  }
  mqttClient.loop();
  // Publish sample data to ThingsBoard
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"temperature\": %f}", tempC);
  mqttClient.publish("v1/devices/me/telemetry", payload);
  delay(1000); // Adjust the delay as needed
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.print(tempC);
  if(tempC>=40 || tempC<=20)
  {
     digitalWrite(13,HIGH);
     tone(13,6000,500);
     delay(1000);
     digitalWrite(13,LOW);
  }
  lcd.setCursor(0, 0); 
            // Set the cursor to the first column and first row
  lcd.print("Temperature  ");     // Print some text
  lcd.setCursor(0,1);
  lcd.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
  String name = Firebase.pushFloat("Temperature", tempC);
if (Firebase.failed()) {
      Serial.print("setting Temperature failed:");
      Serial.println(Firebase.error());  
      return;
  }
  Serial.println("Pushed Temperature ");
}

void loop(void)
{ 
  digitalWrite(13,LOW);
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");
  printTemperature(insideThermometer); 
  int sensorValue = analogRead(turbidityPin);
  

  float tssValue = convertToTSS(sensorValue); // Replace with your calibration function
  // Print the TSS value to the serial monitor
  Serial.print("TSS (mg/L): ");
  Serial.println(tssValue);
  // Serial.println(b);
   if (!mqttClient.connected()) {
    connectToThingsBoard();
  }
  mqttClient.loop();
  // Publish sample data to ThingsBoard
  
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"tss\": %f}", tssValue);
  mqttClient.publish("v1/devices/me/telemetry", payload);
  
  delay(1000); // Adjust the delay as needed
 String name = Firebase.pushFloat("TSS", tssValue);
  // Delay before taking the next reading
  delay(1000); // Adjust this delay as needed
  float ph=random(6,9);
  Serial.print("pH Location 1: ");
  Serial.println(ph);
   if (!mqttClient.connected()) {
    connectToThingsBoard();
  }
  
  mqttClient.loop();
  
  // Publish sample data to ThingsBoard
  
  char payload2[100];
  snprintf(payload2, sizeof(payload2), "{\"ph Location1\": %f}", ph);
  mqttClient.publish("v1/devices/me/telemetry", payload2);
  
  delay(2000); // Adjust the delay as needed
 String name2 = Firebase.pushFloat("pH Location1", ph);
  delay(1000); // Adjust this delay as needed
}


void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void connectToThingsBoard() {
  Serial.println("Connecting to ThingsBoard...");
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP8266", tbAccessToken, NULL)) {
      Serial.println("Connected to ThingsBoard");
      delay(1000);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

float convertToTSS(int rawValue) {

  float tss = calibrationSlope * rawValue + calibrationIntercept;
  return tss;
}
