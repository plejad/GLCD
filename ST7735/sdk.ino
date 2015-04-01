#include "sdk.h"


#define PACKET_NONE		0
#define PACKET_DEBUG		1
#define PACKET_TELEMETRY	2
#define PACKET_SETTINGS		3

uint8_t   _uiDebugBuf[50] ;		// Buffer for accumulating debug data
uint8_t   _uiTelemetryBuf[50] ;	        // Buffer for accumulating Telemetry data
uint8_t   _uiSettingsBuf[300] ;         // Buffer for accumulating settings data


uint8_t   _uiLastChar=0 ;				// Last char received
uint8_t   _uiInPacket=0 ;				// Indicates which packet we are currently processing
int16_t   _iPacketRead=0 ;			// Indicates which packet we are currently processing

int16_t	  _iSettingsSize=0 ;
int16_t	  _iDebugSize=0 ;
int16_t	  _iSoftwareVersion=0 ;
int16_t	  _iSettingsVersion=0 ;
uint32_t  _uiLastTicks=0 ;


SDKRESULT Connect()
{
	SDKRESULT res = SDKRESULT_OK ;

	res = RequestVersionInfo() ;

	return res ;
}

void Disconnect()
{
}

SDKRESULT RequestVersionInfo()
{
	int  iAttempts=0 ;
	bool fHi   = true ;
	bool fInVersionsBlock = false ;

	SDKRESULT res = SDKRESULT_OK ;
	int16_t iClock = 0 ;
        uint8_t  uiChar =0 ;        // Actual char received
	uint8_t  uiLastChar = 0 ;   // Last char received
	uint8_t  uiRead =0 ;        // payload chars read
	uint8_t  uiVersionBuf[8] ;  // Buffer for storing version data

        LEDON ;

	while (iAttempts < 6)
	{
		if (fHi)
		{
			// Try highspeed connect at 115KBaud to newer generation hardware
                        Serial.begin(115200);
			delay(10) ;
                        res=SDKRESULT_OK ;
		}
		else
		{
			// Try lowspeed connect for older hardware
                        Serial.begin(9600);
			delay(10) ;
                        res=SDKRESULT_OK ;

		}

		if (SDKRESULT_OK != res)
		{
			return res ;
		}

		fHi = !fHi ;

		outChar2((uint8_t)'V') ;  // Request version command
        	delay(20) ;

		iClock=0 ;
		fInVersionsBlock=false ;

		while (iClock < 5000)
		{
			if (isCharReady2())
			{
				uiLastChar=uiChar ;
				uiChar = inChar2() ;

				if ((uiLastChar=='#') && (uiChar=='V'))  // we detected the start of the version info block in the stream
				{
				  uiRead=0 ;
				  fInVersionsBlock = true ;
				}
				else if (fInVersionsBlock)
				{

					if (uiRead < 6)
					{
						uiVersionBuf[uiRead]=uiChar ;
						uiRead++ ;
					}

					if (uiRead>=6)
					{
						// analyze the version information block for validity
						if (uiVersionBuf[5]==0xff)
						{
							// Version block seems to be complete -> store size infos and continue processing
							_iSettingsSize = (int16_t)uiVersionBuf[2] + 255 ;
							_iDebugSize = (int16_t)uiVersionBuf[3] ;
							_iSoftwareVersion = (int16_t)uiVersionBuf[1] ;
							_iSettingsVersion = (int16_t)uiVersionBuf[0];

							return SDKRESULT_OK ;
						}

						fInVersionsBlock=false ;
					}
				}

			  }
                          else
                          {
                              DELAY_US(10);
                          }

			  iClock++ ;
		}

		iAttempts++ ;
		delay(1000) ;
	}

	return SDKRESULT_NOCONN ;
}

SDKRESULT RequestSettings()
{
	outChar2((uint8_t)'G') ;  // Request settings command
	delay(20) ;

	return SDKRESULT_OK ;
}

