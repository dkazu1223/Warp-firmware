/*
	Authored 2016-2018. Phillip Stanley-Marbell, Youchao Wang.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;



void
initINA219(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (	kWarpTypeMaskConfiguration |
						kWarpTypeMaskShuntVoltage  |
						kWarpTypeMaskBusVoltage    |
						kWarpTypeMaskPower         |
						kWarpTypeMaskCurrent       |
						kWarpTypeMaskCalibration
					);
	return;
}

WarpStatus
writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payload, uint16_t menuI2cPullupValue)
{
	uint8_t		payloadByte[2], commandByte[1];
	i2c_status_t	status;

	switch (deviceRegister)
	{
		case 0x00: case 0x05:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	commandByte[0] = deviceRegister;
	payloadByte[1] = payload & 0xFF;
	payloadByte[0] = (payload >> 8) & 0xFF;
	
	status = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadByte,
							2,
							gWarpI2cTimeoutMilliseconds);
	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

WarpStatus
configureSensorINA219(uint16_t payloadF_SETUP,  uint16_t menuI2cPullupValue)
{
	WarpStatus	i2cWriteStatus;
	i2cWriteStatus = writeSensorRegisterINA219(kWarpSensorConfigurationRegisterINA219_Calibration /* register address */,
							payloadF_SETUP /* payload: Disable FIFO */,
							menuI2cPullupValue);
	
	return i2cWriteStatus;
}

WarpStatus
readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes)
{
	uint8_t		cmdBuf[1] = {0xFF};
	uint16_t	LSB;
	uint16_t	MSB;
	uint16_t	out;
	i2c_status_t	status;


	USED(numberOfBytes);
	switch (deviceRegister)
	{
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}


	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	cmdBuf[0] = deviceRegister;

	status = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceINA219State.i2cBuffer,
							numberOfBytes,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}
	//LSB = deviceINA219State.i2cBuffer[0];
	//MSB = deviceINA219State.i2cBuffer[1];
	
	//out = ((LSB & 0xFF) << 8) | (MSB & 0xFF);
		
	return kWarpStatusOK;
}


	/*
	int k;
	int p;
	k=0;
	p=0;
	int shunt;
	int bus;
	int power;
	int current;
	
	SEGGER_RTT_printf(0, "\r\n\Shunt voltage\n"); 
	while( k < 3 ) 
	{
	k++;
				
	//OSA_TimeDelay(1);
				
	shunt = readSensorRegisterINA219(0x01,2);
	bus = readSensorRegisterINA219(0x02,2);
	power = readSensorRegisterINA219(0x03,2);
	current = readSensorRegisterINA219(0x04,2);
				
	//SEGGER_RTT_printf(0, "\r\n\tread\n");
				
	SEGGER_RTT_printf(0, "\r\n\Current\n");	
	SEGGER_RTT_printf(0," %d,", shunt);				
	SEGGER_RTT_printf(0," %d,", current);
				
	SEGGER_RTT_printf(0, "\r\n\ \n");
		
	}
	*/
void	
printSensorDataINA219(bool hexModeFlag)
{
	uint16_t	LSB;
	uint16_t	MSB;
	uint16_t	readSensorRegisterValue;
	WarpStatus	i2cReadStatus;
	uint16_t	ShuntVoltage;
	uint16_t	Current;
//kWarpSensorOutputRegisterINA219ShuntVoltage
	
	i2cReadStatus = readSensorRegisterINA219(0x01, 2 /* numberOfBytes */);	
	LSB = deviceINA219State.i2cBuffer[0];
	MSB = deviceINA219State.i2cBuffer[1];
	
	readSensorRegisterValue = ((LSB & 0xFF) << 8 | (MSB));
		
	
	if (i2cReadStatus != kWarpStatusOK)
	{
		SEGGER_RTT_WriteString(0, " ----,");
	}
	else
	{
		if (hexModeFlag)
		{
			SEGGER_RTT_printf(0, " %d", readSensorRegisterValue);
		}
		else
		{
			SEGGER_RTT_printf(0, " %d", readSensorRegisterValue);
		}
		
	}
	/*	
	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219Current, 2 );	
	LSB = deviceINA219State.i2cBuffer[0];
	MSB = deviceINA219State.i2cBuffer[1];
	
	readSensorRegisterValue = ((LSB & 0xFF) << 8) | (MSB & 0xFF);
	
	if (i2cReadStatus != kWarpStatusOK)
	{
		SEGGER_RTT_WriteString(0, " ----,");
	}
	else
	{
		if (hexModeFlag)
		{
			SEGGER_RTT_printf(0, " %d,", readSensorRegisterValue);
		}
		else
		{
			SEGGER_RTT_printf(0, " %d,", readSensorRegisterValue);
		}
	}
	*/	
	
	
					
}
