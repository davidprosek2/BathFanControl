 #include <ESP8266WiFi.h>
 #include <PubSubClient.h>


 
const char* ssid = "DPJP"; // fill in here your router or wifi SSID
const char* password = "SR71-Blackbird"; // fill in here your router or wifi password
 #define RELAY 0 // relay connected to  GPIO0
 #define BUTTON 2
WiFiServer server(80);

volatile unsigned long buttonPressTime = 0;
volatile unsigned long buttonReleaseTime = 0;
volatile unsigned long lastChange = 0;

volatile bool buttonPressed = false;
unsigned long relayOffTime = 0;
bool relayOn = false;
const char* mqttBroker = "homeassistant.local";  // Replace with your broker's IP address
const int mqttPort = 1883;
const char* mqttUser = "mqtt_user";  // Replace with your MQTT username
const char* mqttPassword = "acheron";  // Replace with your MQTT password
const char* mqttTopicBathFan = "bathFan2";
const char* mqttTopicBathFanState = "bathFanState2";
const char* mqttTopicBathFanTime = "bathFanTime2";

WiFiClient wifiClient;
PubSubClient client(wifiClient);



void ICACHE_RAM_ATTR Change()
{
  Serial.println("Change on " + String(millis()));
  if((millis()-lastChange) < 150)
  {
    lastChange = millis();
    return;
  }
  Serial.println("Filtered change on " + String(millis()));

  if (digitalRead(BUTTON) == LOW) 
  {
    buttonPressTime = millis();
    buttonPressed = true;
  } 
  else 
  {
    buttonReleaseTime = millis();
    unsigned long pressDuration = buttonReleaseTime - buttonPressTime;
    buttonPressed = false;
    if(pressDuration < 100)
    {}
    else if (pressDuration < 1000) 
    {
      if(relayOn)
        toggleRelay();
      else
       setRelay(450000);  // 7.5 minutes
    } 
    else if (pressDuration < 2000) 
    {
      setRelay(450000*2);  // 15 minutes
      //setRelay(30000);  // 0.5 minutes
    } 
    else if (pressDuration >= 2000) 
    {
      setRelay(450000*4); // 30 minutes
      //setRelay(60000); // 30 minutes
    }
    else if (pressDuration >= 5000) 
    {
      lastChange = millis();
      return;
      //setRelay(250000); // 30 minutes
      //setRelay(60000); // 30 minutes
    }
  }
  lastChange = millis();
}


void toggleRelay() {
  relayOn = !relayOn;
  digitalWrite(RELAY, relayOn ? LOW : HIGH);
  
  
  Serial.println("Relay toggled");
}

void setRelay(unsigned long duration) {
  digitalWrite(RELAY, LOW);
  relayOn = true;
  relayOffTime = millis() + duration;  
  
  Serial.println("Relay set for " + String((double)duration / 60000.0) + " minutes");
}





void connectWiFi() {
  delay(10);
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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" + String(WiFi.macAddress());
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) 
    {
     Serial.println("connected");
       client.subscribe(mqttTopicBathFan);
       client.setCallback(messageReceived);
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
  client.setServer(mqttBroker, mqttPort);
  client.setCallback(messageReceived);
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  if (message == "ON") 
  {
    digitalWrite(RELAY, LOW);
    relayOffTime = millis() + 3600000;  
    relayOn = true;
    client.publish(mqttTopicBathFanState, "ON");
    Serial.println("Relay turned on via MQTT");
  } 
  else if (message == "OFF") 
  {
    digitalWrite(RELAY, HIGH);
    relayOn = false;
    client.publish(mqttTopicBathFanState, "OFF");
    Serial.println("Relay turned off via MQTT");
  }
}

String padLeftZeros(String input, int length) { while (input.length() < length) { input = '0' + input; } return input; }

 
void setup() 
{
  Serial.begin(115200); // must be same baudrate with the Serial Monitor
 
  pinMode(RELAY,OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  digitalWrite(RELAY, HIGH);
 
  // Connect to WiFi network
  Serial.println();
  Serial.println();

  connectWiFi();
  connectMQTT();
  reconnect();
  // Start the server
  
  
  
  attachInterrupt(digitalPinToInterrupt(BUTTON), Change, CHANGE);
  
  digitalWrite(RELAY, HIGH);
 
}

bool lastState = false;
 
void loop() 
{

    
   


    if (relayOn && millis() >= relayOffTime) 
    {
      digitalWrite(RELAY, HIGH);
      relayOn = false;
      relayOffTime = 0;
      Serial.println("Relay off");
    }

    if (!client.connected()) 
    {
      reconnect();
    }
  
  client.loop();
  if(relayOn)
  {
    int sec = (relayOffTime-millis())/1000;
    String min = String(sec/60);
    String sec2 = padLeftZeros(String(sec%60),2);
    String msg = min + ":" + sec2;
    client.publish(mqttTopicBathFanTime,msg.c_str());
  }
  else
  {
    client.publish(mqttTopicBathFanTime,"0");
  }

  if(lastState != relayOn)
  {
    client.publish(mqttTopicBathFanState, relayOn? "ON":"OFF");
  }
  lastState = relayOn;
  delay(1000);
 
}
