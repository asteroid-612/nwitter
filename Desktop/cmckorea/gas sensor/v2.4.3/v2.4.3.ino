#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <WiFiClient.h>

#include <DNSServer.h>

#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "MQ7.h"
#include <ArduinoJson.h>

#include "RunningMedian.h"

#include <Thread.h>
#include <ThreadController.h>

#include "miseSensor.h"

#include <string>
#include "Ucglib.h"

const String DEVICE_NAME_PREFIX = "MISE";
const String FIRMWARE_PREFIX = "MISE-";
const String FIRMWARE_VERSION = "2.4.3";

/**
 * Wifi 제어 변수 및 Thread
 * 로직
 *  - 1. 와이파이가 연결되어 있지 않으면 5초에 한번씩 총 5번 와이파이 연결 시도
 *  - 2. 1번에서 연결이 안되면 다음으로 1분에 한번씩 총 5번 와이파이 연결 시도
 *  - 3. 2번에서 연결이 안되면 다음으로 5분에 한번씩 연결이 될 때까지 연결 시도
 */
const String DATA_UPLOAD_SERVER_PROTOCALL = "http://";
const String DATA_UPLOAD_SERVER_URL = "api.mise.today";
const String DATA_UPLOAD_SERVER_RESTAPI_URI = "/rest/v1/data/datas";

int _wifiConnectCheckDepth = 1;
int _wifiConnectCheckCount = 0; // 최대 5번
boolean _isWifiCheckThreadRun = false;
boolean _isWifiCheckProcessThreadRun = false;

// ThreadController that will controll all threads

// 최상위 Thread Controller
ThreadController _miseController = ThreadController();
//  - 센서의 데이터 처리를 위한 Thread Controller
ThreadController _sensorGroupController = ThreadController();
//  - wifi 연결 처리를 위한 Thread Controller
ThreadController _wifiGroupController = ThreadController();

// 센서 제어 Thread
//  1.센서 데이터의 값을 가져오기 위한 Thread
Thread _sensorDataThread = Thread();
//  2.센서 데이터의 평균값을 Display화면에 보이기 위한 Thread
Thread _sensorTotalDataDisplayThread = Thread();
//  3.센서 데이터 서버 전송을 위한 Thread
Thread _sensorDataSendToServerThread = Thread();

// wifi 제어 Thread
Thread _wifiCheckThread = Thread();
Thread _wifiCheckProcessThread = Thread();

const int BAUDRAIT_9600 = 9600;
const int BAUDRAIT = 115200;
const int UQ_CHIP_ID = ESP.getChipId();

String _language = ""; // 기기언어
String _isOfflineMode;
boolean _isInitSettingFinished;
String _apiKey = "";

uint8_t _macArray[6];
char _macChar[18];

/*** WIFI 설정 ***/
WiFiClient _client;
// 여러개의 wifi 설정을 위한 변수
ESP8266WiFiMulti _wiFiMulti;   
DNSServer _dnsServer;
ESP8266WebServer _webServer(80);

const byte DNS_PORT = 53;

//const IPAddress _apIP(192, 168, 1, 1);
int _apIpAddress1 = 192;
int _apIpAddress2 = 168;
int _apIpAddress3 = 1;
int _apIpAddress4 = 1;

//const String APP_SSID = "PMCIN_" + (String)UQ_CHIP_ID;
const String APP_SSID = "MISE_TODAY_" + (String)UQ_CHIP_ID;
String _apPW = "dust" + (String)UQ_CHIP_ID;       //default PW : dust(UQID)
String _wifiSsid = "";
String _wifiPass = "";
String _wifiSsidList;
boolean _isWifiConnected = false;

const String WIFI_SETTING_RECOMMAND_MSG = "Please set WIFI";
int _wifiSettingRecommandCount = 0;

// Html 설정
String _resHTMLCss, _resHTMLScript, _dftInfoEn, _dftInfoKo;

//long _newMillis, _apiMillis, _scrMillis;

/***** 센서 정보 ******/
/* PIN
A0 - CO일산화탄소 뒷면3
D3 - 온습도
D1 - 기압SCL
D2 - 기압SDA
D7 - 미세먼지5 (GPIO13) - PMS7003
D8 - 미세먼지4
3v - 미세먼지1, 온습도2, co
gnd - 미세먼지2, 온습도3
*/
// set the LCD number of columns and rows
const int LCD_COLUMNS = 16;
const int LCD_ROWS = 2;

int _l2cAddress;
boolean _isLcd = false;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
//LiquidCrystal_I2C _lcd(0x27, LCD_COLUMNS, LCD_ROWS);   //   0x27 (39) or 0x3f (63) -> i2c  address
//LiquidCrystal_I2C _lcd2(0x3f, LCD_COLUMNS, LCD_ROWS);
LiquidCrystal_I2C *_lcd = NULL;

// 미세먼지 센서 (https://www.arduino.cc/en/Reference/SoftwareSerialConstructor)
boolean _isDustSensor = true;
SoftwareSerial _dustSensor(DUST_RX_PIN, DUST_TX_PIN, false, 256);

// 온습도 센서
boolean _isDhtSensor = true;
DHT _dht22(DHT_PIN22, DHT_TYPE22);

// 센서 데이터 변수 정의
int _sendSensorDataCount = 0;
int _sendSumPM01 = 0;
int _sendSumPM2_5 = 0;
int _sendSumPM10 = 0;
float _sendSumTemp = 0;
float _sendSumHumi = 0;

int previous_pm_2_5 = 0;
int previous_chk_range = 0;

int _medianDataCount = 0;
RunningMedian _runningMedianPM01 = RunningMedian(5);
RunningMedian _runningMedianPM2_5 = RunningMedian(5);
RunningMedian _runningMedianPM10 = RunningMedian(5);
RunningMedian _runningMedianTemp = RunningMedian(5);
RunningMedian _runningMedianHumi = RunningMedian(5);

