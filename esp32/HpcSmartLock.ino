/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleWrite.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/
/** 调库  **/
//蓝牙
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <MD5Builder.h>
//持久化存储
#include <Preferences.h>
//WIFI webserevr
#include <WiFi.h>
#include <WebServer.h>
//RC522_NFC
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
//定时器
#include <Ticker.h>
//舵机
#include <ESP32Servo.h>



/**    全局变量     **/
//默认值
#define DEFAULT_PASSWORD "defaultpw"
#define DEFAULT_ONCEOPENTIME 2000
#define DEFAULT_ONCEOPENANGLE 180
#define DEFAULT_NFC_REBOOTTIME 1200
//BLE蓝牙配置
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_SALT_UUID "0410BB38-F577-9276-A9FC-47F94B9646ED"
#define CHARACTERISTIC_WEBSERVER_UUID "9e641724-2638-4250-9955-ce0576bd7bd0"
#define LED_BUILTIN 2
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristicSalt = NULL;
BLECharacteristic* pCharacteristic_Webserver = NULL;
bool deviceConnected = false;                //本次连接状态
bool oldDeviceConnected = false;             //上次连接状态
bool webserverStatus = false;                //AP_Webserver状态
int32_t salt = 0;

// bool doorIsOpen = false;
// TaskHandle_t TASK_HandleOne = NULL;
//舵机
int servoGpio = 22;
Servo myServo;

//Web
WebServer server(80);

//NFC
MFRC522DriverPinSimple ss_pin(5); // Configurable, see typical pin layout above.
MFRC522DriverSPI driver{ss_pin}; // Create SPI driver.
//MFRC522DriverI2C driver{}; // Create I2C driver.
MFRC522 mfrc522{driver};  // Create MFRC522 instance.
#define SS_PIN_SET 5

Ticker ticker;// 建立Ticker用于实现定时功能
//配置变量
String password;
String password_AP;
String password_admin;
bool webserverAlwaysStart = false;
bool NFCstart = false;
int32_t onceOpenTime;
int32_t onceOpenAngle;

int32_t NFC_rebootTime;
byte *NFC_id1 = new byte[4];
bool useNFC_id1 = false;
byte *NFC_id2 = new byte[4];
bool useNFC_id2 = false;
byte *NFC_id3 = new byte[4];
bool useNFC_id3 = false;
byte *NFC_id4 = new byte[4];
bool useNFC_id4 = false;
byte *NFC_id5 = new byte[4];
bool useNFC_id5 = false;
byte *NFC_id6 = new byte[4];
bool useNFC_id6 = false;

Preferences prefs; // 声明Preferences对象
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
void loadConfig() {
    prefs.begin("config"); // 打开命名空间m
    //uint32_t count = prefs.getUInt("WIFI_STA", 0);
    password = prefs.getString("password", DEFAULT_PASSWORD);
    password_AP = prefs.getString("password_AP", DEFAULT_PASSWORD);
    password_admin = prefs.getString("password_admin", DEFAULT_PASSWORD);
    webserverAlwaysStart = prefs.getBool("webAlwaysStart", false);
    NFCstart = prefs.getBool("NFCstart", false);
    onceOpenTime = prefs.getInt("onceOpenTime", DEFAULT_ONCEOPENTIME);
    onceOpenAngle = prefs.getInt("onceOpenAngle", DEFAULT_ONCEOPENANGLE);
    //Serial.println(password);
    //password = String("defaultpw").c_str();
    prefs.end();
}

