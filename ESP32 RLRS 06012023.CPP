#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <DHT.h>  
#include <SimpleTimer.h>
#include <IRsend.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>

/* 
IR Code
pw 0x1FE48B7 (32 Bits)
md 0x1FE58A7 (32 Bits)
mu 0x1FE7887 (32 Bits)
pl 0x1FE807F (32 Bits)
bw 0x1FE40BF (32 Bits)
fw 0x1FEC03F (32 Bits)
e  0x1FE20DF (32 Bits)
v+ 0x1FEA05F (32 Bits)
v- 0x1FE609F (32 Bits)
0  0x1FEE01F (32 Bits)
rt 0x1FE10EF (32 Bits)
sd 0x1FE906F (32 Bits)
1  0x1FE50AF (32 Bits)
2  0x1FED827 (32 Bits)
3  0x1FEF807 (32 Bits)
4  0x1FE30CF (32 Bits)
5  0x1FEB04F (32 Bits)
6  0x1FE708F (32 Bits)
7  0x1FE00FF (32 Bits)
8  0x1FEF00F (32 Bits)
9  0x1FE9867 (32 Bits)
*/

// IR Remote Code for Lights
#define IR1            0x1FE20DF // eq
#define IR2            0x1FEA05F // v+
#define IR3            0x1FE609F // v-
#define IR4            0x1FE10EF // rpt
#define IR5            0x1FE906F // u/sd
#define IRF0           0x1FEE01F // 0
#define IRF1           0x1FE50AF // 1
#define IRF2           0x1FED827 // 2
#define IRF3           0x1FEF807 // 3
#define IRF4           0x1FE30CF // 4

// mygt
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
// WiFi Credentils
const char* ssid = "Your-SSID";
const char* password = "Your-Password";
// BLE Credentils
const char *service_name = "PROV_1234";
const char *pop = "abcd1234";
WebServer server(80);

#define DEBUG_SW 1
#define Rainmaker_SW 1
#define LocalWebS_SW 1
#define IRRemoteS_SW 1

#define DEFAULT_Temperature 0
#define DEFAULT_Humidity 0

String ssid1 = String(ssid);
char Device1[] = "Socket";
char Device2[] = "Extension";
char Device3[] = "Computer";
char Device4[] = "Night";
char Device5[] = "light";
String All = "ON";
String switch1 = "OFF";
String switch2 = "OFF";
String switch3 = "OFF";
String switch4 = "OFF";
String switch5 = "ON";
String Fan = "Speed4";
String AC = "OFF";
String FL = "ON";
String Mannual = "OFF";
String FanMode = "Mannual";
String FanBreeze = "OFF";

// for rainmaker
bool switch1status = false;
bool switch2status = false;
bool switch3status = false;
bool switch4status = false;
bool switch5status = true;
bool fan_power     = 0;

int t1 = 0;
int t2 = 0;

/* ESP32 Pin
EN 23
36 22
39 1 Tx0
34 3 Rx0
35 21
32 19
33 18
25 5 
26 17 Tx2
27 16 Rx2
14 4
12 2
13 15
 G  G
5V 3V
 USB
 */

static uint8_t D0             =  2;
static uint8_t D1             =  4;
//                               1;    Tx0 
static uint8_t DHTPIN         =  3; //Rx0
static uint8_t D2             =  5;
static uint8_t D3             = 18;
static uint8_t D5             = 19;
static uint8_t D6             = 21;
//                              17;    Tx2 
//                              16;    Rx2
static uint8_t D7             = 22;
static uint8_t D8             = 23;
//------------------------------------------------------
static uint8_t D4             = 21;
static uint8_t gpio_reset     =  0;
//------------------------------------------------------
//                              EN 
//static uint8_t sw           = 36;
//static uint8_t sw           = 39;
//static uint8_t sw           = 34;
static uint8_t sw_control_4   = 35;
static uint8_t sw_control_5   = 32;
static uint8_t sw_control_6   = 33;
//static uint8_t sw           = 25;
//static uint8_t sw           = 26;
static uint8_t IR_Sensor0     = 34; // left
static uint8_t IR_Sensor1     = 26; // right
static uint8_t IR_Transmitter = 12;
static uint8_t IR_Reciver     = 13;
//                              Gnd 
//                              5v 