/**
 * EEPROM
 * 
 * 0     30  31      40 41        50     100        200        300                           512
 * |------|--|-------|--|----------|------|----------|----------|-----------------------------|
 *      Language  OfflineModeYN      SSID   SSID PWD    API KEY   Change Pwd
 */

/**
 * 초기 설정 - Reset 버튼을 눌러도 이 함수가 호출됨
 */
void setup() {
  _isInitSettingFinished = false;

  Wire.begin(BAUDRAIT_9600);
  Serial.begin(BAUDRAIT_9600);
  EEPROM.begin(512);
  //pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(FLASH_PIN, INPUT_PULLUP);

  delay(10);

  _dht22.begin();
  _dustSensor.begin(BAUDRAIT_9600);

  delay(10);

  Serial.println("[PMCIN] Particulate Matter Citizen Information Network\t미세먼지 시민정보 네트워크\n");

  setDeviceMacAddress();
  delay(10);

  //resetUserData();

  // LCD가 있는지 확인
  if(getLcdChannelAddress() > 0){
    _lcd = new LiquidCrystal_I2C(_l2cAddress, LCD_COLUMNS, LCD_ROWS);
    setLcdClear();
    delay(10);
    setLcdPrintBooting();
  }

  // 미세먼지 센서 유무 확인 - available() 이 함수를 사용하게 되면 이곳에서 읽기 부분을 완료해야 함
  /*if(_dustSensor.available()){
    _isDustSensor = true;
    Serial.println("Dust Sensor [PMS3003] is available");
  }*/

  // 초기설정
  if(initRomConfig()) {
    wifiSetupMode();

    Serial.println("Finish initRomConfig");

    if (_isOfflineMode == "N"){
      // 센서의 median data 평균값을 api.mise.today서버에 전송
      _sensorDataSendToServerThread.onRun(sensorDataSendToServerThreadCallback);
      _sensorDataSendToServerThread.setInterval(30*1000);
      _sensorDataSendToServerThread.enabled = true;

      _sensorGroupController.add(&_sensorDataSendToServerThread);
    }

    // 센서에 해당하는 데이터정보 가져오기 (1초당 호출)
    _sensorDataThread.onRun(sensorDataThreadCallback);
    _sensorDataThread.setInterval(1000);
    _sensorDataThread.enabled = true;

    _sensorGroupController.add(&_sensorDataThread);

    //if(_isLcd){
      // 디스플레이에 센서 데이터 표시 (5초당 호출)
      _sensorTotalDataDisplayThread.onRun(sensorTotalDataDisplayThreadCallback);
      _sensorTotalDataDisplayThread.setInterval(5*1000);
      _sensorTotalDataDisplayThread.enabled = true;

      _sensorGroupController.add(&_sensorTotalDataDisplayThread);
    //}

    _miseController.add(&_sensorGroupController);
  }

}

void loop() {
  //Serial.println("===== loop() =====");
  //_newMillis = millis();

  if (_isInitSettingFinished) {
    _dnsServer.processNextRequest();
    _webServer.handleClient();
  }

  /*if (digitalRead(RESET_PIN) == LOW) { //Reset Button
    Serial.println("===== Reset Button Clicked =====");
    resetUserData();
  }*/

  if(digitalRead(FLASH_PIN) == LOW){
    //Serial.println("===== Flsh Button Clicked =====");
    resetDevicePwd();
  }

  // run ThreadController
  // this will check every thread inside ThreadController,
  // if it should run. If yes, he will run it;
  _miseController.run();

  // Rest of code
  //float h = 3.1415;
  //h/=2;
}

/**
 * 디바이스의 MAC Address 값 설정하기
 */
void setDeviceMacAddress(){
  WiFi.macAddress(_macArray);
  for (int i = 0; i < sizeof(_macArray); ++i){
    sprintf(_macChar,"%s%02x:", _macChar, _macArray[i]);
  }
  //Serial.println("MacAddress : " + getDeviceMacAddress());
}

/**
 * 디바이스의 MAC Address 값 가져오기
 */
String getDeviceMacAddress(){
  String tempStr = (String)_macChar;
  int length = tempStr.length(); // 문자열 길이
  tempStr = tempStr.substring(0, length-1);
  return tempStr;
}

/**
 * 기본 설정값을 Rom에서 가져오기
 */
boolean initRomConfig() {
  Serial.println("Reading EEPROM...");
  // Language
  for (int i = 30; i < 32; ++i) {
    _language += char(EEPROM.read(i));
  }
  
  if(_language == ""){
    _language = "en";  
  }
  
  Serial.print("language: ");
  Serial.println(_language);

  _isOfflineMode = "";
  for (int i = 40; i < 41; ++i) {
    _isOfflineMode = char(EEPROM.read(i));
  }

  if(_isOfflineMode == ""){
    _isOfflineMode = "Y";
  }

  Serial.print("isOfflineMode: ");
  Serial.println(_isOfflineMode);

  // self_pw - 설정화면을 위한 비밀번호
  char tempApPw;
  int i = 300;
  if(EEPROM.read(i)){
    _apPW = "";
  }
  while(tempApPw = char(EEPROM.read(i))){
    _apPW += tempApPw;
    i++;
  }
  Serial.print("Self PW: ");
  Serial.println(_apPW);

  if (EEPROM.read(50) != 0) {
    // WIFI SSID
    for (int i = 50; i < 100; ++i) {
      _wifiSsid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(_wifiSsid);

    // WIFI PW
    for (int i = 100; i < 150; ++i) {
      _wifiPass += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(_wifiPass);

    if(_isOfflineMode == "N"){
      WiFi.begin(_wifiSsid.c_str(), _wifiPass.c_str());
    }

    // Mise.Today API_KEY
    char tempApiKey;
    int j = 200;
    while(tempApiKey = char(EEPROM.read(j))){
      _apiKey += tempApiKey;
      j++;
    }
    Serial.print("API KEY: ");
    Serial.println(_apiKey);
  } else {
    Serial.println("Config not found.");
    //return false;
  }

  return true;
}

/**************** LCD start **********************/
/**
 * LCD의 채널 주소값 가져오기
 */
int getLcdChannelAddress(){
  byte error, address;
  int nDevices;

  _l2cAddress = 0;

  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        //Serial.print("0");
      }
      //Serial.println(address, HEX);
      Serial.println(address);

      _l2cAddress = address;
      nDevices++;
    }else if (error == 4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        //Serial.print("0");
      }
      //Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    _isLcd = false;
  }else {
    _isLcd = true;
  }

  return _l2cAddress;
}

