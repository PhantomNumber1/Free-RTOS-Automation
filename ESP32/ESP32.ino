#include <Arduino.h>
#include <WiFi.h>
#include <sMQTTBroker.h>
#include <PubSubClient.h>

// Wi-Fi credentials
const char* ssid = "Srs707";
const char* password = "1234567s";

// MQTT broker settings
const unsigned short mqttPort = 1883;

// WiFi client
WiFiClient espClient;

// MQTT broker instance
sMQTTBroker broker;

// PubSubClient instance
PubSubClient client(espClient);

// Task handles
TaskHandle_t mqttBrokerTaskHandle = NULL;
TaskHandle_t publisherTaskHandle = NULL;
TaskHandle_t subscriberTaskHandle = NULL;

// Function prototypes
void mqttBrokerTask(void * parameter);
void publisherTask(void * parameter);
void subscriberTask(void * parameter);

void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  
  // Initialize MQTT broker
  broker.init(mqttPort);

  // Create tasks
  xTaskCreatePinnedToCore(mqttBrokerTask, "mqttBrokerTask", 4096, NULL, 1, &mqttBrokerTaskHandle, 0);
  xTaskCreatePinnedToCore(publisherTask, "publisherTask", 4096, NULL, 1, &publisherTaskHandle, 1);
  xTaskCreatePinnedToCore(subscriberTask, "subscriberTask", 4096, NULL, 1, &subscriberTaskHandle, 1);
}

void loop() {
  // Empty loop as tasks are controlling everything
}

// Task function for MQTT broker functionality
void mqttBrokerTask(void * parameter) {
  while(1) {
    broker.update();
    vTaskDelay(pdMS_TO_TICKS(100)); // Adjust delay as needed
  }
}

// Task function for publishing messages
void publisherTask(void * parameter) {
  while(1) {
    // Your publishing logic here
    const char* topic = "test/topic";
    const char* message = "Hello from ESP32!";
    broker.publish(topic, message);
    vTaskDelay(pdMS_TO_TICKS(5000)); // Publish message every 5 seconds
  }
}

// Task function for subscribing to MQTT topic
void subscriberTask(void * parameters){
  client.setServer(WiFi.localIP(), 1883);
  client.setCallback(callback);
  while(1){
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    vTaskDelay(pdMS_TO_TICKS(100)); // Adjust delay as needed
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
