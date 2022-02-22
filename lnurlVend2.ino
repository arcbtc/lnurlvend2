
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
using WebServerClass = WebServer;
fs::SPIFFSFS &FlashFS = SPIFFS;
#define FORMAT_ON_FAIL true

#include <Keypad.h>
#include <AutoConnect.h>
#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include "qrcoded.h"
#include "Bitcoin.h"
#include "esp_adc_cal.h"

#define PARAM_FILE "/elements.json"
#define KEY_FILE "/thekey.txt"

//Variables
String inputs;
String thePin;
String nosats;
String cntr = "0";
String lnurl;
String currency;
String lncurrency;
String key;
String preparedURL;
String baseURL;
String apPassword = "ToTheMoon1"; //default WiFi AP password
String masterKey;
String lnbitsServer;
String invoice;
String baseURLvend;
String secretvend;
String currencyvend;
String baseURLATM;
String secretATM;
String currencyATM;
String lnurlATMMS;
String dataIn = "0";
String amountToShow = "0.00";
String noSats = "0";
String qrData;
String dataId;
String addressNo;
String pinToShow;
char menuItems[4][12] = {"LNvend", "LNURLvend", "OnChain", "LNURLATM"};
int menuItemCheck[4] = {0, 0, 0, 0};
String selection;
int menuItemNo = 0;
int randomPin;
int calNum = 1;
int sumFlag = 0;
int converted = 0;
String key_val;
bool onchainCheck = false;
bool lnCheck = false;
bool lnurlCheck = false;
bool unConfirmed = true;
bool selected = false;
bool lnurlCheckvend = false;
bool lnurlCheckATM = false;
String lnurlVendProdOne;
String lnurlVendProdOneAmount;
String lnurlVendProdTwo;
String lnurlVendProdTwoAmount;
String lnurlVendProdThree;
String lnurlVendProdThreeAmount;

//Custom access point pages
static const char PAGE_ELEMENTS[] PROGMEM = R"(
{
  "uri": "/vendconfig",
  "title": "vend Options",
  "menu": true,
  "element": [
    {
      "name": "text",
      "type": "ACText",
      "value": "bitcoinvend options",
      "style": "font-family:Arial;font-size:16px;font-weight:400;color:#191970;margin-botom:15px;"
    },
    {
      "name": "password",
      "type": "ACInput",
      "label": "Password for vend AP WiFi",
      "value": "ToTheMoon1"
    },
    {
      "name": "lnurlvend",
      "type": "ACInput",
      "label": "LNURLvend String"
    },
    {
      "name": "lnurlvendprodone",
      "type": "ACInput",
      "label": "Product One String"
    },
    {
      "name": "lnurlvendprodtwo",
      "type": "ACInput",
      "label": "Product Two String"
    },
    {
      "name": "lnurlvendprobthree",
      "type": "ACInput",
      "label": "Product Three String"
    },
    {
      "name": "load",
      "type": "ACSubmit",
      "value": "Load",
      "uri": "/vendconfig"
    },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/save"
    },
    {
      "name": "adjust_width",
      "type": "ACElement",
      "value": "<script type='text/javascript'>window.onload=function(){var t=document.querySelectorAll('input[]');for(i=0;i<t.length;i++){var e=t[i].getAttribute('placeholder');e&&t[i].setAttribute('size',e.length*.8)}};</script>"
    }
  ]
 }
)";

static const char PAGE_SAVE[] PROGMEM = R"(
{
  "uri": "/save",
  "title": "Elements",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "format": "Elements have been saved to %s",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "validated",
      "type": "ACText",
      "style": "color:red"
    },
    {
      "name": "echo",
      "type": "ACText",
      "style": "font-family:monospace;font-size:small;white-space:pre;"
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/vendconfig"
    }
  ]
}
)";

TFT_eSPI tft = TFT_eSPI();
SHA256 h;

//////////////KEYPAD///////////////////

const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[rows] = {12, 14, 27, 26};
byte colPins[cols] = {25, 33, 32};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
int checker = 0;
char maxdig[20];

//////////////MAIN///////////////////

