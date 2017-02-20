//needed for ESP8266 WiFi operations
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for wifi manager library
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

//needed for Json Response parse
#include <ArduinoJson.h>

//needed for eeprom access
#include <EEPROM.h>
#include <Arduino.h>
#include "EEPROMAnything.h"

//needed for OLED Display
#include <Wire.h>
#include "SH1106.h" //alis for `#include "SH1106Wire.h"

//Include custom images
#include "images.h"

//Object Display--> for OLED Display SH1106 Class
SH1106 display(0x3c, D4, D3);

//Object client--> for ESP8266 SSL WiFi Client for HTTPS connection
WiFiClientSecure client;

//GPIO Definations
#define IR1 D7 //IR Sensor1 Pin
#define IR2 D6 //IR Sensor2 Pin
#define IR3 D5 //IR Sensor3 Pin
#define PB1 D8 //Push Button1 Pin--> Set ESP8266 as WiFi Access point in first 10 sec after powerup than Reel-1 count reset
#define PB2 D2 //Push Button2 Pin--> Reel-2 count reset
#define PB3 D1 //Push Button3 Pin--> Set ESP8266 as WiFi Station in first 10 sec after powerup than Reel-3 count reset

const unsigned int REEL_RST_CNT = 100;
const unsigned int TH_RE1 = 30;
const unsigned int TH_RE2 = 30;
const unsigned int TH_RE3 = 30;

const unsigned int CNT1A = 2000; //EEPROM Address for hold Reel-1 available turns Count
const unsigned int CNT2A = 2004; //EEPROM Address for hold Reel-2 available turns Count
const unsigned int CNT3A = 2008; //EEPROM Address for hold Reel-3 available turns Count
String RFID1 = "6D003575A18C";   //RFID1 Tag on Reel-1
String RFID2 = "6D0034917FB7";   //RFID2 Tag on Reel-2
String RFID3 = "6D00348CF623";   //RFID3 Tag on Reel-3

volatile int reel1_count; //Variable holding Runtime Reel-1 available turns Count
volatile int reel2_count; //Variable holding Runtime Reel-2 available turns Count
volatile int reel3_count; //Variable holding Runtime Reel-3 available turns Count

int reel=0; //Variable holding reel No currently inserted in 3d printer

int re1_rep_flag = 0;
int re2_rep_flag = 0;
int re3_rep_flag = 0;

int rst1_flag = 0;
int rst2_flag = 0;
int rst3_flag = 0;


//AMAZON DRS parameters

const char* device_model = "c0282353-3360-48d0-94ea-eae22b6acfea"; //Product model ID of your device 
const char* device_identifier = "ABCDE12345"; // Serial No of device any thing you wish at production end
const char* client_id = "amzn1.application-oa2-client.30cb1d72e67857fbbd860962161ca335"; // Client Id from ur amazon developer account
const char* client_secret = "34a5e877410e1786712598bfbca296049b356e229a354e68780eecbb0e819e0f"; //Client Secret from your amazon developer account
const char* slot_id1_reel1 = "63d62e80-8839-454d-8d51-0d54ea8fef47"; // Slot 1 ID for reel 1 preference
const char* slot_id2_reel2 = "1d1e7765-2a91-4f5a-94fd-fce8e873ac79"; // Slot 2 ID for reel 2 preference
const char* slot_id3_reel3 = "326b6ff0-536d-41de-8968-37783f3b5256"; // Slot 3 ID for reel 3 preference
const char* redirect_uri = "https%3A%2F%2Fnathwani.tk%2Findexauth.html"; //should be encoded and same as amazon developer device settings
                            
int startup = 1,con_flag=0;


void setup() {

  EEPROM.begin(2048);
  //eepromWriteInt(CNT1A, 100);
  //eepromWriteInt(CNT2A, 200);
  //eepromWriteInt(CNT3A, 300);
  //EEPROM.commit();

  // put your setup code here, to run once:
  pinMode(IR1,INPUT_PULLUP);
  pinMode(IR2,INPUT_PULLUP);
  pinMode(IR3,INPUT_PULLUP);

  pinMode(PB1,INPUT_PULLUP);
  pinMode(PB2,INPUT_PULLUP);
  pinMode(PB3,INPUT_PULLUP);
  
  Serial.begin(9600);
  reel1_count = eepromReadInt(CNT1A);
  reel2_count = eepromReadInt(CNT2A);
  reel3_count = eepromReadInt(CNT3A);
  display_start();
  int loopI = 0;
  for(loopI = 0; loopI <= 120; loopI++)
  {
    delay(50);
    if(digitalRead(PB1)==LOW)
    {
      display_ap();
      config_ap_auth();
      loopI = 121;
    }
    else if(digitalRead(PB3)==LOW)
    {
      display_connecting();
      //delay(3000);
      loopI = 121;
    }
  }
 
  attachInterrupt(IR1, isr_ir1, FALLING);
}

