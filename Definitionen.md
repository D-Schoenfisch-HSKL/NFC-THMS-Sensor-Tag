# Definitionen zum Datenaustausch

## Formatierung des NDEF Textes
Daten werden im NFC-Speicher als NDEF abgelegt in der Form:  
> z.B. "Do:01;No:1;SS:123;MS:456;RSQPB:1203;"

Hierbei stehen die einzelnen Teile für folgende Eigenschaften:

Eigenschaft| Funktion
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

Do Instruction (uint_8) | Funktion
-------------- | --------
0x00  | Initialisierung (Wird vom Tag bei jedem Neustart ausgeführt)
0x01  | Idle/Leerlauf. Auslesen von Instruktionen aus dem NFC-Speicher
0x02  | Messung durchführen. Tag kehrt nach Messung zurück zu 0x01.
0x04  | Zurücksetzen/Neustarten des NFC-Chips. Spannung für Mikrocontroller (LPC812) bleibt erhalten, wird also nicht zurückgesetzt.
0x06  | Abrufen der Firmware-Version und Konfigurationsdaten (Sensorsignal Type und Messsignal Typ) . Antwort z.B. "Do: 1;FWV:1.3.3;SST:sqrt(ns)/LSB;MST:nV"
0xFF  | Error. Es wird ein Fehler-code über die UART-Schnittstelle ausgegeben.

Wenn auf den Tag also "Do:02;" als NDEF geschrieben wird fürt dieser eine Mesung durch.  
!! Auchtung: Das Semikolon (;) am Ende muss auch geschrieben werden.

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