/**
 * LCD 화면 초기 설정
 */
void setLcdPrintBooting() {
  setLcdBegin();
  setLcdBacklight();
  //setLcdPrint("PMCIN ");
  setLcdPrint(DEVICE_NAME_PREFIX + " ");
  setLcdPrint(UQ_CHIP_ID);
  setLcdSetCursor(0,1);
  setLcdPrint("Booting...v");
  setLcdPrint(FIRMWARE_VERSION);
}

void setLcdBegin() {
  if(_lcd != NULL){
    _lcd -> begin();
  }
}

void setLcdBacklight() {
  if(_lcd != NULL){
    _lcd -> backlight();
  }
}

void setLcdSetCursor(int a, int b) {
  if(_lcd != NULL){
    _lcd -> setCursor(a, b);
  }
}

void setLcdPrint(String str) {
  if(_lcd != NULL){
    _lcd -> print(str);
  }
}

void setLcdPrint(int a) {
  if(_lcd != NULL){
    _lcd -> print(a);
  }
}

void setLcdPrint(float a) {
  if(_lcd != NULL){
    _lcd -> print(a);
  }
}

void setLcdHome() {
  if(_lcd != NULL){
    _lcd -> home();
  }
}

void setLcdClear() {
  if(_lcd != NULL){
    _lcd -> clear();
  }
}

void setLcdPrintReboot(){
  setLcdClear();
  setLcdHome();
  setLcdPrint("Setting Finished");
  setLcdSetCursor(0,1);
  setLcdPrint("Rebooting...");
}

void setPrintError(int code) {
  setLcdClear();
  setLcdHome();
  setLcdPrint("Error Accured!");
  setLcdSetCursor(0,1);
  setLcdPrint("Code: ");
  setLcdPrint(code);

  Serial.println("Error Accured!");
  Serial.print("Code: ");
  Serial.println(code);
}

/**
 * 데이터 LCD에 표시하기
 */
void setLcdPrintSensorData(int tmpPM01, int tmpPM2_5, int tmpPM10, float tmpTemp, float tmpHumi){
  boolean isWifiSettingRecommand = false;

  setLcdClear();
  setLcdHome();

  if(_isOfflineMode == "N" && !_isWifiConnected){
    if(_wifiSettingRecommandCount < 3){
      _wifiSettingRecommandCount++;
    }else{
      isWifiSettingRecommand = true;
      _wifiSettingRecommandCount = 0;
    }
  }

  if(isWifiSettingRecommand){
    setLcdPrint(DEVICE_NAME_PREFIX + " ");
    setLcdPrint(UQ_CHIP_ID);
    setLcdSetCursor(0,1);
    setLcdPrint(WIFI_SETTING_RECOMMAND_MSG);
  }else{
    // 온도
    char tempTempVal[4];
    dtostrf(tmpTemp, 4, 1, tempTempVal);

    setLcdSetCursor(0,0);
    setLcdPrint(tempTempVal);
    //setLcdPrint("\xDF" "C");

    // 습도
    char tempHumiVal[4];
    dtostrf(tmpHumi, 4, 1, tempHumiVal);

    setLcdSetCursor(0,1);
    setLcdPrint(tempHumiVal);
    setLcdPrint("%");

    // PM1
    setLcdSetCursor(5,0);
    setLcdPrint(tmpPM01);
    setLcdPrint(" ");

    // PM2.5
    setLcdSetCursor(9,0);
    setLcdPrint(tmpPM2_5);
    setLcdPrint(" ");

    // PM10
    setLcdSetCursor(13,0);
    setLcdPrint(tmpPM10);
    setLcdPrint(" ");
  }

}

/**************** LCD end **********************/

/**************** Rom Setting start **********************/
void resetUserData(){
  for (int i = 0; i < 512; ++i) {
    EEPROM.write(i, 0);
  }

  // 언어 설정 (기본 : en)
  for (int i = 0; i < 2; ++i) {
    EEPROM.write(30+i, _language[i]);
  }

  // offline mode 설정
  EEPROM.write(40, _isOfflineMode[0]);

  EEPROM.commit();
}

/**
 * 디바이스 장비의 비번 초기화
 */
void resetDevicePwd(){
  for (int i = 300; i < 512; ++i) {
    EEPROM.write(i, 0);
  }
  
  EEPROM.commit();

  setLcdPrintReboot();
  ESP.restart();
}
/**************** Rom Setting end **********************/

