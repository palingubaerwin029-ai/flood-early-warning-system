// === All 6 Features Combined ===
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// --- Pin definitions ---
#define TRIG_PIN   13
#define ECHO_PIN   12
#define LED_R      25
#define LED_G      26
#define LED_B      27
#define BUZZER     14
#define RAIN_AOUT  35
#define RAIN_DOUT  34

// --- Thresholds ---
#define MOUNT_HEIGHT  150  // cm — measure yours
#define WARN_CM        30
#define CRIT_CM        60
#define RAIN_HEAVY     15  // mm/hr

// --- Credentials ---
const char* ssid      = "YOUR_WIFI";
const char* password  = "YOUR_PASS";
#define API_KEY   "AIzaSyArXXSkoBAUBQ8w5BgqC1m9NvzJKN5n2L8"
#define DB_URL    "https://flood-warning-system-29346-default-rtdb.asia-southeast1.firebasedatabase.app/"
const char* contacts[] = {"+639XX","+639XX"};
const int   numContacts = 2;

// --- Objects ---
HardwareSerial      sim(2);
Adafruit_SSD1306    display(128,64,&Wire,-1);
FirebaseData        fbdo;
FirebaseAuth        auth;
FirebaseConfig      config;

// --- State ---
float    waterLevel=0, rainRate=0;
int      alertLevel=0;
bool     raining=false;
unsigned long lastSMS=0, lastLog=0;
int      logIdx=0;

// --- Helpers (see previous steps for full code) ---
float getWaterLevel() { /* Step 2 code */ return 0; }
float getRainRate()   { /* Step 6 code */ return 0; }
bool  isRaining()     { return digitalRead(RAIN_DOUT)==LOW; }
void  sendSMS(String m){ /* Step 3 code */ }
void  setLED(bool r,bool g,bool b){
  digitalWrite(LED_R,r);digitalWrite(LED_G,g);
  digitalWrite(LED_B,b);
}
void  buzz(int ms){
  digitalWrite(BUZZER,HIGH);delay(ms);
  digitalWrite(BUZZER,LOW);
}
void  logToFirebase(float l,int a,float r,bool rn){
  /* Step 5 code */ }

void setup() {
  Serial.begin(9600);
  sim.begin(9600,SERIAL_8N1,16,17);
  pinMode(TRIG_PIN,OUTPUT); pinMode(ECHO_PIN,INPUT);
  pinMode(LED_R,OUTPUT); pinMode(LED_G,OUTPUT);
  pinMode(LED_B,OUTPUT); pinMode(BUZZER,OUTPUT);
  pinMode(RAIN_DOUT,INPUT);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  delay(3000);
  sim.println("AT+CMGF=1");
  WiFi.begin(ssid,password);
  int w=0;
  while(WiFi.status()!=WL_CONNECTED&&w++<20)delay(500);
  config.api_key=API_KEY; config.database_url=DB_URL;
  Firebase.begin(&config,&auth);
  setLED(false,true,false); // green = system ready
  buzz(100); delay(100); buzz(100);
}

void loop() {
  waterLevel = getWaterLevel();
  rainRate   = getRainRate();
  raining    = isRaining();
  unsigned long now = millis();

  // Determine alert level
  if      (waterLevel >= CRIT_CM) alertLevel = 2;
  else if (waterLevel >= WARN_CM) alertLevel = 1;
  else                            alertLevel = 0;

  // Apply LED + buzzer
  if (alertLevel==2) {
    setLED(true,false,false);
    for(int i=0;i<5;i++){buzz(300);delay(200);}
  } else if (alertLevel==1) {
    setLED(false,false,true); // blue
  } else {
    setLED(false,true,false); // green
    digitalWrite(BUZZER,LOW);
  }

  // SMS logic with cooldown
  if (alertLevel==2 && now-lastSMS>120000) {
    sendSMS("!! FLOOD CRITICAL !! Water:"
      +String((int)waterLevel)+"cm EVACUATE NOW!");
    lastSMS=now;
  } else if (alertLevel==1 && now-lastSMS>300000) {
    sendSMS("FLOOD WARNING: Water "
      +String((int)waterLevel)+"cm rising.");
    lastSMS=now;
  }

  // Pre-flood rain SMS (Feature 6)
  if (rainRate>=RAIN_HEAVY && alertLevel==0
      && now-lastSMS>600000) {
    sendSMS("HEAVY RAIN: "+String((int)rainRate)
      +"mm/hr. Flood possible in ~20min.");
    lastSMS=now;
  }

  // Firebase log every 60 seconds
  if (now-lastLog>60000) {
    logToFirebase(waterLevel,alertLevel,
                  rainRate,raining);
    lastLog=now;
  }

  // Update OLED
  String status = alertLevel==2?"CRITICAL":
                  alertLevel==1?"WARNING":"SAFE";
  showOLED(waterLevel,status,rainRate,raining);

  delay(10000); // check every 10 seconds
}