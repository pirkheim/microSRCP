/*
	GLLegoPir - Motorentreiber fuer Lego Power Function Züge

	Copyright (c) 2010 - 2013 Marcel Bernet, Christian Pirkheim.  All right reserved.

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
 
#include "GLLegoPIR.h"
#include <Arduino.h>
#include <LEGOPowerFunctions.h>

namespace dev
{

GLLegoPIR::GLLegoPIR( int addr, LEGOPowerFunctions& pir, uint8_t pir_output, uint8_t pir_channel)
{
	this->addr = addr;
	
	if (pir_output != 0 && pir_output != 1) //value out of bounds
		m_output = 0;
	else
		m_output = pir_output;
	
	if (pir_channel < 0 || pir_channel > 3) //out of bounds
		m_channel = 0;
	else		
		m_channel = pir_channel;

	m_pir = &pir;
}

//make destructor
//GLLegoPIR::GLLegoPIR()

int GLLegoPIR::set( int addr, int drivemode, int v, int v_max, int fn[] )
{
	//range check
	if ( v > v_max )
		v = v_max;
	if ( v < 0 )
		v = 0;

	//map to 0..7
	v = map( v, 0, v_max, 0, 7 );

	
	if (drivemode == 2) //emergency break
	{
		SendBreak();
	}
	else
	{
		//if reverse -> -
		if (drivemode == 0)
			v = -v;
		
		SendSpeed(v);
	}

	return (200);
}

//void GLLegoPIR::refresh()
//{
//}

void GLLegoPIR::setPower( int on )
{
	if (on == 0)
		SendBreak();
}

void GLLegoPIR::SendSpeed(int8_t speed)
{
	//see LEGOPowerFunctions.h for values 

	//value range check
	if (speed > 7)
		speed = 7;
	else if (speed < -7)
		speed = -7;
	
	if (speed < 0) //calculate reverse value
		speed = 16 - speed;

	//send now
	m_pir->SingleOutput(PWM, speed, m_output, m_channel);
#ifdef PIR_DOUBLE_SEND
	m_pir->SingleOutput(PWM, speed, m_output, m_channel);				
#endif
}

void GLLegoPIR::SendBreak()
{
	m_pir->SingleOutput(PWM, PWM_BRK, m_output, m_channel);
#ifdef PIR_DOUBLE_SEND
	m_pir->SingleOutput(PWM, PWM_BRK, m_output, m_channel);		
#endif
#ifdef PIR_SECURE_STOP
	m_pir->SingleOutput(PWM, PWM_BRK, m_output, m_channel);
	//m_pir->SingleOutput(PWM, PWM_BRK, m_output, m_channel);
#endif
}

}
