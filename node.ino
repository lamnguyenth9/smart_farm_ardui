#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
 
#define DHTPIN 5          //pin where the dht22 is connected
DHT dht(DHTPIN, DHT11);
#define relay 7
#define ss 10
#define rst 9
#define dio0 2
#define ONE_WIRE_BUS 6
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

String LoRaMessage = "";
int counter = 0;

const int AirValue = 590;   //you need to replace this value with Value_1
const int WaterValue = 300;  //you need to replace this value with Value_2
const int SensorPin = A0;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
int soilmoisturepercent1 = 0;
float temp;
float h ;
float t ;
String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte MasterNode = 0xFF;
byte Node1 = 0xBB;
String Mymessage = "";


void setup() 
{
  Serial.begin(115200);
  dht.begin();
  sensors.begin(); // Dallas temperature
  pinMode(relay, OUTPUT);
  
  while (!Serial);
  Serial.println("LoRa Sender");
  LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    delay(100);
    while (1);
  }
}
 
void loop() 
{
  soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, 0, 1023, 0, 100);
  int soilmoisturepercent1 = 100 - soilmoisturepercent;
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
 
  if (isnan(h) || isnan(t)) 
  {
  Serial.println("Failed to read from DHT sensor!");
  return;
  }
  Serial.print("Soil Moisture Value: ");
  Serial.println(soilMoistureValue);
  
  Serial.print("Soil Moisture: ");
  Serial.print(soilmoisturepercent);
  Serial.println("%");
  
  Serial.print("Soil Temperature: ");
  Serial.print(temp);
  Serial.println("°C");
  
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("°C");
  
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println("%");
  Serial.println("");
  
  Serial.print("Sending packet: ");
  Serial.println(counter);
  
    if (soilmoisturepercent1 >= 30)
  {
    Serial.println("Soil Moisture level looks good..."); 
    digitalWrite(relay, LOW);
    Serial.println("Motor is OFF");
    //WidgetLED PumpLed(V5);
    //PumpLed.on();
     }
  else 
  {
    Serial.println("Plants need water..., notification sent");
    digitalWrite(relay, HIGH);
    Serial.println("Motor is ON");
    //WidgetLED PumpLed(V5);
    //PumpLed.off();
  }
  
  onReceive(LoRa.parsePacket());
 
  counter++;
  delay(1000);
  
}
void sendMessage(String outgoing, byte MasterNode, byte Node1) {
   LoRa.beginPacket();                   // start packet
   LoRa.write(MasterNode);              // add destination address
  LoRa.write(Node1);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}
void onReceive(int packetSize) {
  soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, 0, 1023, 0, 100);
  int soilmoisturepercent1 = 100 - soilmoisturepercent;
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  if (packetSize == 0) return;          // if there's no packet, return
  // read packet header bytes:
  int recipient =  LoRa.read();          // recipient address
  byte sender =  LoRa.read();  
  Serial.println(sender);          // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length
  String incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
  if (incomingLength != incoming.length()) {   // check length for error
    // Serial.println("error: message length does not match length");
    ;
    return;                             // skip rest of function
  }
  // if the recipient isn't this device or broadcast,
  if (recipient != Node1 && recipient != MasterNode) {
    //Serial.println("This message is not for me.");
    ;
    return;                             // skip rest of function
  }
  Serial.println(incoming);
  int Val = incoming.toInt();
  if (Val == 10)
  {
    Mymessage =String(counter) +"/" + String(soilMoistureValue) + "&" + String(soilmoisturepercent1)
                + "#" + String(temp) + "@" + String(t) + "$" + String(h);
    Serial.println(Mymessage);
    sendMessage(Mymessage, MasterNode, Node1);
    delay(100);
    Mymessage = "";

  }
}