void loop() 
{

  check_card();
  check_reset();
  check_rep_need();
  display_info();
  // put your main code here, to run repeatedly:
  if(is_wifi_connected() == 1)
  {
    if(startup==1)
    {
      display_connecting();
      Serial.println("connecting to AP..");
      delay(1000);
      unsigned int x = obtain_access_and_refresh_token();
      Serial.println(x);
      Serial.println("Connected to AP..DRS Enabled..");
      delay(2000);
      startup++;
    }
    
    if(re1_rep_flag == 1 && rst1_flag == 0)
    {
      con_flag=1;
      rst1_flag = 1;
      display.clear();
      display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
      display.setFont(ArialMT_Plain_16);      
      display.drawString(0, 16, "REEL-1");
      display.drawString(0, 32, "START");
      display.drawString(0, 48, "REPLENISH");
      display.display();
      Serial.println("...REPLENISHING reel1...");
      unsigned int y = refresh_access_token();
      Serial.println(y);
      delay(500);
      unsigned int z=end_point_request_replenishment(slot_id1_reel1);
      Serial.println(z);
      if(z==11)
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);        
        display.drawString(0, 16, "REEL-1");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "PLACED");
        display.display();
        re1_rep_flag = 0;
      }
      else if(z==13)
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);        
        display.drawString(0, 16, "REEL-1");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "IN-PROGRESS");
        display.display();
        re1_rep_flag = 0;
      }
      else
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);        
        display.drawString(0, 16, "REEL-1");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "FAILED");
        display.display();
        re1_rep_flag = 0;
      }
      delay(5000);
    }
    
  else if(re2_rep_flag == 1 && rst2_flag == 0)
    {
      con_flag=1;
      rst2_flag = 1;
      display.clear();
      display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
      display.setFont(ArialMT_Plain_16);      
      display.drawString(0, 16, "REEL-2");
      display.drawString(0, 32, "START");
      display.drawString(0, 48, "REPLENISH");
      display.display();
      Serial.println("...REPLENISHING REEL2...");
      unsigned int y = refresh_access_token();
      Serial.println(y);
      delay(500);
      unsigned int z=end_point_request_replenishment(slot_id2_reel2);
      Serial.println(z);
      if(z==11)
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, 16, "REEL-2");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "PLACED");
        display.display();
        re2_rep_flag = 0;
      }
      else if(z==13)
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);        
        display.drawString(0, 16, "REEL-2");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "IN-PROGRESS");
        display.display();
        re2_rep_flag = 0;
      }
      else
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, 16, "REEL-2");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "FAILED");
        display.display();
        re2_rep_flag = 0;
      }
      delay(5000);
    }
    else if(re3_rep_flag == 1 && rst3_flag == 0)
    {
      con_flag=1;
      rst3_flag = 1;
      display.clear();
      display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 16, "REEL-3");
      display.drawString(0, 32, "START");
      display.drawString(0, 48, "REPLENISH");
      display.display();
      Serial.println("...REPLENISHING REEL3...");
      unsigned int y = refresh_access_token();
      Serial.println(y);
      delay(500);
      unsigned int z=end_point_request_replenishment(slot_id3_reel3);
      Serial.println(z);
      if(z==11)
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, 16, "REEL-3");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "PLACED");
        display.display();
        re3_rep_flag = 0;
      }
      else if(z==13)
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, 16, "REEL-3");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "IN-PROGRESS");
        display.display();
        re3_rep_flag = 0;
      }
      else
      {
        display.clear();
        display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, 16, "REEL-3");
        display.drawString(0, 32, "ORDER");
        display.drawString(0, 48, "FAILED");
        display.display();
        re3_rep_flag = 0;
       }
       delay(5000);
    }
  }
  delay(200);
}

