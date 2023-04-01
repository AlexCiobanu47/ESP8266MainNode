#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Screen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int currentMenu = 0;
const int maxMenuItems = 3;
int currentMenuSwitchValue = 0; //valoarea curenta a butonului de la rotary encoder
int lastMenuSwitchValue = 1; //ultima valoare a butonului de la rotary encoder
// 0 Temp
// 1 Hum
// 2 Lights
// 3 Gas
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
//Rotary encoder
const int menuSwitchPin = 14; //pin pentru butonul de la rotary encoder
const int encoderDTPin = 12;
const int encoderCLKPin = 13;
//Wifi
#define WIFI_SSID "TP-Link_165C"
#define WIFI_PASSWORD "99368319"

#define MQTT_HOST IPAddress(192, 168, 0, 105)
#define MQTT_PORT 1883
#define MQTT_SUB_TEST "test"
#define MQTT_SUB_TEMP "temp"
#define MQTT_SUB_HUM "hum"

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
    }
    else{
      Serial.println("left");
      if(currentMenu == 0){
        ambientTemp--;
      }
      if(currentMenu == 2){
        currentLightStatus = 0;
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
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

}
void displayInformation(int valueData, String valueName,int initialXPos, int initialYPos, int displayXIndex, int displayYIndex){
  display.setCursor(initialXPos,initialYPos);
  display.print(valueName);
  display.setCursor(displayXIndex, displayYIndex);
  display.print(valueData);
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
 Serial.println(currentMenu);
  lastMenuSwitchValue = currentMenuSwitchValue;
  if(currentMenu == 0){
    display.clearDisplay();
    displayInformation(currentTemp, tempString, 0, 0, 74, 0);
    displayInformation(ambientTemp, ambientTempString, 0, 16, 104, 16);
    display.display();
  }
  else if(currentMenu == 1){
    display.clearDisplay();
    displayInformation(currentHum, humString,0, 0,60, 0);
    display.display();
  }
  else if(currentMenu == 2){
    display.clearDisplay();
    displayInformation(currentLightStatus, lightString,0, 0, 50, 0);
    display.display();
  }
  else if(currentMenu == 3){
    display.clearDisplay();
    displayInformation(currentGasStatus, gasString, 0, 0, 40, 0);
    display.display();
  }
}