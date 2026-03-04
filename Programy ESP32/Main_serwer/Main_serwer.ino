#include <Arduino.h>
//Biblioteki do DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

//Biblioteka do I2C
#include <Wire.h>
#include <Adafruit_VL53L1X.h>

//Biblioteka Modbus TCP/IP
#include <ModbusIP_ESP8266.h>        

#define SDA_PIN 21       // VL53L1 SDA
#define SCL_PIN 22       // VL53L1 SCL
#define ONE_WIRE_PIN 23  // DS18B20
#define MAX_PIN 32       //MAX4466
#define MAX_LED 26
#define ONE_WIRE_LED 25
#define VL_LED 27


#include <WiFi.h>

bool PokazOdleglosc = LOW;
bool PokazTemp = LOW;
bool PokazMinMaxTemp = LOW;
bool AlarmCzujniaTemp = LOW;

const char* ssid = "MikroTik";     // Nazwa sieci WiFi
const char* pass = "12345678";    // Haslo sieci WiFi

IPAddress local_IP(192,168,0,253);  // Adres IP ESP32
IPAddress gateway(192,168,0,1);      // IP rutera
IPAddress subnet(255,255,255,0);     // Maska sieci


// Modbus 
ModbusIP mb;    // obiekt Modbus TCP (port 502 domyślnie)

//Ustawienie Adresów Modbus
const uint16_t REG_TEMP_1 = 0;  // Adress czujnik temperatury 1  => w PLC to 40001 
const uint16_t REG_TEMP_2 = 1;  // Adress czujnik temperatury 2  => w PLC to 40002 
const uint16_t REG_TEMP_3 = 2;  // Adress czujnik temperatury 3  => w PLC to 40003 
const uint16_t REG_VL     = 3;

const uint16_t REG_Alarm_1 = 4;
const uint16_t REG_Alarm_2 = 5;
const uint16_t REG_Alarm_3 = 6;

const uint16_t REG_HMI_D1_ERROR = 7;
const uint16_t REG_HMI_D2_ERROR = 8;
const uint16_t REG_HMI_D3_ERROR = 10;

const uint16_t REG_MaxDS_1 = 11;
const uint16_t REG_MinDS_1 = 12;

const uint16_t REG_MaxDS_2 = 13;
const uint16_t REG_MinDS_2 = 14;

const uint16_t REG_MaxDS_3 = 15;
const uint16_t REG_MinDS_3 = 16;

const uint16_t REG_IleCzujnikow = 17;
const uint16_t REG_Alarm_VL = 18;
const uint16_t REG_IleDetali = 19;

const uint16_t REG_VLMaxPomiar  = 20;
const uint16_t REG_VLMinPomiar  = 21;
const uint16_t REG_MaxAnalowgowyOdczyt = 22;
const uint16_t REG_MaxOdczytFiltr = 23;
const uint16_t REG_PrzekroczenieWartosc = 24;
const uint16_t REG_MaxAlarm = 25;




//Czujnik VL53L1
uint16_t distance_mode = 1; //distance mode (1=short, 2=long).
int I2C_Speed = 400000; // Hz

Adafruit_VL53L1X vl = Adafruit_VL53L1X(); 

uint16_t V1budget = 100;  //Budget pomiarowy 
uint16_t V1SamplingTime = 150; // Co ile pomiar 
int16_t mm = 0;
bool Alarm_VL = LOW;
uint16_t poprzednie_mm = 0;
uint16_t IleDetali = 0;
uint16_t OdlegloscDetal = 0;
bool FlagaDetal = HIGH;
uint16_t MaxPomiar = 0;
uint16_t MinPomiar = 10000;

uint32_t VL_CzasAktualny = 0;
uint32_t VL_CzasPomoc = 0;
uint16_t VL_CzasPotwierdzenie = 500;




// Utworzenie obiektu OneWire i DallasTemperature
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

//Flaga pomocnicza do ponownego uruchomienia OneWire
bool Flaga_mm = HIGH;

//Adresy czujników DS18B20
int LiczbaDS18 = 0;

DeviceAddress Adresy[3]={
{0x28,0x20,0xC4,0xBC,0x00,0x00,0x00,0x16},
{0x28,0x85,0x64,0xBC,0x00,0x00,0x00,0x92},
{0x28,0xFF,0x64,0x1F,0x69,0xAB,0x7B,0xDA}
};

//Odczyt co ile z czujników
uint32_t DS_czas_pomiar = 500;  // pomiar temperatury co 500ms
uint32_t DS_czas_pomoc_pomiar = 0;

