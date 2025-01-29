#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <WiFi.h>
#include <PubSubClient.h>

// OLED display width, in pixels
#define SCREEN_WIDTH 128
// OLED display height, in pixels
#define SCREEN_HEIGHT 32

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_HTU21DF htu = Adafruit_HTU21DF();


const char* ssid = "DPJP"; // fill in here your router or wifi SSID
const char* password = "SR71-Blackbird"; // fill in here your router or wifi password
const char* mqttBroker = "homeassistant.local";  // Replace with your broker's IP address
const int mqttPort = 1883;
const char* mqttUser = "mqtt_user";  // Replace with your MQTT username
const char* mqttPassword = "acheron";  // Replace with your MQTT password
//const char* mqttTopicBathFanState = "bathFanState2";
const char* mqttTopicBathFanTime = "bathFanTime2";
char fanTime[6];

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void connectWiFi() {
  delay(10);
  Serial.println();
  display.clearDisplay();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  display.print("Connecting to ");
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("WiFi connected");
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();

  delay(1000);
}

void reconnect() {
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" + String(WiFi.macAddress());
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) 
    {
     Serial.println("connected");
       client.subscribe(mqttTopicBathFanTime,0);
       client.setCallback(messageReceived);
       client.setKeepAlive(100);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}




void connectMQTT() {
  Serial.print("Connecting to MQTT broker...");
  display.clearDisplay();
  display.println("Connecting to MQTT broker...");
  display.display();
  client.setServer(mqttBroker, mqttPort);
 // client.setCallback(messageReceived);
  display.println("Mqtt connected");
  display.display();
}

void setup() {
  Serial.begin(115200);
  // initialize with the I2C addr 0x3C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.setRotation(0);
  // Display text
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  if(!htu.begin())
    display.println(F("HTU init failed"));  
  else
    display.println(F("Hello"));
  display.display();           // Show initial text

  connectWiFi();
  connectMQTT();
  reconnect();
  
  delay(500);

  // Update text
  
  
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  //String message("");
  
  Serial.print(topic);
  Serial.print(": ");
  char message [length+1];
  for (unsigned int i = 0; i < length; i++) {
    //message += (char)payload[i];
    message[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();
  message[length] = '\0';
  
  
  
  strcpy(fanTime, message);
  payload[0] = 0;
  payload = nullptr;
  //Serial.println(fanTime);
  
  
  //client.publish(mqttTopicBathFanTime,fanTime,false);
  
  
}

void displayData(float temp,float hum,char* time)
{
  
  String t = String(temp,1);
  String h = String(hum,1);

     display.clearDisplay();
   display.setTextSize(1);
   display.setFont(&FreeMono9pt7b);
   display.setCursor(0, 10);     // Start at top-left corner
   display.println(t+"°C");
   display.println(h+"%");
   //display.println("1:29");
   display.setCursor(70, 20);     // Start at top-left corner
   display.drawLine(65, 0, 65,32, 1);
   display.println(time );
   display.display();           // Show initial text
}

void publishData(float temp,float hum)
{
  
  String t = String(temp,1);
  String h = String(hum,1);
  client.publish("bathTemperature",t.c_str());
  client.publish("bathHumidity",h.c_str());
}

void loop() {
 // if (!client.connected()) 
    {
      reconnect();
    }
  
  client.loop();
  float humidity =  htu.readHumidity(); 
  float temperature = htu.readTemperature();
  // client.unsubscribe(mqttTopicBathFanTime);
  // client.subscribe(mqttTopicBathFanTime,0);

  displayData(temperature,humidity,fanTime);
  publishData(temperature, humidity);
  //client.disconnect();
  delay(1000);
}
