#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "Smart_Irrigation_ESP32"; 
const char* password = "password123";

WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int sensorPin = 34;
const int solenoidPin = 26;
int moistureValue = 0;
bool isAutoMode = true; 
bool lastModeState = true; 

void handleRoot() {
  String modeStr = isAutoMode ? "AUTO" : "MANUAL";
  // Active Low logic: LOW = ON, HIGH = OFF
  String status = (digitalRead(solenoidPin) == LOW) ? "ON" : "OFF"; 
  
  String html = "<html><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>\
  <style>body{text-align:center; font-family: sans-serif; background:#f4f4f4;}\
  .box{background:white; padding:20px; border-radius:15px; display:inline-block; margin-top:20px; border:2px solid #2980b9;}\
  .btn{padding:15px; font-size:18px; cursor:pointer; border-radius:8px; border:none; color:white; margin:5px; text-decoration:none; display:inline-block; width:130px;}\
  .on{background:green;} .off{background:red;} .mode{background:orange; width:270px;}\
  </style>\
  <div class='box'><h1>Smart Control</h1>\
  <p>Mode: <b id='m_val'>" + modeStr + "</b></p>\
  <p>Moisture: <span id='moist'>" + String(moistureValue) + "%</span></p>\
  <p>Motor: <b id='stat'>" + status + "</b></p>\
  <a href='/toggleMode' class='btn mode'>Switch to " + (isAutoMode ? "MANUAL" : "AUTO") + "</a><br>";
  
  if(!isAutoMode) {
    html += "<a href='/on' class='btn on'>Motor ON</a>\
             <a href='/off' class='btn off'>Motor OFF</a>";
  }
  
  html += "</div><script>setInterval(function(){\
    fetch('/data').then(r=>r.json()).then(d=>{\
      document.getElementById('moist').innerText=d.m+'%';\
      document.getElementById('stat').innerText=(d.s==0)?'ON':'OFF';\
    });\
  }, 1000);</script></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(solenoidPin, OUTPUT);
  
  // Active Low relay শুরুতে অফ রাখতে HIGH সিগন্যাল দিতে হয়
  digitalWrite(solenoidPin, HIGH);

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/data", [](){ 
    String json = "{\"m\":" + String(moistureValue) + ",\"s\":" + String(digitalRead(solenoidPin)) + "}";
    server.send(200, "application/json", json); 
  });
  server.on("/toggleMode", [](){ isAutoMode = !isAutoMode; digitalWrite(solenoidPin, HIGH); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/on", [](){ if(!isAutoMode) digitalWrite(solenoidPin, LOW); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/off", [](){ if(!isAutoMode) digitalWrite(solenoidPin, HIGH); server.sendHeader("Location", "/"); server.send(303); });
  server.begin();
  lcd.clear();
}

void loop() {
  server.handleClient();
  int rawValue = analogRead(sensorPin);
  
  // সেন্সর বাতাসে থাকলে ০% এবং পানিতে ১০০% দেখাবে
  moistureValue = map(rawValue, 4095, 1200, 0, 100);
  if(moistureValue < 0) moistureValue = 0; if(moistureValue > 100) moistureValue = 100;

  if (isAutoMode) {
    if (moistureValue < 30) digitalWrite(solenoidPin, LOW); // মাটি শুকনা হলে রিলে ON (LOW)
    else if (moistureValue > 80) digitalWrite(solenoidPin, HIGH); // মাটি ভিজে গেলে রিলে OFF (HIGH)
  }

  // মোড পরিবর্তন হলে ডিসপ্লে একবার ক্লিয়ার করবে যাতে বাড়তি লেখা না থাকে
  if (isAutoMode != lastModeState) {
    lcd.clear();
    lastModeState = isAutoMode;
  }

  lcd.setCursor(0,0);
  lcd.print("Mode: "); 
  lcd.print(isAutoMode ? "AUTO  " : "MANUAL"); 
  
  lcd.setCursor(0,1);
  lcd.print("M:"); lcd.print(moistureValue); lcd.print("%  ");
  
  lcd.setCursor(8,1);
  lcd.print("Mot:"); 
  lcd.print(digitalRead(solenoidPin) == LOW ? "ON " : "OFF"); // ডিসপ্লে লজিক ফিক্সড
  
  delay(200);
}