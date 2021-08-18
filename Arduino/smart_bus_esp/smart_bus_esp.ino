
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

/* this can be run with an emulated server on host:
        cd esp8266-core-root-dir
        cd tests/host
        make ../../libraries/ESP8266WebServer/examples/PostServer/PostServer
        bin/PostServer/PostServer
   then put your PC's IP address in SERVER_IP below, port 9080 (instead of default 80):
*/
#define SERVER_IP "192.168.43.219"
#define PORT "5000"
#ifndef STASSID
#define STASSID "DankWidow"
#define STAPSK  "#123456789"
#endif

int _mode = -1; // 0 check for rfid, 1 check pin
String rfid = "";
String pincode = "";
String amount = "";
void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

}

String data = "";
// 09 AE 64 40

// function to delimit the string
String delimit_string(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "-1";
}


void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    if (Serial.read() == '#') {
      data = Serial.readStringUntil('\n');
      _mode = delimit_string(data, ',', 0).toFloat();
      if (_mode == 0.0F) {
        rfid = delimit_string(data, ',', 1);
        rfid.trim();
        if ((WiFi.status() == WL_CONNECTED)) {
          WiFiClient client;
          HTTPClient http;
          String toSend = "http://" SERVER_IP ":" PORT "/api/user/";
          toSend += rfid;
          http.begin(client, toSend); //HTTP

          int httpResponseCode = http.GET();
          if (httpResponseCode > 0) {
            const String& payload = http.getString();
            Serial.println(payload);
          }
          else {
            Serial.println("#0,0,Request Timeout");
          }
          http.end();
        }
      }
      if (_mode == 1.0F) {
        
        Serial.println((String)"[DEBUG]" + data);
        rfid = delimit_string(data, ',', 1);
        rfid.trim();
        pincode = delimit_string(data, ',', 2);
        pincode.trim();
        amount = delimit_string(data, ',', 3);
        amount.trim();
        amount = amount == "" ? "0" : amount;
        Serial.println((String)"[DEBUG]" + rfid + '\t' + pincode + '\t' + amount);
        if ((WiFi.status() == WL_CONNECTED)) {
          WiFiClient client;
          HTTPClient http;

          Serial.print("[HTTP] begin...\n");
          // configure traged server and url
          String toSend = "http://" SERVER_IP ":" PORT "/api/user/";
          toSend += rfid;
          toSend += "/";
          toSend += pincode;
          toSend += "/";
          toSend += amount;

          http.begin(client, toSend); //HTTP

          http.begin(client, toSend); //HTTP

          int httpResponseCode = http.GET();
          if (httpResponseCode > 0) {
            const String& payload = http.getString();
            Serial.println(payload);
          }
          else {
            Serial.println("#1,0,Request Timeout");
          }
          http.end();

        }
//        Serial.println((String)"[Check Pincode] : " + rfid + "\t" + pincode + "\t" + amount);
      }
    }
  }
  //  Serial.println((String) _mode + "\t"+data);
}