/**************** WIFI Setting end **********************/
boolean checkWifiConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while ( count < 20 ) {
    //if (WiFi.status() == WL_CONNECTED) {
    if (_wiFiMulti.run() == WL_CONNECTED) {  
      Serial.println();
      Serial.println("Connected!");
      return true;
    }else{
      Serial.print("checkWifiConnection - SSID: ");
      Serial.print(_wifiSsid.c_str());
      Serial.print(" SSID PWD: ");
      Serial.println(_wifiPass.c_str());

      //WiFi.begin(_wifiSsid.c_str(), _wifiPass.c_str());
      
      WiFi.mode(WIFI_STA);
      
      _wiFiMulti.addAP(_wifiSsid.c_str(), _wifiPass.c_str());
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

void wifiSetupMode() {
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);
  int n = WiFi.scanNetworks();

  delay(100);
  Serial.println("");

  for (int i = 0; i < n; ++i) {
    _wifiSsidList += "<option value=\"";
    _wifiSsidList += WiFi.SSID(i);
    _wifiSsidList += "\">";
    _wifiSsidList += WiFi.SSID(i);
    _wifiSsidList += "</option>";
  }

  wifiSetupApMode();
}

void wifiSetupApMode(){
  delay(100);
  WiFi.mode(WIFI_AP);

  IPAddress apIP(_apIpAddress1, _apIpAddress2, _apIpAddress3, _apIpAddress4);

  boolean isApConfig = WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if(isApConfig){
    Serial.println("isApConfig is true : " + APP_SSID);
    WiFi.softAP(APP_SSID, _apPW);

    _dnsServer.start(DNS_PORT, "*", apIP);
    startWebServer();

    Serial.print("Starting Access Point at \"");
    Serial.print(APP_SSID);
    Serial.println("\"");

  }else{
    Serial.println("isApConfig is false");

    if(_apIpAddress4 < 256){
      _apIpAddress4++;
    }else{
      _apIpAddress3++;
      _apIpAddress4 = 1;
    }

    wifiSetupApMode();
  }
}

void startWebServer(){
  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.softAPIP());

  Serial.print("startWebServer _isOfflineMode : ");
  Serial.println(_isOfflineMode);

  // 기본 - 영문
  _dftInfoEn = "<h1>FineDust Device Setting</h1>";
  _dftInfoEn += "<p>(Firmware : " + FIRMWARE_PREFIX + FIRMWARE_VERSION + ")</p>";
  _dftInfoEn += "<p>(Chip ID : " + (String)UQ_CHIP_ID + ")</p>";
  _dftInfoEn += "<p>(MAC : " + getDeviceMacAddress() + ")</p>";
  _dftInfoEn += "<p><a>Be sure to follow the steps in the documentation.</a></p>";
  _dftInfoEn += "<p><a href=\"/dft_settings\">Default Setup</a></p>";
  if(_isOfflineMode == "N"){
    _dftInfoEn += "<p><a href=\"/settings\">Network Setup</a></p>";
  }
  _dftInfoEn += "<p><a href=\"/reset\">Reset Settings</a></p>";
  _dftInfoEn += "<p><a href=\"/change_language\">한국어</a></p>";

  // 기본 - 한글
  _dftInfoKo = "<h1>미세먼지 기기 설정</h1>";
  _dftInfoKo += "<p>(펌웨어 : " + FIRMWARE_PREFIX + FIRMWARE_VERSION + ")</p>";
  _dftInfoKo += "<p>(Chip ID : " + (String)UQ_CHIP_ID + ")</p>";
  _dftInfoKo += "<p>(MAC : " + getDeviceMacAddress() + ")</p>";
  _dftInfoKo += "<p><a>반드시 설명서 단계를 따라 진행해 주시기바랍니다.</a></p>";
  _dftInfoKo += "<p><a href=\"/dft_settings\">기본 설정</a></p>";
  if(_isOfflineMode == "N"){
    _dftInfoKo += "<p><a href=\"/settings\">네크워크 설정</a></p>";
  }
  _dftInfoKo += "<p><a href=\"/reset\">모든 설정 초기화</a></p>";
  _dftInfoKo += "<p><a href=\"/change_language\">English</a></p>";

  // 초기화면
  _webServer.on("/change_language", []() {
    String s = "";
    if(_language == "ko") {
      _language = "en";
      s = _dftInfoEn;
    }else{
      _language = "ko";
      s = _dftInfoKo;
    }

    for (int i = 0; i < 2; ++i) {
      EEPROM.write(30+i, _language[i]);
    }
    EEPROM.commit();
    _webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  });

  _webServer.on("/dft_settings", []() {
    String s = "";
    if(_language == "ko") {
      s = "<h1>기본 설정</h1><p>반드시 설명서 단계를 따라 진행해 주시기바랍니다.</p><br>";
      s += "<form method=\"get\" action=\"setdftap\">";
      s += "<p><b>오프라인 모드 : </b>";
      if(_isOfflineMode == "Y"){
        s += "<label><input type=\"radio\" name=\"is_offline_mode\" value=\"N\" onclick=\"setOfflineMode(this)\">아니오</label> <label><input type=\"radio\" name=\"is_offline_mode\" value=\"Y\" onclick=\"setOfflineMode(this)\" checked>예</label></p>";
      }else{
        s += "<label><input type=\"radio\" name=\"is_offline_mode\" value=\"N\" onclick=\"setOfflineMode(this)\" checked>아니오</label> <label><input type=\"radio\" name=\"is_offline_mode\" value=\"Y\" onclick=\"setOfflineMode(this)\">예</label></p>";
      }

      s += "<p><b>기기 패스워드변경: </b><input name=\"change_pw\" length=64 type=\"password\"><br>";
      
      s += "<input type=\"submit\" value=\"등록\"></form>";
    }else {
      s = "<h1>Default Setting</h1><p>Be sure to follow the steps in the documentation.</p><br>";
      s += "<form method=\"get\" action=\"setdftap\">";
      s += "<p><b>Offline Mode : </b>";
      if(_isOfflineMode == "Y"){
        s += "<label><input type=\"radio\" name=\"is_offline_mode\" value=\"N\" onclick=\"setOfflineMode(this)\">No</label> <input type=\"radio\" name=\"is_offline_mode\" value=\"Y\" onclick=\"setOfflineMode(this)\" checked>Yes</label></p>";
      }else{
        s += "<label><input type=\"radio\" name=\"is_offline_mode\" value=\"N\" onclick=\"setOfflineMode(this)\" checked>No</label> <label><input type=\"radio\" name=\"is_offline_mode\" value=\"Y\" onclick=\"setOfflineMode(this)\">Yes</label></p>";
      }

      s += "<p><b>Change Device Password: </b><input name=\"change_pw\" length=64 type=\"password\"><br>";
      
      s += "<input type=\"submit\" value=\"submit\"></form>";
    }
    _webServer.send(200, "text/html", makePage("Default Settings", s));
  });

  _webServer.on("/settings", []() {
    String s = "";
    if(_language == "ko") {
      s = "<h1>네트워크 설정</h1><p>반드시 설명서 단계를 따라 진행해 주시기바랍니다.</p><br>";
      s += "<form method=\"get\" action=\"setap\">";
      s += "<p><b>Wi-Fi 정보</b></p>&nbsp;&nbsp;<label>SSID: </label><select name=\"ssid\">";
      s += _wifiSsidList;
      s += "</select><br>&nbsp;&nbsp;패스워드: <input name=\"pass\" id=\"pass\" length=64 type=\"password\" minlength=\"8\" required><br><br>";
      s += "<p><b>Mise.today</b><br>";
      s += "&nbsp;&nbsp;Write API Key: <input name=\"key\" id=\"key\" length=64 type=\"text\" minlength=\"32\" maxlength=\"32\" value=\"" + _apiKey +"\" required></p><br>";
      s += "<input type=\"submit\" value=\"등록\"></form>";
    }else {
      s = "<h1>Network Setting</h1><p>Be sure to follow the steps in the documentation.</p><br>";
      s += "<form method=\"get\" action=\"setap\">";
      s += "<p><b>Wi-Fi</b></p>&nbsp;&nbsp;<label>SSID: </label><select name=\"ssid\">";
      s += _wifiSsidList;
      s += "</select><br>&nbsp;&nbsp;Password: <input name=\"pass\" id=\"pass\" length=64 type=\"password\" required><br><br>";
      s += "<p><b>Mise.today</b><br>";
      s += "&nbsp;&nbsp;Write API Key: <input name=\"key\" id=\"key\" length=64 type=\"text\" minlength=\"32\" maxlength=\"32\" value=\"" + _apiKey +"\" required></p><br>";
      s += "<input type=\"submit\" value=\"submit\"></form>";
    }
    _webServer.send(200, "text/html", makePage("Network Settings", s));
  });

  _webServer.on("/wifi_reconnect", []() { // 재시도
    String s = "";
    if(_language == "ko") {
      s = "<h1>기기 Wi-Fi 연결 재시도</h1><p>기존에 설정한 Wi-Fi AP로 다시 연결을 시도합니다.</p>";
    }else {
      s = "<h1>Retry Wi-Fi Connection</h1><p>Trying to reconnect to a Wi-Fi AP that you have already set up.</p>";
    }
    _webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    ESP.restart();
  });

  _webServer.on("/reset", []() { // 초기화
    resetUserData();

    String s = "";
    if(_language == "ko") {
      s = _dftInfoKo;
    }else{
      s = _dftInfoEn;
    }

    _webServer.send(200, "text/html", makePage("Settings", s));
    ESP.restart();
  });

  _webServer.on("/wifi_settings", []() {
    String s = "";
    if(_language == "ko") {
      s = "<h1>기기 Wi-Fi 재설정</h1><p>반드시 설명서 단계를 따라 진행해 주시기바랍니다.</p><p>Wi-Fi 정보</p><form method=\"get\" action=\"wifiset\"><label>SSID: </label><select name=\"ssid\">";
      s += _wifiSsidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><br><input type=\"submit\"></form>";
    }else {
      s = "<h1>Reset Wi-Fi Connection Config</h1><p>Be sure to follow the steps in the documentation.</p><p>Wi-Fi</p><form method=\"get\" action=\"wifiset\"><label>SSID: </label><select name=\"ssid\">";
      s += _wifiSsidList;
      s += "</select><br>패스워드: <input name=\"pass\" length=64 type=\"password\"><br><input type=\"submit\"></form>";
    }

    _webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  });

  // 기본 설정 정보 저장 (Offline 모드 설정)
  _webServer.on("/setdftap", []() {
    for (int i = 40; i < 41; ++i) {
      EEPROM.write(i, 0);
    }

    _isOfflineMode = urlDecode(_webServer.arg("is_offline_mode"));
    Serial.print("isOfflineMode: ");
    Serial.println(_isOfflineMode);

    Serial.println("Writing isOfflineMode to EEPROM...");
    for (int i = 0; i < _isOfflineMode.length(); ++i) {
      EEPROM.write(40 + i, _isOfflineMode[i]);
    }

    // 기기 비번 변경
    String change_pw = urlDecode(_webServer.arg("change_pw"));
    Serial.print("change_pw: ");
    Serial.println(change_pw);
    
    if (change_pw.length() > 0) {
      Serial.println("Writing self.PW to EEPROM...");
      for (int i = 0; i < change_pw.length(); ++i) {
        EEPROM.write(300 + i, change_pw[i]);
      }
    }
    
    EEPROM.commit();
    Serial.println("Write EEPROM done!");

    String s = "";
    if(_language == "ko") {
      s = _dftInfoKo;
    }else{
      s = _dftInfoEn;
    }
    _webServer.send(200, "text/html", makePage("AP mode", s));

    setLcdPrintReboot();
    ESP.restart();
  });

  // 네트워크 설정 정보 저장
  _webServer.on("/setap", []() {
    for (int i = 50; i < 300; ++i) {
      EEPROM.write(i, 0);
    }

    _wifiSsid = urlDecode(_webServer.arg("ssid"));
    Serial.print("SSID: ");
    Serial.println(_wifiSsid);

    _wifiPass = urlDecode(_webServer.arg("pass"));
    Serial.print("Password: ");
    Serial.println(_wifiPass);

    _apiKey = urlDecode(_webServer.arg("key"));
    Serial.print("Mise.today API Key : ");
    Serial.println(_apiKey);

    Serial.println("Writing SSID to EEPROM...");
    for (int i = 0; i < _wifiSsid.length(); ++i) {
      EEPROM.write(50 + i, _wifiSsid[i]);
    }

    Serial.println("Writing Password to EEPROM...");
    for (int i = 0; i < _wifiPass.length(); ++i) {
      EEPROM.write(100 + i, _wifiPass[i]);
    }

    Serial.println("Writing API Key to EEPROM...");
    for (int i = 0; i < _apiKey.length(); ++i) {
      EEPROM.write(200 + i, _apiKey[i]);
    }

    EEPROM.commit();
    Serial.println("Write EEPROM done!");

    String s = "";
    if(_language == "ko") {
      s = "<h1>기기 설정 완료</h1><p>재부팅후 설정하신 \"";
      s += _wifiSsid;
      s += "\" 에 연결됩니다.";
    }else {
      s = "<h1>Device Setting Finished</h1><p>device will be connected to \"";
      s += _wifiSsid;
      s += "\" after the restart.";
    }
    _webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    setLcdPrintReboot();
    ESP.restart();
  });

  _webServer.on("/wifiset", []() {
    for (int i = 50; i < 150; ++i) {
      EEPROM.write(i, 0);
    }

    String ssid = urlDecode(_webServer.arg("ssid"));
    Serial.print("SSID: ");
    Serial.println(ssid);
    String pass = urlDecode(_webServer.arg("pass"));
    Serial.print("Password: ");
    Serial.println(pass);

    Serial.println("Writing SSID to EEPROM...");
    for (int i = 0; i < ssid.length(); ++i) {
      EEPROM.write(50 + i, ssid[i]);
    }

    Serial.println("Writing Password to EEPROM...");
    for (int i = 0; i < pass.length(); ++i) {
      EEPROM.write(100 + i, pass[i]);
    }

    EEPROM.commit();
    Serial.println("Write EEPROM done!");

    String s = "";
    if(_language == "ko") {
      s = "<h1>기기 설정 완료</h1><p>재부팅후 설정하신 \"";
      s += ssid;
      s += "\" 에 연결됩니다.";
    }else{
      s = "<h1>Device Setting Finished</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
    }
    _webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    setLcdPrintReboot();
    ESP.restart();
  });

  _webServer.onNotFound([]() {
    String s = "";
    if(_language == "ko") {
      s = _dftInfoKo;
    }else{
      s = _dftInfoEn;
    }
    _webServer.send(200, "text/html", makePage("AP mode", s));
  });

  _isInitSettingFinished = true;
  _webServer.begin();
}