SDKRESULT EnableTelemetry()
{
	int  iAttempts=0 ;
	bool fInDebugBlock = false ;

	SDKRESULT res = SDKRESULT_OK ;
	int16_t iClock = 0 ;
        uint8_t  uiChar =0 ;        // Actual char received
	uint8_t  uiLastChar = 0 ;   // Last char received
	uint8_t  uiRead =0 ;        // payload chars read
	uint8_t  uiDebugBuf[100] ;  // Buffer for storing version data
	uint8_t	 uiCRC = 0 ;

	_uiLastChar=0 ;
	_uiInPacket=0 ;
	_iPacketRead=0 ;

	while (iAttempts < 10)
	{
		outChar2((uint8_t)'D') ;  // Try to switch PCC to 115200 Baud debug mode
		delay(5) ;
		Serial.begin(115200) ;


		iClock=0 ;
		fInDebugBlock=false ;

		while (iClock<5000)    // Search max 5000 chars for debug info
		{
		    if (isCharReady2())
		    {
				uiLastChar=uiChar ;
				uiChar = inChar2() ;

				if ((uiLastChar=='#') && (uiChar=='D'))  // we detected the start of the debug info block in the stream
				{
				  uiRead=0 ;
				  fInDebugBlock = true ;
				}
				else if (fInDebugBlock)
				{
					if (uiRead < _iDebugSize-2)
					{
						uiDebugBuf[uiRead]=uiChar ;
						uiRead++ ;
					}

					if (uiRead>=_iDebugSize-2)
					{
						// We received the entire debug block, check if the CRC is valid
						uiCRC = ComputeCRC(0, 0, uiDebugBuf, _iDebugSize-3) ;

						if (uiCRC == uiDebugBuf[_iDebugSize-3])  // CRC is the last UINT8 in the debug block
						{
							return SDKRESULT_OK ;
						}
						fInDebugBlock=false ;
					}


				}
			}
                        else
                        {
                            DELAY_US(10);
                        }

			iClock++ ;
		}

		iAttempts++ ;
		delay(1000) ;

	}

	return SDKRESULT_NOTELEMETRY ;
}