void setup()
{
  Serial.begin(115200);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  //Load screen
  tft.init();
  tft.invertDisplay(false);
  tft.setRotation(1);
  logo();

  //Load buttons
  h.begin();
  pinMode(3, OUTPUT); 
  pinMode(22, OUTPUT); 
  pinMode(15, OUTPUT); 
  pinMode(2, OUTPUT); 
  pinMode(4, OUTPUT); 
  
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);
  //Get the saved details and store in global variables
  File paramFile = FlashFS.open(PARAM_FILE, "r");
  if (paramFile)
  {
    StaticJsonDocument<2500> doc;
    DeserializationError error = deserializeJson(doc, paramFile.readString());

    JsonObject passRoot = doc[0];
    const char *apPasswordChar = passRoot["value"];
    const char *apNameChar = passRoot["name"];
    if (String(apPasswordChar) != "" && String(apNameChar) == "password")
    {
      apPassword = apPasswordChar;
    }

    JsonObject lnurlvendRoot = doc[1];
    const char *lnurlvendChar = lnurlvendRoot["value"];
    String lnurlvend = lnurlvendChar;
    baseURLvend = getValue(lnurlvend, ',', 0);
    secretvend = getValue(lnurlvend, ',', 1);
    currencyvend = getValue(lnurlvend, ',', 2);

    JsonObject lnurlvendRoot = doc[2];
    const char *lnurlVendProdOneChar = lnurlvendRoot["value"];
    String lnurlVendProdOneStr = lnurlVendProdOneChar;
    lnurlVendProdOne = getValue(lnurlVendProdOneStr, ',', 0);
    lnurlVendProdOneAmount = getValue(lnurlVendProdOneStr, ',', 1);

    JsonObject lnurlvendRoot = doc[2];
    const char *lnurlVendProdTwoChar = lnurlvendRoot["value"];
    String lnurlVendProdTwoStr = lnurlVendProdTwoChar;
    lnurlVendProdTwo = getValue(lnurlVendProdTwoStr, ',', 0);
    lnurlVendProdTwoAmount = getValue(lnurlVendProdTwoStr, ',', 1);

    JsonObject lnurlvendRoot = doc[2];
    const char *lnurlVendProdThreeChar = lnurlvendRoot["value"];
    String lnurlVendProdThreeStr = lnurlVendProdThreeChar;
    lnurlVendProdThree = getValue(lnurlVendProdThreeStr, ',', 0);
    lnurlVendProdThreeAmount = getValue(lnurlVendProdThreeStr, ',', 1);
  }
  paramFile.close();

  //Handle access point traffic
  server.on("/", []() {
    String content = "<h1>bitcoinvend</br>Free open-source bitcoin vend</h1>";
    content += AUTOCONNECT_LINK(COG_24);
    server.send(200, "text/html", content);
  });

  elementsAux.load(FPSTR(PAGE_ELEMENTS));
  elementsAux.on([](AutoConnectAux &aux, PageArgument &arg) {
    File param = FlashFS.open(PARAM_FILE, "r");
    if (param)
    {
      aux.loadElement(param, {"password", "lnurlvend"});
      param.close();
    }
    if (portal.where() == "/vendconfig")
    {
      File param = FlashFS.open(PARAM_FILE, "r");
      if (param)
      {
        aux.loadElement(param, {"password", "lnurlvend"});
        param.close();
      }
    }
    return String();
  });

  saveAux.load(FPSTR(PAGE_SAVE));
  saveAux.on([](AutoConnectAux &aux, PageArgument &arg) {
    aux["caption"].value = PARAM_FILE;
    File param = FlashFS.open(PARAM_FILE, "w");
    if (param)
    {
      // Save as a loadable set for parameters.
      elementsAux.saveElement(param, {"password", "lnurlvend"});
      param.close();
      // Read the saved elements again to display.
      param = FlashFS.open(PARAM_FILE, "r");
      aux["echo"].value = param.readString();
      param.close();
    }
    else
    {
      aux["echo"].value = "Filesystem failed to open.";
    }
    return String();
  });

  config.auth = AC_AUTH_BASIC;
  config.authScope = AC_AUTHSCOPE_AUX;
  config.ticker = true;
  config.autoReconnect = true;
  config.apid = "vend-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  config.psk = apPassword;
  config.menuItems = AC_MENUITEM_CONFIGNEW | AC_MENUITEM_OPENSSIDS | AC_MENUITEM_RESET;
  config.reconnectInterval = 1;
  config.title = "bitcoinvend";
  int timer = 0;

  //Give few seconds to trigger portal
  while (timer < 2000)
  {
    char key = keypad.getKey();
    if (key != NO_KEY)
    {
      portalLaunch();
      config.immediateStart = true;
      portal.join({elementsAux, saveAux});
      portal.config(config);
      portal.begin();
      while (true)
      {
        portal.handleClient();
      }
    }
    timer = timer + 200;
    delay(200);
  }
  if (baseURLvend.length() < 1)
  {
    portal.join({elementsAux, saveAux});
    config.autoRise = false;
    portal.config(config);
    portal.begin();
  }
}

