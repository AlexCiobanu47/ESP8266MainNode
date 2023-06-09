#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
//own headers
#include "Credentials.h"
#include "MQTTTopics.h"
#include "ScreenProperties.h"
//Screen

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int currentMenu = 0;
const int maxMenuItems = 4;
int currentMenuSwitchValue = 0; //valoarea curenta a butonului de la rotary encoder
int lastMenuSwitchValue = 1; //ultima valoare a butonului de la rotary encoder
// 0 Temp
// 1 Hum
// 2 Lights
// 3 Gas
// 4 Alarm
int currentTemp = 0;
String tempString = "temperature";
int ambientTemp = 26;
String ambientTempString = "room temperature";
int currentHum = 0;
String humString = "humidity";
int currentLightStatus = 0;
String lightString = "lights";
int currentGasStatus = 0;
String gasString = "gas";
int currentAlarmStatus = 0;
String alarmString = "alarm";
//Rotary encoder
const int menuSwitchPin = 14; //pin pentru butonul de la rotary encoder
const int encoderDTPin = 12;
const int encoderCLKPin = 13;
//
const int light1Pin = 15;
const int light2Pin = 3;
const int alarmLEDPin = 10;
//
int light1State = LOW;
int light2State = LOW;
int buzzerState = LOW;

//Telegram
unsigned long lastTimeReceivedMessage = 0;
const int messageScanTime = 1500;
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  configTime(0,0,"pool.ntp.org");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSubTest = mqttClient.subscribe(MQTT_SUB_TEST, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSubTest);
  uint16_t packetIdSubTemp = mqttClient.subscribe(MQTT_SUB_TEMP, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSubTemp);
  uint16_t packetIdSubHum = mqttClient.subscribe(MQTT_SUB_HUM, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSubHum);
  uint16_t packetIdSubLight1 = mqttClient.subscribe(MQTT_SUB_LIGHT1, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSubLight1);
  uint16_t packetIdSubLight2 = mqttClient.subscribe(MQTT_SUB_LIGHT2, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSubLight2);
  uint16_t packetIdSubAlarm = mqttClient.subscribe(MQTT_SUB_ALARM, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSubAlarm);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  // Serial.println("Publish received.");
  // Serial.print("  topic: ");
  // Serial.println(topic);
  // Serial.print("  qos: ");
  // Serial.println(properties.qos);
  // Serial.print("  dup: ");
  // Serial.println(properties.dup);
  // Serial.print("  retain: ");
  // Serial.println(properties.retain);
  // Serial.print("  len: ");
  // Serial.println(len);
  // Serial.print("  index: ");
  // Serial.println(index);
  // Serial.print("  total: ");
  // Serial.println(total);
  String messagePayload = "";
  for(size_t i = 0; i < len; i++){
    messagePayload += (char)payload[i];
  }
  if(strcmp(topic, MQTT_SUB_TEMP) == 0){
    //Serial.println(messagePayload);
    currentTemp = messagePayload.toInt();
  }
  if(strcmp(topic, MQTT_SUB_HUM) == 0){
    //Serial.println(messagePayload);
    currentHum = messagePayload.toInt();
  }
  if(strcmp(topic, MQTT_SUB_LIGHT1) == 0){
    //Serial.println(messagePayload);
    Serial.println(messagePayload);
    if(messagePayload == "off"){
      light1State = LOW;
    }
    else{
      light1State = HIGH;
    }
    digitalWrite(light1Pin, light1State);
  }
  if(strcmp(topic, MQTT_SUB_LIGHT2) == 0){
    //Serial.println(messagePayload);
    Serial.println(messagePayload);
    if(messagePayload == "off"){
      light2State = LOW;
    }
    else{
      light2State = HIGH;
    }
    digitalWrite(light2Pin, light2State);
  }
  if(strcmp(topic, MQTT_SUB_ALARM) == 0){
    //Serial.println(messagePayload);
    Serial.println(messagePayload);
    if(messagePayload == "off"){
      currentAlarmStatus = LOW;
    }
    else{
      currentAlarmStatus = HIGH;
    }
    Serial.print("alarm is turned ");
    Serial.println(currentAlarmStatus);
  }
}
void IRAM_ATTR rotary_moved(){
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if(interruptTime - lastInterruptTime > 5){
    if(digitalRead(encoderCLKPin) != digitalRead(encoderDTPin)){
      Serial.println("right");
      if(currentMenu == 0){
        ambientTemp++;
      }
      if(currentMenu == 2){
        currentLightStatus = 1;
      }
      if(currentMenu == 4){
        currentAlarmStatus = 1;
      }
    }
    else{
      Serial.println("left");
      if(currentMenu == 0){
        ambientTemp--;
      }
      if(currentMenu == 2){
        currentLightStatus = 0;
      }
      if(currentMenu == 4){
        currentAlarmStatus = 0;
      }
    }
    lastInterruptTime = interruptTime;
  }
}
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  //initializare intrerupere encoder
  attachInterrupt(digitalPinToInterrupt(encoderCLKPin), rotary_moved, CHANGE);
  //initializare pini
  pinMode(menuSwitchPin, INPUT_PULLUP);
  pinMode(encoderDTPin, INPUT);
  pinMode(light1Pin, OUTPUT);
  pinMode(light2Pin, OUTPUT);
  pinMode(alarmLEDPin, OUTPUT);
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
  secured_client.setTrustAnchors(&cert);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}