SDKRESULT ProcessTelemetry(int16_t* piInfoAvailable)
{
	SDKRESULT res = SDKRESULT_OK ;
	int16_t  i ;
        uint8_t  uiChar =0 ;        // Actual char received
	uint8_t	 uiCRC = 0 ;
	bool     fProcessChar = false ;

	if (NULL != piInfoAvailable)
	{
		*piInfoAvailable = SDK_NOTHING ;
	}

	if ((millis() - _uiLastTicks)>250)
	{
		outChar2((uint8_t)'D') ;  // Try to switch PCC to 115200 Baud debug mode
		_uiLastTicks = millis();
	}


	while (isCharReady2())
	{
		uiChar = inChar2() ;

		fProcessChar = true ;

		if (_uiInPacket==PACKET_NONE)
		{
			if ((_uiLastChar=='#') && (uiChar=='D'))  // we detected the start of the debug info block in the stream
			{
				_uiInPacket=PACKET_DEBUG ;
				_iPacketRead=0 ;
				fProcessChar = false;
			}

			if ((_uiLastChar=='#') && (uiChar=='T'))  // we detected the start of the telemetry block in the stream
			{
				_uiInPacket=PACKET_TELEMETRY ;
				_iPacketRead=0 ;
				fProcessChar = false;
			}

			if ((_uiLastChar=='#') && (uiChar=='S'))  // we detected the start of the settings block in the stream
			{
				_uiInPacket=PACKET_SETTINGS ;
				_iPacketRead=0 ;
				fProcessChar = false;
			}

		}

		if (fProcessChar)
		{
			if (_uiInPacket==PACKET_DEBUG)
			{
				if (_iPacketRead < _iDebugSize-2)
				{
					_uiDebugBuf[_iPacketRead]=uiChar ;
					_iPacketRead++ ;
				}

				if (_iPacketRead >=_iDebugSize-2)
				{
					// We received the entire debug block, check if the CRC is valid
					uiCRC = ComputeCRC(0, 0, _uiDebugBuf, _iDebugSize-3) ;


					_uiInPacket=PACKET_NONE ;
					if (uiCRC == _uiDebugBuf[_iDebugSize-3])  // CRC is the last UINT8 in the debug block
					{
						if (NULL != piInfoAvailable)
						{
							(*piInfoAvailable) |= SDK_DEBUG ;
                                                        return SDKRESULT_OK ;
						}
					}
				}

			}

                        
			if (_uiInPacket==PACKET_SETTINGS)
			{
				if (_iPacketRead < _iSettingsSize-2)
				{
					_uiSettingsBuf[_iPacketRead]=uiChar ;
					_iPacketRead++ ;
				}

				if (_iPacketRead >=_iSettingsSize-2)
				{
					// We received the entire debug block, check if the CRC is valid
					uiCRC = ComputeCRC(0, 0, _uiSettingsBuf, _iSettingsSize-3) ;

					_uiInPacket=PACKET_NONE ;
					if (uiCRC == _uiSettingsBuf[_iSettingsSize-3])  // CRC is the last UINT8 in the settings block
					{
						if (NULL != piInfoAvailable)
						{
							(*piInfoAvailable) |= SDK_SETTINGS ;
                                                        return SDKRESULT_OK ;
						}
					}
				}

			}                        

			if (_uiInPacket==PACKET_TELEMETRY)
			{
				if (_iPacketRead < sizeof(TRM_TELEMETRY))
				{
					_uiTelemetryBuf[_iPacketRead]=uiChar ;
					_iPacketRead++ ;
				}

				if (_iPacketRead >= sizeof(TRM_TELEMETRY))
				{
  
					// We received the entire debug block, check if the CRC is valid
					uiCRC = ComputeCRC(0, 0, _uiTelemetryBuf, sizeof(TRM_TELEMETRY)-1) ;

					_uiInPacket=PACKET_NONE ;
					if (uiCRC == _uiTelemetryBuf[sizeof(TRM_TELEMETRY)-1])  // CRC is the last UINT8 in the debug block
					{
						if (NULL != piInfoAvailable)
						{
							(*piInfoAvailable) |= SDK_TELEMETRY ;
                                                        return SDKRESULT_OK ;
						}
					}

				}

			}

		}

		_uiLastChar=uiChar ;
	}



	return SDKRESULT_OK ;
}

TRM_DEBUG* DebugData()
{
	return (TRM_DEBUG*)_uiDebugBuf ;
}

TRM_TELEMETRY* TelemetryData()
{
	return (TRM_TELEMETRY*)_uiTelemetryBuf ;
}

UINT8* SettingsData()
{
	return (UINT8*)_uiSettingsBuf ;
}

SDKRESULT SendCommand(uint8_t uiCommand, uint8_t uiOpcode, int8_t iParam)
{
	uint8_t buf[4] ;

	uiCommand = (uiCommand & 0x7) | (uiOpcode << 3) ;

	buf[0]='$' ;
	buf[1]=uiCommand ;
	buf[2]=(uint8_t)iParam ;
	buf[3]=uiCommand;

	// outChar2(buf,sizeof(buf)) ;  // Request settings command

	return SDKRESULT_OK ;
}

uint8_t ComputeCRC(uint8_t uiSeed, int16_t iStart, uint8_t* pBuffer, int16_t iSize)
{
	uint8_t uiCRC = uiSeed ;
	int16_t i ;

	for (i=iStart ; i < iSize ; i++)
	{
		uiCRC = uiCRC ^ pBuffer[i] ;
	}

	return uiCRC ;
}


int16_t	SettingsSize()
{
	return _iSettingsSize;
}

int16_t	DebugSize()
{
	return _iDebugSize ;
}

int16_t	SoftwareVersion()
{
	return _iSoftwareVersion ;
}

int16_t	SettingsVersion()
{
	return _iSettingsVersion ;
}

