#include <AltSoftSerial.h>
/**
   AltSoftSerial used 8 for tx and 9 for rx
   used for smooth serial communication with bluetooth HC-05 module
*/

AltSoftSerial BtSerial; // 8 TX | 9 RX

// logic gates
bool _mode = 0; // 0 - USB, 1 - Bluetooth
bool _serial_available = 0;

// pin defination
int key = 3;
int reverse_l = 10;
int reverse_r = 11;


int opt = 11;
int bt_state = 4;

// pin gate
bool _key = 0;
bool _reverse_l = 0;
bool _reverse_r = 0;
int _speed_1 = 0;

unsigned long previousMillis = 0;

// output serial monitor // usb - 0 , bluetooth - 1
void serialOut() {
  String out = ((String) "g," + _key + "," + _reverse_l + "," + _reverse_r + "," + _speed_1 + ",g#");
  if (_mode == 0) {
    Serial.println(out);
  }
  if (_mode == 1) {
    Serial.println(out);
    BtSerial.print(out);
  }
}

// read input string from serial // usb - 0 , bluetooth - 1
String makeInputString(int _mode) {
  /**
     Recommended input in the form of "1#1#1#100#10\n"
     It breaks a single line of string with \n
  */
  bool r = true;
  String buffer = "";
  char buf;
  if (_mode == 0) {
    Serial.println("USB on");
    while (r) {
      buf = (char)Serial.read();
      if (buf == '\n') {
        r = false;
        break;
      } else {
        if (isDigit(buf) || buf == '#' || buf == '-') {
          buffer.concat(String(buf));
        }
      }
    }
  } else {
    Serial.println("Bluetooth on");
    while (r) {
      buf = (char) BtSerial.read();
      if (buf == '\n') {
        r = false;
        break;
      } else {
        if (isDigit(buf) || buf == '#' || buf == '-') {
          buffer.concat(String(buf));
        }
      }
    }
  }
  return buffer;
}

// function to delimit the string
int delimit_string(String data, char separator, int index)
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
  return found > index ? data.substring(strIndex[0], strIndex[1]).toInt() : -1;
}


void setup() {
  // put your setup code here, to run once:
  delay(200);
  Serial.begin(9600);
  BtSerial.begin(9600);
  pinMode(key, OUTPUT);
  pinMode(reverse_l, OUTPUT);
  pinMode(reverse_r, OUTPUT);
  pinMode(opt, OUTPUT);
  pinMode(bt_state, INPUT);

  digitalWrite(key, HIGH);
  digitalWrite(reverse_l, HIGH);
  digitalWrite(reverse_r, HIGH);

}
void resetPins() {
  _key = 0;
  _reverse_r = 0;
  _reverse_l = 0;
  _speed_1 = 0;
}
void loop() {
  unsigned long currentMillis = millis();

  String serialInput;
  if (BtSerial.available() > 0) {
    _mode = 1;
    serialInput = makeInputString(1);
  }
  if (Serial.available() > 0) {
    _mode = 0;
    serialInput = makeInputString(0);
  }


  if (_mode == 1 && digitalRead(bt_state) == 0) {
    resetPins();
  }

  if (serialInput != "\0") {
    _key = delimit_string(serialInput, '#', 0);
    _reverse_l = delimit_string(serialInput, '#', 1);
    _reverse_r = delimit_string(serialInput, '#', 2);
    _speed_1 = delimit_string(serialInput, '#', 3);
  }

  //  debugLed();

  // custom threader (if needed)
  //  if(currentMillis - previousMillis >= 1000){
  //    previousMillis = currentMillis;
  //  }
  if (_key == 1) {
    digitalWrite(key, LOW);

    if (_reverse_l == 1) {
      digitalWrite(reverse_l, LOW);
    } else {
      digitalWrite(reverse_l, HIGH);
    }

    if (_reverse_r == 1) {
      digitalWrite(reverse_r, LOW);
    } else {
      digitalWrite(reverse_r, HIGH);
    }
  } else {
    digitalWrite(reverse_l, HIGH);
    digitalWrite(reverse_r, HIGH);
    digitalWrite(key, HIGH);
  }


  serialOut();
}