void displayNumber(int posX, int posY, int number){
  display.setCursor(posX, posY);
  display.print(number);
}
void displayText(int posX, int posY, String text){
  display.setCursor(posX, posY);
  display.print(text);
}
void handleNewMessages(int newMessagesCount)
{

  for (int i = 0; i < newMessagesCount; i++)
  {
    String chatID = bot.messages[i].chat_id;
    String textMessage = bot.messages[i].text;

    if (textMessage == "/start")
    {
      String welcomeString = "Welcome to Room Monitoring Bot\n";
      welcomeString += "Type /help to see all available commands";
      bot.sendMessage(CHAT_ID, welcomeString);
    }
    if(textMessage == "/help"){
      String helpString = "Available commands:\n";
      helpString += "/start\n";
      helpString += "/temp\n";
      helpString += "/hum\n";
      helpString += "/lights";
      bot.sendMessage(CHAT_ID, helpString);
    }
    //temp
    if(textMessage == "/temp"){
      String botTempString = "Room temperature: ";
      botTempString += currentTemp;
      bot.sendMessage(CHAT_ID, botTempString);
    }
    //umiditate
    if(textMessage == "/hum"){
      String botHumString = "Room humidity: ";
      botHumString += currentHum;
      bot.sendMessage(CHAT_ID, botHumString);
    }
    //lumini
    if(textMessage == "/lights"){
      String botLightString = "Light are: ";
      botLightString += currentLightStatus;
      bot.sendMessage(CHAT_ID, botLightString);
    }
  }
}
void loop() {
  //verificare daca s-a apasat butonul de la rotary encoder
  //si avansare prin meniu
  //cand se ajunge la ultimul tab al meniului se revine la inceput
  //ROTARY SWITCH
  currentMenuSwitchValue = digitalRead(menuSwitchPin);
  if (currentMenuSwitchValue == LOW && lastMenuSwitchValue == HIGH) {
      if(currentMenu == maxMenuItems){
        currentMenu = 0;
      }
      else{
        currentMenu++;
      }
	}
  //ROTARY ROTATE
  lastMenuSwitchValue = currentMenuSwitchValue;
  if(currentMenu == 0){
    display.clearDisplay();
    displayText(0,0, tempString);
    displayNumber(74, 0, currentTemp);
    displayText(0, 16, ambientTempString);
    displayNumber(104, 16, ambientTemp);
    display.display();
  }
  else if(currentMenu == 1){
    display.clearDisplay();
    displayText(0,0, humString);
    displayNumber(60, 0, currentHum);
    display.display();
  }
  else if(currentMenu == 2){
    display.clearDisplay();
    displayText(0,0, lightString);
    displayNumber(50,0, currentLightStatus);
    display.display();
  }
  else if(currentMenu == 3){
    display.clearDisplay();
    displayText(0,0,gasString);
    displayNumber(40, 0, currentGasStatus);
    display.display();
  }
  else if(currentMenu == 4){
    display.clearDisplay();
    displayText(0,0, alarmString);
    displayNumber(0, 16, currentAlarmStatus);
    display.display();
  }
  if(currentAlarmStatus == LOW){
    digitalWrite(alarmLEDPin, LOW);
  }
  else{
    digitalWrite(alarmLEDPin, HIGH);
  }
  if(millis() - lastTimeReceivedMessage > messageScanTime){
    int newMessageCount = bot.getUpdates(bot.last_message_received + 1);
    while(newMessageCount != 0){
      handleNewMessages(newMessageCount);
      newMessageCount = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeReceivedMessage = millis();
  }
}