void loop() {
  wakeUpScreen();
  unsigned long check = millis();
  bool cntr = false;
  selectProduct();
  while (cntr == false){
   char key = keypad.getKey();
   if (key != NO_KEY){
     virtkey = String(key);
       if (virtkey == "1"){
          cntr = true;
          selection = virtkey;
          amount = prodOneAmount * 100;
       }
       else if (virtkey == "2"){
          cntr = true;
          selection = virtkey;
          amount = prodTwoAmount * 100;
       }
       else if (virtkey == "3"){
          cntr = true;
          selection = virtkey;
          amount = prodThreeAmount * 100;
       }
      else if (virtkey == "*"){
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_WHITE);
        key_val = "";
        inputs = "";  
        nosats = "";
        virtkey = "";
        cntr = true;
      }  

     }
    if (millis()-check>30000){   
     tft.setFreeFont(SMALLFONT);
     tft.fillScreen(TFT_BLACK);
     tft.setCursor(0, 55);
     tft.setTextColor(TFT_RED, TFT_BLACK);
     tft.print("Sleeping...");
     delay(3000);
     gotoSleep();
    }
  }
  makeLNURL();
  qrShowCode();
  inputs = "";
  cntr = false;
  int pinAttempts = 0;
  while (cntr == false){
    
   char key = keypad.getKey();
   if (key != NO_KEY){
     virtkey = String(key);
       if (virtkey == "*"){
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_WHITE);
        key_val = "";
        inputs = "";  
        nosats = "";
        virtkey = "";
        cntr = true;
      }
      showPin();
    }
    if(inputs.length() == 4 && inputs.toInt() == randomPin){
      if(selection == "1"){
        digitalWrite(15, HIGH);
        delay(2500);
        digitalWrite(15, LOW);
        cntr = true;
      }
      if(selection == "2"){
        digitalWrite(2, HIGH);
        delay(2500);
        digitalWrite(2, LOW);
        cntr = true;
      }
      if(selection == "3"){
        digitalWrite(4, HIGH);
        delay(2500);
        digitalWrite(4, LOW);
        cntr = true;
      }
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_WHITE);
        key_val = "";
        inputs = "";  
        nosats = "";
        virtkey = "";
        cntr = true;
    }
    else if (inputs.length() == 4 && inputs.toInt() != randomPin){
        tft.setFreeFont(MIDFONT);
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 55);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.print(" Wrong Pin");
        key_val = "";
        inputs = "";  
        nosats = "";
        virtkey = "";
        pinAttempts ++;
        if (pinAttempts > 2){
          tft.setFreeFont(SMALLFONT);
          tft.fillScreen(TFT_BLACK);
          tft.setCursor(0, 55);
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.println("   Too many");
          tft.print("   attempts");
          cntr = true;
          delay(3000);
        }
        delay(2000);
        showPin();
    }
  }
}

///////////DISPLAY///////////////
void portalLaunch()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_PURPLE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(15, 50);
  tft.println("AP LAUNCHED");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 100);
  tft.setTextSize(1);
  tft.println("WHEN FINISHED RESET");
}

void wakeUpScreen(){
  digitalWrite(22, HIGH);
  delay(500);
  digitalWrite(3, HIGH);
  tft.begin();
  tft.setRotation(1);
  logo();
  delay(2000);
}

void qrShowCode(){
  tft.fillScreen(TFT_WHITE);
  lnurl.toUpperCase();
  const char* lnurlChar = lnurl.c_str();
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  qrcode_initText(&qrcode, qrcodeData, 6, 0, lnurlChar);
    for (uint8_t y = 0; y < qrcode.size; y++) {

        // Each horizontal module
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if(qrcode_getModule(&qrcode, x, y)){       
                tft.fillRect(40+2*x, 10+2*y, 2, 2, TFT_BLACK);
            }
            else{
                tft.fillRect(40+2*x, 10+2*y, 2, 2, TFT_WHITE);
            }
        }
    }
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setFreeFont(TINYFONT);
  tft.setCursor(20, 110);
  tft.println("PAY AND ENTER PIN FROM RECEIPT ");
}

