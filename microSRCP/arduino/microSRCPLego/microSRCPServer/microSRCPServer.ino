/*
	microSRCPServer - Arduino Standardboard fungiert als Zentrale
	fuer eine Modelleisenbahn wird mittels USB Kabel an den PC angeschlossen.
	
	Zusaetzlich wird einen Modelleisenbahnsteuerungssoftware z.B.
	RocRail, benoetigt, welche das SRCP Protokoll beherscht.

	Fuer benoetigte Shields und Details siehe: 
	- https://github.com/mc-b/microSRCP/wiki/Steuerungkonfiguration
	- https://github.com/mc-b/microSRCP/wiki

	Copyright (c) 2010 - 2013 Marcel Bernet.  All right reserved.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//////////////////////////////////////////////////////////////////////////////////////////
// Konfiguration Protokoll
#define SRCP_ETHERNET	100
#define SRCP_SERIAL		101
#define SRCP_I2C		102
#define SRCP_PROTOCOL	SRCP_SERIAL

#define SENSOR_POLLING	500			// Intervall in welcher Sensor Aenderungen an die Zentrale gemeldet werden

//////////////////////////////////////////////////////////////////////////////////////////
// Konfiguration Board
#define BOARD_STANDARD		200
#define BOARD_I2C_MASTER 	201
#define BOARD_FB			203		// Beispiel ATmega328P: nur Sensoren (Feedback)
#define BOARD_GA			204		// Beispiel ATmega328P: nur Generic Accessoires - Geraete
#define BOARD_GL			205		// Beispiel ATmega328P: nur Motorenansteuerung
#define BOARD_LEGO			206		// Beispiel ATmega328P: nur Motorenansteuerung

//#define BOARD 	BOARD_STANDARD
#define BOARD 	BOARD_LEGO

//////////////////////////////////////////////////////////////////////////////////////////
// Konfiguration I2C
#define I2C_ADDR		0	// Eigene I2C Adresse - muss pro I2C Board angepasst werden! - Master = 0
#define I2C_OFFSET		16	// Offset, d.h. wieviele Adressen pro Board reserviert werden
#define I2C_ENABLED		0
#define I2C_MAX_SLAVES	10	// Maximale Anzahl I2C Slave Boards am Master
#define ADDR(x)			((I2C_ADDR * I2C_OFFSET) + x)	// Berechnung effektive Adresse

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Logger.h>			// in dieser Datei kann das Logging an/abgeschaltet werden

#include <SRCPCommand.h>
#include <SRCPDevice.h>
#include <SRCPDeviceManager.h>

#if ( SRCP_PROTOCOL == SRCP_ETHERNET )
#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#include <EthernetSRCPServer.h>
#include <EthernetSocket.h>
#endif

#include <SRCPFeedback.h>
#include <SRCPGenericAccessoire.h>
#include <SRCPGenericLoco.h>
#include <SRCPMessages.h>
#include <SRCPParser.h>
#if	( SRCP_PROTOCOL == SRCP_SERIAL )
#include <SRCPServerSerial.h>
#endif
#include <SRCPSession.h>

#include <Wire.h>
#include <I2CDeviceManager.h>
#include <I2CFBProxy.h>
#include <I2CGAProxy.h>
#include <I2CGLProxy.h>
#include <I2CServer.h>

#include <Servo.h>
#include <FBSwitchSensor.h>
#include <FBSwitchSensor8574.h>
#include <GASignal.h>
#include <GAPWMServo.h>
#include <GASlowServo.h>
#include <GASignal.h>
#include <GLArduinoMotor.h>
#include <GLMotoMamaAnalog.h>
#include <GLLegoPIR.h>


//create lego pir object with ir led pin 13
#include <LEGOPowerFunctions.h>
LEGOPowerFunctions lego(13);
//

#if	( SRCP_PROTOCOL == SRCP_SERIAL )
// SRCP I/O Server
srcp::SRCPServerSerial server;
#elif ( SRCP_PROTOCOL == SRCP_ETHERNET )
lan::EthernetSRCPServer server;
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip( 192, 168, 178, 241 );
#elif  ( SRCP_PROTOCOL == SRCP_I2C )
// SRCP I2C - Slave
i2c::I2CServer server = WireServer;
#else
#error "kein Prokotoll definiert"
#endif

/**
 * Initialisierung - Protokoll, Geraete etc.
 */