//Pomocnicze DC18B20
bool konwersjaTrwa = LOW;

//Temperatury Graniczne dla wszystkich czujników
int16_t TempZaWysoka = 80;
int16_t TempZaNiska = -10;

//Max i Min Temp dla czujników:
int16_t MaxDS_1 = -5600;
int16_t MinDS_1 = 12700;

int16_t MaxDS_2 = -5600;
int16_t MinDS_2 = 12700;

int16_t MaxDS_3 = -5600;
int16_t MinDS_3 = 12700;

struct Limit{
  int32_t pMax = -10000;
  int32_t pMin = 100000;
};

Limit limit[3];
  

//Alarmy granicznych Temp
bool Alarm_1 = LOW;
bool Alarm_2 = LOW;
bool Alarm_3 = LOW;

//Zmienne Temperatura
float DS_1 =  0.0;
float DS_2 =  0.0;
float DS_3 =  0.0;

int16_t DS_1x100 = 12701;
int16_t DS_2x100 = 12701;
int16_t DS_3x100 = 12701;

//HMI błąd czujnika

bool HMI_D1_ERROR = LOW;
bool HMI_D2_ERROR = LOW;
bool HMI_D3_ERROR = LOW;

//Zmienne Max4466

int16_t MaxAnalowgowyOdczyt = 0;
const uint16_t MaxProgWykrycia = 1800; 
int16_t MaxLiniaOdniesienia = 4096/2;
int16_t MaxOdczytFiltr = 0;
int32_t CzasMaxOdczyt = 0;
int32_t CzasMaxOdczytPomoc = 0;
uint8_t TrwanieDzwieku = 5;
bool MaxAlarm = LOW; //LOW
uint8_t PrzekroczenieWartosc = 0;

int32_t CzasZgloszenie = 0;
int32_t pCzasZgloszenie = 0;
int8_t IleOdczekac = 10;

uint32_t CzasAlarmMAX = 0;
uint32_t pCzasAlarmMAX = 0;

//Ledy:
  struct Led {
  bool StanLed = HIGH;
  bool StanLeD = HIGH;
  uint32_t Pomocled = 0;
  uint32_t mocLED = 0;
  uint32_t PomocledNara = 0;
  uint32_t LedDuble_millis = 0;
  uint32_t PomocledDuble = 0;
  int a =0;
  bool LedOnoFF = HIGH;
  uint32_t TimeWaitDuble = 300;
};

 Led leds[3];

uint32_t Led_millis = 0;
const uint32_t TimeWait = 200;  // Co ile miga
uint32_t NarastanieTime = 0;
const uint32_t TimeWaitNaras = 5; 
uint32_t LedDuble_millis = 0;
//bool LedOnoFF = HIGH;
uint32_t TimeWaitDuble = 300;


//Możliwe Stany Czujnikow / ledy

 enum StanCzujnikow {     
  DS18b20_NOK = 1 << 0,  // 0001
  VL_NOK      = 1 << 1,  // 0010
  MAX_NOK     = 1 << 2,   // 0100
};



// zmienne inne
int sprawdzam = 0;
bool Bylo1 = HIGH;
bool Bylo2 = HIGH;
bool Bylo3 = HIGH;
bool Bylo4 = HIGH;
bool Bylo5 = HIGH;
bool Bylo6 = HIGH;
bool Bylo7 = HIGH;
bool Bylo8 = HIGH;

void ResetLedow(){
    for(int i = 0; i < 3; i++){   
    leds[i].Pomocled = 0;
    leds[i].StanLed = HIGH;
    leds[i].StanLeD= HIGH;
    leds[i].mocLED = 0;
    leds[i].PomocledNara = 0;
    //leds[i].LedDuble_millis = 0;
    leds[i].PomocledDuble = 0;
    leds[i].a = 0;
    leds[i].LedOnoFF = HIGH;
    leds[i].TimeWaitDuble = 300;
    }
  Led_millis = 0;
  NarastanieTime = 0;
  LedDuble_millis = 0;
  TimeWaitDuble = 300;

    }
    //digitalWrite(MAX_LED, 0);
    //digitalWrite(ONE_WIRE_LED, 0);
    //digitalWrite(VL_LED, 0);
    //delay(100);