void selectProduct()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(0, 10);
  tft.print("1: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(lnurlVendProdOne);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("   " + currency);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println(lnurlVendProdOneAmount);
  tft.setCursor(0, 55);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("2: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(lnurlVendProdTwo);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("   " + currency);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println(lnurlVendProdTwoAmount);
  tft.setCursor(0, 100);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("3: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(lnurlVendProdThree);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("   " + currency);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println(lnurlVendProdThreeAmount);
}

void showPin(){
  inputs += virtkey;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(0, 40);
  tft.println(" PROOF PIN");
  tft.setCursor(22, 80);
  tft.setTextColor(TFT_RED, TFT_BLACK); 
  tft.setFreeFont(BIGFONT);
  tft.println(inputs);
  delay(100);
  virtkey = "";
}

void logo(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(MIDFONT);
  tft.setCursor(20,60);
  tft.print("LNURLVEND");

  tft.setTextColor(TFT_PURPLE, TFT_BLACK);
  tft.setFreeFont(TINYFONT);
  tft.setCursor(20,70);
  tft.print("Powered by LNbits");
}

void gotoSleep(){ 
  touchAttachInterrupt(T5, wakeUpScreen, 20);
  esp_sleep_enable_touchpad_wakeup();
  esp_deep_sleep_start();
}
void to_upper(char * arr){
  for (size_t i = 0; i < strlen(arr); i++)
  {
    if(arr[i] >= 'a' && arr[i] <= 'z'){
      arr[i] = arr[i] - 'a' + 'A';
    }
  }
}

//////////LNURL AND CRYPTO///////////////

void makeLNURL()
{
  randomPin = random(1000, 9999);
  byte nonce[8];
  for (int i = 0; i < 8; i++)
  {
    nonce[i] = random(256);
  }
  byte payload[51]; // 51 bytes is max one can get with xor-encryption
  if (selection == "LNURLvend")
  {
    size_t payload_len = xor_encrypt(payload, sizeof(payload), (uint8_t *)secretvend.c_str(), secretvend.length(), nonce, sizeof(nonce), randomPin, dataIn.toInt());
    preparedURL = baseURLvend + "?p=";
    preparedURL += toBase64(payload, payload_len, BASE64_URLSAFE | BASE64_NOPADDING);
  }
  else
  {
    size_t payload_len = xor_encrypt(payload, sizeof(payload), (uint8_t *)secretATM.c_str(), secretATM.length(), nonce, sizeof(nonce), randomPin, dataIn.toInt());
    preparedURL = baseURLATM + "?atm=1&p=";
    preparedURL += toBase64(payload, payload_len, BASE64_URLSAFE | BASE64_NOPADDING);
  }

  Serial.println(preparedURL);
  char Buf[200];
  preparedURL.toCharArray(Buf, 200);
  char *url = Buf;
  byte *data = (byte *)calloc(strlen(url) * 2, sizeof(byte));
  size_t len = 0;
  int res = convert_bits(data, &len, 5, (byte *)url, strlen(url), 8, 1);
  char *charLnurl = (char *)calloc(strlen(url) * 2, sizeof(byte));
  bech32_encode(charLnurl, "lnurl", data, len);
  to_upper(charLnurl);
  qrData = charLnurl;
  Serial.println(qrData);
}

int xor_encrypt(uint8_t *output, size_t outlen, uint8_t *key, size_t keylen, uint8_t *nonce, size_t nonce_len, uint64_t pin, uint64_t amount_in_cents)
{
  // check we have space for all the data:
  // <variant_byte><len|nonce><len|payload:{pin}{amount}><hmac>
  if (outlen < 2 + nonce_len + 1 + lenVarInt(pin) + 1 + lenVarInt(amount_in_cents) + 8)
  {
    return 0;
  }
  int cur = 0;
  output[cur] = 1; // variant: XOR encryption
  cur++;
  // nonce_len | nonce
  output[cur] = nonce_len;
  cur++;
  memcpy(output + cur, nonce, nonce_len);
  cur += nonce_len;
  // payload, unxored first - <pin><currency byte><amount>
  int payload_len = lenVarInt(pin) + 1 + lenVarInt(amount_in_cents);
  output[cur] = (uint8_t)payload_len;
  cur++;
  uint8_t *payload = output + cur;                                 // pointer to the start of the payload
  cur += writeVarInt(pin, output + cur, outlen - cur);             // pin code
  cur += writeVarInt(amount_in_cents, output + cur, outlen - cur); // amount
  cur++;
  // xor it with round key
  uint8_t hmacresult[32];
  SHA256 h;
  h.beginHMAC(key, keylen);
  h.write((uint8_t *)"Round secret:", 13);
  h.write(nonce, nonce_len);
  h.endHMAC(hmacresult);
  for (int i = 0; i < payload_len; i++)
  {
    payload[i] = payload[i] ^ hmacresult[i];
  }
  // add hmac to authenticate
  h.beginHMAC(key, keylen);
  h.write((uint8_t *)"Data:", 5);
  h.write(output, cur);
  h.endHMAC(hmacresult);
  memcpy(output + cur, hmacresult, 8);
  cur += 8;
  // return number of bytes written to the output
  return cur;
}