void setup()
{
	BEGIN( 9600 );
	INFO( "Logger ready" );

#if	( SRCP_PROTOCOL != SRCP_ETHERNET )											// Board mit Ethernet Shield hat keine Geraete um Konflikte mit Shield zu vermeiden!
//custom lego
#if	( BOARD == BOARD_LEGO )
	// Geraete initialisieren, je nach Board und Verwendung
	
	//DeviceManager.addFeedback( new dev::FBSwitchSensor( ADDR(1), A0, A2 ) ); 	// Sensoren, jeweils in Gruppen von 8 (auch wenn nicht 8 Pins belegt). A4+A5 = I2C Bus
	//DeviceManager.addFeedback( new dev::FBSwitchSensor( ADDR(9), 11, 12 ) ); 	// Sensoren, jeweils in Gruppen von 8 (auch wenn nicht 8 Pins belegt). A4+A5 = I2C Bus

	DeviceManager.addFeedback( new dev::FBSwitchSensor8574( ADDR(1), 0x38 ) );
        DeviceManager.addFeedback( new dev::FBSwitchSensor8574( ADDR(9), 0x39 ) );

        dev::GLLegoPIR* lpir;
        
        lpir = new dev::GLLegoPIR( ADDR(1), lego, 0, 0, 800 );
        DeviceManager.addLoco( lpir ); //lego pir RED, channel 1 -> yellow cargo train
        DeviceManager.addSM( lpir );
        
        lpir = new dev::GLLegoPIR( ADDR(2), lego, 1, 0, 500); //lego pir BLUE, channel 1 -> red passenger train
        DeviceManager.addLoco( lpir );
        DeviceManager.addSM( lpir );
        
        lpir = new dev::GLLegoPIR( ADDR(3), lego, 0, 2, 900); //lego pir RED, channel 2 -> red cargo train
        DeviceManager.addLoco( lpir );
        DeviceManager.addSM( lpir );

        lpir = new dev::GLLegoPIR( ADDR(4), lego, 0, 3, 900); //lego pir RED, channel 2 -> red cargo train
        DeviceManager.addLoco( lpir );
        DeviceManager.addSM( lpir );

//        DeviceManager.addLoco(new dev::GLLegoPIR( ADDR(1), lego, 0, 0));//lego pir RED, channel 1 -> yellow cargo train        
//        DeviceManager.addLoco(new dev::GLLegoPIR( ADDR(2), lego, 1, 0));//lego pir BLUE, channel 1 -> red passenger train
//        DeviceManager.addLoco(new dev::GLLegoPIR( ADDR(3), lego, 0, 1));//lego pir RED, channel 2 -> red cargo train

        DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(1), 3, 50, 90, 1, 10) );//track switch 1
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(2), 4, 50, 90, 1, 10) );//track switch 2
        DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(3), 5, 50, 90, 1, 10 ) );//track switch 3
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(4), 6, 50, 90, 1, 10 ) );//track switch 4
        DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(5), 7, 50, 90, 1, 10 ) );//track switch 5
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(6), 8, 50, 90, 1, 10 ) );//track switch 6
        DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(7), 9, 50, 90, 1, 10 ) );//track switch 7
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(8), 10, 50, 90, 1, 10 ) );//track switch 8

//--custom lego
#elif	( BOARD == BOARD_STANDARD )
	// Geraete initialisieren, je nach Board und Verwendung
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(1), 4, 5 ) ); 			// 2 Signale mit 2 LED an Ports 4 - 7.
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(2), 6, 7 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(3), 2, 60, 90 ) );  	// Servo mit Addr 3 an Pin 2, min. Stellung 60, max. Stellung 90 von 180
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(4), 3, 60, 90 ) );			// und Weiterschalten um 1ne Position alle 5 Millisekunden
	DeviceManager.addFeedback( new dev::FBSwitchSensor( ADDR(1), A0, A3 ) ); 	// Sensoren, jeweils in Gruppen von 8 (auch wenn nicht 8 Pins belegt). A4+A5 = I2C Bus