void ONE_WIRE_DS() {
  uint32_t DS_czas = millis();
  if (DS_czas - DS_czas_pomoc_pomiar >= DS_czas_pomiar && !konwersjaTrwa) {
      DS_czas_pomoc_pomiar = DS_czas;
      sensors.requestTemperatures();        // rozpocznij pomiar
      konwersjaTrwa = HIGH;
  }
  if (sensors.isConversionComplete() && konwersjaTrwa){ //Czy sensor jest gotowy na odczyt

    DS_1 = sensors.getTempC(Adresy[0]);   // pierwszy czujnik
    DS_2 = sensors.getTempC(Adresy[1]);    // drugi czujnik
    DS_3 = sensors.getTempC(Adresy[2]);    // trzeci czujnik

    DS_1x100 = DS_1 * 100; // Pomiary z czujników pomnożone przez 100
    DS_2x100 = DS_2 * 100;
    DS_3x100 = DS_3 * 100;
    konwersjaTrwa = LOW;

    if(PokazTemp == HIGH){
    Serial.printf("DS1: %.2f", DS_1);
    Serial.println("C");
    Serial.printf("DS2: %.2f", DS_2);
    Serial.println("C");
    Serial.printf("DS3: %.2f", DS_3);
    Serial.println("C");
      }
    }

    if(mm !=-1 && mm <= 10 && Flaga_mm){
      sensors.begin();
      LiczbaDS18 = sensors.getDeviceCount();
      Serial.printf("Wykryto: %d czujniki \n",LiczbaDS18);
      Flaga_mm = LOW;
    }
    else Flaga_mm = HIGH;
    
    if(PokazTemp == HIGH){
    Serial.printf("DS1: %.2f", DS_1);
    Serial.println("C");
    Serial.printf("DS2: %.2f", DS_2);
    Serial.println("C");
    Serial.printf("DS3: %.2f", DS_3);
    Serial.println("C");
   }
       
}

void ZapiszMinTemp(float odczyt, int16_t odczytX100,  int16_t *Min, int a,DeviceAddress Adres){

 if (*Min > odczytX100 && odczyt != -127.00){
      *Min = odczytX100;

        if(PokazMinMaxTemp) {
        if(*Min < limit[a].pMin){
          Serial.printf("Minimalna Temperatura czujnika %d : %d \n",Adres,HEX, *Min);
          if(odczyt != -127.00){
            limit[a].pMin = *Min;
          }
          else{
              limit[a].pMin = 0;
          }
          }
          
        }
      }
    }

void ZapiszMaxTemp(float odczyt, int16_t odczytX100, int16_t *Max, int a,DeviceAddress Adres){

  if (*Max < odczytX100 && odczyt != -127.00 && odczytX100 != 12701 && odczyt != 85.00){
      *Max = odczytX100;
      
      if(PokazMinMaxTemp){
        
      if(*Max > limit[a].pMax ){
        Serial.printf("Maksymalna Temperatura czujnika %d : %d \n",Adres, *Max);
        if(odczyt != -127.00){
          limit[a].pMax = *Max;
        }
        else{
          limit[a].pMax = 0;
        }
      }
     }  
    }
} 


void BladCzujnikaTempHMI(float czujnikpomiar, bool *FlagaDlaCzujnika){

    if ( czujnikpomiar == -127.00){
        *FlagaDlaCzujnika = HIGH;
    } 
    else {
      *FlagaDlaCzujnika = LOW;
    }
}
    //85.00 - brak konwersji domyśla wartość czujnika DS18b20
    //-127.000 - błąd komunikacji / brak czujnika

void AlarmTemp(DeviceAddress Adres, bool *StanAlarmu, float odczyt){

     if (sensors.hasAlarm(Adres) && odczyt != -127.00 && odczyt != +85.0){
      *StanAlarmu = HIGH;
      Serial.printf("%f \n",odczyt);
      if(AlarmCzujniaTemp){
        Serial.printf("%f \n",odczyt);
        Serial.printf("Alarm Czujnika: %d \n",Adres);
      } 
    }
     
   
    else *StanAlarmu = LOW;
  }


void DubleBlink(int LED, int idx){
   LedDuble_millis = millis();
  
  if (LedDuble_millis - leds[idx].PomocledDuble > leds[idx].TimeWaitDuble && leds[idx].a < 4) {
    leds[idx].PomocledDuble = LedDuble_millis;
    leds[idx].StanLed = !leds[idx].StanLed;
    digitalWrite(LED, leds[idx].StanLed);
    leds[idx].a = leds[idx].a + 1;
    leds[idx].TimeWaitDuble = 300;
  }
  else if (LedDuble_millis - leds[idx].PomocledDuble > leds[idx].TimeWaitDuble && leds[idx].a==4){
    leds[idx].PomocledDuble = LedDuble_millis;
    leds[idx].StanLed = HIGH;
    digitalWrite(LED, leds[idx].StanLed);
    leds[idx].TimeWaitDuble = 600;
    leds[idx].a=0;
  }
  //Serial.println(leds[idx].a);
 }