int SensorInterval = 5000; // 5 Seconds
float FanSetTemp = 0.0;

int timer_id[5];

DHT dht(DHTPIN, DHT11);
IRrecv irrecv(IR_Reciver);
decode_results results;

IRsend irsend(IR_Transmitter);

SimpleTimer Timer;

static Switch my_switch1(Device1, &D0);
static Switch my_switch2(Device2, &D1);
static Switch my_switch3(Device3, &D2);
static Switch my_switch4(Device4, &D3);
static Switch my_switch5(Device5, &D8);
static Switch my_fan("Fan");

static TemperatureSensor temperature("Temperature");
static TemperatureSensor humidity("Humidity");

void sysProvEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32S2
        Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n", service_name, pop);
        printQR(service_name, pop, "softap");
#else
        Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
        printQR(service_name, pop, "ble");
#endif
        break;
        default:;
    }
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
    const char *device_name = device->getDeviceName();
    const char *param_name = param->getParamName();

    if(strcmp(device_name, Device1) == 0) {
      if(strcmp(param_name, "Power") == 0) {
        switch1status = val.val.b;
        (switch1status == false) ? digitalWrite(D0, HIGH) : digitalWrite(D0, LOW);
        if (switch1status == false) {
          switch1 = "OFF";
        }
        else {
          switch1 = "ON";
        }
        param->updateAndReport(val);
      }
    }
    else if (strcmp(device_name, Device2) == 0) {
      if(strcmp(param_name, "Power") == 0) {
          switch2status = val.val.b;
          (switch2status == false) ? digitalWrite(D1, HIGH) : digitalWrite(D1, LOW);
          if (switch2status == false) {
            switch2 = "OFF";
          }
          else {
            switch2 = "ON";
          }
          param->updateAndReport(val);
      }
    }
    else if (strcmp(device_name, Device3) == 0) {
      if(strcmp(param_name, "Power") == 0) {
          switch3status = val.val.b;
          (switch3status == false) ? digitalWrite(D2, HIGH) : digitalWrite(D2, LOW);
          if (switch3status == false) {
            switch3 = "OFF";
          }
          else {
            switch3 = "ON";
          }
          param->updateAndReport(val);
      }
    }
    else if (strcmp(device_name, Device4) == 0) {
      if(strcmp(param_name, "Power") == 0) {
          switch4status = val.val.b;
          (switch4status == false) ? digitalWrite(D3, HIGH) : digitalWrite(D3, LOW);
          if (switch4status == false) {
            switch4 = "OFF";
          }
          else {
            switch4 = "ON";
          }
          param->updateAndReport(val);
      }
    }
    else if (strcmp(device_name, Device5) == 0) {
      if(strcmp(param_name, "Power") == 0) {
          switch5status = val.val.b;
          (switch5status == false) ? digitalWrite(D8, HIGH) : digitalWrite(D8, LOW);
          if (switch5status == false) {
            switch5 = "OFF";
          }
          else {
            switch5 = "ON";
          }
          param->updateAndReport(val);
      }
    }

  if (strcmp(device_name, "Fan") == 0)
  {
    if (strcmp(param_name, "My_Speed") == 0)
    {
      int Slider_Value = val.val.i;
      if (Slider_Value == 1)
      {
        FanSpeed1();
      }
      if (Slider_Value == 2)
      {
        FanSpeed2();
      }
      if (Slider_Value == 3)
      {
        FanSpeed3();
      }
      if (Slider_Value == 4)
      {
        FanSpeed4();
      }
      if (Slider_Value == 0)
      {
        FanOFF();
      }
      param->updateAndReport(val);
    }
  }
}