//***********************  Init OLED display and show project Logos ***********
void display_start(void)
{
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  
  // clear the display
  display.clear();
  display.drawXbm(0, 0, rfl_scr_w, rfl_scr_h, rfl_scr);
  display.display();
  delay(3500);

  display.clear();
  display.drawXbm(0, 0, drs_scr_w, drs_scr_h, drs_scr);
  display.display();
  delay(3500);

  display.clear();
  display.drawXbm(0, 0, ccn_scr_w, ccn_scr_h, ccn_scr);
  display.display();
  delay(5);
}

//********************* Display Connecting Logo *****************
void display_connecting(void)
{
  display.clear();
  display.drawXbm(0, 0, cls_scr_w, cls_scr_h, cls_scr);
  display.display();
  delay(5);
}

//********************* Display Access Point Logo *****************
void display_ap(void)
{
  display.clear();
  display.drawXbm(0, 0, aps_scr_w, aps_scr_h, aps_scr);
  display.display();
  delay(5);
}

//********************* Display WiFI ok Logo (Small)*****************
void display_wifiok(void)
{
  display.clear();
  display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
  display.display();
  delay(5);
}

//********************* Display WiFI not ok Logo (Small)*****************
void display_wifinotok(void)
{
  display.clear();
  display.drawXbm(0, 0, nonw_img_w, nonw_img_h, nonw_img);
  display.display();
  delay(5);
}

//********************* Display INFO on OLED screen ****************
void display_info(void)
{
  display.clear();
  if(is_wifi_connected()==1)display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
  else display.drawXbm(0, 0, nonw_img_w, nonw_img_h, nonw_img);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  char info_reel1[] = "X. Reel-1=100 %   ";
  char info_reel2[] = "X. Reel-1=100 %   ";
  char info_reel3[] = "X. Reel-1=100 %   ";
  sprintf(info_reel1, "X. Reel-1=%d %%",reel1_count);
  sprintf(info_reel2, "X. Reel-2=%d %%",reel2_count);
  sprintf(info_reel3, "X. Reel-3=%d %%",reel3_count);
  if(reel == 1)info_reel1[0] = 'V';
  else if(reel == 2)info_reel2[0] = 'V';
  else if(reel == 3)info_reel3[0] = 'V';
  
  display.drawString(0, 16, info_reel1);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 32, info_reel2);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 48, info_reel3);
  display.display();
  delay(5);
}

//*********************** Read RFID tag (Detect reel No inserted in 3d Printer) *****************
void check_card(void)
{
  if(Serial.available()>=12)
  {
    char rfid[13];
    int i;
    for(i=0;i<=11;i++)rfid[i]=Serial.read();
    rfid[12]='\0';
    Serial.println(rfid);
    while(Serial.available()>0) Serial.read();
    String testcard=String(rfid);
    if (testcard.equals(RFID1)) reel = 1;
    else if (testcard.equals(RFID2)) reel = 2;
    else if (testcard.equals(RFID3)) reel = 3;
  }
}

//********************* Check for Reel Reset*********
void check_reset(void)
{
  if(digitalRead(PB1)==LOW)
  {
    reel1_count = REEL_RST_CNT;
    rst1_flag = 0;
    re1_rep_flag = 0;
    eepromWriteInt(CNT1A, reel1_count);
    EEPROM.commit();
  }
  else if(digitalRead(PB2)==LOW)
  {
    reel2_count = REEL_RST_CNT;
    rst2_flag = 0;
    re2_rep_flag = 0;
    eepromWriteInt(CNT2A, reel2_count);
    EEPROM.commit();
  }
  else if(digitalRead(PB3)==LOW)
  {
    reel3_count = REEL_RST_CNT;
    rst3_flag = 0;
    re3_rep_flag = 0;
    eepromWriteInt(CNT3A, reel3_count);
    EEPROM.commit();
  }
}

//**************************Check if replenishment needed? *******************
void check_rep_need(void)
{
  if(reel1_count < TH_RE1 && re1_rep_flag == 0)re1_rep_flag=1;
  else if(reel2_count < TH_RE2 && re2_rep_flag == 0)re2_rep_flag=1;
  else if(reel3_count < TH_RE3 && re3_rep_flag == 0)re3_rep_flag=1;
}

//************************** ISR for IR Sensor-1 event *************************
void isr_ir1()
{
  detachInterrupt(IR1);
  Serial.println("c1");
  attachInterrupt(IR2, isr_ir2, FALLING);
}

//************************** ISR for IR Sensor-2 event *************************
void isr_ir2()
{
  detachInterrupt(IR2);
  Serial.println("c2");
  attachInterrupt(IR3, isr_ir3, FALLING);
}