void NarastanieLED(int LED, int idx){

  NarastanieTime = millis();

  if(NarastanieTime - leds[idx].PomocledNara > TimeWaitNaras){
      leds[idx].PomocledNara = NarastanieTime;

    if (leds[idx].mocLED <= 255 && leds[idx].LedOnoFF){
        leds[idx].mocLED = 2 + leds[idx].mocLED;
        analogWrite(LED,leds[idx].mocLED);
        if (leds[idx].mocLED >= 256){
          leds[idx].LedOnoFF=!leds[idx].LedOnoFF;
        }
      }

      else if (!leds[idx].LedOnoFF){

        if (leds[idx].mocLED > 0){
        leds[idx].mocLED = leds[idx].mocLED - 2;
        analogWrite(LED,leds[idx].mocLED);
        if (leds[idx].mocLED <= 0){
          leds[idx].LedOnoFF=!leds[idx].LedOnoFF;
        }
      }
    }
    }
  }
void blinking_led(int LED, int idx) {

  Led_millis = millis();
  if (LedDuble_millis  - leds[idx].Pomocled > TimeWait) {
    leds[idx].Pomocled = LedDuble_millis ;

    leds[idx].StanLeD = !leds[idx].StanLeD;
    digitalWrite(LED, leds[idx].StanLeD);
  }

 }

void LedyError(){

  static uint8_t ostatniStan = 255; // cos spoza zakresu 0..7 na start
  bool ds_nok = LOW;
  bool v1_nok  = LOW;
  bool max_nok = LOW;
  uint8_t stan = 0;
  

  
    if (DS_1 == -127.00 || DS_2 == -127.00 || DS_3 == -127.00 ) ds_nok = HIGH;
    if (mm == -1) v1_nok = HIGH; 
    if(MaxAlarm) max_nok = HIGH; 

    if (ds_nok ) {
      stan |= DS18b20_NOK;  // ustaw bit 0
    }  
    if (v1_nok  ){
      stan |= VL_NOK;   // ustaw bit 1
    }        
    if (MaxAlarm  ){
      stan |= MAX_NOK;     // ustaw bit 2
    } 


 // restart przy zmianie stanu ledów
  if (stan != ostatniStan ) {
    //Serial.println("Restart diod");
    ResetLedow();
    ostatniStan = stan;

  }

 //Serial.println(stan);
    
  /*
  if(stan == 0b0001 && Bylo2 == HIGH && Bylo3 == HIGH && Bylo4 == HIGH && Bylo5 == HIGH && Bylo6 == HIGH && Bylo7 == HIGH && Bylo8 == HIGH){
    Bylo1 = LOW;
  }
  else if (stan == 0b0010 && Bylo1 == HIGH && Bylo3 == HIGH && Bylo4 == HIGH && Bylo5 == HIGH && Bylo6 == HIGH && Bylo7 == HIGH && Bylo8 == HIGH){
    Bylo2 = LOW;
  }
  else if (stan == 0b0100 && Bylo1 == HIGH && Bylo2 == HIGH && Bylo4 == HIGH && Bylo5 == HIGH && Bylo6 == HIGH && Bylo7 == HIGH && Bylo8 == HIGH){
    Bylo3 = LOW;
  }
  else if (stan == 0b0111 && Bylo1 == HIGH && Bylo2 == HIGH && Bylo3 == HIGH && Bylo5 == HIGH && Bylo6 == HIGH && Bylo7 == HIGH && Bylo8 == HIGH){
    Bylo4 = LOW;
  }
  else if (stan == 0b0110 && Bylo1 == HIGH && Bylo2 == HIGH && Bylo3 == HIGH && Bylo4 == HIGH && Bylo6 == HIGH && Bylo7 == HIGH && Bylo8 == HIGH){
    Bylo5 = LOW;
  }
  else if (stan == 0b0101 && Bylo1 == HIGH && Bylo2 == HIGH && Bylo3 == HIGH && Bylo4 == HIGH && Bylo5 == HIGH && Bylo7 == HIGH && Bylo8 == HIGH){
    Bylo6 = LOW;
  }
  else if (stan == 0b0011 && Bylo1 == HIGH && Bylo2 == HIGH && Bylo3 == HIGH && Bylo4 == HIGH && Bylo5 == HIGH && Bylo6 == HIGH && Bylo8 == HIGH){
    Bylo7 = LOW;
  }
  else if (stan == 0b0000 && Bylo1 == HIGH && Bylo2 == HIGH && Bylo3 == HIGH && Bylo4 == HIGH && Bylo5 == HIGH && Bylo6 == HIGH && Bylo7 == HIGH){
    Bylo8 = LOW;
  }
  else {
    if (Bylo1 == LOW || Bylo2 == LOW || Bylo3 == LOW || Bylo4 == LOW || Bylo5 == LOW || Bylo6 == LOW || Bylo7 == LOW || Bylo8 == LOW) {
    Serial.println("Restart");
    ResetLedow();   
    Bylo1 = HIGH;
    Bylo2 = HIGH;
    Bylo3 = HIGH; 
    Bylo4 = HIGH;
    Bylo5 = HIGH;  
    Bylo6 = HIGH;
    Bylo7 = HIGH;
    Bylo8 = HIGH;
    return;
    }

  }
   */ 
    switch (stan) {

      case DS18b20_NOK: 
        blinking_led(ONE_WIRE_LED, 1);
        DubleBlink(MAX_LED, 0);
        DubleBlink(VL_LED, 2);
        break;

      case VL_NOK: 
        DubleBlink(ONE_WIRE_LED, 1);
        blinking_led(VL_LED, 2);
        DubleBlink(MAX_LED, 0);
        break;

      case MAX_NOK: 
        DubleBlink(ONE_WIRE_LED, 1);
        DubleBlink(VL_LED, 2);
        blinking_led(MAX_LED, 0);;
        break;

      case DS18b20_NOK | VL_NOK:
        blinking_led(ONE_WIRE_LED, 1);
        blinking_led(VL_LED, 2);
        DubleBlink(MAX_LED, 0);
        break;

      case DS18b20_NOK | MAX_NOK:
        blinking_led(ONE_WIRE_LED, 1);
        DubleBlink(VL_LED, 2);
        blinking_led(MAX_LED, 0);
        break;

      case VL_NOK | MAX_NOK:
        DubleBlink(ONE_WIRE_LED, 1);
        blinking_led(VL_LED, 2);
        blinking_led(MAX_LED, 0);
        break;

      case DS18b20_NOK | VL_NOK | MAX_NOK:
      Serial.println("wszystkieNOK");
        blinking_led(ONE_WIRE_LED, 1);
        blinking_led(VL_LED, 2);
        blinking_led(MAX_LED, 0);
        break;

      default:
        DubleBlink(MAX_LED, 0);
        DubleBlink(ONE_WIRE_LED, 1);
        DubleBlink(VL_LED, 2);
        break;

      }
    }

