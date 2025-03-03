/*
Carte ESP32S2 avec un capteur AHT20 et une LED RGB
Commande de cette carte via api REST de la carte

https://microcontrollerslab.com/esp32-rest-api-web-server-get-post-postman/

Test sur carte ESP32-S2-Saola-1
https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32s2/hw-reference/esp32s2/user-guide-saola-1-v1.2.html

Capteur aht20
https://wiki.seeedstudio.com/Grove-AHT20-I2C-Industrial-Grade-Temperature&Humidity-Sensor/

Pilotage de la led
http://192.168.1.96/led  POST
{
    "red":0,
    "green":255,
    "blue":0
}

curl --location 'http://192.168.1.96/led' \
--header 'Content-Type: application/json' \
--data '{
    "red":255,
    "green":0,
    "blue":0
}'
Lecture température
http://192.168.1.96/temperature GET
{
    "type": "temperature",
    "valeur": 18.60294,
    "unite": "°C"
}

Lecture humidité
http://192.168.1.96/humidite GET
{
    "type": "humidite",
    "valeur": 38.81283,
    "unite": "%"
}
Lecture des 2 valeurs GET
http://192.168.1.96/data
[{"type":"temperature","value":18.50529,"unit":"°C"},{"type":"humidite","value":38.90657,"unit":"%"}]

*/

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>

const char *SSID = "ORBI50";
const char *PWD = "modernwater884";

//Sur la carte ESP32S2 SAOLA GPIO is the NeoPixel.
#define PIN        18 
//une seule LED NeoPixel
Adafruit_NeoPixel pixels(1, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 25 // Temps (en millisecondes) de pause entre les changements de couleur

WebServer server(80);
 
Adafruit_AHTX0 aht;

StaticJsonDocument<250> jsonDocument;
char buffer[250];

float temperature;
float humidite;
 
void setup_routing() {     
  server.on("/temperature", getTemperature);     
  server.on("/humidite", getHumidite);     
  server.on("/data", getData);     
  server.on("/led", HTTP_POST, handlePost);    
          
  server.begin();    
}
 
void create_json(char *tag, float value, char *unit) {  
  jsonDocument.clear();  
  jsonDocument["type"] = tag;
  jsonDocument["valeur"] = value;
  jsonDocument["unite"] = unit;
  serializeJson(jsonDocument, buffer);
}
 
void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["valeur"] = value;
  obj["unite"] = unit; 
}

void read_sensor_data(void * parameter) {
   for (;;) {
     sensors_event_t hum, temp;
     aht.getEvent(&hum, &temp);
     temperature = temp.temperature;
     humidite = hum.relative_humidity;
    
     Serial.println("Lecture des valeurs du capteur");
 
     vTaskDelay(60000 / portTICK_PERIOD_MS);
   }
}
 
void getTemperature() {
  //Serial.println("Lecture de la temperature");
  create_json("temperature", temperature, "°C");
  server.send(200, "application/json", buffer);
}
 
void getHumidite() {
  Serial.println("Lecture humidite");
  create_json("humidite", humidite, "%");
  server.send(200, "application/json", buffer);
}
 

void getData() {
  //Serial.println("Lecture des valeurs du capteur AHT20 ");
  jsonDocument.clear();
  add_json_object("temperature", temperature, "°C");
  add_json_object("humidite", humidite, "%");
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void handlePost() {
  if (server.hasArg("plain") == false) {
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  int red_value = jsonDocument["red"];
  int green_value = jsonDocument["green"];
  int blue_value = jsonDocument["blue"];
  pixels.setPixelColor(0,pixels.Color(red_value, green_value, blue_value));
  // Send the updated pixel colors to the hardware.
   pixels.show();   

  server.send(200, "application/json", "{}");
}

void setup_task() {    
  xTaskCreate(     
  read_sensor_data,      
  "Read sensor data",      
  1000,      
  NULL,      
  1,     
  NULL     
  );     
}

void setup() {     
  Serial.begin(115200); 
  //Ce pixel est trop brillant, baissez-le à 10 pour le regarder.
  pixels.setBrightness(10);
  pixels.begin(); // INITIALISE NeoPixel (REQUIRED)
         
  if (! aht.begin()) {
    Serial.println("Le capteur AHT ne répond pas ? Vérifier le cablage");
    while (1) delay(10);
  }
  Serial.println("AHT10 ou AHT20 trouvé");

  Serial.print("Connecté à la borne Wi-Fi");
  WiFi.begin(SSID, PWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  Serial.print("Carte connectée! Adresse IP: ");
  Serial.println(WiFi.localIP());
  setup_task();    
  setup_routing();      
}    
       
void loop() {    
  server.handleClient();     
}