//***************** ISR for IR Sensor-3 event(detect 1 unwound) *****************
void isr_ir3()
{
  detachInterrupt(IR3);
  attachInterrupt(IR1, isr_ir1, FALLING);
  Serial.println("c3");
  if(reel == 1 && reel1_count>0)
  {
    reel1_count--;
    eepromWriteInt(CNT1A, reel1_count);
    EEPROM.commit();
  }
  else if(reel == 2 && reel2_count>0)
  {
    reel2_count--;
    eepromWriteInt(CNT2A, reel2_count);
    EEPROM.commit();
  }
  else if(reel == 3 && reel3_count>0)
  {
    reel3_count--;
    eepromWriteInt(CNT3A, reel3_count);
    EEPROM.commit();
  }
}
unsigned int obtain_access_and_refresh_token() 
{
  char _tmp_token[500];
  unsigned int _len_str;
  Serial.println();
  Serial.println("OBTAIN ACCESS AND REFRESH TOKEN FROM AUTH CODE:\r\nConnecting to Host: api.amazon.com");
  _len_str = eepromReadString(0, 30, _tmp_token);
  Serial.println(_tmp_token);
  // Use WiFiClientSecure class to create TCP connections
  if (!client.connect("api.amazon.com", 443)) {
    Serial.println("Error 5 --> Connection failed");
    return 5;
  }
  Serial.println("Connected\r\nSending Request for obtain Tokens");
  // This will send request for refresh of Access token to the server
  client.println("POST /auth/o2/token HTTP/1.1");
  client.println("Host: api.amazon.com"); 
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("Cache-Control: no-cache");
  client.print("Content-Length: ");client.println("262");
  client.println();
  client.print("grant_type=authorization_code");
  client.print("&code=");
  client.print(_tmp_token);
  client.print("&client_id=");
  client.print(client_id);
  client.print("&client_secret=");
  client.print(client_secret);
  client.print("&redirect_uri=");
  client.println(redirect_uri);
  Serial.println("Request Send");
  
  unsigned long timeout = millis();
  while (client.available() == 0) 
  {
    if (millis() - timeout > 35000) 
    {
      Serial.println("Error 4 --> Connection Timeout !");
      client.stop();
      return 4;
    }
  }
  // Read all the lines of the reply from server and print them to Serial
  String _line = "temp";
  unsigned int _line_no = 1;
  while(client.available())
  {
    _line = client.readStringUntil('\r');
    Serial.print(_line);
    if ((_line_no == 1)&& (_line != "HTTP/1.1 200 OK"))
    {
      Serial.println();
      Serial.println("Error 3 --> Invalid Response");
      client.stop();
      return 3;
    }
    _line_no++;
  }
  DynamicJsonBuffer jsonBuffer;
  // Responce last line have access token and referesh token json oblect 
  JsonObject& root = jsonBuffer.parseObject(_line);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("Error 2 --> Json parseObject() failed");
    return 2;
  }
  const char* _refresh_token = root["refresh_token"];
  // Print values
  Serial.println();
  Serial.print("Refresh token: ");
  Serial.println(_refresh_token);
  _len_str = eepromWriteString1(100, 500, _refresh_token);
  EEPROM.commit();
  Serial.print("Length: ");
  Serial.println(_len_str);
  //_len_str = eepromReadString(105, 500, _tmp_token);
  //Serial.print("Refresh token in EEPROM: ");
  //Serial.println(_tmp_token);
  //Serial.println(_len_str);
  Serial.println("SUCCESS");
  return 1;
}


