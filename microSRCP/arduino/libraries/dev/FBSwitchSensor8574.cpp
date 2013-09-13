/*
	FBSwitchSensor - Einfacher Sensor an einem Digitalen Eingang
	Der interne Pullup Wiederstand wird aktiviert, dadurch muss
	der Sensor gegen GND geschaltet werden um aktiv zu sein.

	Mittels refresh werden die Sensoren lokal abgefragt, ein
	aktiver Sensor wird gespeichert und erst bei info oder
	get wieder zurueckgesetzt.

	Copyright (c) 2010 Marcel Bernet.  All right reserved.

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

#include "FBSwitchSensor8574.h"
#include "../log/Logger.h"
#include <Wire.h>

namespace dev
{

FBSwitchSensor8574::FBSwitchSensor8574( int addr, int i2c_addr )
{
	this->addr = addr;
	this->i2c_addr = i2c_addr;
	
	Wire.begin();
	
	oldSensor = 0;
	sensor = 0;
}

void FBSwitchSensor8574::refresh( )
{
	// alle Werte abfragen, nur der Wechsel auf LOW fuehrt zu Aenderung des Sensorstatuses
	//read 8bit from i2c device
	Wire.requestFrom(this->i2c_addr, 1);
	if (Wire.available())
	{
		sensor = Wire.read();
	}
}

int FBSwitchSensor8574::info( int addr, srcp::feedback fb[] )
{
	refresh();

	memset( fb, 0, 9 );

	int pos = 0;
	for	( int i = 0; i < 8; i++ )
		if	( bitRead(oldSensor, i) != bitRead(sensor,i) )
		{
			fb[pos].pin = i + this->addr;
			fb[pos].value = bitRead(sensor, i);
#if ( LOGGER_LEVEL >= TRACE_LEVEL )
		TRACE( "Sensor: " );
		Logger.print( fb[pos].pin );
		Logger.print( ":" );
		Logger.print( fb[pos].value );
#endif
			pos++;
		}

	oldSensor = sensor;
	sensor = 0;
	return	( 0 );
}

}
