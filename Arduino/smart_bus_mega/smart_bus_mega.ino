/**
   ----------------------------------------------------------------------------
   This is a MFRC522 library example; see https://github.com/miguelbalboa/rfid
   for further details and other examples.

   NOTE: The library file MFRC522.h has a lot of useful info. Please read it.

   Released into the public domain.
   ----------------------------------------------------------------------------
   This sample shows how to read and write data blocks on a MIFARE Classic PICC
   (= card/tag).

   BEWARE: Data will be written to the PICC, in sector #1 (blocks #4 to #7).


   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15

*/

#include <SPI.h>
//For RFID Library
#include <MFRC522.h>

//For Keypad Library
#include <Keypad.h>

//For Display Library
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN         5           // Configurable, see typical pin layout above
#define SS_PIN          53          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

bool reset = true;
bool rfid = false;
bool pinmode = false;
unsigned long rfidThread = 0;
unsigned long pinThread = 0;

int _row = 0;
/**
   Initialize.
*/

//Keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {A4, A5, A6, A7};

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
//char* password="4567";
//int currentposition=0;
//Keypad

String current_rfid = "";

String amount = "";
String pin = "";
bool go_back = true;
//Display
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display
//Display
void lcdReset() {
  reset = true;
  rfid = false;
  pinmode = false;
  _row = 0;
  amount = "";
  pin = "";
  current_rfid = "";  
  pinThread -= 10000;
  rfidThread -= 10000;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd. print("Sajha yatayat");
  lcd.setCursor(0, 1);
  lcd. print("Swipe RFID");
}
void lcdResult(String data) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd. print(data);
  delay(2000);
  lcdReset();
}
void lcdRfidApprove(String data) {
  reset = false;
  pinmode = false;
  rfid = true;
  _row = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd. print(data);
  lcd.setCursor(0, 1);
  lcd.setCursor(0, 1); lcd.print("Loading...");
  go_back = false; 
}

void lcdPinmodeApprove(String data) {
  reset = false;
  pinmode = true;
  rfid = false;
  _row = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd. print(data);
  lcd.setCursor(0, 1);
  lcd.setCursor(0, 1); lcd.print("Loading...");
  go_back = false;
  
}


void lcdHash() {
  reset = false;
  _row = 0;
  rfid = false;
  pinmode = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd. print("Set Amount");
}

void setup() {
  lcd.init();                      // initialize the lcd

  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();
  lcdReset();

  Serial.begin(115200); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("Scan a MIFARE Classic PICC to demonstrate read and write."));
  Serial.print(F("Using key (for A and B):"));
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();

  Serial.println(F("BEWARE: Data will be written to the PICC, in sector #1"));
}


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

/**
   Main loop.
*/
bool rfid_response = false;
bool pinmode_response = false;
String data;
int _mode = -1; // 0 check for rfid, 1 check pin
String _rfid = "";
String _pinmode = "";

void loop() {

  unsigned long currentMillis = millis();

  char customKey = customKeypad.getKey();
  if (Serial.available() > 0) {
    data = Serial.readStringUntil('\n');
    if (data[0] == '#') {
      _mode = delimit_string(data, ',', 0).toFloat();
      
      String message = delimit_string(data, ',', 2);

      Serial.println((String) "[Serial Read] : " + _mode + "\t" + rfid);

      Serial.println((String) "[Serial Test Gate] : " +  1 == 1);
      Serial.println((String) "[Serial Gate] : " + (_mode == 0) + "\t" + (rfid == true));
      if (_mode == 0 && rfid == true) {
        _rfid = delimit_string(data, ',', 1);
        Serial.println((String) "[RFID] : " + _rfid + "\t[RFID ==1] : " + (_rfid == 1));
        if (_rfid.toInt() == 1) {
          rfid_response = true;
          lcd.setCursor(0, 1); lcd.print("           ");
          lcd.setCursor(0, 1);
          lcd.print("Pin Here");
          rfidThread = currentMillis;
        } else {
          rfid_response = false;
          lcdResult(message);
        }
      }
      if(_mode == 1 && pinmode == true){
        _pinmode = delimit_string(data, ',', 1);
        if (_pinmode.toInt() == 1) {
          pinmode_response = true;
        } else {
          pinmode_response = false;
          lcdResult(message);
        }
      }
    }
  }

  if (customKey == 'B') {
    
    lcdReset();
  }

  if (currentMillis >= rfidThread + 10000) {
    if (reset == false && currentMillis > pinThread + 10000) {
      lcdReset();
    }
  } else {
    //    Serial.println((String) rfid + "\t" + rfid_response);
    if (rfid == true && rfid_response == true) {

      if (customKey == 'C') {
        pin = "";
        lcd.setCursor(0, 1); lcd.print("        ");
        _row = 0;
        rfidThread = currentMillis;
      }
      if (customKey == 'D') {
        lcdHash();
        pinThread = currentMillis;
        customKey = '\0';
      }
      if (isDigit(customKey)) {
        rfidThread = currentMillis;
        if (pin.length() == 0) {
          lcd.setCursor(0, 1); lcd.print("        ");
          pin += customKey;
          lcd.setCursor(_row, 1);
          lcd.print(customKey);
          lcd.setCursor(_row, 1);
          lcd.print("*");
          _row += 1;
        } else {
          if (pin.length() <= 3) {
            pin += customKey;
            lcd.setCursor(_row, 1);
            lcd.print(customKey);
            lcd.setCursor(_row, 1);
            lcd.print("*");
            _row += 1;
          }
        }

      }
    }

    //      if (customKey == '#') {
    //        // pin function here;
    //        Serial.println(customKey);
    //        lcdHash();
    //        pinThread = currentMillis;
    //      }
    //    }

  }
  if (pinmode && currentMillis < pinThread + 10000 ) {
    if (customKey == 'C') {
      amount = "";
      lcd.setCursor(0, 1); lcd.print("   ");
      _row = 0;
      pinThread = currentMillis;
    }
    if (customKey == 'D') {
      lcdPinmodeApprove("Checking Pin");
      Serial.println((String)"#1," + pin +"," + amount);
    }
    if(pinmode == true && pinmode_response == true){
      lcdResult("Amount Paid"); 
      pinThread -= 10000;
    }
    if (isDigit(customKey)) {
      pinThread = currentMillis;
      if (amount.length() == 0) {
        if (customKey != '0') {
          amount += customKey;
          lcd.setCursor(_row, 1);
          lcd.print(customKey);
          _row += 1;
        }
      } else {
        if (amount.length() <= 2) {
          amount += customKey;
          lcd.setCursor(_row, 1);
          lcd.print(customKey);
          _row += 1;
        }
      }
    }
  }
  
  if (rfid == true || pinmode == true) return;
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;
  //  Serial.println(mfrc522.PICC_IsNewCardPresent());
  // Show some details of the PICC (that is: the tag/card)
  //  Serial.print(F("Card UID:"));
  char str[32] = "";
  array_to_string(mfrc522.uid.uidByte, 4, str);
  //  Serial.print(str);
  if (current_rfid != str) {
    Serial.println((String)'#' + 0 + "," + str);
    lcdRfidApprove(str);
  }
  current_rfid = str;

  rfidThread = currentMillis;
}


void displayscreen()
{

  lcd.setCursor(0, 0);
  lcd.println("ENTER THE AMOUNT");
  lcd.setCursor(1 , 1);
}


void array_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0 ) & 0x0F;
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
  }
  buffer[len * 2] = '\0';
}
/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
