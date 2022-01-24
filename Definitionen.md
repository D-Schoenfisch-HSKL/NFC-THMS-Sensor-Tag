# Definitionen zum Datenaustausch

## Formatierung des NDEF Textes
Daten werden im NFC-Speicher als NDEF abgelegt in der Form:  
> z.B. "Do:01;No:1;SS:123;MS:456;RSQPB:1203;"

Hierbei stehen die einzelnen Teile für folgende Eigenschaften:

Mnemonic| Funktion
-------------- | --------
Do | Letzter Do-Befehl
No | Ist die Nummer der letzten Messung. Startet bei 1 nach jedem Neustart des Mikrocontrollers.
SS | Wert des Sensorsignals der letzten THMS-Messung. (Sensorsignal ist die inverse Steigung)
MS | Wert des Messsignals der letzten THMS-Messung. (Messsignal ist die erste gemessene Brüceknspannung und proportional zur Umgebungstempertur).
RSQPB | Ist das Bestimmtheitsmaß (R²) multipliziert mit 32^2.


## Do-Befehle
Die Do-Befehle sind zugleich die States der "Finite-State-Machine" die auf dem Mikrocontroller läuft.
Sie werden auf dem NFC-Speicher nach dem "Do:" angezeigt als HEX-ASCII, sind auf dem Tag aber als uint_8 gehandelt.
Wenn der Tag im Leerlauf (0x01) ist können neue States in den NFC-Speicher geschrieben werden und werden dann vom Tag abgehandelt.
Folgende Do-Befehle sind bereits implementiert:

Do Instruction (uint_8) | Mnemonic | Funktion
-------------- | -------- | -------------
0x00  | FSM_INIT |  Initialisierung (Wird vom Tag bei jedem Neustart ausgeführt)
0x01  | FSM_IDLE |  Idle/Leerlauf. Auslesen von Instruktionen aus dem NFC-Speicher
0x02  | FSM_DO_SINGLE_MEASUREMENT |  Messung durchführen. Tag kehrt nach Messung zurück zu 0x01.
0x04  | FSM_NFCS_I2C_RST |  Zurücksetzen/Neustarten des NFC-Chips. Spannung für Mikrocontroller (LPC812) bleibt erhalten, wird also nicht zurückgesetzt.
0x06  | FSM_GET_CONFIG |  Abrufen der Firmware-Version und Konfigurationsdaten (Sensorsignal Type, Messsignal Typ, Pulse Länge (ms), Puls_fitstart (ms)...). 
0xFF  | FSM_ERROR |  Error. Es wird ein Fehler-code über die UART-Schnittstelle ausgegeben.

Wenn auf den Tag also "Do:02;" als NDEF geschrieben wird fürt dieser eine Mesung durch.  
!! Auchtung: Das Semikolon (;) am Ende muss auch geschrieben werden.

## Sensor-Konfiguration
Verschiedene Einstellungen können bei dem kompillieren des Mikrocontroller-Programms bereits vorgenommen werden.
So z.B. Messsignal-Typ, Pulslänge oder auch Fin-Fenster (Startzeitpunkt für Fit). 
Diese Einstellungsparameter sind abhängig von der Anwendung des Tags (z.B. Textilmaterial oder für Anwendugn auf der Haut).
Mit dem Do-Befehl zu "GetConf" kann diese Konfiguration vom Sensor-Tag ausgegeben weren.
>**Zur Info:** Einige Einstellungen werden in der Software als Variable gehandelt und könnten so über einen zusätzlichen Do-Befehl zur Konfiguration abgespeichert werden.  
Allerdings werden alle Variabeln nach einem Neustart des Mikrocontrollers wieder zurück gesetzt. Sie bleiben also nur erhalten solange das NFC-Feld aufrecht erhalten bleibt.

Eine Antwort zum Do-Befhel GetConf kann z.B. so aussehen.  
> "Do: 1;FWV:1.3.3;SST:sqrt(ns)/LSB;MST:nV;PLEN:600ms;PST:50ms;TCR:6 10^-3 K^-1;LSBM:15625000;". 

**!! Achtung,** SST und MST werde erst nach einer Messung richtig gesetzt.

Folgende Eigenschaften sind konfiguriert:

Mnemonic | Funktion
-------- | -------------
 FWV    | Firmware Version.
 SST    | Typ/Einheit des Sensorsignals.
 MST    | Tpy/Einheit des Messsignals.
 PLEN	| Länge des Heizpulses (= zugleich Endzeitpunkt für Fit). Gegeben in "ms".
 PST	| Startzeitpunkt für den Fit des Heizpulses. Gegeben in "ms".
 TCR	| "Theral Coefficient of Resistance". Gegeben in "1E-3 K^-1".
 LSBM	| Wertigkeit eines LSB in pico Volt. 



## Fehlercodes
Über Uart

Fehlercode| Definition
-------------- | --------
0x00U | ERROR_UNKNOWN					
0x01U | ERROR_FSM_INSTRUCTION_UNKNOWN	
0x03U | ERROR_NO_INSTRUCTION_FOUND		
0x04U | ERROR_NDEF_MESSAGE_TO_LONG		
0x05U | ERROR_NDEF_MESSAGE_CONSTRUCTION
0x06U | ERROR_NTAG_COMMUNICATION		
0x10U | ERROR_CAN_NOT_SET_NTAG_FSM_INST
0x20U | ERROR_CAN_NOT_GET_NTAG_FSM_INST
