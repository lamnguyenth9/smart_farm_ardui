

#define APP_DEBUG


#define USE_NODE_MCU_BOARD
//#define USE_WITTY_CLOUD_BOARD
//#define USE_WEMOS_D1_MINI

#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>


#define ss 15
#define rst 16
#define dio0 4 


#define WIFI_SSID "Lau 3_3"
#define WIFI_PASSWORD "147147147"
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define API_KEY "AIzaSyCzd2fiHPcXUPLRQZd_oWg52L3SeXDmJnw"
#define DATABASE_URL "https://lora-smartfarming-default-rtdb.firebaseio.com/"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String counter;
String soilMoistureValue;
String soilmoisturepercent;
String soiltemp;
String temperature;
String humidity;
FirebaseData fbdo;
FirebaseConfig config;
FirebaseAuth auth;
bool signUpOK = false;
unsigned long sendDataPrevMillis = 0;
byte MasterNode = 0xFF;
byte Node1 = 0xBB;
byte Node2 = 0xCC;
String SenderNode = "";
String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
String incoming = "";

unsigned long previousMillis = 0;
unsigned long int previoussecs = 0;
unsigned long int currentsecs = 0;
unsigned long currentMillis = 0;
int interval = 1 ; // updated every 1 second
int Secs = 0;


void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.print("Dang ket noi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  while (!Serial);
  Serial.println ("");
  Serial.println ("Da ket noi WiFi!");
  Serial.println(WiFi.localIP());
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signUpOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  

  Serial.println("LoRa Receiver");

  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)
  display.clearDisplay();
}

void loop() {
  
  // try to parse packet
  int pos1, pos2, pos3, pos4, pos5;
  
  currentMillis = millis();
  currentsecs = currentMillis / 1000;
  if ((unsigned long)(currentsecs - previoussecs) >= interval) {
    Secs = Secs + 1;
    //Serial.println(Secs);
    if ( Secs >= 11 )
    {
      Secs = 0;
    }
    if ( (Secs >= 1) && (Secs <= 5) )
    {
      String message = "10";
      sendMessage(message, MasterNode, Node1);
    }
    if ( (Secs >= 6 ) && (Secs <= 10))
    {
      String message = "20";
      sendMessage(message, MasterNode, Node2);
    }
    previoussecs = currentsecs;
  }
  onReceive( LoRa.parsePacket());
}
void sendMessage(String outgoing, byte MasterNode, byte otherNode) {
   LoRa.beginPacket();                   // start packet
  LoRa.write(otherNode);              // add destination address
  LoRa.write(MasterNode);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}