void webserverInit() {
  webserverStatus = true;
  const char* ssid = "HpcSmartLock";  
  IPAddress local_ip(192, 168, 1, 1); 
  IPAddress gateway(192, 168, 1, 1); 
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAP(ssid, password_AP); 
  WiFi.softAPConfig(local_ip, gateway, subnet);

  server.on("/", webserver_root);
    // server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    //     String message;
    //     if (request->hasParam("message")) {
    //         message = request->getParam("message")->value();
    //     } else {
    //         message = "No message sent";
    //     }
    //     request->send(200, "text/plain", "Hello, GET: " + message);
    // });
  server.on("/"+password_admin+"/admin", HTTP_GET, webserver_admin);
  server.on("/"+password_admin+"/recieve", HTTP_POST, webserevr_receive);
  server.on("/"+password_admin+"/opendoor-api", HTTP_GET, webserver_opendoor_api);
  server.on("/"+password_admin+"/opendoor", HTTP_GET, webserevr_opendoor);
  server.begin();
  delay(200);
//  Serial.println("APrun");
//  Serial.println(password_admin);
}

void webserver_root() {
  String html = "<html> \
    <head> \
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, minimum-scale=0.5, maximum-scale=2.0, user-scalable=yes\" /> \
    <title>HpcSmartLock</title> \
    <script> \
    function redirectToUserPage() { \
    var userInput = document.getElementById(\"input\").value; \
    if (userInput) { \
    var url = userInput + \"/admin\"; \
    window.location.href = url; \
    } \
    } \
    </script> \
    </head> \
    <body> \
    <div style=\"text-align:center;\"> \
    <h1>煤专智能解锁</h1> \
    <h2>管理页面</h2> \
    <hr> \
    <label for=\"input\">请输入管理页面密码：</label> \
    <input type=\"text\" id=\"input\"> \
    <br><br> \
    <button onclick=\"redirectToUserPage()\">登录</button> \
    </div> \
    </body> \
    </html>";
  server.send(200, "text/html", html);
}
void webserver_admin() {
    // webserverAlwaysStart = prefs.getBool("webserverAlwaysStart", false);
    // NFCstart = prefs.getBool("NFCstart", false);
    // onceOpenTime = prefs.getInt("onceOpenTime", 2000);
  prefs.begin("config");
  String webserverAlwaysStart_true;
  String webserverAlwaysStart_false;
  if (prefs.getBool("webAlwaysStart", false)) {
    webserverAlwaysStart_true = "checked";
    webserverAlwaysStart_false = "";
  } else {
    webserverAlwaysStart_true = "";
    webserverAlwaysStart_false = "checked";
  }
  String NFCstart_true;
  String NFCstart_false;
  if (prefs.getBool("NFCstart", false)) {
    NFCstart_true = "checked";
    NFCstart_false = "";
  } else {
    NFCstart_true = "";
    NFCstart_false = "checked";
  }
  String html = "<html> \
    <head> \
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, minimum-scale=0.5, maximum-scale=2.0, user-scalable=yes\" /> \
    <title>HpcSmartLockAdmin</title> \
    </head> \
    <body> \
    <div style=\"text-align:center;\"> \
    <h1>煤专智能解锁</h1> \
    <h2>系统设置</h2> \
    <hr> \
    \
    <form action=\"recieve\" method=\"post\"> \
    <h3>解锁设置</h3> \
    <labl for=\"password_unlock\">解锁密码(请勿输入特殊字符)：</label> \
    <input type=\"text\" id=\"password_unlock\" name=\"password_unlock\" maxlength=\"12\" value=\"" + prefs.getString("password", DEFAULT_PASSWORD) + "\" required> \
    <br> \
    <label for=\"onceOpenTime\">单次解锁时间（毫秒）：</label> \
    <input type=\"text\" id=\"onceOpenTime\" name=\"onceOpenTime\" oninput=\"value=value.replace(/\D/g,'')\" maxlength=\"5\" required value=\"" + prefs.getInt("onceOpenTime", 2000) + "\"> \
    <br> \
    <label for=\"onceOpenAngle\">舵机旋转角度（0-180）：</label> \
    <input type=\"text\" id=\"onceOpenAngle\" name=\"onceOpenAngle\" oninput=\"value=value.replace(/\D/g,'');if(value>180)value=180;if(value<0)value=0\" maxlength=\"3\" required value=\"" + prefs.getInt("onceOpenAngle", DEFAULT_ONCEOPENANGLE) + "\"> \
    <br> \
    \
    <h3>管理热点设置</h3> \
    <label>开机自启动管理热点:</label> \
    <input type=\"radio\" id=\"webserverAlwaysStart_true\" name=\"webserverAlwaysStart\" value=\"true\" " +webserverAlwaysStart_true+ "> \
    <label for=\"webserverAlwaysStart_true\">启动</label> \
    <input type=\"radio\" id=\"webserverAlwaysStart_false\" name=\"webserverAlwaysStart\" value=\"false\" " +webserverAlwaysStart_false+ "> \
    <label for=\"webserverAlwaysStart_false\">关闭</label> \
    <br> \
    \
    <label for=\"password_adminAP\">管理热点密码：</label> \
    <input type=\"text\" id=\"password_AP\" name=\"password_AP\" maxlength=\"12\" required value=\"" +prefs.getString("password_AP", DEFAULT_PASSWORD)+ "\"> \
    <br> \
    \
    <label for=\"password_adminWeb\">管理页面密码：</label> \
    <input type=\"text\" id=\"password_admin\" name=\"password_admin\" maxlength=\"12\" required value=\"" +prefs.getString("password_admin", DEFAULT_PASSWORD)+ "\"> \
    <br> \
    \
    <h3>NFC设置</h3> \
    <label>NFC功能开关:</label> \
    <input type=\"radio\" id=\"NFCstart_true\" name=\"NFCstart\" value=\"true\" " +NFCstart_true+ "> \
    <label for=\"NFCstart_true\">启动</label> \
    <input type=\"radio\" id=\"NFCstart_false\" name=\"NFCstart\" value=\"false\" " +NFCstart_false+ "> \
    <label for=\"NFCstart_false\">关闭</label> \
    <br> \
    <label for=\"NFC_rebootTime\">NFC模块定时重启（秒）：</label> \
    <input type=\"text\" id=\"NFC_rebootTime\" name=\"NFC_rebootTime\" oninput=\"value=value.replace(/\D/g,'')\" maxlength=\"5\" required value=\"" +prefs.getInt("NFC_rebootTime", DEFAULT_NFC_REBOOTTIME)+ "\"> \
    <br> \
    \
    \
    <label for=\"NFC_id1\">NFCID1：</label> \
    <input type=\"text\" id=\"NFC_id1\" name=\"NFC_id1\" oninput=\"value=value.replace(/[\W]/g,'')\" maxlength=\"8\" minlength=\"8\" value=\"" +prefs.getString("NFC_id1", "")+ "\"> \
    <br> \
    <label for=\"NFC_id2\">NFCID2：</label> \
    <input type=\"text\" id=\"NFC_id2\" name=\"NFC_id2\" oninput=\"value=value.replace(/[\W]/g,'')\" maxlength=\"8\" minlength=\"8\" value=\"" +prefs.getString("NFC_id2", "")+ "\"> \
    <br> \
    <label for=\"NFC_id1\">NFCID3：</label> \
    <input type=\"text\" id=\"NFC_id3\" name=\"NFC_id3\"  oninput=\"value=value.replace(/[\W]/g,'')\" maxlength=\"8\" minlength=\"8\" value=\"" +prefs.getString("NFC_id3", "")+ "\"> \
    <br> \
    <label for=\"NFC_id1\">NFCID4：</label> \
    <input type=\"text\" id=\"NFC_id4\" name=\"NFC_id4\" oninput=\"value=value.replace(/[\W]/g,'')\" maxlength=\"8\" minlength=\"8\" value=\"" +prefs.getString("NFC_id4", "")+ "\"> \
    <br> \
    <label for=\"NFC_id1\">NFCID5：</label> \
    <input type=\"text\" id=\"NFC_id5\" name=\"NFC_id5\" oninput=\"value=value.replace(/[\W]/g,'')\" maxlength=\"8\" minlength=\"8\" value=\"" +prefs.getString("NFC_id5", "")+ "\"> \
    <br> \
    <label for=\"NFC_id1\">NFCID6：</label> \
    <input type=\"text\" id=\"NFC_id6\" name=\"NFC_id6\"  oninput=\"value=value.replace(/[\W]/g,'')\" maxlength=\"8\" minlength=\"8\" value=\"" +prefs.getString("NFC_id6", "")+ "\"> \
    <br> \
    \
    <br> \
    <input type=\"submit\" value=\"提交\"> \
    </form> \
    <hr> \
    <button onclick=\"window.location.href='opendoor'\" type=\"button\" id=\"add\">进入Oendoor页面</button> \
    <br><br> \
    <p>HpcOpenDoor v1.0</p> \
    </div> \
    \
    </body> \
    </html>";
  prefs.end();
  server.send(200, "text/html", html);
}
void webserevr_receive() {
  if(server.hasArg("password_unlock")) {
    prefs.begin("config");
    prefs.putString("password", server.arg("password_unlock"));
    prefs.putString("password_AP", server.arg("password_AP"));
    prefs.putString("password_admin", server.arg("password_admin"));
    if (server.arg("webserverAlwaysStart").compareTo("true")==0) {
      prefs.putBool("webAlwaysStart",true);
    } else {
      prefs.putBool("webAlwaysStart",false);
    }
    prefs.putInt("NFC_rebootTime", server.arg("NFC_rebootTime").toInt());
    prefs.putInt("onceOpenAngle", server.arg("onceOpenAngle").toInt());
    prefs.putInt("onceOpenTime", server.arg("onceOpenTime").toInt());
    if (server.arg("NFCstart").compareTo("true")==0) {
      prefs.putBool("NFCstart",true);
    } else {
      prefs.putBool("NFCstart",false);
    }
    prefs.putString("NFC_id1", server.arg("NFC_id1"));
    prefs.putString("NFC_id2", server.arg("NFC_id2"));
    prefs.putString("NFC_id3", server.arg("NFC_id3"));
    prefs.putString("NFC_id4", server.arg("NFC_id4"));
    prefs.putString("NFC_id5", server.arg("NFC_id5"));
    prefs.putString("NFC_id6", server.arg("NFC_id6"));
    prefs.end();

    String html = "<html> \
      <head> \
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, minimum-scale=0.5, maximum-scale=2.0, user-scalable=yes\" /> \
      <title>HpcSmartLockAdmin</title> \
      </head> \
      <body> \
      <div style=\"text-align:center;\"> \
      <h1>修改成功</h1> \
      <h2>设置将在重启后生效</h2> \
      <button onclick=\"window.location.href='admin'\" type=\"button\" id=\"add\">返回</button> \
      </div> \
      </body> \
      </html>";
      server.send(200, "text/html", html);
  } else {
      server.send(200, "text/html", "ERROR");
  }

}
void webserevr_opendoor() {
  String html = "<html> \
    <head> \
    <meta charset=\"UTF-8\"> \
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> \
    <title>HpcOpendoor</title> \
    <style> \
    body { \
    display: flex; \
    flex-direction: column; \
    align-items: center; \
    justify-content: center; \
    height: 100vh; \
    margin: 0; \
    } \
    #description { \
    font-size: 18px; \
    margin-bottom: 20px; \
    } \
    #opendoorBtn { \
    font-size: 36px; \
    padding: 20px 30px; \
    width: 300px; \
    height: 100px; \
    text-align: center; \
    line-height: 60px; \
    } \
    </style> \
    </head> \
    <body> \
    <button id=\"opendoorBtn\">OpenDoor</button> \
    <script> \
    var opendoorBtn = document.getElementById('opendoorBtn'); \
    opendoorBtn.addEventListener('click', function() { \
    var xhr = new XMLHttpRequest(); \
    xhr.open('GET', 'opendoor-api', true); \
    xhr.onreadystatechange = function() { \
    if (xhr.readyState == 4) { \
    if (xhr.status == 200) { \
    alert('请求成功！'); \
    } else { \
    alert('请求失败！'); \
    } \
    } \
    }; \
    xhr.send(); \
    }); \
    </script> \
    </body> \
    </html>";
  server.send(200, "text/html", html);
}
void webserver_opendoor_api() {
  server.send(200, "text/plain", "Success");
  opendoor();
}

