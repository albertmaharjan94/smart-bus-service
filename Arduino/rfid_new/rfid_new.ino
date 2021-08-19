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

unsigned long resetThread = 0;

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

//Display
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {
  lcd.init();                      // initialize the lcd

  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();
  resetAll();

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
int MODE_INIT = 0;
int MODE_RFID = 1;
int MODE_PIN = 2;
int MODE_AMOUNT = 3;
int MODE = MODE_INIT;
int THREAD_TIMER = 10000;
bool RFID_PROCESS = true;
int RFID_RESPONSE = 0;
bool PIN_PROCESS = true;
bool AMOUNT_PROCESS = true;
int AMOUNT_RESPONSE = 0;
bool PIN_PENDING = true;
int LCDSCREEN_LENGTH = 16;

String current_rfid = "";
String amount = "";
String pin = "";

void resetAll() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd. print("Sajha yatayat");
  lcd.setCursor(0, 1);
  lcd. print("Swipe RFID");

  resetThread -= THREAD_TIMER;
  MODE = MODE_INIT;
  RFID_PROCESS = true;
  RFID_RESPONSE = 0;
  PIN_PROCESS = true;
  AMOUNT_PROCESS = true;
  PIN_PENDING = true;
  AMOUNT_RESPONSE = 0;
  _row = 0;
  current_rfid = "";
  pin = "";
  amount = "";
}
void waitProcess(String title = "", String subtitle = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (title == "")
    lcd.print("Timeout");
  else
    lcd.print(title);

  lcd.setCursor(0, 1);
  if (subtitle == "")
    lcd.print("Reseting in 2s");
  else
    lcd.print(subtitle);

  delay(2000);
}
void clearLcd(int row = 0) {
  if (row == 0) {
    lcd.setCursor(0, 0);
  } else if (row == 1) {
    lcd.setCursor(0, 1);
  }
  for (int i = 0; i < LCDSCREEN_LENGTH; i++) {
    lcd.print(' ');
  }
}
void loop() {
  String data = "";
  unsigned long currentMillis = millis();
  if (Serial.available() > 0) {
    data = Serial.readStringUntil('\n');
    data.trim();
  }
  char customKey = customKeypad.getKey();

  if (currentMillis > resetThread + THREAD_TIMER) {
    if (MODE != MODE_INIT) {
      waitProcess();
      resetAll();
    }
  }

  if (customKey == 'B') {
    if (MODE != MODE_INIT) {
      waitProcess("CANCELED", "IN PROGRESS..");
      resetAll();
    }
  }

  if (MODE == MODE_INIT) {
    _modeRFID(currentMillis);
  }
  if (MODE == MODE_RFID) {
    if (RFID_PROCESS == true) {
      Serial.println((String)'#' + 0 + "," + current_rfid);
      _row = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(current_rfid);
      lcd.setCursor(0, 1); lcd.print("Loading...");
      RFID_PROCESS = false;
    }
    if (data != "" && data[0] == '#') {
      data.remove(0, 1);
      int _mode = delimit_string(data, ',', 0).toFloat();
      if (_mode == 0) {
        RFID_RESPONSE = delimit_string(data, ',', 1).toInt();
        String message = delimit_string(data, ',', 2);
        if (RFID_RESPONSE == 1) {
          MODE = MODE_AMOUNT;
          AMOUNT_PROCESS = true;
          lcd.setCursor(10, 0);
          lcd.print(message);
          clearLcd(1);
        } else {
          waitProcess("INVALID RFID", message);
          resetAll();
        }
      }
    }
  }

  if (MODE == MODE_PIN) {
    if (PIN_PROCESS == true) {
      _row = 0;
      resetThread = currentMillis;
      lcd.setCursor(0, 1);
      lcd.print("PIN HERE");
      PIN_PROCESS = false;
    }
    customKey = _modePin(currentMillis, customKey);
    if (data != "" && data[0] == '#') {
      data.remove(0, 1);
      int _mode = delimit_string(data, ',', 0).toFloat();

      if (_mode == 1) {
        AMOUNT_RESPONSE = delimit_string(data, ',', 1).toInt();
        if (AMOUNT_RESPONSE == 1) {
          long REMAINING = delimit_string(data, ',', 2).toInt();
          MODE = MODE_INIT;
          waitProcess("PAYMENT RECEIVED", (String) "BAL: " + REMAINING );
          resetAll();
        } else {
          String message = delimit_string(data, ',', 2);
          waitProcess("INVALID PAYMENT", message);
          resetAll();
        }
      }
    }
  }

  if (MODE == MODE_AMOUNT) {
    if (AMOUNT_PROCESS == true) {

      Serial.println(MODE);
      _row = 0;
      resetThread = currentMillis;
      lcd.setCursor(0, 1);
      lcd.print("AMOUNT HERE");
      AMOUNT_PROCESS = false;
    }
    _modeAmount(currentMillis, customKey, data);

  }


}
char _modeAmount(long currentMillis, char customKey, String data) {
  if (customKey == 'C') {
    amount = "";
    _row = 0;
    clearLcd(1);
    lcd.setCursor(0, 1);
    lcd.print("AMOUNT HERE");
    resetThread = currentMillis;
  }
  if (customKey == 'D') {
    if(amount.length() == 0) return;


    MODE = MODE_PIN;
    PIN_PROCESS = true;
    customKey = '\0';
  }

  if (isDigit(customKey)) {
    resetThread = currentMillis;

    if (amount.length() == 0) {
      if (customKey != '0') {
        clearLcd(1);
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
  return customKey;
}

char _modePin(long currentMillis, char customKey) {
  if (customKey == 'C') {
    pin = "";
    clearLcd(1);
    lcd.setCursor(0, 1);
    lcd.print("PIN HERE");
    _row = 0;
    resetThread = currentMillis;
  }
  if (customKey == 'D') {
    //    MODE = MODE_AMOUNT;
    //    AMOUNT_PROCESS = true;
    //    customKey = '\0';
    
    if(pin.length() == 0) return;
    if (PIN_PENDING == true) {
      PIN_PENDING = false;
      clearLcd(1);
      lcd.setCursor(0, 1); lcd.print("Loading...");
      Serial.println((String)"#1," + current_rfid + "," + pin + "," + amount);
    }
  }
  if (isDigit(customKey)) {
    resetThread = currentMillis;
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
  return customKey;
}

void _modeRFID(long currentMillis) {
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  char str[32] = "";
  array_to_string(mfrc522.uid.uidByte, 4, str);
  current_rfid = str;
  resetThread = currentMillis;
  MODE = MODE_RFID;
  RFID_PROCESS = true;
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