#if ( __AVR_ATmega1280__ || __AVR_ATmega2560__ )
	DeviceManager.addFeedback( new dev::FBSwitchSensor( ADDR(9), A8, A15 ) ); 	// Sensoren, Mega 8 zusaetzlich
	DeviceManager.addLoco( new dev::GLMotoMamaAnalog( ADDR(1), 10,  8,  9 ) ); 	// Moto Mama Shield, Pin 10 Geschwindigkeit, 8 Vor-, 9 Rueckwaerts - nur Mega
#endif
	DeviceManager.addLoco( new dev::GLMotoMamaAnalog( ADDR(2), 11, 12, 13 ) );
#elif	( BOARD == BOARD_MINIMAL )
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(1), 9, 60, 90 ) ); 		// 6 x Servo's an Pins 0 - 4 und 9
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(2), 0, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(3), 1, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(4), 2, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(5), 3, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(6), 4, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(7), 5, 6 ) ); 			// 2 x Signal mit 2 LED an Pins 5 - 8
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(8), 7, 8 ) );
	DeviceManager.addFeedback( new dev::FBSwitchSensor( ADDR(1), 10, 17 ) );	// 1 x Sensorblock mit 8 x Sensor an Pin D10 - D13 und C0 - C3
#elif	( BOARD == BOARD_FB && __AVR_ATmega328P__ )
	DeviceManager.addFeedback( new dev::FBSwitchSensor( ADDR(1),  2,  9 ) );	// 1 x Sensorblock mit 8 x Sensor an Pin D2 - D9
	DeviceManager.addFeedback( new dev::FBSwitchSensor( ADDR(9), 10, 17 ) );	// 1 x Sensorblock mit 7 x Sensor an Pin D10 - A4 (A0 = 14), Pin 13 ohne Funktion wegen LED
#elif	( BOARD == BOARD_GA && __AVR_ATmega328P__ )
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(1),  2, 60, 90 ) );	// 8 x Servo (max. fuer ATmega328P)
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(2),  3, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(3),  8, 60, 90 ) ); // abgestimmt auf DF Robot I/ Expansion Shield, darum die Luecke
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(4),  9, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(5), 10, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(6), 11, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(7), 12, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASlowServo( ADDR(8), 13, 60, 90 ) );
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(9), 4, 5 ) ); 			// 4 x Lichtsignal
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(10), 6, 7 ) );
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(11), A0, A1 ) );
	DeviceManager.addAccessoire( new dev::GASignal( ADDR(12), A2, A3 ) );
#elif	( BOARD == BOARD_GL && __AVR_ATmega328P__ )
	DeviceManager.addLoco( new dev::GLMotoMamaAnalog( ADDR(1), 10,  8,  9 ) ); 	// Moto Mama Shield, Pin 10 Geschwindigkeit, 8 Vor-, 9 Rueckwaerts
	DeviceManager.addLoco( new dev::GLMotoMamaAnalog( ADDR(2), 11, 12, 13 ) );
#endif
#endif

#if	( SRCP_PROTOCOL != SRCP_I2C && I2C_ENABLED )
	// initialize I2C - Master braucht keine Adresse
	i2c::I2CDeviceManager::begin( I2C_MAX_SLAVES );		// weitere Boards am I2C Bus, beginnend mit Adressen (I2C_ADDR * I2C_OFFSET) + x).
#endif

	INFO_DEVICES();									// Ausgabe der installierten Geraete

	// SRCP Kommunikation oeffnen
#if	( SRCP_PROTOCOL == SRCP_SERIAL )
	server.begin( 115200 );
#elif ( SRCP_PROTOCOL == SRCP_ETHERNET )
	server.begin( mac, ip, 4303 );
#elif  ( SRCP_PROTOCOL == SRCP_I2C )
	// initialize I2C - Slave
	server.begin( I2C_ADDR );
#endif

	INFO ( "Server listen " );
}

/**
 * Endlosloop
 */
void loop()
{
	// Host Meldungen verarbeiten
	server.dispatch( SENSOR_POLLING );

	// Refresh der Sensoren bzw. Abfragen ob Aenderungen stattgefunden haben
	DeviceManager.refresh();
}