unsigned int refresh_access_token() 
{
  char _tmp_token[500];
  unsigned int _len_str;
  Serial.println();
  Serial.println("REQUEST NEW ACCESS TOKEN BY PROVIDING REFRESH TOKEN:\r\nConnecting to Host: api.amazon.com");
  _len_str = eepromReadString(105, 500, _tmp_token);
  //Serial.println(_tmp_token);
  // Use WiFiClientSecure class to create TCP connections
  if (!client.connect("api.amazon.com", 443)) {
    Serial.println("Error 5 --> Connection failed");
    return 5;
  }
  Serial.println("Connected\r\nSending Request for obtain Tokens");
  // This will send request for refresh of Access token to the server
  client.println("POST /auth/o2/token HTTP/1.1");
  client.println("Host: api.amazon.com"); 
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("Cache-Control: no-cache");
  client.print("Content-Length: ");client.println(_len_str+197);
  client.println();
  client.print("grant_type=refresh_token");
  client.print("&refresh_token=Atzr%7C");
  client.print(_tmp_token);
  client.print("&client_id=");
  client.print(client_id);
  client.print("&client_secret=");
  client.println(client_secret);
  Serial.println("Request Send");
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 35000) {
      Serial.println("Error 4 --> Connection Timeout !");
      client.stop();
      return 4;
    }
  }
  // Read all the lines of the reply from server and print them to Serial
  String _line = "temp";
  unsigned int _line_no = 1;
  while(client.available())
  {
    _line = client.readStringUntil('\r');
    Serial.print(_line);
    if ((_line_no == 1)&& (_line != "HTTP/1.1 200 OK"))
    {
      Serial.println();
      Serial.println("Error 3 --> Invalid Response");
      client.stop();
      return 3;
    }
    _line_no++;
  }
  DynamicJsonBuffer jsonBuffer;
  // Responce last line have access token and referesh token json oblect 
  JsonObject& root = jsonBuffer.parseObject(_line);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("Error 2 --> Json parseObject() failed");
    return 2;
  }
  const char* access_token = root["access_token"];
  // Print values
  Serial.println();
  Serial.print("Access token: ");
  Serial.println(access_token);
  _len_str = eepromWriteString1(600, 500, access_token);
  EEPROM.commit();
  Serial.print("Length: ");
  Serial.println(_len_str);
  //len_str = eepromReadString(605,500, _tmp_token);
  //Serial.print("Access token in EEPROM: ");
  //Serial.println(_tmp_token);
  //Serial.println(_len_str);
  Serial.println("SUCCESS");
  return 1;
}

unsigned int end_point_request_replenishment(const char* _slot_id)
{
  char _tmp_token[500];
  unsigned int _len_str;
  Serial.println();
  Serial.println("REQUEST FOR REPLENISHMENT:\r\nConnecting to Host: dash-replenishment-service-na.amazon.com");
  _len_str = eepromReadString(605,500, _tmp_token);
  Serial.println(_tmp_token);
  // Use WiFiClientSecure class to create TCP connections
  if(!client.connect("dash-replenishment-service-na.amazon.com", 443)) 
  {
    Serial.println("Error 5 --> Connection failed");
    return 5;
  }
  Serial.println("Connected\r\nSending Request for Replenishment");
  //This will send the replenishment request to the Amazon DRS server
  client.print("POST /replenish/");client.print(_slot_id); client.println(" HTTP/1.1");
  client.println("Host: dash-replenishment-service-na.amazon.com");
  client.print("Authorization: Bearer Atza|");
  client.println(_tmp_token);
  client.println("Content-Length: 0");
  client.println("X-Amzn-Accept-Type: com.amazon.dash.replenishment.DrsReplenishResult@1.0");
  client.println("X-Amzn-Type-Version: com.amazon.dash.replenishment.DrsReplenishInput@1.0");
  client.println();
  Serial.println("Request Send");
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 20000) {
      Serial.println("Error 4 --> Connection Timeout !");
      client.stop();
      return 4;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  String _line = "temp";
  unsigned int _line_no = 1;
  while(client.available())
  {
    _line = client.readStringUntil('\r');
    Serial.print(_line);
    if ((_line_no == 1)&& (_line != "HTTP/1.1 200 OK"))
    {
      Serial.println();
      Serial.println("Error 3 --> Invalid Response");
      client.stop();
      return 3;
    }
    _line_no++;
  }
  DynamicJsonBuffer jsonBuffer;
  // Responce last line have access token and referesh token json oblect 
  JsonObject& root = jsonBuffer.parseObject(_line);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("Error 2 --> Json parseObject() failed");
    return 2;
  }
  const char* _detailCode = root["detailCode"];
  // Print values
  Serial.println();
  Serial.print("detailCode: ");
  Serial.println(_detailCode);
  //_len_str = eepromWriteString1(600, 500, _detailCode);
  //EEPROM.commit();
  //Serial.print("Length: ");
  //Serial.println(_len_str);
  //len_str = eepromReadString(605,500, _tmp_token);
  //Serial.print("Access token in EEPROM: ");
  //Serial.println(_tmp_token);
  //Serial.println(_len_str);
  if(strcmp(_detailCode,"STANDARD_ORDER_PLACED")==0)
  {
    Serial.println("SUCCESS 11 --> Standard Order Placed");
    return 11;
  }
  else if(strcmp(_detailCode,"TEST_ORDER_PLACED")==0)
  {
    Serial.println("SUCCESS 12 --> Test Order Placed");
    return 12;
  }
  else if(strcmp(_detailCode,"ORDER_INPROGRESS")==0)
  {
    Serial.println("SUCCESS 13 --> Order In Progress");
    return 13;
  }
  else
  {
    return 0;
  }
}