/*
void LedyError(){

    if((DS_1 == -127.00 || DS_2 == -127.00 || DS_3 == -127.00) && mm != -1 && Bylo2 == HIGH && Bylo3 == HIGH && !MaxAlarm) { 
    blinking_led(ONE_WIRE_LED, 1);
    DubleBlink(MAX_LED, 0);
    DubleBlink(VL_LED, 2);
    Bylo1 = LOW;
  }
  else if (mm == -1 && !(DS_1 == -127.00 || DS_2 == -127.00 || DS_3 == -127.00) && Bylo1 == HIGH && Bylo3 == HIGH && !MaxAlarm){
    DubleBlink(ONE_WIRE_LED, 1);
    DubleBlink(MAX_LED, 0);
    blinking_led(VL_LED, 2);
    Bylo2 = LOW;
  }
  else if (mm == -1 && (DS_1 == -127.00 || DS_2 == -127.00 || DS_3 == -127.00) && Bylo2 == HIGH && Bylo1 == HIGH && !MaxAlarm){
    blinking_led(ONE_WIRE_LED, 1);
    DubleBlink(MAX_LED, 0);
    blinking_led(VL_LED, 2);
    Bylo3 = LOW;
  }
  else if(mm == -1 && !(DS_1 == -127.00 || DS_2 == -127.00 || DS_3 == -127.00) && Bylo2 == HIGH && Bylo1 == HIGH && !MaxAlarm){
    blinking_led(ONE_WIRE_LED, 1);
    DubleBlink(MAX_LED, 0);
    blinking_led(VL_LED, 2);

  }
  else {
  if(Bylo1 == LOW ){
    ResetLedow();
    Bylo1 = HIGH;
  }
  else if(Bylo2 == LOW ){
    ResetLedow();
    Bylo2 = HIGH;
    }
  else if (Bylo3 == LOW ){
    ResetLedow();
    Bylo3 = HIGH;
    }   
      DubleBlink(MAX_LED, 0);
      DubleBlink(ONE_WIRE_LED, 1);
      DubleBlink(VL_LED, 2);
    }

}
*/

