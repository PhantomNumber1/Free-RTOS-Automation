#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <Arduino.h>
#include <WiFi.h>
#include <sMQTTBroker.h>
#include <PubSubClient.h>
#include <WebServer.h>

const char* ssid = "Srs707";
const char* password = "1234567s";
const unsigned short mqttPort = 1883;
WebServer server(80);
int led1state = 0;
int led2state = 0;
const uint64_t switch1 = 16753245;

unsigned long touch3StartTime = 0;
const unsigned long touch3Delay = 10000;

#define touch1 14
#define touch2 27
#define touch3 26

#define data 25
#define enable 15
#define s0 4
#define s1 2
#define light 32

WiFiClient espClient;

sMQTTBroker broker;

PubSubClient client(espClient);

TaskHandle_t mqttBrokerTaskHandle = NULL;
TaskHandle_t publisherTaskHandle = NULL;
TaskHandle_t subscriberTaskHandle = NULL;
TaskHandle_t remoteTaskHandle = NULL;

SemaphoreHandle_t semaphore;


void mqttBrokerTask(void * parameter);
void publisherTask(void * parameter);
void subscriberTask(void * parameter);
void remoteTask(void * paremeter);

IRrecv irrecv(23);
decode_results results;

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
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.begin();
  pinMode(touch1, INPUT);
  pinMode(touch2, INPUT);
  pinMode(touch3, INPUT);
  pinMode(data, OUTPUT);
  pinMode(enable, OUTPUT);
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(light, OUTPUT);

  digitalWrite(enable,HIGH);
  digitalWrite(s1,HIGH);
  digitalWrite(s0,HIGH);
  digitalWrite(light,LOW);

  broker.init(mqttPort);
  irrecv.enableIRIn();

  digitalWrite(light, HIGH);

  semaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphore);
  xTaskCreatePinnedToCore(mqttBrokerTask, "mqttBrokerTask", 4096, NULL, 1, &mqttBrokerTaskHandle, 0);
  xTaskCreatePinnedToCore(publisherTask, "publisherTask", 4096, NULL, 1, &publisherTaskHandle, 1);
  xTaskCreatePinnedToCore(subscriberTask, "subscriberTask", 4096, NULL, 1, &subscriberTaskHandle, 1);
}

void loop() {
  server.handleClient();

  if(irrecv.decode(&results)){
    Serial.println(results.value);
    if(results.value == 16753245){
    xSemaphoreTake(semaphore, portMAX_DELAY);
      digitalWrite(s1,LOW);
      digitalWrite(s0, LOW);
      digitalWrite(enable,HIGH);
      if(led1state){
        led1state = 0;
        digitalWrite(data, LOW);
        delay(500);
      }
      else{
        led1state = 1;
        digitalWrite(data, HIGH);
        delay(500);
      }
      digitalWrite(enable,LOW);
      digitalWrite(s1,HIGH);
      digitalWrite(s0, HIGH);
      xSemaphoreGive(semaphore);
    }
    else if(results.value == 16736925){
    xSemaphoreTake(semaphore, portMAX_DELAY);
      digitalWrite(s1,LOW);
      digitalWrite(s0, HIGH);
      digitalWrite(enable, HIGH);
      if(led2state){
        led2state = 0;
        digitalWrite(data, LOW);
        delay(500);
      }
      else{
        led2state = 1;
        digitalWrite(data, HIGH);
        delay(500);
      }
      digitalWrite(enable, LOW);
      digitalWrite(s1,HIGH);
      digitalWrite(s0, HIGH);
      xSemaphoreGive(semaphore);
  }
  else if(results.value == 16769565){
    digitalWrite(light, HIGH);
    touch3StartTime = millis();
  }
    delay(100);
    irrecv.resume();
  }


  if(digitalRead(touch1)){
    xSemaphoreTake(semaphore, portMAX_DELAY);
    digitalWrite(s1,LOW);
    digitalWrite(s0, LOW);
    digitalWrite(enable, HIGH);
    if(led1state){
      led1state = 0;
      digitalWrite(data, LOW);
      delay(500);
    }
    else{
      led1state = 1;
      digitalWrite(data, HIGH);
      delay(500);
    }
    digitalWrite(enable, LOW);
    digitalWrite(s1,HIGH);
    digitalWrite(s0, HIGH);
    xSemaphoreGive(semaphore);
  }

  if(digitalRead(touch2)){
    xSemaphoreTake(semaphore, portMAX_DELAY);
    digitalWrite(s1,LOW);
    digitalWrite(s0, HIGH);
    digitalWrite(enable, HIGH);
    if(led2state){
      led2state = 0;
      digitalWrite(data, LOW);
      delay(500);
    }
    else{
      led2state = 1;
      digitalWrite(data, HIGH);
      delay(500);
    }
    digitalWrite(enable, LOW);
    digitalWrite(s1,HIGH);
    digitalWrite(s0, HIGH);
    xSemaphoreGive(semaphore);
  }
  if (digitalRead(touch3)) {
    digitalWrite(light, HIGH);
    touch3StartTime = millis();
  }

  if (millis() - touch3StartTime >= touch3Delay) {
    digitalWrite(light, LOW);
  }
}