unsigned int end_point_request_subscriptioninfo(const char* _slot_id)
{
  char _tmp_token[500];
  unsigned int _len_str;
  Serial.println();
  Serial.println("REQUEST FOR REPLENISHMENT:\r\nConnecting to Host: dash-replenishment-service-na.amazon.com");
  _len_str = eepromReadString(605,500, _tmp_token);
  Serial.println(_tmp_token);
  // Use WiFiClientSecure class to create TCP connections
  if(!client.connect("dash-replenishment-service-na.amazon.com", 443)) 
  {
    Serial.println("Error 5 --> Connection failed");
    return 5;
  }
  Serial.println("Connected\r\nSending Request for Replenishment");
  //This will send the replenishment request to the Amazon DRS server
  client.print("POST /replenish/");client.print(_slot_id); client.println("HTTP/1.1");
  client.println("Host: dash-replenishment-service-na.amazon.com");
  client.print("Authorization: Bearer Atza|");
  client.println(_tmp_token);
  client.println("Content-Length: 0");
  client.println("X-Amzn-Accept-Type: com.amazon.dash.replenishment.DrsReplenishResult@1.0");
  client.println("X-Amzn-Type-Version: com.amazon.dash.replenishment.DrsReplenishInput@1.0");
  client.println();
  Serial.println("Request Send");
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 20000) {
      Serial.println("Error 4 --> Connection Timeout !");
      client.stop();
      return 4;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  String _line = "temp";
  unsigned int _line_no = 1;
  while(client.available())
  {
    _line = client.readStringUntil('\r');
    Serial.print(_line);
    if ((_line_no == 1)&& (_line != "HTTP/1.1 200 Accepted"))
    {
      Serial.println();
      Serial.println("Error 3 --> Invalid Response");
      client.stop();
      return 3;
    }
    _line_no++;
  }
  DynamicJsonBuffer jsonBuffer;
  // Responce last line have access token and referesh token json oblect 
  JsonObject& root = jsonBuffer.parseObject(_line);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("Error 2 --> Json parseObject() failed");
    return 2;
  }
  const char* _detailCode = root["detailCode"];
  // Print values
  Serial.println();
  Serial.print("detailCode: ");
  Serial.println(_detailCode);
  //_len_str = eepromWriteString1(600, 500, _detailCode);
  //EEPROM.commit();
  //Serial.print("Length: ");
  //Serial.println(_len_str);
  //len_str = eepromReadString(605,500, _tmp_token);
  //Serial.print("Access token in EEPROM: ");
  //Serial.println(_tmp_token);
  //Serial.println(_len_str);
  if(strcmp(_detailCode,"STANDARD_ORDER_PLACED")==0)
  {
    Serial.println("SUCCESS 11 --> Standard Order Placed");
    return 11;
  }
  else if(strcmp(_detailCode,"TEST_ORDER_PLACED")==0)
  {
    Serial.println("SUCCESS 12 --> Test Order Placed");
    return 12;
  }
  else if(strcmp(_detailCode,"ORDER_INPROGRESS")==0)
  {
    Serial.println("SUCCESS 13 --> Order In Progress");
    return 13;
  }
  else
  {
    return 0;
  }
}

void config_ap_auth(void) 
{
  char auth_temp[30];
  eepromReadString(0, 30, auth_temp);
  //WiFiManager
  WiFiManagerParameter custom_auth_code("Authorization", "authorization code", auth_temp, 25);
  WiFiManagerParameter custom_text("<br/>DRS Authorization Code");
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_auth_code);
  if (!wifiManager.startConfigPortal("ReFILL-ament")) 
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  strcpy(auth_temp, custom_auth_code.getValue());
  Serial.println(auth_temp);
  if(auth_temp[0] != 0x00)
  {
    eepromWriteString(0,21, auth_temp);
    EEPROM.commit();
    Serial.println("Auth code updated");
  }
  else
  {
    Serial.println("Auth code not Supplied");
  }
  //if you get here you have connected to the WiFi
  Serial.println("AP and Auth config done :)");
}

unsigned int is_wifi_connected()
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
    return 0;
  }
  else
  {
     return 1;
  }  
}