void Send_Sensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  float maxtemp = FanSetTemp + 0.2;
  float mintemp = FanSetTemp - 0.2;


  Serial.print("Room Humidity    - "); Serial.println(h);
  Serial.print("Room Temperature - "); Serial.println(t);
  //Serial.print("Fan Temperature  - "); Serial.println(FanSetTemp);
  //Serial.print("maxtemp : "); Serial.println(maxtemp);
  //Serial.print("mintemp : "); Serial.println(mintemp);

  temperature.updateAndReportParam("Temperature", t);
  humidity.updateAndReportParam("Temperature", h);

  if (FanMode == "Auto") {
    if (t > maxtemp) {
      Serial.print("room temp high then max temp");
      if (Fan == "OFF") {
      FanSpeed1();
      delay(200);
      Serial.print("speed1");
      }
      else if (Fan == "Speed1") {
      FanSpeed2();
      delay(200);
      Serial.print("speed2");
      }
      else if (Fan == "Speed2") {
      FanSpeed3();
      delay(200);
      Serial.print("speed3");
      }
      else if (Fan == "Speed3") {
      FanSpeed4();
      delay(200);
      Serial.print("speed4");
      }
    }

    if (t < mintemp) {
      Serial.print("room temp less then min temp");
      if (Fan == "Speed4") {
      FanSpeed3();
      delay(200);
      Serial.print("speed3");
      }
      else if (Fan == "Speed3") {
      FanSpeed2();
      delay(200);
      Serial.print("speed2");
      }
      else if (Fan == "Speed2") {
      FanSpeed1();
      delay(200);
      Serial.print("speed1");
      }
      else if (Fan == "Speed1") {
      FanOFF();
      delay(200);
      Serial.print("fanOFF");
      }
    }
    delay(200);
  }

  if (FanBreeze == "ON") {
    FanSpeed1();
    // set timer
    FanOFF();
    // set timer
  }

}

void handleRoot() {
 server.send(200, "text/html", 
"<!DOCTYPE html>" \
"<head>" \
"<head>" \
"<title>home</title>" \
"</head>" \
"<body>" \
"<h1>Smart Home</h1>" \
"<h2>Connected to "+ ssid1 +"</h2>" \
"<h3>Fan mode: "+ FanMode +"    Fan: "+ Fan +"</h3>" \
"<table>" \
  "<tr>" \
    "<td><a href=\"/Switch1\"><button class=\"button\">"+ Device1 +"<br />"+ switch1 +"</button></a></td>" \
    "<td><a href=\"/Switch2\"><button class=\"button\">"+ Device2 +"<br />"+ switch2 +"</button></a></td>" \
    "<td><a href=\"/Switch3\"><button class=\"button\">"+ Device3 +"<br />"+ switch3 +"</button></a></td>" \
    "<td><a href=\"/Switch4\"><button class=\"button\">"+ Device4 +"<br />"+ switch4 +"</button></a></td>" \
    "<td><a href=\"/Switch5\"><button class=\"button\">"+ Device5 +"<br />"+ switch5 +"</button></a></td>" \
  "</tr>" \
"</table>" \
"<table>" \
  "<tr>" \
    "<td><a href=\"FanOFF\"><button class=\"button\">Fan "+ Fan +"<br />"+ "OFF" +"</button></a></td>" \
    "<td><a href=\"/FanSpeed1\"><button class=\"button\">Fan "+ Fan +"<br />"+ "Speed1" +"</button></a></td>" \
    "<td><a href=\"/FanSpeed2\"><button class=\"button\">Fan "+ Fan +"<br />"+ "Speed2" +"</button></a></td>" \
    "<td><a href=\"/FanSpeed3\"><button class=\"button\">Fan "+ Fan +"<br />"+ "Speed3" +"</button></a></td>" \
    "<td><a href=\"/FanSpeed4\"><button class=\"button\">Fan "+ Fan +"<br />"+ "Speed4" +"</button></a></td>" \
  "</tr>" \
"</table>" \
"<table>" \
  "<tr>" \
    "<td><a href=\"/FanBreeze\"><button class=\"button\">"+ "Fan Breeze" +"<br />"+ FanBreeze +"</button></a></td>" \
    "<td><a href=\"/Fandec\"><button class=\"button\">"+ "Fan Temp" +"<br />"+ "Decrease" +"</button></a></td>" \
    "<td><a href=\"/Fanmode\"><button class=\"button\">"+ "Fan Temp" +"<br />"+ FanSetTemp +"</button></a></td>" \
    "<td><a href=\"/Faninc\"><button class=\"button\">"+ "Fan Temp" +"<br />"+ "Increase" +"</button></a></td>" \
    "<td><a href=\"/Fanmode\"><button class=\"button\">"+ "Auto Mode" +"<br />"+ FanMode +"</button></a></td>" \
  "</tr>" \
"</table>" \
"<table>" \
  "<tr>" \
    "<td><a href=\"/All\"><button class=\"button\">ALL<br />" + All + "</button></a></td>" \
    "<td><a href=\"/FL\"><button class=\"button\">F&L<br />" + FL + "</button></a></td>" \
    "<td><a href=\"/AcOFF\"><button class=\"button\">AC<br />OFF</button></a>" \
    "<td><a href=\"/AcON\"><button class=\"button\">AC<br />ON</button></a>" \
    "<td><a href=\"/Mannual\"><button class=\"button\">Mannual<br />" + Mannual + "</button></a></td>" \
  "</tr>" \
"</table>" \
"</body>" \
"</html>" \
);
}

