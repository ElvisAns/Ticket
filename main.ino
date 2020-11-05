#include <SPI.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "MFRC522.h"
#include "MFRC522Extended.h"
#include "KeyPad.h"
#include <EEPROM.h>
#define RST_PIN 9           // Configurable
#define SS_PIN 10          // Configurable
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define ledActive 4
#define ledLogin 3
#define ledDenied 2
#define buzzer 0
#define entrer 1

bool read_rfid = false;

const int numOfCards = 5;//get the len(cards)
byte cards[numOfCards][4] = {{0xE9, 0xEB, 0xF6, 0x62}, {0xEA, 0x53, 0xA6, 0x78}, {0xB0, 0x92, 0xEC, 0x06}, {0x06, 0xE4, 0xCD, 0xEF}}; // array of UIDs of rfid cards
byte cards_adresses[4] = {0x32, 0x37, 0x42, 0x47};
String utilisateur = "JOHN";
String codeModule = "MOD001";
int numCard[numOfCards];
String names[numOfCards] = {"ISIDORE", "YOMBE", "NGOMA", "DESTINEE", "ISRAEL"};
long sNumbers[numOfCards] = {23323524698, 23483166120, 17614623606, 6228205239, 642289516};

#define pass_length 7// mot d pass max, argent max qui peux etre contenue sur la carte = 99 999FC
String Data;
byte Data2[5] = {};
char MASTER[pass_length] = "ABC123";
byte data_count = 0, master_count = 0;
bool pass_is_good;
char customKey;// Character to hold key input
const byte ROWS = 4;// Constants for row and column sizes
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {// Array to represent keys on keypad
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {8, 7, 6, 5};
byte colPins[COLS] = {A0, A1, A2, A3};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//RFID W&R Settings
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;



void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC //while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  pinMode(ledDenied, OUTPUT);
  pinMode(ledActive, OUTPUT);
  pinMode(ledLogin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(entrer, INPUT);
  lcd.backlight();
  lcd.init();
  lcd.setCursor(0, 0);
  lcd.print("SOCIETE PROVINCI");
  lcd.setCursor(0, 1);
  lcd.print("ALE DE TRANSPORT");
  delay(2000);
  lcd.clear();
  mfrc522.PCD_Init(); // Init MFRC522 card
  // Prepare the security key for the read and write functions.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  //keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }
}
void loop() {
  if (read_rfid == false) {
    lcd.setCursor(3, 0);
    lcd.print("INSEREZ PIN");
    digitalWrite(ledLogin, HIGH);
    digitalWrite(ledActive, LOW);
    customKey = customKeypad.getKey();
    if (customKey) {
      Data += customKey;
      //Data.concat(customKey);
      lcd.setCursor(data_count + 3, 1);
      lcd.print("*");//lcd.print(Data[data_count]);
      data_count++;
    }
    if (data_count == pass_length - 1) {
      lcd.clear();
      if (Data.equals(MASTER)){ //str
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Acces Accordee");
        lcd.setCursor(0, 1);
        lcd.print(codeModule);
        lcd.setCursor(6, 1);
        lcd.print("-");
        lcd.setCursor(7, 1);
        lcd.print(utilisateur);
        read_rfid = true;
        delay(2000);
        lcd.clear();
        clearData();
      }
      else {
        lcd.clear();
        lcd.setCursor(0, 2);
        lcd.print("Acces Refusee");
        delay(500);
        read_rfid = false;
        delay(1000);
      }
      lcd.clear();
      clearData();
    }
  }
  else {//FULL CODE : START TO READ AND WRITE
    lcd.setCursor(0, 0);
    lcd.print("Bienvenu ");
    lcd.setCursor(9, 0);
    lcd.print(utilisateur);
    digitalWrite(ledActive, HIGH);
    digitalWrite(ledLogin, LOW);
    //lcd.clear();
    customKey = customKeypad.getKey();
    if (customKey) {
      if (data_count >= 5) {
        Serial.println(Data);
        Serial.print("Start reading");
        //lcd.clear();
        readRFID(Data);
        lcd.clear();
        clearData();
        //return;
      }
      else {
        Data += String(customKey);
        lcd.setCursor(data_count, 1);
        lcd.print((char)Data[data_count]);
        data_count++;
        delay(200);
      }
    }
  }
}
void readRFID(String MYDATA) { //lecture des cartes
  int j = -1;
  byte card_ID[4];
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    Data='\0';
    return;//got to start of loop if there is no card present
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
   Data='\0';
    return;
  }
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    card_ID[i] = mfrc522.uid.uidByte[i];
  }
  for (int i = 0; i < numOfCards; i++) {
    if (card_ID[0] == cards[i][0] && card_ID[1] == cards[i][1] && card_ID[2] == cards[i][2] && card_ID[3] == cards[i][3]) {
      j = i;
      process(cards_adresses[i], MYDATA);
    }
  }
  if (j == -1) {
    invalid();
  }
  delay(1000);
}
void invalid() {
  digitalWrite(ledDenied, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("Carte Invalide...");
  tone(buzzer, 100);
  delay(700);
  noTone(buzzer);
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  digitalWrite(ledDenied, LOW);
  Serial.println("Invalid card....");
  clearData();
}

void process(byte adress, String MYDATA) {
  // LA CARTE EST VALIDE, PROCEDER A LA TRANSCRIPTION D'ARGENT
  if (MYDATA.toInt() == 0 & MYDATA.toInt()%100 != 0) {
    Serial.println("Invalid money balance entered");
    Data='\0';
    return;
  }
  lcd.setCursor(0, 1);
  lcd.print("Processing...");
  delay(200);
  unsigned long balance = readbalance(adress);
  
  lcd.setCursor(0, 1);
  lcd.print("Bal:");
  lcd.setCursor(9, 1);

  lcd.print(String(balance) + " Frc");
  delay(1000);
  lcd.print("                                ");
  Serial.println("Old data : ");
  Serial.println(String(balance*100) + " Fc");


  //___________________________________
  Serial.println("Now you want to add");
  Serial.println(MYDATA + "Frc");
  //calculations
  unsigned long want;
  unsigned long newBal;
  unsigned long data2;

  want=(MYDATA.toInt()/100);

  newBal = want+balance;                             //preparation nouvelle balance
  Serial.println("Data to add :");
  Serial.println(" = " + String(want*100)+" Frc");
  Serial.println("New Balance to write  :");
  Serial.println(String(newBal*100)+" Frc");
  
  writebalance(adress,newBal);                                  //ecrire nouvelle balance
  delay(100);
  readnewBalance(adress);
  digitalWrite(ledLogin, HIGH);
  Data='\0';
  delay(1000);
  digitalWrite(ledLogin, LOW);
  clearData();
}





void clearData() {
  Data='\0';
}
void writebalance(byte adress,int newbalance) {
 EEPROM.write(adress,newbalance);
 delay(100);
}

void readnewBalance(byte adress) {
  
  int balanceNow = EEPROM.read(adress);

  lcd.setCursor(0, 1);
  lcd.print("New Bal:");
  lcd.setCursor(9, 1);
 
  lcd.print(balanceNow*100 + "Frc");
  Serial.print("New Balance : ");
  Serial.println(balanceNow*100 + "Frc");
  delay(1000);
}

unsigned long readbalance(byte adress) {

  int balanceNow = EEPROM.read(adress);

  Serial.println("Got previous balance");

  balanceNow = balanceNow * 1;

  return balanceNow;
}

