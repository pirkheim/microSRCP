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

#ifndef GLLEGOPIR_H_
#define GLLEGOPIR_H_

#include "../srcp/SRCPGenericLoco.h"
//#include <LEGOPowerFunctions.h>


#define PIR_SECURE_STOP //define if you would like a secure stop signal -> simply send three times
#define PIR_DOUBLE_SEND //define this to send all commands twice
#define PIR_SEND_FIRST_STEP_IMMEDIATE //when an acceleration or deceleration value is specified the first step is sent immediately

class LEGOPowerFunctions;

namespace dev
{

	class GLLegoPIR : public srcp::SRCPGenericLoco
	{
	private:
		uint8_t m_output; //red or blue
		uint8_t m_channel; //channel 1-4
		LEGOPowerFunctions* m_pir;
		
		int8_t m_current_speed; //the current speed of the pir (- is reverse)
		int8_t m_desired_speed; //the desired speed of the pir (- is reverse)
		
		unsigned int m_acc; //the acceleration delay in ms between speed steps
		unsigned int m_dec; //the deceleration delay in ms between speed steps
		
		unsigned long m_last_time; //time of the last refresh
		
		//send the speed to the lego ir receiver (speed has to be between -7 and 7
		void SendSpeed(int8_t speed);
		
		//sends the break signal
		void SendBreak();

	public:
		//output -> 0,1 = red or blue on lego remote
		//channel 1-4
		//acc = acceleration delay in ms
		//dec = deceleration delay in ms
		GLLegoPIR( int addr, LEGOPowerFunctions& pir, uint8_t pir_output, uint8_t pir_channel, unsigned int acc = 0, unsigned int dec = 0);
		
		int set( int addr, int drivemode, int v, int v_max, int fn[] );
		void refresh();
		void setPower( int on );
	};

}

#endif /* GLLEGOPIR_H_ */