void switch1function(){
  if (switch1 == "ON") {
    switch1 = "OFF";
    switch1status = false;
    my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch1status);
    digitalWrite(D0, HIGH);
    handleRoot();
    delay(200);
  }
  else {
    switch1 = "ON";
    switch1status = true;
    my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch1status);
    digitalWrite(D0, LOW);
    handleRoot();
    delay(200);
  }
}

void switch2function(){
  if (switch2 == "ON") {
    switch2 = "OFF";
    switch2status = false;
    my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch2status);
    digitalWrite(D1, HIGH);
    handleRoot();
    delay(200);
  }
  else {
    switch2 = "ON";
    switch2status = true;
    my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch2status);
    digitalWrite(D1, LOW);
    handleRoot();
    delay(200);
  }
}

void switch3function(){
  if (switch3 == "ON") {
    switch3 = "OFF";
    switch3status = false;
    my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch3status);
    digitalWrite(D2, HIGH);
    handleRoot();
    delay(200);
  }
  else {
    switch3 = "ON";
    switch3status = true;
    my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch3status);
    digitalWrite(D2, LOW);
    handleRoot();
    delay(200);
  }
}

void switch4function(){
  if (switch4 == "ON") {
    switch4 = "OFF";
    switch4status = false;
    my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch4status);
    digitalWrite(D3, HIGH);
    handleRoot();
    delay(200);
  }
  else {
    switch4 = "ON";
    switch4status = true;
    my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch4status);
    digitalWrite(D3, LOW);
    handleRoot();
    delay(200);
  }
}

void switch5function(){
  if (switch5 == "ON") {
    switch5 = "OFF";
    switch5status = false;
    my_switch5.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch5status);
    digitalWrite(D8, HIGH);
    handleRoot();
    delay(200);
  }
  else {
    switch5 = "ON";
    switch5status = true;
    my_switch5.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch5status);
    digitalWrite(D8, LOW);
    handleRoot();
    delay(200);
  }
}

void FanOFF(){
  Fan = "OFF";
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, HIGH);
  handleRoot();
}

void FanSpeed1(){
  Fan = "Speed1";
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, HIGH);
  delay(500);
  digitalWrite(D5, LOW);
  handleRoot();
}

void FanSpeed2(){
  Fan = "Speed2";
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, HIGH);
  delay(500);
  digitalWrite(D6, LOW);
  handleRoot();
}

void FanSpeed3(){
  Fan = "Speed3";
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, HIGH);
  delay(500);
  digitalWrite(D5, LOW);
  digitalWrite(D6, LOW);
  handleRoot();
}

void FanSpeed4(){
  Fan = "Speed4";
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, HIGH);
  delay(500);
  digitalWrite(D7, LOW);
  handleRoot();
}

void switchfanfunction(){
  if (Fan != "OFF") {
    FanOFF();
  }
  else {
    FanSpeed4();
  }
}

void FanBreezefunction(){
  if (FanBreeze == "ON") {
    FanBreeze = "OFF";
    handleRoot();
    delay(200);
  }
  else {
    FanMode = "Mannual";
    FanSetTemp = 0.0;
    FanBreeze = "ON";
    handleRoot();
    delay(200);
  }
}

void Fanincfunction(){
  if (FanMode == "Auto") {
    FanSetTemp = FanSetTemp + 1.0;
  }
  handleRoot();
  delay(200);
}

