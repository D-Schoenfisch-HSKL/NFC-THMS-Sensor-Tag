# Informationen zu der Mikrocontroller-Code Software 

## Generelles
Die verwendete IDE ist MCUXpresso (nicht LPCXpresso).  
Fertige Hex-Files haben die Endung *.axf.
Weitere Infos sind in der Datei "Definitionen.md"

## Makro-Konfigurationen im Mikrocontroller-Code
Konfig ist gekennzeichnet mit Kürzeln bei Hex-Files

### Main Konfig
- **PULSE_LENGTH_MS (PL)**
> Länge des Heizpulses (Üblicherweise auf 600 ms)

- **PULSE_FIT_START_MS (FS)**
> Startzeitpunkt für den Sensorsignal-Fit (Üblicherweise bei 50 ms)

- **SELF_RESET_AFTER_MEASUREMENT (SR)**
> Tag startet sich automatisch neu nach Messung.  
Dadurch wird Tag automatisch neu erkannt.

- **AUTO_MEASUREMENT (AM)**
> Automatische Messung sobald Tag in NFC-Feld kommt (Sobald Mikrocontroller startet).

- **PRINT_UART_DEBUG_INFO (PUDI)**
> Es werden Debug-Informationen aus der Main ausgegeben.

- **PRINT_UART_ERROR_INFO (PUEI)**
> Es werden Fehlermeldungen über UART ausgegeben.


### THMS Konfig
- **PRINT_UART_PULSE_CURVE (PUPC)**
> Ausgeben der "Pulskurve" via UART  
Üblicherweise in den Einheiten _sqrt(ns)_ und *LSB*.  
!! Achtung: Die PRINTF hat Schwierigkeiten mit negativen Zahlen.   
*LSB* ist abhängig von dem eingestellten Gain des ADS1115 (üblicherweise Gain Eight).
  
- **PRINT_UART_THMS_DEBUG_INFO (PUTDI)**
> Ausgeben von Debug-Informationen aus der THMS-Lib

- **ADS_GAIN (AG)**
> Eingestellter Gain des ADS1115 (Üblicherweise 8):  
AG4:  +/-1,024 V  
AG8:  +/-0,512 V (1LSB = 1,024 V/(2^16))  
AG16: +/-0,256 V

- **ADS_DR (AD)**
> Eingestellte Datenrate des ADS1115 (Üblicherweise 4):  
AD3: 32 Sps  
AD4: 64 Sps  
AD5: 128 Sps  


## Versionshistorie
- **V1.3.2** (Aktuell) 
  - Abfrage der Konfigurationsdaten und Firmware-Version jetzt möglich über "Do:06"
- **V1.3.1**   
  - Stand Q1 2022  
  - Wichtige Änderungen: Sensorsignal wurde geändert zu sqrt(ns)/LSB (Vorher sqrt(ms)/V).  
  - 1LSB = 1562500pV  
  - Flash Usage: 85.08%  
- **V1.3**   
  - Stand Q4 2021  
  - Flash Usage: 91.16%  

## Bugs und mögliche Verbesserungen
- [ ] Handeln von I2C Blocks aufgrund von Zugriff auf NTAG via NFC.
- [x] Unabhängigkeit wo die NDEF-Nachricht im Speicher beginnt (zu prüfen).
 
	
	

