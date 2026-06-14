#include <Keypad.h>
#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define sensorPin 10
#define RelayPin 11
#define ButtonAir 12

int SensorValue;

boolean flag = true;
boolean airMode = false; // Menambahkan variabel untuk mengecek apakah mode air aktif
boolean airOtomatis = false;
boolean airManual = false;
boolean SensorNyala = false;

int WaktuAir;
String inputValue = "";

boolean scheduleEveryDay = false;
boolean scheduleEveryTwoDays = false;
DateTime lastRunTime;

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte colPins[ROWS] = {5, 4, 3, 2};
byte rowPins[COLS] = {9, 8, 7, 6};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2);

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  Serial.begin(9600);
  pinMode(ButtonAir, INPUT_PULLUP);
  pinMode(RelayPin, OUTPUT);
  pinMode(sensorPin, INPUT);

  lcd.init();
  lcd.backlight();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(_DATE), F(TIME_)));
   }

  DateTime now = rtc.now();

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Press Button");

  if (now.hour() >= 7) {
    lastRunTime = DateTime(now.year(), now.month(), now.day(), 7, 0, 0); // Set lastRunTime ke jam 7 pagi hari ini
  } else {
    lastRunTime = DateTime(now.year(), now.month(), now.day() - 2, 7, 0, 0); // Set lastRunTime ke dua hari lalu jam 7 pagi
  }

}