void V1_Pomiar() {
    
  poprzednie_mm = mm;

  if (vl.dataReady()) {
    mm = vl.distance();          // odległość w mm
    vl.clearInterrupt();         // wyczyść flagę gotowości

    if (mm == -1) {

      if (PokazOdleglosc == HIGH) Serial.println("Błąd pomiaru");
      

      Alarm_VL = HIGH;
    } else {
      if (PokazOdleglosc == HIGH) Serial.printf("Odległość: %d mm\n", mm);
      
      
      Alarm_VL = LOW;
     }
    }

  //Liczenie Detali 
  if (abs(mm - poprzednie_mm) < 50 && abs(mm - poprzednie_mm) > 10 && FlagaDetal && mm != -1){
    VL_CzasAktualny = millis();
    if(VL_CzasAktualny - VL_CzasPomoc  >= 600 && abs(mm - poprzednie_mm) < 50){
      VL_CzasPomoc = VL_CzasAktualny;
      IleDetali ++;
      FlagaDetal = LOW;
      OdlegloscDetal = mm;

    if (PokazOdleglosc == HIGH) Serial.printf("Liczba Detali: %d \n" ,IleDetali);

      }
  }
  if (abs(mm - OdlegloscDetal) > 50 && mm != -1){
    FlagaDetal = HIGH;
  }
  
  //Max Odległość
  if ( mm >  MaxPomiar && mm != -1){
    MaxPomiar = mm;
    Serial.printf("Maksymalny Pomiar : %d \n" ,MaxPomiar);
  }
  //Min Odległość
  if (mm < MinPomiar && mm != -1 && MinPomiar != MaxPomiar){
    MinPomiar = mm;
    Serial.printf("Minimalny Pomiar: %d \n" ,MinPomiar);
  }





 }


void MAX4466_pomiar(){

  CzasMaxOdczyt = millis();
 
 if((CzasMaxOdczyt - CzasMaxOdczytPomoc) > TrwanieDzwieku ){
    CzasMaxOdczytPomoc = CzasMaxOdczyt;
    MaxAnalowgowyOdczyt = analogRead(MAX_PIN);
    MaxOdczytFiltr = abs(MaxAnalowgowyOdczyt - MaxLiniaOdniesienia );
    CzasZgloszenie = millis();
    
    if(MaxOdczytFiltr >= MaxProgWykrycia && CzasZgloszenie - pCzasZgloszenie >= IleOdczekac ){
      pCzasZgloszenie = CzasZgloszenie;
      PrzekroczenieWartosc = PrzekroczenieWartosc + 1;
      Serial.printf("Wykryto trzask o wartości: %d \n ",MaxOdczytFiltr);
    }
  }
    CzasAlarmMAX = millis();
  if(MaxAnalowgowyOdczyt == 0 && CzasAlarmMAX - pCzasAlarmMAX > 100) {
    MaxAlarm = HIGH;
    pCzasAlarmMAX =CzasAlarmMAX;
  }
  else MaxAlarm = LOW;
 }