void Fandecfunction(){
  if (FanMode == "Auto") {
    FanSetTemp = FanSetTemp - 1.0;
  }
  handleRoot();
  delay(200);
}

void FanModefunction(){
  if (FanMode == "Mannual") {
    FanBreeze = "OFF";
    FanMode = "Auto";
    FanSetTemp = 29.0;
    handleRoot();
    delay(200);
  }
  else {
    FanMode = "Mannual";
    FanSetTemp = 0.0;
    handleRoot();
    delay(200);
  }
}

void Allfunction(){
  if (All == "ON" || switch1 == "ON" || switch2 == "ON" || switch3 == "ON" || switch4 == "ON" || switch5 == "ON" || Fan != "OFF" || AC != "OFF") { // All off
    All = "OFF";
    switch1 = "OFF";
    switch2 = "OFF";
    switch3 = "OFF";
    switch4 = "OFF";
    switch5 = "OFF";
    Fan = "OFF";
    AC = "OFF";
    switch1status = false;
    switch2status = false;
    switch3status = false;
    switch4status = false;
    switch5status = false;
    my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch1status);
    my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch2status);
    my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch3status);
    my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch4status);
    my_switch5.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch5status);
    digitalWrite(D0, HIGH);
    digitalWrite(D1, HIGH);
    digitalWrite(D2, HIGH);
    digitalWrite(D3, HIGH);
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH); // Fan
    digitalWrite(D7, HIGH);
    digitalWrite(D8, HIGH);
    handleRoot();
  }
  else { // All on
    All = "ON";
    switch1 = "ON";
    switch2 = "ON";
    switch3 = "ON";
    // switch4 = "ON";
    switch5 = "ON";
    Fan = "Speed4";
    AC = "ON";
    switch1status = true;
    switch2status = true;
    switch3status = true;
    //switch4status = true;
    switch5status = true;
    my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch1status);
    my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch2status);
    my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch3status);
    my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch4status);
    my_switch5.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch5status);
    digitalWrite(D0, LOW);
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);
    // digitalWrite(D3, LOW);
    digitalWrite(D8, LOW);
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH);
    digitalWrite(D7, HIGH);
    delay(500);
    digitalWrite(D7, LOW);
    handleRoot();
    delay(500);
  }
}

void FLfunction(){
  if (FL == "ON" || switch5 == "ON" || Fan != "OFF") { // FL off
    FL = "OFF";
    switch5 = "OFF";
    Fan = "OFF";
    my_switch5.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch5status);
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH); // Fan
    digitalWrite(D7, HIGH);
    digitalWrite(D8, HIGH);
    handleRoot();
  }
  else { // FL on
    FL = "ON";
    switch5 = "ON";
    Fan = "Speed4";
    my_switch5.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch5status);
    digitalWrite(D8, LOW);
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH);
    digitalWrite(D7, HIGH);
    delay(500);
    digitalWrite(D7, LOW);
    handleRoot();
    delay(500);
  }
}

void AcON(){
  AC = "ON";
  irsend.sendSony(0x240A, 15); //irsend.sendNEC(142607175, 28);
  delay(200);
  handleRoot();
}

void AcOFF(){
  AC = "OFF";
  irsend.sendSony(0x640A, 15); //irsend.sendNEC(143392849, 28);
  delay(200);
  handleRoot();
}

void Mannualfunction(){
  if (Mannual == "ON") {
    Mannual = "OFF";
    handleRoot();
    delay(200);
  }
  else {
    Mannual = "ON";
    handleRoot();
    delay(200);
  }
}