void loop() {
  DateTime now = rtc.now();

  if (digitalRead(ButtonAir) == LOW) {
    delay(5);
    flipflop();
    digitalWrite(RelayPin, LOW);
    scheduleEveryDay = false;
    scheduleEveryTwoDays = false;
  }

  if (airMode) {
    char KeypadAir = customKeypad.getKey();
    if (KeypadAir) {

      if (KeypadAir == 'A') {
        Serial.println("Manual");

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mode: Manual");

        airManual = true;
        airOtomatis = false;
      }
      else if (KeypadAir == 'B') {
        Serial.println("Otomatis");

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mode: Auto");

        airManual = false;
        airOtomatis = true;
      }
    }
  }

  if(airManual){
    airMode = false;
    //Serial.println("Masuk ke mode manual...");
    char KeypadAirManual = customKeypad.getKey();

    if(KeypadAirManual >= '0' && KeypadAirManual <= '9'){
      inputValue += KeypadAirManual; // Tambahkan angka ke inputValue
      Serial.println(inputValue);

      lcd.setCursor(0, 1);
      lcd.print("Menit: ");
      lcd.print(inputValue);

    }
    else if (KeypadAirManual == 'A') {
      Serial.println("Manual");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode: Manual");

      airManual = true;
      airOtomatis = false;
    }
    else if (KeypadAirManual == 'B') {
      Serial.println("Otomatis");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode: Auto");

      airManual = false;
      airOtomatis = true;
    }
    else if(KeypadAirManual == '#'){
      WaktuAir = inputValue.toInt(); // Ubah inputValue menjadi integer dan simpan ke WaktuAir
      Serial.print("Pompa nyala selama ");
      Serial.print(WaktuAir);
      Serial.println(" Menit");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pompa ON ");
      lcd.setCursor(0, 1);
      lcd.print(WaktuAir);
      lcd.print(" Menit");

      inputValue = ""; // Reset inputValue setelah disimpan

      long int WaktuAirMenit = WaktuAir * 10000;

      while(WaktuAirMenit>0){
        digitalWrite(RelayPin, HIGH);
        Serial.println(WaktuAirMenit);
        WaktuAirMenit--;
        if(WaktuAirMenit == 0){
          digitalWrite(RelayPin, LOW);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Pompa OFF");
        }
        else if(digitalRead(ButtonAir) == LOW){

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Pompa OFF");

          delay(5);
          flipflop();
          digitalWrite(RelayPin, LOW);
          WaktuAirMenit = 0;
        }
      }

      airManual = false;
      airMode = true;
    }
    else if(KeypadAirManual == '*'){
       if (inputValue.length() > 0) {
        inputValue = inputValue.substring(0, inputValue.length() - 1); // Hapus karakter terakhir
        //Serial.print("Angka setelah penghapusan: ");
        Serial.println(inputValue);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mode: Manual");
        lcd.setCursor(0, 1);
        lcd.print("Menit: ");
        lcd.print(inputValue);
      }
    }
    else if(KeypadAirManual == 'C'){
      //Serial.println("Mati pake sensor");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pompa ON");
      lcd.setCursor(0, 1);
      lcd.print("OFF by Sensor");

      airManual = false;
      airMode = true;
      SensorNyala = true;
      unsigned long relayStartTime = millis();

      while(SensorNyala){
        SensorValue = digitalRead(sensorPin);
        Serial.println(SensorValue);

        if(SensorValue == 1){
        digitalWrite(RelayPin, HIGH);
        }
        else if(SensorValue == 0){

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Pompa OFF");
          delay(3000);
          lcd.clear();
          lcd.setCursor(7, 0);
          lcd.print("ON");

          digitalWrite(RelayPin, LOW);
          SensorNyala = false;
        }
        if(millis() - relayStartTime>= 600000){

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Pompa OFF");
          delay(3000);
          lcd.clear();
          lcd.setCursor(7, 0);
          lcd.print("ON");

          digitalWrite(RelayPin, LOW);
          SensorNyala = false;
        }
      }
    }
  }

  if(airOtomatis){
    airMode = false;
    //Serial.println("Masuk ke mode otomatis...");
    char KeypadAirOtomatis = customKeypad.getKey();

    if (KeypadAirOtomatis == 'C') {
      Serial.println("Relay Nyala 1 Hari");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1x Sehari");

      scheduleEveryDay = true;
      scheduleEveryTwoDays = false;
    } 
    else if (KeypadAirOtomatis == 'D') {
      Serial.println("Relay Nyala 2 Hari");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("2x Sehari");

      scheduleEveryDay = false;
      scheduleEveryTwoDays = true;
    }
    else if (KeypadAirOtomatis == 'A') {
      Serial.println("Manual");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode: Manual");

      airManual = true;
      airOtomatis = false;
    }
    else if (KeypadAirOtomatis == 'B') {
      Serial.println("Otomatis");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode: Auto");

      airManual = false;
      airOtomatis = true;
    }
  }

  if (scheduleEveryDay) {

    TampilkanWaktu();

    if (now.hour() == 7 && now.minute() == 0) {
      Serial.println("Menyalakan relay (jadwal harian)");

      TampilkanWaktu();
      
      airOtomatis = false;
      airMode = true;
      SensorNyala = true;
      unsigned long relayStartTime = millis();

      while(SensorNyala){

        if(millis() - relayStartTime>= 600000){
        digitalWrite(RelayPin, LOW);
        SensorNyala = false;
        break;
        }

        SensorValue = digitalRead(sensorPin);
        Serial.println(SensorValue);
        TampilkanWaktu();

        if(SensorValue == 1){
        digitalWrite(RelayPin, HIGH);
        TampilkanWaktu();
        }
        else if(SensorValue == 0){
        digitalWrite(RelayPin, LOW);
        TampilkanWaktu();
        SensorNyala = false;
        }
      }
    }
  } 
  else if (scheduleEveryTwoDays) {
    
    TampilkanWaktu();

    if(now >= lastRunTime + TimeSpan(2, 0, 0, 0) && now.hour() == 7 && now.minute() == 0){
      Serial.println("Menyalakan relay (2x sehari)");

      TampilkanWaktu();
      
      airOtomatis = false;
      airMode = true;
      SensorNyala = true;
      lastRunTime = now;

      while(SensorNyala){
        SensorValue = digitalRead(sensorPin);
        //Serial.println(SensorValue);

        if(SensorValue == 1){
        digitalWrite(RelayPin, HIGH);
        TampilkanWaktu();
        }
        else if(SensorValue == 0){
        digitalWrite(RelayPin, LOW);
        TampilkanWaktu();
        SensorNyala = false;
        }
        else if(now.hour() == 7 && now.minute() == 10){
          digitalWrite(RelayPin, LOW);
          TampilkanWaktu();
          SensorNyala = false;
        }
      }
    }
  }
}

void flipflop() {
  flag = !flag;
  
  if (flag == HIGH) {
    Serial.println("Nyala");

    lcd.clear();
    lcd.setCursor(7, 0);
    lcd.print("ON");

    airMode = true; // Aktifkan mode air

  } else {
    Serial.println("Mati");

    lcd.clear();
    lcd.setCursor(7, 0);
    lcd.print("OFF");

    airMode = false; // Nonaktifkan mode air
  }
  
  while (digitalRead(ButtonAir) == LOW);
  delay(50);
}

void TampilkanWaktu(){

  DateTime now = rtc.now();

  Serial.print(now.hour(), DEC);
  Serial.print(" ");
  Serial.print(now.minute(), DEC);
  Serial.print(" ");
  Serial.println(now.second(), DEC);

  // lcd.setCursor(0, 0);
  // lcd.print(now.year(), DEC);
  // lcd.print('/');
  // lcd.print(now.month(), DEC);
  // lcd.print('/');
  // lcd.print(now.day(), DEC);
  // lcd.setCursor(0, 1);
  // lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
  // lcd.print(' ');
  // lcd.print(now.hour(), DEC);
  // lcd.print(':');
  // lcd.print(now.minute(), DEC);
  // lcd.print(':');
  // lcd.print(now.second(), DEC);

}