void handleRoot() {
  if (server.method() == HTTP_POST) { 
    String inputValue = server.arg("input");
    processInput(inputValue);
  }

String htmlContent = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Web Server</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: black;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }
        .container {
            background-color: #fff;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            max-width: 400px;
            width: 100%;
        }
        h1 {
            text-align: center;
            color: #333;
        }
        form {
            margin-top: 20px;
            text-align: center;
        }
        label {
            display: block;
            margin-bottom: 10px;
            font-weight: bold;
        }
        input[type="text"] {
            width: 100%;
            padding: 10px;
            margin-bottom: 10px;
            border: 1px solid #ccc;
            border-radius: 4px;
            box-sizing: border-box;
        }
        input[type="submit"] {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }
        input[type="submit"]:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Welcome to your ESP32!</h1>
        <form method="post">
            <label for="input">Enter your command:</label><br>
            <input type="text" id="input" name="input"><br>
            <input type="submit" value="Submit">
        </form>
    </div>
</body>
</html>
)";


  server.send(200, "text/html", htmlContent);
}

void handleStatus() {
  String statusResponse = "{\"light1\": \"on\", \"light2\": \"off\"}";
  server.send(200, "application/json", statusResponse);
}

void processInput(String inputValue) {
  Serial.print("Input received: ");
  Serial.println(inputValue);

  if(inputValue == "1"){
    xSemaphoreTake(semaphore, portMAX_DELAY);
    digitalWrite(s1,LOW);
    digitalWrite(s0, LOW);
    digitalWrite(enable, HIGH);
    if(led1state){
      led1state = 0;
      digitalWrite(data, LOW);
      delay(500);
    }
    else{
      led1state = 1;
      digitalWrite(data, HIGH);
      delay(500);
    }
    digitalWrite(enable, LOW);
    digitalWrite(s1,HIGH);
    digitalWrite(s0, HIGH);
    xSemaphoreGive(semaphore);
  }

  if(inputValue == "2"){
    xSemaphoreTake(semaphore, portMAX_DELAY);
    digitalWrite(s1,LOW);
    digitalWrite(s0, HIGH);
    digitalWrite(enable, HIGH);
    if(led2state){
      led2state = 0;
      digitalWrite(data, LOW);
      delay(500);
    }
    else{
      led2state = 1;
      digitalWrite(data, HIGH);
      delay(500);
    }
    digitalWrite(enable, LOW);
    digitalWrite(s1,HIGH);
    digitalWrite(s0, HIGH);
    xSemaphoreGive(semaphore);
  }
  if (inputValue == "3") {
    digitalWrite(light, HIGH);
    touch3StartTime = millis();
  }
}


void mqttBrokerTask(void * parameter) {
  while(1) {
    broker.update();
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void remoteTask(void* parameter){
  ;
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
            str[i] = '\0'; 
            break;
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

  String messageStr;
  for (int i = 0; i < length; i++) {
    messageStr += (char)message[i];
  }

  if (messageStr.equals("1")) {
    xSemaphoreTake(semaphore, portMAX_DELAY);
    digitalWrite(s1, LOW);
    digitalWrite(s0, LOW);
    digitalWrite(enable, HIGH);
    if (led1state) {
      led1state = 0;
      digitalWrite(data, LOW);
      delay(500);
    } else {
      led1state = 1;
      digitalWrite(data, HIGH);
      delay(500);
    }
    digitalWrite(enable, LOW);
    digitalWrite(s1, HIGH);
    digitalWrite(s0, HIGH);
    xSemaphoreGive(semaphore);
  }
  if (messageStr.equals("2")) {
    xSemaphoreTake(semaphore, portMAX_DELAY);
    digitalWrite(s1, LOW);
    digitalWrite(s0, HIGH);
    digitalWrite(enable, HIGH);
    if (led2state) {
      led2state = 0;
      digitalWrite(data, LOW);
      delay(500);
    } else {
      led2state = 1;
      digitalWrite(data, HIGH);
      delay(500);
    }
    digitalWrite(enable, LOW);
    digitalWrite(s1, HIGH);
    digitalWrite(s0, HIGH);
    xSemaphoreGive(semaphore);
  }
  if (messageStr.equals("3")) {
    digitalWrite(light, HIGH);
    touch3StartTime = millis();
  }
  if (messageStr.equals("69")){
    xSemaphoreTake(semaphore, portMAX_DELAY);
    digitalWrite(s1,LOW);
    digitalWrite(s0,LOW);
    digitalWrite(enable,HIGH);
    digitalWrite(light, HIGH);
    for(int i = 0; i<=100; i++){
      touch3StartTime = millis();
      digitalWrite(data,HIGH);
      delay(50);
      digitalWrite(s0, HIGH);
      delay(50);
      digitalWrite(data,LOW);
      delay(50);
      digitalWrite(s0,LOW);
      delay(50);
    }
    digitalWrite(light, LOW);
    digitalWrite(enable,LOW);
    digitalWrite(s1,HIGH);
    digitalWrite(s0,HIGH);
    xSemaphoreGive(semaphore);
  }
}