void setup() {

  Serial.begin(115200);
  delay(1000);
  
  Wire.begin(SDA_PIN, SCL_PIN);  //Inicjalizacja I²C dla pinów 21 i 22
  Wire.setClock(I2C_Speed);      //Ustawienie prędkość I²C



  //Ustawienie statycznego IP 
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Blad konfiguracji statycznego IP");
  }
  WiFi.begin(ssid, pass);
  delay(1000);


    //Ustawienie Pinów
    pinMode(MAX_LED, OUTPUT);
    pinMode(ONE_WIRE_LED, OUTPUT);
    pinMode(VL_LED, OUTPUT);
    pinMode(ONE_WIRE_PIN, INPUT_PULLUP);

  while (WiFi.status() != 3) { 
    Serial.print("."); 
    NarastanieLED(MAX_LED, 0);
    NarastanieLED(ONE_WIRE_LED, 1);
    NarastanieLED(VL_LED, 2);
  }
    
   
  Serial.println("\nPołączono z siecią Wi-Fi");
  Serial.print("\nESP32 IP: "); 
  Serial.println(WiFi.localIP());
  
  // Start serwera Modbus
  mb.server();
  
  // Definicja Początkowych Wartość Rejestrów 

  mb.addHreg(REG_TEMP_1, 0);    // tworzy rejestr 40001 z początkową wartością zero
  mb.addHreg(REG_TEMP_2, 0);    // tworzy rejestr 40002 z początkową wartością zero
  mb.addHreg(REG_TEMP_3, 0);    // tworzy rejestr 40003 z początkową wartością zero
  mb.addHreg(REG_VL,     0);    // tworzy rejestr 40004 z początkową wartością zero
  mb.addHreg(REG_Alarm_1,0);    
  mb.addHreg(REG_Alarm_2,0);
  mb.addHreg(REG_Alarm_3,0);    
  mb.addHreg(REG_IleCzujnikow,0);
  mb.addHreg(REG_Alarm_VL,0);
  mb.addHreg(REG_IleDetali,0);
  mb.addHreg(REG_VLMaxPomiar,0);
  mb.addHreg(REG_VLMinPomiar,0);
  mb.addHreg(REG_MaxDS_1,0);
  mb.addHreg(REG_MinDS_1,0);
  mb.addHreg(REG_MaxDS_2,0);
  mb.addHreg(REG_MinDS_2,0);
  mb.addHreg(REG_MaxDS_3,0);
  mb.addHreg(REG_MinDS_3,0); 
  mb.addHreg(REG_HMI_D1_ERROR,0); 
  mb.addHreg(REG_HMI_D2_ERROR,0); 
  mb.addHreg(REG_HMI_D3_ERROR,0); 
  mb.addHreg(REG_MaxAnalowgowyOdczyt,0); 
  mb.addHreg(REG_MaxOdczytFiltr,0);
  mb.addHreg(REG_PrzekroczenieWartosc,0);
  mb.addHreg(REG_MaxAlarm,0); 
    //...



  //Konfiguracja czujnika LV53L1
  if (!vl.begin(0x29, &Wire)) { // Czy czujnik jest widoczny else Error
    Serial.println("Czujnik VL53L1X jest nie dostępny na I2C (0x29)");
    while (1) 
    delay(100);
  }
    vl.VL53L1X_SetDistanceMode(distance_mode); //distance mode (1=short, 2=long).
    vl.setTimingBudget(V1budget); // Im większy dokładniej stabilniej ale wolniej
    vl.VL53L1X_SetInterMeasurementInMs(V1SamplingTime); // Okres miedzy pomiarami
    vl.startRanging();   // start trybu ciągłego
    

  //Obsługa DS18
  sensors.begin();  // start magistrali OneWire
  sensors.setWaitForConversion(false); // tryb nieblokujący (płynna pętla)

  LiczbaDS18 = sensors.getDeviceCount();

  Serial.printf("Wykryto: %d czujniki \n",LiczbaDS18);

  // Ustawienie alarmów za wysokiej/niskiej temperatury
    sensors.setHighAlarmTemp(Adresy[0], TempZaWysoka);  
    sensors.setLowAlarmTemp(Adresy[0], TempZaNiska); 

    sensors.setHighAlarmTemp(Adresy[1], TempZaWysoka);  
    sensors.setLowAlarmTemp(Adresy[1], TempZaNiska);   

    sensors.setHighAlarmTemp(Adresy[2], TempZaWysoka);  
    sensors.setLowAlarmTemp(Adresy[2], TempZaNiska);   

 

  //Częstotliwość OneWire
  analogWriteFrequency(MAX_LED, 1000);
  analogWriteFrequency(ONE_WIRE_LED, 1000);
  analogWriteFrequency(VL_LED, 1000);

  //indeksowanie ledów
  for (int i = 0; i < 3; i++) {
    leds[i].Pomocled = 0;
    leds[i].StanLed = HIGH;
    leds[i].StanLeD= HIGH;
    leds[i].mocLED = 0;
    leds[i].PomocledNara = 0;
    leds[i].LedDuble_millis = 0;
    leds[i].PomocledDuble = 0;
    leds[i].a = 0;
    leds[i].LedOnoFF = HIGH;
    leds[i].TimeWaitDuble = 300;
  }

  // indeksowanie progów temp 
 for (int i = 0; i < LiczbaDS18 ; i++) {
    limit[i].pMax = -10000;
    limit[i].pMin = 100000;
 }
    //Ustawianie Max4466 
    pinMode(MAX_PIN, INPUT); 
    analogReadResolution(12); // Odczyt w zakresie 0–4095          
    analogSetAttenuation(ADC_11db);    // zakres do ~3.3 V

    //Wyznaczenie Lini Odniesienia Z przetwonika ADC odbierającego sygnał z MAX4466
    int LiczbaProbek = 500;
    long SumaProbek = 0;
    for(int i = 0; i < LiczbaProbek; i ++){
      SumaProbek = SumaProbek + analogRead(MAX_PIN);
      delay(2);
    }
    MaxLiniaOdniesienia = SumaProbek / LiczbaProbek;
    Serial.printf("Linia Odniesienia równa sie: %d \n", MaxLiniaOdniesienia);
  }   

