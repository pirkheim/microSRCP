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

GLLegoPIR::GLLegoPIR( int addr, LEGOPowerFunctions& pir, uint8_t pir_output, uint8_t pir_channel, unsigned int acc, bool only_use_acc, unsigned int dec)
{
	//this->addr = addr;
	SRCPGenericLoco::addr = addr;
	SRCPGenericSM::addr = addr;
	
	if (pir_output != 0 && pir_output != 1) //value out of bounds
		m_output = 0;
	else
		m_output = pir_output;
	
	if (pir_channel < 0 || pir_channel > 3) //out of bounds
		m_channel = 0;
	else		
		m_channel = pir_channel;

	m_pir = &pir;

	m_current_speed = 0;
	m_desired_speed = 0;
	
	m_only_use_acc = only_use_acc;
	
	m_acc = acc;
	if (only_use_acc) //set acc value for both
		m_dec = acc;
	else
		m_dec = dec;
	
	m_last_time = millis();
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
		
		//reset speeds
		m_desired_speed = 0;
		m_current_speed = 0;
	}
	else
	{
		//if reverse -> -
		if (drivemode == 0)
			v = -v;
		
		//set desired speed to calculated speed
		m_desired_speed = v;
	
		//if current and desired speed is the same -> send now
		if (m_desired_speed == m_current_speed)
		{
			SendSpeed(m_desired_speed);
		}
		else //different desired/current speed -> check for 0 delay
		{
			if (m_current_speed < m_desired_speed) //we are slower -> accelerate
			{
				if (m_acc == 0) //no delay -> set it now
				{
					m_current_speed = m_desired_speed;
					SendSpeed(m_current_speed);
				}
				else 
				{
#ifdef PIR_SEND_FIRST_STEP_IMMEDIATE //we have delay but want to send first immediately
					++m_current_speed;
					SendSpeed(m_current_speed);
#endif
				}
			}
			else if (m_current_speed > m_desired_speed) //we are faster -> decelerate
			{
				if (m_dec == 0) //no delay -> set it now
				{
					m_current_speed = m_desired_speed;
					SendSpeed(m_current_speed);
				}
				else
				{
#ifdef PIR_SEND_FIRST_STEP_IMMEDIATE //we have delay but want to send first immediatly
					--m_current_speed;
					SendSpeed(m_current_speed);
#endif
				}
			}
			
		}
	}

	m_last_time = millis(); //save time

	return (200);
}

void GLLegoPIR::refresh()
{
	unsigned long time = millis();
	unsigned long dt = time - m_last_time;

	if (m_current_speed != m_desired_speed) //different desired speed -> we have to do something
	{
		if (m_current_speed < m_desired_speed) //we are slower -> accelerate
		{
			while (dt > m_acc && m_current_speed != m_desired_speed) //we need to accelerate
			{
				++m_current_speed;
				SendSpeed(m_current_speed);
				dt -= m_acc;
				m_last_time += m_acc;
			}
		}
		else //we are faster -> decelerate
		{
			while (dt > m_dec && m_current_speed != m_desired_speed) //we need to accelerate
			{
				--m_current_speed;
				SendSpeed(m_current_speed);
				dt -= m_dec;
				m_last_time += m_dec;
			}		
		}		
	}
	
}

void GLLegoPIR::setPower( int on )
{
	if (on == 0)
		SendBreak();
}

int GLLegoPIR::set( int bus, int addr, int device, int cv, int value )
{
	//set CV3 = acceleration delay in 10ms*value
	if (cv == 3)
	{
		//check value range: 0..10sec
		if (value < 0 || value > 1000)
			return 423;
		m_acc = value*10;
		if (m_only_use_acc) //write acc value to dec
			m_dec = m_acc;
		return (200);
	}
	else if (cv == 4)//set CV4 = deceleration delay in 10ms*value
	{
		//check value range: 0..10sec
		if (value < 0 || value > 1000)
			return 423;
		m_dec = value*10;
		return (200);
	}
	
	return (423);
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
