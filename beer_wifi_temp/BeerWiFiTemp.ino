// *********** OLED Display Options
#include <Wire.h>
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, D12, D11);

// *********** Dallas Temperatute Options
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS D8
#define RelayPin D2
#define Rel_ON LOW
#define Rel_OFF HIGH
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Temp1Thermometer = { 0x28, 0xFF, 0x7A, 0x3A, 0x93, 0x15, 0x01, 0x4A };
DeviceAddress Temp2Thermometer = { 0x28, 0xFF, 0xF7, 0x67, 0x93, 0x15, 0x03, 0x94 };
float MinTemp = 999, MaxTemp = -999;
float tempC = 0;
float tempRoom = 0;
float BeerTT = 0;
unsigned long _time1 = 0;
unsigned long _time2 = 0;

// *********** WiFi Options
#include <ESP8266WiFi.h>
const char* ssid = "hudnet";
const char* password = "7950295932";
WiFiServer server(80);

void setup() {
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, Rel_OFF);

  EEPROM.begin(512);
  EEPROM.get(0, BeerTT);
  if (BeerTT == 0) {
    BeerTT = 20;
    EEPROM.put(0, BeerTT);
    EEPROM.commit();
  }

  Serial.begin(115200);
  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  WiFi.mode(WIFI_STA);

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  sensors.begin();
  sensors.setResolution(Temp1Thermometer, 11);
  sensors.setResolution(Temp2Thermometer, 10);

  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_10);
  _time1 = millis();
  _time2 = millis() + 5001;
}

void loop() {
  if (_time2 - _time1 > 5000) {
    sensors.requestTemperatures(); // Send the command to get temperatures
    tempC = sensors.getTempC(Temp1Thermometer);
    if (tempC < MinTemp) MinTemp = tempC;
    if (tempC > MaxTemp) MaxTemp = tempC;

    // reset is temp fail
    if (MinTemp == -127) MinTemp = tempC;
    if (MaxTemp == -127) MaxTemp = tempC;

    tempRoom = sensors.getTempC(Temp2Thermometer);
    _time1 = millis();
  }
  _time2 = millis();


  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "IP " + WiFi.localIP().toString());

  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 13, "Min " + String(MinTemp) + " C");
  display.setFont(ArialMT_Plain_10);
  display.drawString(60, 13, "Max " + String(MaxTemp) + " C");

  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 25, "Room: " + String(tempRoom) + " C");

  display.drawString(75, 25, "tt: " + String(BeerTT) + " C");

  display.setFont(ArialMT_Plain_24);
  display.drawString(25, 42, String(tempC) + " C");
  display.display();

  // RefrigeratorControl
  if (tempC >= BeerTT + 0.5)
    digitalWrite(RelayPin, Rel_ON);
  else if (tempC < BeerTT - 0.5)
    digitalWrite(RelayPin, Rel_OFF);


  // Check if a client has connected
  WiFiClient client = server.available();

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  String s = "";
  if (req.indexOf("/gettemp") != -1)
    s = "Temp:" + String(tempC) + ", Min:" + String(MinTemp) + ", Max:" + String(MaxTemp) + ", Room:" + String(tempRoom);
  else if (req.indexOf("/reset") != -1)
  {
    MinTemp = tempC;
    MaxTemp = tempC;
    s = "Temp is reset!";
  }
  else if (req.indexOf("/set") != -1)
  {
    if (req.indexOf("?tt=") != -1) {
      String _s = req.substring(req.indexOf("?tt=") + 4);
      BeerTT = _s.toInt();
      EEPROM.begin(512);
      EEPROM.put(0, BeerTT);
      EEPROM.commit();
      s = "Terget temp set to "+String(BeerTT);
    }
  }
  else {
    client.stop();
  }

  if (s != "")
  {
    // Send the response to the client
    client.print(s);
    delay(1);
    //  Serial.println("Client disonnected");
  }
}