void loop() {

  if(WiFi.status() != 3){

    WiFi.reconnect(); 
    NarastanieLED(MAX_LED, 0); 
    NarastanieLED(ONE_WIRE_LED, 1);
    NarastanieLED(VL_LED, 2);
    //Serial.print(".");
    //delay(200);
    Bylo1 = LOW;
    
  }
  else{
  LedyError();

  ONE_WIRE_DS();

  AlarmTemp(Adresy[0], &Alarm_1,DS_1);
  AlarmTemp(Adresy[1], &Alarm_2,DS_2);
  AlarmTemp(Adresy[2], &Alarm_3,DS_3);

  ZapiszMinTemp(DS_1, DS_1x100, &MinDS_1,0,Adresy[0]);
  ZapiszMinTemp(DS_2, DS_2x100, &MinDS_2,1,Adresy[1]);
  ZapiszMinTemp(DS_3, DS_3x100, &MinDS_3,2,Adresy[2]);

  ZapiszMaxTemp(DS_1, DS_1x100, &MaxDS_1, 0,Adresy[0]);
  ZapiszMaxTemp(DS_2, DS_2x100, &MaxDS_2, 1,Adresy[1]);
  ZapiszMaxTemp(DS_3, DS_3x100, &MaxDS_3, 2,Adresy[2]);
  

  BladCzujnikaTempHMI(DS_1,&HMI_D1_ERROR);
  BladCzujnikaTempHMI(DS_2,&HMI_D2_ERROR);
  BladCzujnikaTempHMI(DS_3,&HMI_D3_ERROR);
 

  V1_Pomiar();

  MAX4466_pomiar();

  //Odbieranie zaptań serwera i przypisywanie wartości do rejestrów
  mb.task();           

  //Rejestry DC18B20
  mb.Hreg(REG_TEMP_1 ,  DS_1x100); // do rejestru REG_TEMP przypisuje DS_1x100
  mb.Hreg(REG_TEMP_2 ,  DS_2x100);
  mb.Hreg(REG_TEMP_3 ,  DS_3x100);
  
  mb.Hreg(REG_Alarm_1 , Alarm_1);
  mb.Hreg(REG_Alarm_2 , Alarm_2);
  mb.Hreg(REG_Alarm_3 , Alarm_3);

  mb.Hreg(REG_HMI_D1_ERROR,HMI_D1_ERROR); 
  mb.Hreg(REG_HMI_D2_ERROR,HMI_D2_ERROR); 
  mb.Hreg(REG_HMI_D3_ERROR,HMI_D3_ERROR); 
  
  mb.Hreg(REG_MaxDS_1,MaxDS_1);
  mb.Hreg(REG_MinDS_1,MinDS_1);
  mb.Hreg(REG_MaxDS_2,MaxDS_2);
  mb.Hreg(REG_MinDS_2,MinDS_2);
  mb.Hreg(REG_MaxDS_3,MaxDS_3);
  mb.Hreg(REG_MinDS_3,MinDS_3);
  mb.Hreg(REG_IleCzujnikow , LiczbaDS18);

  //Rejestry LV53L1X
  mb.Hreg(REG_VL, mm);
  mb.Hreg(REG_Alarm_VL,Alarm_VL);
  mb.Hreg(REG_IleDetali,IleDetali);
  mb.Hreg(REG_VLMaxPomiar,MaxPomiar);
  mb.Hreg(REG_VLMinPomiar,MinPomiar);

  //Rejestry MAX4466
  mb.Hreg(REG_MaxAnalowgowyOdczyt,MaxAnalowgowyOdczyt); 
  mb.Hreg(REG_MaxOdczytFiltr,MaxOdczytFiltr);
  mb.Hreg(REG_PrzekroczenieWartosc,PrzekroczenieWartosc);
  mb.Hreg(REG_MaxAlarm,MaxAlarm); 

  }
}
