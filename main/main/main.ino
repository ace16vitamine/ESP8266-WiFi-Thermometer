// Temperaturmessung und Übertragung durch MQTT 
// Stefan Eggert
// Das Toppic wird dynamisch mit der MAC erstellt. Hierdurch sind OTA Updates möglich

const char* this_version = "1.1.10";

#include <OneWire.h>                        //OneWire Bibliothek einbinden
#include <DallasTemperature.h>              //DallasTemperatureBibliothek einbinden
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#define ONE_WIRE_BUS 2                      //Data ist an Pin 2 des Arduinos angeschlossen
#include <Credentials.h>                    // <---- REMOVE THIS LINE
#include <otadrive_esp.h>
#include <Arduino.h>

//Boot Counter
#define RTC_MARKER 0x1234
unsigned int marker = 0;
unsigned int reboots = 0;

//MQTT Variablen
const char* clientId = "";     //Client ID
const char* mqttstringshort = "/esp/temp/"; // Toppic: 

/*
// Uncomment this Block for your own settings
const char* mqtt_server = "192.168.1.1";
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_USER";
const char* mqtt_password = "YOUR_PASSWORD";
const char* ssid = "YOUR_WLAN-SSID";
const char* password = "YOUR_WLAN-PASSWORD";
*/



// Sensor

OneWire oneWire(ONE_WIRE_BUS);              //Start des OneWire Bus
DallasTemperature sensors(&oneWire);        //Dallas Temperature referenzieren

WiFiClient espClient;
PubSubClient client(espClient); 


void setup(void) { 
 Serial.begin(9600);                        // Start der seriellen Konsole 

// Mit Wifi verbinden
  Serial.print("Verbinden mit: "); 
  Serial.println(ssid);
  WiFi.begin(ssid, wifi_password);
 
while (WiFi.status() != WL_CONNECTED) {
Serial.print("WL not connected, Connect now");
WiFi.begin(ssid, wifi_password);
delay(10000); // 10k damit die Verbindung vollständig aufgebaut ist bevor ein Reconnect kommt 

}

//Setup OTA Update
OTADRIVE.setInfo(ota_api, "v@1.1.10");

// Starte Boot Counter
boot_count();

delay(1000);

  Serial.println("");
  Serial.println("WiFi verbunden");

  // Print the IP address
  Serial.print(WiFi.localIP()) ;
  Serial.println(""); 
  sensors.begin();                           // Sensor Start

} 

void loop(void) { 



 sensors.requestTemperatures();             // Temperaturen Anfragen 
 Serial.print("Temperatur: "); 
 Serial.print(sensors.getTempCByIndex(0));  // "byIndex(0)" spricht den ersten Sensor an  
 delay(1000);                               // eine Sekunde warten bis zur nächsten Messung

// MQTT
  char msgBuffer[20]; 
  char msgBuffer_IP[20];


// Get Mac
  String wifiMacString = WiFi.macAddress();
  wifiMacString.replace(":", "");
  Serial.println(wifiMacString);

// toppic = mqttstringshort + Mac
  String toppic = mqttstringshort + wifiMacString + "/result";
  String toppic_ip = mqttstringshort + wifiMacString + "/ip";
  String toppic_version = mqttstringshort + wifiMacString + "/version";
  

//Umwandeln in Char
  connectToMQTT();
  const char* mqtt_toppic=toppic.c_str();
  const char* mqtt_toppic_ip=toppic_ip.c_str();
  const char* mqtt_toppic_version=toppic_version.c_str();
  
   
//Übertragung
  client.publish(mqtt_toppic, dtostrf(sensors.getTempCByIndex(0), 6, 2, msgBuffer));
  client.publish(mqtt_toppic_ip, (WiFi.localIP().toString().c_str()));
  client.publish(mqtt_toppic_version, this_version);

  delay(1000); // Verzögerung, damit MQTT Datenübertragung abgeschlossen wird
  Serial.println("Going to deep sleep...");
  ESP.deepSleep(1200 * 1000000 ); /* Sleep für 1200 -> Messung alle 20 Minuten in Sekunden */

}

void connectToMQTT() {

 client.setServer(mqtt_server, mqtt_port);
  
  if (client.connect(clientId , mqtt_user, mqtt_password)) {
    Serial.println("connected");
    Serial.println(this_version);
  }
}

void boot_count()
{

// READ BOOT
ESP.rtcUserMemoryRead(0, &marker, sizeof(marker));

  if (marker != RTC_MARKER) {
    // first reboot
    marker = RTC_MARKER;
    reboots = 0;
    ESP.rtcUserMemoryWrite(0, &marker,sizeof(marker));
  } else {
    // read count of reboots
    ESP.rtcUserMemoryRead(sizeof(marker), &reboots, sizeof(reboots));
  }
  reboots++;
  ESP.rtcUserMemoryWrite(sizeof(marker), &reboots, sizeof(reboots));

  Serial.printf("Number of reboots : %d\r\n", reboots);

  if (reboots > 1) { // Update Anfrage nach X Reboot
    unsigned int marker = 0;
    unsigned int reboots = 0;
     
    ESP.rtcUserMemoryWrite(sizeof(marker), &reboots, sizeof(reboots)); // Set Counter to NULL
    
     ota(); // Starte Update
   
  }


}

void ota()
{
  Serial.printf("Prüfe auf Updates...");
  
    OTADRIVE.updateFirmware();
    
}