void opendoor() {
  // if (!doorIsOpen) {
  //     xTaskCreate(
  //                   light_blink,          /* 任务函数 */
  //                   "opendoor",         /* 任务名 */
  //                   8*1024,            /* 任务栈大小，根据需要自行设置*/
  //                   NULL,              /* 参数，入参为空 */
  //                   1,
  //                   &TASK_HandleOne);  /* 任务句柄 */
  // }
  light_blink();
}
void light_blink() {
  // doorIsOpen = true;
  digitalWrite(LED_BUILTIN, HIGH);
  myServo.write(onceOpenAngle);
  delay(onceOpenTime);
  myServo.write(0);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  // vTaskDelete(TASK_HandleOne);
  // doorIsOpen = false;
  // digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  // delay(600);                      // wait for a second
  // digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  // delay(200);                      // wait for a second
}


std::string makeEncryptedPw() {
  MD5Builder md5;
  md5.begin();
  md5.add(String(salt)+password);
  md5.calculate();
  return md5.toString().c_str();
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();

      randomSeed(analogRead(5));
      salt = random(2147483646);
      String salt_string = String(salt);
      pCharacteristicSalt->setValue(salt_string.c_str());
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
/**
      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.print(salt);
        Serial.println("*********");
      }
      **/
      if (makeEncryptedPw().compare(value)==0) {
        opendoor();
      }
    }
};

