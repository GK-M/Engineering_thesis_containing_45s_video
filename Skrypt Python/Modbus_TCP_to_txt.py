from pymodbus.client import ModbusTcpClient
import time
# Konfiguracja połączenie z serwerem Modbus
client = ModbusTcpClient('192.168.0.251')
# Identyfikatro urządzenia slave
client.unit = 5
# Połączenie z serwerem Modbus
client.connect()

# Wstawienie nagłówków pliku txt
with open("data.txt", "a") as file:
    file.write("CZAS\tTEMP_SONDA\tTEMP_POMIESZCZENIE\tTEMP_ESP32\tREG_VL\tREG_Alarm_1\tREG_Alarm_2\tREG_Alarm_3\tREG_HMI_D1_ERROR\tREG_HMI_D2_ERROR\tpusty\tREG_HMI_D3_ERROR\tREG_MaxDS_1\tREG_MinDS_1\tREG_MaxDS_2\tREG_MinDS_2\tREG_MaxDS_3\tREG_MinDS_3\tREG_IleCzujnikow\tREG_Alarm_VL\tREG_IleDetali\tREG_VLMaxPomiar\tREG_VLMinPomiar\tREG_MaxAnalowgowyOdczyt\tREG_MaxOdczytFiltr\tREG_PrzekroczenieWartosc\tREG_MaxAlarm\n")

# Zapis odczytów z serwera do pliku / tryb 'a' - dopisywanie)
with open("data.txt", "a") as file:
    while True:
        # odczyt rejestrów
        result = client.read_holding_registers(address = 0, count = 20)
        # odczyt aktualnego czasu
        current_time = time.strftime('%H:%M:%S', time.localtime())
        file.write(f"{current_time}\t")

        #Kontorola odczytu danych
        if not result.isError():

            for i, value in enumerate(result.registers):
                file.write(f"{value}\t")
            file.write("\n")
        else:
            print("Błąd odczytu rejestu")

        time.sleep(1)

client.close()