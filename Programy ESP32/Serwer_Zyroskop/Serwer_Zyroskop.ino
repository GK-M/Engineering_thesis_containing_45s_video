#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <GY521.h>

//Biblioteka do Wifi i Modbus TCP/IP
#include <WiFi.h>
#include <ModbusIP_ESP8266.h>   

#define SDA 21
#define SCL 22

// KONFIGURACJA SIECI 
const char* ssid = "MikroTik";     // Nazwa sieci WiFi
const char* pass = "12345678";    // Haslo sieci WiFi

// Statyczne IP 
IPAddress local_IP(192,168,0,252);  // Adres ESP32
IPAddress gateway(192,168,0,1);      // Brama (router)
IPAddress subnet(255,255,255,0);     // Maska sieci

int I2C_speed = 400000;

GY521 sensor(0x68);
ModbusIP mb;   
//GRYRO
float ax, ay, az;
float gx, gy, gz;

int16_t ax_1000, ay_1000, az_1000;
int16_t gx_1000, gy_1000, gz_1000;

float t = 0;
int32_t tx100 = 0;

//kalibracja żyroskopu
int probki = 50;
float gx_off = 0;
float gy_off = 0;
float gz_off = 0;
float pgx_off = 0;
float pgy_off = 0;
float pgz_off = 0;

//kalibracja akcelerometru
float ax_off = 0;
float ay_off = 0;
float az_off = 0;
float pax_off = 0;
float pay_off = 0;
float paz_off = 0;


const int16_t REG_ax_1000 = 0;
const int16_t REG_ay_1000 = 1;
const int16_t REG_az_1000 = 2;
const int16_t REG_gx_1000 = 3;
const int16_t REG_gy_1000 = 4;
const int16_t REG_gz_1000 = 5;
const int16_t REG_tx100 = 6;

uint32_t CzasGY = 0;
uint32_t pCzasGY = 0;


void GYRO_ACCE(){
  CzasGY = millis();
  if(CzasGY - pCzasGY >= 100){
  
   pCzasGY = CzasGY;

   sensor.read();
   ax = sensor.getAccelX() - pax_off;
   ay = sensor.getAccelY() - pay_off;
   az = sensor.getAccelZ() - paz_off;
   gx = sensor.getGyroX() - pgx_off;
   gy = sensor.getGyroY() - pgy_off;
   gz = sensor.getGyroZ() - pgz_off;
   t = sensor.getTemperature();
  
   ax_1000  = ax * 1000;
   ay_1000  = ay * 1000;
   az_1000  = az * 1000;
   gx_1000  = gx * 100;
   gy_1000  = gy * 100;
   gz_1000  = gz * 100;
   tx100    = t * 100;



  Serial.println("\n\tAkcelerometr\t\tŻyroskop\t\tTemperatura");
  Serial.println("\tax\tay\taz\tgx\tgy\tgz\t   T");

  Serial.print('\t');
  Serial.print(ax, 3);
  Serial.print('\t');
  Serial.print(ay, 3);
  Serial.print('\t');
  Serial.print(az, 3);
  Serial.print('\t');
  Serial.print(gx, 3);
  Serial.print('\t');
  Serial.print(gy, 3);
  Serial.print('\t');
  Serial.print(gz, 3);
  Serial.print('\t');
  Serial.print(t, 3);
  Serial.println();

  }


}

void setup() {

  Serial.begin(115200);
  delay(500);
  Wire.begin(SDA, SCL);
  Wire.setClock(I2C_speed);
  delay(500);
  
  gx_off = gy_off = gz_off = 0;
  ax_off = ay_off = az_off = 0;

  while (sensor.wakeup() == false) {
    Serial.println("Nie znaleziono GY521");
    }
    
  for (int i = 0;i<probki;i++){
    sensor.read();
   gx_off = sensor.getGyroX() + gx_off;
   gy_off = sensor.getGyroY() + gy_off;
   gz_off = sensor.getGyroZ() + gz_off;
  
   ax_off = sensor.getAccelX() + ax_off;
   ay_off = sensor.getAccelY() + ay_off;
   az_off = sensor.getAccelZ() + az_off;

  delay(100);
  }

  pgx_off= gx_off/probki;
  pgy_off= gy_off/probki; 
  pgz_off= gz_off/probki;
  pax_off = ax_off/probki;
  pay_off = ay_off/probki;
  paz_off = az_off/probki - 1.0f;
  Serial.print(gx_off);
  Serial.println("\t");
  Serial.print(gy_off);
  Serial.println("\t");
  Serial.print(gz_off);
  Serial.println("\t");

  
  
 if (!WiFi.config(local_IP, gateway, subnet)) {

    Serial.println("Blad konfiguracji statycznego IP!");
  }

  WiFi.begin(ssid, pass);
  delay(1000);

  while (WiFi.status() != WL_CONNECTED) 
  { delay(500); 
  Serial.print("."); 
  }
  Serial.print("\nESP32 IP: "); 
  Serial.println(WiFi.localIP());

  // Start serwera Modbus
  mb.server();
  
  // Wartości Początkowe rejestrów / 7
  mb.addHreg(REG_ax_1000, 0);
  mb.addHreg(REG_ay_1000, 0);
  mb.addHreg(REG_az_1000, 0);
  mb.addHreg(REG_gx_1000, 0);
  mb.addHreg(REG_gy_1000, 0);
  mb.addHreg(REG_gz_1000, 0);
  mb.addHreg(REG_tx100, 0); 
}

void loop() {

  GYRO_ACCE();

  mb.task();                  
  mb.Hreg(REG_ax_1000, ax_1000);
  mb.Hreg(REG_ay_1000, ay_1000);
  mb.Hreg(REG_az_1000, az_1000);
  mb.Hreg(REG_gx_1000, gx_1000);
  mb.Hreg(REG_gy_1000, gy_1000);
  mb.Hreg(REG_gz_1000, gz_1000);
  mb.Hreg(REG_tx100,    tx100); 
  
}