class MyCallbacks_Webserver: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic_Webserver->getValue();
      if (makeEncryptedPw().compare(value)==0) {
        if(!webserverStatus) {
          webserverStatus = true;
          webserverInit();
        }
      }
    }
};

void BLEinit() {
  BLEDevice::init("HpcSmartLock");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristicSalt = pService->createCharacteristic(
                                         CHARACTERISTIC_SALT_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       );

  pCharacteristic_Webserver = pService->createCharacteristic(
                                         CHARACTERISTIC_WEBSERVER_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic_Webserver->setCallbacks(new MyCallbacks_Webserver());

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void hexCharacterStringToBytes(byte *byteArray, const char *hexString) {
    bool oddLength = strlen(hexString) & 1;

    byte currentByte = 0;
    byte byteIndex = 0;

    for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
    {
        bool oddCharIndex = charIndex & 1;

        if (oddLength)
        {
            // If the length is odd
            if (oddCharIndex)
            {
                // odd characters go in high nibble
                currentByte = nibble(hexString[charIndex]) << 4;
            }
            else
            {
                // Even characters go into low nibble
                currentByte |= nibble(hexString[charIndex]);
                byteArray[byteIndex++] = currentByte;
                currentByte = 0;
            }
        }
        else
        {
            // If the length is even
            if (!oddCharIndex)
            {
                // Odd characters go into the high nibble
                currentByte = nibble(hexString[charIndex]) << 4;
            }
            else
            {
                // Odd characters go into low nibble
                currentByte |= nibble(hexString[charIndex]);
                byteArray[byteIndex++] = currentByte;
                currentByte = 0;
            }
        }
    }
}
byte nibble(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0; // Not a valid hexadecimal character
}
void NFCinit() {
    mfrc522.PCD_Init();
    prefs.begin("config");
    String NFC_id1_string = prefs.getString("NFC_id1", "");
    String NFC_id2_string = prefs.getString("NFC_id2", "");
    String NFC_id3_string = prefs.getString("NFC_id3", "");
    String NFC_id4_string = prefs.getString("NFC_id4", "");
    String NFC_id5_string = prefs.getString("NFC_id5", "");
    String NFC_id6_string = prefs.getString("NFC_id6", "");
    NFC_rebootTime = prefs.getInt("NFC_rebootTime",DEFAULT_NFC_REBOOTTIME);
    //String NFC_id1_string = "BA171C51";
    prefs.end();
  if(NFC_id1_string.length()==8) {
    hexCharacterStringToBytes(NFC_id1, NFC_id1_string.c_str());
    useNFC_id1 = true;
  }
  if(NFC_id2_string.length()==8) {
    hexCharacterStringToBytes(NFC_id2, NFC_id2_string.c_str());
    useNFC_id2 = true;
  }
  if(NFC_id3_string.length()==8) {
    hexCharacterStringToBytes(NFC_id3, NFC_id3_string.c_str());
    useNFC_id3 = true;
  }
  if(NFC_id4_string.length()==8) {
    hexCharacterStringToBytes(NFC_id4, NFC_id4_string.c_str());
    useNFC_id4 = true;
  }
  if(NFC_id5_string.length()==8) {
    hexCharacterStringToBytes(NFC_id5, NFC_id5_string.c_str());
    useNFC_id5 = true;
  }
  if(NFC_id6_string.length()==8) {
    hexCharacterStringToBytes(NFC_id6, NFC_id6_string.c_str());
    useNFC_id6 = true;
  }
  ticker.attach(NFC_rebootTime, NFCreboot);
}
bool NFCBytesCheck(byte *uidByte, byte *savedByte) {
  if(uidByte[0]==savedByte[0] && uidByte[1]==savedByte[1] && uidByte[2]==savedByte[2] && uidByte[3]==savedByte[3]) {
    //Serial.println("checkTrue");
    return true;
  } else {
    return false;
  }
}
bool NFCcheck(byte *uidByte) {
  if(useNFC_id1){
    if(NFCBytesCheck(uidByte, NFC_id1)) {
      return true;
    }
  }
  if(useNFC_id2){
    if(NFCBytesCheck(uidByte, NFC_id2)) {
      return true;
    }
  }
  if(useNFC_id3){
    if(NFCBytesCheck(uidByte, NFC_id3)) {
      return true;
    }
  }
  if(useNFC_id4){
    if(NFCBytesCheck(uidByte, NFC_id4)) {
      return true;
    }
  }
  if(useNFC_id5){
    if(NFCBytesCheck(uidByte, NFC_id5)) {
      return true;
    }
  }
  if(useNFC_id6){
    if(NFCBytesCheck(uidByte, NFC_id6)) {
      return true;
    }
  }
  return false;
}
void NFCloop() {
  if ( !mfrc522.PICC_IsNewCardPresent()) {
		return;
	}
	// Select one of the cards.
	if ( !mfrc522.PICC_ReadCardSerial()) {
		return;
	}
  if (NFCcheck(mfrc522.uid.uidByte)) {
    opendoor();
  }
}
void NFCreboot() {
  digitalWrite(SS_PIN_SET, LOW);
 	delayMicroseconds(2);   //In order to perform a reset, the signal must be LOW for at least 100 ns => 2us is more than enough
	digitalWrite(SS_PIN_SET, HIGH);
 	delay(50);
  mfrc522.PCD_Init();
  delay(200);
}
void setup() {
  Serial.begin(115200);

  myServo.attach(servoGpio);
  myServo.write(0);

  loadConfig();
  BLEinit();
  pinMode(LED_BUILTIN, OUTPUT);
  if (webserverAlwaysStart) {
    webserverInit();
  }
  if (NFCstart) {
    NFCinit();
  }
};

void loop() {
  if(webserverStatus) {
      server.handleClient();
  }
  // put your main code here, to run repeatedly:
      // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(200); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }

  if (NFCstart) {
    NFCloop();
  }

};