/*
void makeCss() {
  _resHTMLCss = "<style>\n";
  _resHTMLCss += "body{text-align: center;font-family:verdana;}\n";
  _resHTMLCss += "div,input{padding:5px;font-size:1em;}\n";
  _resHTMLCss += "input{width:95%;}\n";
  _resHTMLCss += ".c{text-align: center;}\n";
  _resHTMLCss += "button submit{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}\n";
  _resHTMLCss += ".q{float: right;width: 64px;text-align: right;}\n";
  _resHTMLCss += "</style>\n";
}

void makeJavascript() {
   _resHTMLScript = "<SCRIPT>\n";
   _resHTMLScript += "alert('ok');\n";
   _resHTMLScript += "function setOfflineMode(radio){\n";
   _resHTMLScript +=  "alert(radio.value);\n";
   //_resHTMLScript +=  "document.getElementById(\"pass\").attributes.required = \"required\";\n";
   //_resHTMLScript +=  "document.getElementById(\"key\").attributes.required = \"required\";\n";
   _resHTMLScript += "}\n";
   _resHTMLScript += "</SCRIPT>\n";
}*/

String makePage(String title, String contents) {
  //makeCss();
  //makeJavascript();

  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<meta charset=\"utf-8\">";
  s += "<title>";
  s += title;
  s += "</title></head>";
  //s += _resHTMLCss;
  //s += _resHTMLScript;
  s += "<body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

/**************** WIFI Setting end **********************/

/**************** Thread function start **********************/

/**
 * WIFI 연결확인
 */
void wifiCheckThreadCallback(){
  Serial.print(F("wifiCheckThreadCallback : "));
  Serial.println(millis());

  if(_isWifiConnected){
    Serial.println(F("wifiCheckThreadCallback Wifi is connected "));

    // 이 thread를 호출하는 곳으로 이동
    if(_isWifiCheckProcessThreadRun){
      _wifiCheckProcessThread.enabled = false;
      _wifiGroupController.remove(&_wifiCheckProcessThread);
      _miseController.remove(&_wifiGroupController);
    }

    _wifiConnectCheckCount = 0;
    _wifiConnectCheckDepth = 1;

    _isWifiCheckProcessThreadRun = false;
    _isWifiConnected = true;
  }else{
    if(!_isWifiCheckProcessThreadRun){
      int intervalTime = 5;
      if(_wifiConnectCheckDepth > 1){
        _wifiCheckProcessThread.enabled = false;
        _wifiGroupController.remove(&_wifiCheckProcessThread);
        _miseController.remove(&_wifiGroupController);
      }

      if(_wifiConnectCheckDepth == 2){
        intervalTime = 60;
      }else if(_wifiConnectCheckDepth == 3){
        intervalTime = 5 * 60;
      }

      _isWifiCheckProcessThreadRun = true;

      _wifiCheckProcessThread.onRun(wifiCheckProcessThreadCallback);
      _wifiCheckProcessThread.setInterval(intervalTime * 1000);
      _wifiCheckProcessThread.enabled = true;

      _wifiGroupController.add(&_wifiCheckProcessThread);
      _miseController.add(&_wifiGroupController);
    }

  }

}

void wifiCheckProcessThreadCallback(){
  Serial.print(F("wifiCheckProcessThreadCallback : "));
  Serial.println(millis());

  if(!_isWifiConnected){
    if(_wifiConnectCheckCount < 5){
      if (checkWifiConnection()) {
        _wifiConnectCheckCount = 0;
        _wifiConnectCheckDepth = 1;

        _isWifiConnected = true;
        //_isWifiCheckProcessThreadRun = false;
      }else{
        _isWifiConnected = false;
        _wifiConnectCheckCount++;
      }
    }else{
      if(_wifiConnectCheckDepth != 3){
        _wifiConnectCheckCount = 0;
        _wifiConnectCheckDepth++;

        //_isWifiCheckProcessThreadRun = false;

        Serial.print(F("_wifiConnectCheckDepth : "));
        Serial.println(_wifiConnectCheckDepth);
      }
    }
  }

}


/**
 * 센서에 해당하는 데이터정보 가져오기 (1초당 호출)
 */
void sensorDataThreadCallback(){
  //Serial.print(F("sensorDataThreadCallback : "));
  //Serial.println(millis());

  int tmpPM01 = -1;
  int tmpPM2_5 = -1;
  int tmpPM10 = -1;
  float tmpTemp = -100.0;
  float tmpHumi = -100.0;

  int dustIndex = 0;
  unsigned char destValue;
  unsigned char previousValue;

  // 1. 미세먼지 데이터 정보 가져오기
  if(_isDustSensor){
    while (_dustSensor.available()) {
      destValue = _dustSensor.read();

      if ((dustIndex == 0 && destValue != 0x42) || (dustIndex == 1 && destValue != 0x4d)){
        //Serial.println("Cannot find the data header.");
        break;
      }

      if (dustIndex == 4 || dustIndex == 6 || dustIndex == 8 || dustIndex == 10 || dustIndex == 12 || dustIndex == 14) {
        previousValue = destValue;
      }
      else if (dustIndex == 5) {
        tmpPM01 = 256 * previousValue + destValue;
      }
      else if (dustIndex == 7) {
        tmpPM2_5 = 256 * previousValue + destValue;
      }
      else if (dustIndex == 9) {
        tmpPM10 = 256 * previousValue + destValue;
      } else if (dustIndex > 15) {
        break;
      }
      dustIndex++;
    }
    while(_dustSensor.available()) _dustSensor.read();
  }

  //while(Serial.available()) Serial.read();

  // 2. 온습도 데이터 정보 가져오기 (기본 : DHT22)
  //if(_isDhtSensor){}
  tmpTemp = _dht22.readTemperature();
  tmpHumi = _dht22.readHumidity();

  //tmpTemp = 27;
  //tmpHumi = 38;

  if(tmpPM01 > -1 && tmpPM2_5 > -1 && tmpPM10 > -1 && !isnan(tmpTemp) && !isnan(tmpHumi)){
    _runningMedianPM01.add(tmpPM01);
    _runningMedianPM2_5.add(tmpPM2_5);
    _runningMedianPM10.add(tmpPM10);
    _runningMedianTemp.add(tmpTemp);
    _runningMedianHumi.add(tmpHumi);

    Serial.print("Count : ");
    Serial.print(_medianDataCount);

    Serial.print(" PM1:  ");
    Serial.print(tmpPM01);
    Serial.print("    ");

    Serial.print("PM2.5:  ");
    Serial.print(tmpPM2_5);
    Serial.print("     ");

    Serial.print("PM10:  ");
    Serial.print(tmpPM10);
    Serial.print("     ");

    Serial.print(tmpTemp);
    Serial.print("ºC   ");

    Serial.print(tmpHumi);
    Serial.print("%    ");
    Serial.println();

    _medianDataCount++;
  }
}

/**
 * 디스플레이에 센서 데이터 표시 (5초당 호출)
 */
void sensorTotalDataDisplayThreadCallback(){
  //Serial.print(F("sensorTotalDataDisplayThreadCallback"));
  //Serial.println(millis());

  // median 값으로 데이터를 추출한다.
  if(_medianDataCount > 0){
    int tmpPM01 = _runningMedianPM01.getMedian();
    int tmpPM2_5 = _runningMedianPM2_5.getMedian();
    int tmpPM10 = _runningMedianPM10.getMedian();
    float tmpTemp = _runningMedianTemp.getMedian();
    float tmpHumi = _runningMedianHumi.getMedian();

    _sendSumPM01 += tmpPM01;
    _sendSumPM2_5 += tmpPM2_5;
    _sendSumPM10 += tmpPM10;
    _sendSumTemp += tmpTemp;
    _sendSumHumi += tmpHumi;
    _sendSensorDataCount++;

    Serial.print(F("LCD Display - "));
    Serial.print("PM1:");
    Serial.print(tmpPM01);
    Serial.print("  PM2.5:");
    Serial.print(tmpPM2_5);
    Serial.print("  PM10:");
    Serial.print(tmpPM10);
    Serial.print("  ");
    Serial.print(tmpTemp);
    Serial.print(" °C  ");
    Serial.print(tmpHumi);
    Serial.print(" %");
    Serial.println();
    Serial.println();

    if(_isLcd){
      setLcdPrintSensorData(tmpPM01, tmpPM2_5, tmpPM10, tmpTemp, tmpHumi);
    }else{
      // 임심 5초당 서버에 데이터 보내기
      //delay(10);
      //sensorDataSendToServerThreadCallback();
    }
  }

  if(_isOfflineMode == "Y"){
    resetMedainDataVaribale();
  }else{
    _medianDataCount = 0;

    _runningMedianPM01.clear();
    _runningMedianPM2_5.clear();
    _runningMedianPM10.clear();
    _runningMedianTemp.clear();
    _runningMedianHumi.clear();
  }
}

/**
 * 센서의 평균값을 api.mise.today서버에 전송 (30초당 호출)
 */
void sensorDataSendToServerThreadCallback(){
  Serial.print(F("sensorDataSendToServerThreadCallback : "));
  Serial.println(millis());

  if(_sendSensorDataCount > 0){
    if(_isWifiConnected){

      //_wifiCheckRunThread.enabled = false;
      //_wifiGroupController.remove(&_wifiCheckRunThread);
      //_miseController.remove(&_wifiGroupController);

      if(_isWifiCheckThreadRun){
        _isWifiCheckThreadRun = false;
        _wifiCheckThread.enabled = false;
        _miseController.remove(&_wifiCheckThread);
      }

      //if(_client.connect(DATA_UPLOAD_SERVER_URL, 80)){
      if((_wiFiMulti.run() == WL_CONNECTED)){
        HTTPClient http;
        
        // 서버에 보낼때 median의 평균값으로 보내도록 수정 - 2019.06.13
        int tmpPM01 = round(_sendSumPM01 / _sendSensorDataCount);
        int tmpPM2_5 = round(_sendSumPM2_5 / _sendSensorDataCount);
        int tmpPM10 = round(_sendSumPM10 / _sendSensorDataCount);
        float tmpTemp = _sendSumTemp / _sendSensorDataCount;
        float tmpHumi = _sendSumHumi / _sendSensorDataCount;

        String params = "{";
               params += "\"writeApiKey\":\"" + _apiKey + "\", ";
               params += "\"dataPm1\":\"" + (String)tmpPM01 + "\", ";
               params += "\"dataPm25\":\"" + (String)tmpPM2_5 + "\", ";
               params += "\"dataPm10\":\"" + (String)tmpPM10 + "\", ";
               params += "\"dataTemp\":\"" + (String)tmpTemp + "\", ";
               params += "\"dataHumi\":\"" + (String)tmpHumi + "\"";
               params += "}";

        //Serial.print("params:");
        //Serial.println(params);

        // http://api.mise.today/rest/v1/data/datas
        http.begin(DATA_UPLOAD_SERVER_PROTOCALL + DATA_UPLOAD_SERVER_URL + DATA_UPLOAD_SERVER_RESTAPI_URI);      //Specify request destination
        http.addHeader("Content-Type", "application/json");  //Specify content-type header
        int httpCode = http.POST(params);   //Send the request
        String payload = http.getString();    //Get the response payload

        //Serial.print("httpCode : ");
        //Serial.println(httpCode);   //Print HTTP return code
        if(httpCode != 200) {
          // TODO : 오류가 연속으로 5번정도 발생할 경우 기존의 WIFI 연결을 끊고 다시 연결하여 데이터 전송을 해야함 - 2019.06.20
          setPrintError(9000 + httpCode);
          Serial.println("Send Error - payload: " + payload);
        } else{
          Serial.print(F("Send to MISE.TODAY - "));
          Serial.print("PM1:");
          Serial.print((String)tmpPM01);
          Serial.print("  PM2.5:");
          Serial.print((String)tmpPM2_5);
          Serial.print("  PM10:");
          Serial.print((String)tmpPM10);
          Serial.print("  ");
          Serial.print((String)tmpTemp);
          Serial.print(" °C  ");
          Serial.print((String)tmpHumi);
          Serial.print(" %");
          Serial.println();
          Serial.println();
          resetMedainDataVaribale();
        }

        http.end();  //Close connection
      }else{
        Serial.println(F("_client disconnect "));
        
        _isWifiConnected = false;
        _isWifiCheckThreadRun = true;

        _wifiCheckThread.onRun(wifiCheckThreadCallback);
        _wifiCheckThread.setInterval(1000);
        _wifiCheckThread.enabled = true;

        _miseController.add(&_wifiCheckThread);
      }

      _client.stop();
    }else{
      Serial.println(F("Wifi is not connected"));

      _isWifiConnected = false;
      _isWifiCheckThreadRun = true;

      _wifiCheckThread.onRun(wifiCheckThreadCallback);
      _wifiCheckThread.setInterval(1000);
      _wifiCheckThread.enabled = true;

      _miseController.add(&_wifiCheckThread);
    }

  }else{
    Serial.println(F("sensorDataSendToServerThreadCallback : _sendSensorDataCount is none"));
  }
}

/**
 * Median 데이터에 대한 변수 초기화
 */
void resetMedainDataVaribale(){
  _medianDataCount = 0;

  _runningMedianPM01.clear();
  _runningMedianPM2_5.clear();
  _runningMedianPM10.clear();
  _runningMedianTemp.clear();
  _runningMedianHumi.clear();

  _sendSensorDataCount = 0;
  _sendSumPM01 = 0;
  _sendSumPM2_5 = 0;
  _sendSumPM10 = 0;
  _sendSumTemp = 0;
  _sendSumHumi = 0;
}


/**************** Thread function end **********************/