void Button(){
  int x = analogRead(A0);
  if (90 < x && x < 110) { // switch2
    //switch2OFF();
    //delay(500);
  }
  if (100 < x && x < 200) { // switch3
    //switch3OFF();
    //delay(500);
  } 
  if (200 < x && x < 300 && Fan == "Speed4") { // Fan decrease
    FanSpeed3();
    delay(500);
  }
  else if (200 < x && x < 300 && Fan == "Speed3") { // Fan decrease
    FanSpeed2();
    delay(500);
  }
  else if (200 < x && x < 300 && Fan == "Speed2") { // Fan decrease
    FanSpeed1();
    delay(500);
  }
  else if (200 < x && x < 300 && Fan == "Speed1") { // Fan decrease
    FanOFF();
    delay(500);
  }
  if (300 < x && x < 400 && Fan != "OFF") { // Fan off 
    FanOFF();
    delay(500);
  }
  else if (300 < x && x < 400) { // Fan on
    FanSpeed4();
    delay(500);
  }
  if (400 < x && x < 500 && Fan == "OFF") { // Fan increase
    FanSpeed1();
    delay(500);
  }
  else if (400 < x && x < 500 && Fan == "Speed1") { // Fan increase
    FanSpeed2();
    delay(500);
  }
  else if (400 < x && x < 500 && Fan == "Speed2") { // Fan increase
    FanSpeed3();
    delay(500);
  }
  else if (400 < x && x < 500 && Fan == "Speed3") { // Fan increase
    FanSpeed4();
    delay(500);
  }
  if (500 < x && x < 600 && switch5 == "ON") { // switch5 off Light
    switch5function();
    delay(500);
  }
  if (600 < x && x < 700 && switch1 == "ON") { // switch1
    switch1function();
    delay(500);
  }
  if (700 < x && x < 800 && switch4 == "ON") { // switch4
    switch4function();
    delay(500);
  }
  if ((800 < x) && (x < 900)) { // All off
    FLfunction();
    delay(500);
  }
}

void ir_remote() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);  
    if (results.value==IR1) {
      switch1function();
    }
    else if (results.value==IR2) {
      switch2function();
    }
    else if (results.value==IR3) {
      switch3function();
    }
    else if (results.value==IR4) {
      switch4function();
    }
    else if (results.value==IR5) {
      switch5function();
    }
    else if (results.value==IRF0) {
      switchfanfunction();
    }
    else if (results.value==IRF1) {
      FanSpeed1();
    }
    else if (results.value==IRF2) {
      FanSpeed2();
    }
    else if (results.value==IRF3) {
      FanSpeed3();
    }
    else if (results.value==IRF4) {
      FanSpeed4();
    }
    else {
      Serial.println("Wrong IR button pressed"); 
      delay(100);
    }
    irrecv.resume(); // receive the next value
  }
}

void ir_sensor_module() {
  bool irs0 = digitalRead(IR_Sensor0);
  bool irs1 = digitalRead(IR_Sensor1);
  if (irs0 == 0) {
    t1 = millis();
    Serial.print("t1: ");
    Serial.println(t1);
    if (t1<t2 && t1 !=0 && t2 !=0) {
      Serial.println("left to right");
      t1 = 0;
      t2 = 0;
    }
    if (t2<t1 && t1 !=0 && t2 !=0) {
      Serial.println("right to left");
      t1 = 0;
      t2 = 0;
    }
  }
  if (irs1 == 0) {
    t2 = millis();
    Serial.print("t2: ");
    Serial.println(t2); 
    if (t1<t2 && t1 !=0 && t2 !=0) {
      Serial.println("left to right");
      t1 = 0;
      t2 = 0;
    }
    if (t2<t1 && t1 !=0 && t2 !=0) {
      Serial.println("right to left");
      t1 = 0;
      t2 = 0;
    }
  }
}  
bool previous_sw4_val = 0;
bool previous_fansw_val = 0;
bool previous_sw5_val = 0;
void switch_control() {
  bool sw4 = digitalRead(sw_control_4); //35
  bool fansw = digitalRead(sw_control_5); //32
  bool sw5 = digitalRead(sw_control_6); //33
  if (sw4 != previous_sw4_val) {
    switch4function();
    previous_sw4_val = sw4;
  }
  if (fansw != previous_fansw_val) {
    switchfanfunction();
    previous_fansw_val = fansw;
  }
  if (sw5 != previous_sw5_val) {
    switch5function();
    previous_sw5_val = sw5;
  }
  delay(500);
}

