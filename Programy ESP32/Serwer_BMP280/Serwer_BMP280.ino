#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_BMP280.h>

//Biblioteka Modbus TCP/IP
#include <ModbusIP_ESP8266.h>        
#include <WiFi.h>

#define SDA 21
#define SCL 22

Adafruit_BMP280 bmp;

uint32_t CzasDane = 0;
uint16_t CoIlePomiar = 500;


float t_bmp = 0;
uint16_t p_hPa = 0;

int16_t t_bmpx100 = 0;

/*
float temp_aht = 0;
float hum_aht = 0;
*/

const char* ssid = "MikroTik";     // Nazwa sieci WiFi
const char* pass = "12345678";    // Haslo sieci WiFi


//Modbus
ModbusIP mb;    

//Statyczne IP
IPAddress local_IP(192,168,0,252);  // Adres IP ESP32
IPAddress gateway(192,168,0,1);      // IP rutera
IPAddress subnet(255,255,255,0);     // Maska sieci

//Ustawienie Adresów Modbus
const uint16_t REG_bmp_temp = 0;  // Adress czujnik temperatury 1  => w PLC to 40001 
const uint16_t REG_bmp_pres = 1;  // Adress czujnik temperatury 2  => w PLC to 40002 

void PomiarZczujnikow() {
  uint32_t czas = millis();
  if (czas - CzasDane > CoIlePomiar) {
        CzasDane = millis();
 
  t_bmp = bmp.readTemperature();
  p_hPa = bmp.readPressure()/10;

  t_bmpx100 = t_bmp * 100;
  

  Serial.printf("Temperatura: %d \n", t_bmpx100);
  Serial.printf("Ciśnienie: %d  \n", p_hPa);
  
  }
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA, SCL);  // I2C do Ekranu/AHT20/BMP280
  Wire.setClock(100000); //odświeżanie I2C

//Ustawienie statycznego IP 
if (!WiFi.config(local_IP, gateway, subnet)) {
  Serial.println("Blad konfiguracji statycznego IP");
}

  WiFi.begin(ssid, pass);
  delay(1000);

  Serial.println("\nPołączono z siecią Wi-Fi");
  Serial.print("\nESP32 IP: "); 
  Serial.println(WiFi.localIP());

   // Start serwera Modbus
  mb.server();

  mb.addHreg(REG_bmp_temp, 0);    
  mb.addHreg(REG_bmp_pres, 0);  

  if (!bmp.begin(0x76)) {
  Serial.println("Nie wykryto BMP280!");
  while (1) {
    Serial.print(".");
    delay(100);
  }
}


}

void loop() {

  PomiarZczujnikow();

  mb.task();                  
  mb.Hreg(REG_bmp_temp, t_bmpx100); // do rejestru REG_TEMP przypisuje DS_1x100
  mb.Hreg(REG_bmp_pres, p_hPa);

  
}
