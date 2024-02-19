#include <Arduino.h>
#include <WiFi.h>
#include <sMQTTBroker.h>
#include <PubSubClient.h>

const char* ssid = "Srs707";
const char* password = "1234567s";
const unsigned short mqttPort = 1883;

WiFiClient espClient;

sMQTTBroker broker;

PubSubClient client(espClient);

TaskHandle_t mqttBrokerTaskHandle = NULL;
TaskHandle_t publisherTaskHandle = NULL;
TaskHandle_t subscriberTaskHandle = NULL;

void mqttBrokerTask(void * parameter);
void publisherTask(void * parameter);
void subscriberTask(void * parameter);

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  
  broker.init(mqttPort);

  xTaskCreatePinnedToCore(mqttBrokerTask, "mqttBrokerTask", 4096, NULL, 1, &mqttBrokerTaskHandle, 0);
  xTaskCreatePinnedToCore(publisherTask, "publisherTask", 4096, NULL, 1, &publisherTaskHandle, 1);
  xTaskCreatePinnedToCore(subscriberTask, "subscriberTask", 4096, NULL, 1, &subscriberTaskHandle, 1);
}

void loop() {
  // Empty loop as tasks are controlling everything
}

void mqttBrokerTask(void * parameter) {
  while(1) {
    broker.update();
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void publisherTask(void * parameter) {
  while(1) {
    const char* topic = "test/topic";
    char message[64];
    while(!Serial.available()){
      ;
    }
    Serial.readBytes(message, sizeof(message));
    removeNewline(message);
    broker.publish(topic, message);
    memset(message, 0, sizeof(message));
    vTaskDelay(pdMS_TO_TICKS(500)); 
  }
}


void subscriberTask(void * parameters){
  client.setServer(WiFi.localIP(), 1883);
  client.setCallback(callback);
  while(1){
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void removeNewline(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0'; // Replace newline character with null terminator
            break; // Stop iterating once newline character is found
        }
    }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("test/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}