void setup(void){
  //pinMode(A0, INPUT);
  pinMode(sw_control_4, INPUT);//35
  pinMode(sw_control_5, INPUT);//32
  pinMode(sw_control_6, INPUT);//33
  pinMode(DHTPIN, INPUT);
  pinMode(IR_Reciver, INPUT);
  pinMode(IR_Transmitter, OUTPUT);
  pinMode(IR_Sensor0, INPUT);
  pinMode(IR_Sensor1, INPUT);
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
  digitalWrite(D0, HIGH);
  digitalWrite(D1, HIGH);
  digitalWrite(D2, HIGH);
  digitalWrite(D3, HIGH);
  digitalWrite(D4, HIGH);
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, LOW);
  digitalWrite(D8, LOW);

  previous_sw4_val = digitalRead(sw_control_4);
  previous_fansw_val = digitalRead(sw_control_5);
  previous_sw5_val = digitalRead(sw_control_6);
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
 
  server.on("/", handleRoot);  
  server.on("/Switch1", switch1function);    
  server.on("/Switch2", switch2function);   
  server.on("/Switch3", switch3function);  
  server.on("/Switch4", switch4function);  
  server.on("/Switch5", switch5function);
  server.on("/FanOFF", FanOFF); 
  server.on("/FanSpeed1", FanSpeed1);   
  server.on("/FanSpeed2", FanSpeed2);
  server.on("/FanSpeed3", FanSpeed3);
  server.on("/FanSpeed4", FanSpeed4);  
  server.on("/FanSpeed4", FanSpeed4);  
  server.on("/FanBreeze", FanBreezefunction); 
  server.on("/Fandec", Fandecfunction); 
  server.on("/Fanmode", FanModefunction); 
  server.on("/Faninc", Fanincfunction); 
  server.on("/Fanmode", FanModefunction); 
  server.on("/All", Allfunction);  
  server.on("/FL", FLfunction);
  server.on("/AcON", AcON);  
  server.on("/AcOFF", AcOFF);  
  server.on("/Mannual", Mannualfunction);   

  server.begin();     
  Serial.println("HTTP server started");

  // RainMaker Part
  Serial.println("IR Receiver Raw Data + Button Decode Test");
  irrecv.enableIRIn();
  dht.begin();

  Node my_node;
    my_node = RMaker.initNode("Smart Home");

    //Standard switch device
    my_switch1.addCb(write_callback);
    my_node.addDevice(my_switch1);

    my_switch2.addCb(write_callback);
    my_node.addDevice(my_switch2);

    my_switch3.addCb(write_callback);
    my_node.addDevice(my_switch3);

    my_switch4.addCb(write_callback);
    my_node.addDevice(my_switch4);

    my_switch5.addCb(write_callback);
    my_node.addDevice(my_switch5);

    my_fan.addCb(write_callback);
    Param speed("My_Speed",ESP_RMAKER_PARAM_RANGE , value(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    speed.addBounds(value(0), value(4), value(1));
    speed.addUIType(ESP_RMAKER_UI_SLIDER);
    my_fan.addParam(speed);
    my_node.addDevice(my_fan);
    my_fan.updateAndReportParam("My_Speed", 0);
    my_fan.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, fan_power);

    my_node.addDevice(temperature);
    my_node.addDevice(humidity);

    RMaker.enableOTA(OTA_USING_PARAMS);
    RMaker.enableTZService();
    RMaker.enableSchedule();
    RMaker.start();

    Timer.setInterval(SensorInterval);

    WiFi.onEvent(sysProvEvent);
#if CONFIG_IDF_TARGET_ESP32S2
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#else
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#endif
    
    my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch5.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, true);
    my_fan.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, true);

}

void loop(void){   
  /*int x = analogRead(A0);
  if (199 < x && Mannual == "ON") {
    Serial.print(x);
    Serial.print("\n");
    Button();
  }*/
  server.handleClient();  

  // RainMaker Part

  ir_remote();
  //ir_sensor_module();
  switch_control();

  if(digitalRead(gpio_reset) == LOW) { //10sec factory reset all
        delay(100);
        int startTime = millis();
        while(digitalRead(gpio_reset) == LOW) delay(50);
        int endTime = millis();
        if ((endTime - startTime) > 10000) {
          RMakerFactoryReset(2);
      }
    }

  if (Timer.isReady()) {                    // Check is ready a second timer
    //Serial.println("Sending Sensor's Data");
    Send_Sensor();
    Timer.reset();                        // Reset a second timer
  }

  delay(100);
}