void onReceive(int packetSize) {
  int pos1, pos2, pos3, pos4, pos5;
  if (packetSize == 0) return;         
  // read packet header bytes:
  int recipient =  LoRa.read();          // recipient address
  byte sender =  LoRa.read(); 
  Serial.println(sender);           // sender address
  if ( sender == 0XBB )
    SenderNode = "Node1:";
  if ( sender == 0XCC )
    SenderNode = "Node2:";
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();  
  String incoming = "";
  Serial.println(incomingLength);  // incoming msg length
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
  Serial.println(incoming);
  if (incomingLength != incoming.length()) {   
    ;
    return;                             // skip rest of function
  }
  // if the recipient isn't this device or broadcast,
  if (recipient != Node1 && recipient != MasterNode) {
    
    ;
    return;                             // skip rest of function
  }
    if(sender==0XBB){
    pos1 = incoming.indexOf('/');
    pos2 = incoming.indexOf('&');
    pos3 = incoming.indexOf('#');
    pos4 = incoming.indexOf('@');
    pos5 = incoming.indexOf('$');
    
    counter = incoming.substring(0, pos1);
    soilMoistureValue = incoming.substring(pos1 + 1, pos2);
    soilmoisturepercent = incoming.substring(pos2 + 1, pos3);
    soiltemp = incoming.substring(pos3 + 1, pos4);
    temperature = incoming.substring(pos4 + 1, pos5);
    humidity = incoming.substring(pos5 + 1, incoming.length());

    float soilMoisturepercent1 = soilmoisturepercent.toFloat();
    float soiltemp1 = soiltemp.toFloat();
    float temperature1 = temperature.toFloat();
    float humidity1 = humidity.toFloat();

    //send data to Firebase
    
    if (Firebase.ready() && signUpOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "test/soilmoisturepercent", soilMoisturepercent1)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo, "test/soiltemp", soiltemp1)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo, "test/temperature", temperature1)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo, "test/humidity", humidity1)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
    
   


   

    Serial.print(F("Packet No = "));
    Serial.println(counter);

    Serial.print(F("Soil Moisture: "));
    Serial.print(soilmoisturepercent);
    Serial.println(F("%"));

    Serial.print(F("Soil Temperature: "));
    Serial.print(soiltemp);
    Serial.println(F("°C"));

    Serial.print(F("Temperature: "));
    Serial.print(temperature);
    Serial.println(F("°C"));

    Serial.print(F("Humidity = "));
    Serial.print(humidity);
    Serial.println(F("%"));

    Serial.print("Soil Moisture Value: ");
    Serial.print(soilMoistureValue);

    Serial.println();

    if (soilmoisturepercent.toInt() > 100)
    {
      display.clearDisplay();

      // display Soil Humidity
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(0, 5);
      display.print("RH of Soil: ");
      display.print("100");
      display.print(" %");

      // display soil temperature
      display.setCursor(0, 20);
      display.print("Soil Temp: ");
      display.print(soiltemp);
      display.print(" ");
      display.cp437(true);
      display.write(167);
      display.print("C");

      // display air temperature
      display.setCursor(0, 35);
      display.print("Air Temp: ");
      display.print(temperature);
      display.print(" ");
      display.cp437(true);
      display.write(167);
      display.print("C");

      // display relative humidity of Air
      display.setCursor(0, 50);
      display.print("RH of Air: ");
      display.print(humidity);
      display.print(" %");

      display.display();
      
    }
    else if (soilmoisturepercent.toInt() < 0)
    {
      display.clearDisplay();

      // display Soil Humidity
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(0, 5);
      display.print("RH of Soil: ");
      display.print("0");
      display.print(" %");

      // display soil temperature
      display.setCursor(0, 20);
      display.print("Soil Temp: ");
      display.print(soiltemp);
      display.print(" ");
      display.cp437(true);
      display.write(167);
      display.print("C");

      // display air temperature
      display.setCursor(0, 35);
      display.print("Air Temp: ");
      display.print(temperature);
      display.print(" ");
      display.cp437(true);
      display.write(167);
      display.print("C");

      // display relative humidity of Air
      display.setCursor(0, 50);
      display.print("RH of Air: ");
      display.print(humidity);
      display.print(" %");

      display.display();
      
    }
    else if (soilmoisturepercent.toInt() >= 0 && soilmoisturepercent.toInt() <= 100)
    {
      display.clearDisplay();

      // display Soil humidity
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(0, 5);
      display.print("RH of Soil: ");
      display.print(soilmoisturepercent);
      display.print(" %");

      // display soil temperature
      display.setCursor(0, 20);
      display.print("Soil Temp: ");
      display.print(soiltemp);
      display.print(" ");
      display.cp437(true);
      display.write(167);
      display.print("C");

      // display air temperature
      display.setCursor(0, 35);
      display.print("Air Temp: ");
      display.print(temperature);
      display.print(" ");
      display.cp437(true);
      display.write(167);
      display.print("C");

      // display relative humidity of Air
      display.setCursor(0, 50);
      display.print("RH of Air: ");
      display.print(humidity);
      display.print(" %");

      display.display();
      
    }
    if (soilmoisturepercent.toInt() >= 0 && soilmoisturepercent.toInt() <= 30)
    {
      Serial.println("needs water, send notification");
      
      Serial.println("Motor is ON");
      
    }
    else if (soilmoisturepercent.toInt() > 30 && soilmoisturepercent.toInt() <= 100)
    {
      Serial.println("Soil Moisture level looks good...");
      Serial.println("Motor is OFF");
      
    }  
}
}
