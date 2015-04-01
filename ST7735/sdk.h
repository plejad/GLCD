/* 
 * File:   sdk.h
 * Author: as
 *
 * Created on 23. Dezember 2012, 17:49
 */

#ifndef SDK_H
#define	SDK_H


#define INT8 char
#define INT16 int
#define UINT8 unsigned char
#define UINT16 unsigned int
#define INT32 long
#define UINT32 unsigned long
#define bool   UINT8

#ifndef NULL
#define NULL (0)
#endif

#define LEDON
#define LEDOFF

#define outChar2(x) Serial.write(x)
#define inChar2() Serial.read()
#define isCharReady2() Serial.available()
#define DELAY_US(x) delay(1)

#define SDKRESULT_OK				0

#define SDKRESULT_INUSE				1			// Access to the port is denied
#define SDKRESULT_NOPORT			2			// Port does not exist
#define SDKRESULT_INVALID_PARAM                 3      			// Invalid parameter
#define SDKRESULT_NOCONN			4			// Could not connect to the copter control board
#define SDKRESULT_NOTELEMETRY                   5			// Did not receive telemetry information
#define SDKRESULT_DEBUG                         6                       // Got debug request from computer

#define SDKRESULT_FAIL				100			// Internal failure

#define SDKRESULT_NOCOMPAT			200			// SDK ist not compatible to firmware installed on control board

typedef int16_t SDKRESULT ;

// #pragma pack(1)
typedef struct tagDebug
{
  // UINT8 iPrefix1 ;
  // UINT8 iPrefix2 ;
  INT8  iChannels[8] ;			   // 0    Channels 1-8 (use CHANNEL_xxx for indexing)
  INT8  iGyro[3] ;				   // 8    Gyro values (use PITCH/ROLL/YAW constants for indexing)
  INT16 iIntegral[3] ;			   // 11   Integral values (use PITCH/ROLL/YAW constants for indexing)
  INT8  iMotor[8] ;				   // 17   Motor values 1-8
  INT16 iVoltage ;                 // 25   Battery voltage
  INT16 iIntern1 ;                 // 27   Internal (used for debugging)
  INT16 iIntern2 ;                 // 29   Internal (used for debugging)
  INT16 iAngle[3] ;				   // 31   Flight angles (available only with ACC Sensor) in Degrees*32
  INT16 iHeading ;                 // 37   Compass Heading (in Degrees)
  INT16 iHeight ;                  // 39   Height over ground
  UINT8  uiCRC ;                   // 41   Checksum of packet
} TRM_DEBUG ;                      // 42

// Telemetry struct -> current size = 35
typedef struct tagTelemetry
{
  // UINT8 iPrefix1 ;
  // UINT8 iPrefix2 ;
  INT32 iGpsLon ;         // 0    GPS Longitude
  INT32 iGpsLat ;         // 4    GPS Latitude
  INT16 iGpsSpeed ;       // 8    GPS Speed
  INT16 iGpsAlt ;         // 10   GPS Altitude
  INT16 iGpsCourse ;      // 12   GPS Course
  INT8  iGpsFix ;         // 14   GPS Fix (0 = no Fix)
  INT8  iGpsNumSat ;      // 15   GPS Number of Satelites in view
  INT32 iGpsTime ;        // 16   GPS Time (GMT)
  INT32 iGpsDate ;        // 20   GPS Date (GMT)
  INT8  iGpsState ;       // 24   GPS Status Bits

  INT8  iGpsRoll ;        // 25   Roll commands issued by GPS controller
  INT8  iGpsNick ;        // 26   Pitch commands issued by GPS controller
  INT8  iGpsYaw ;         // 27   Yaw commands issued by GPS controller

  INT16 iGpsDistWP ;      // 28   Distance to current Target
  INT16 iGpsBearingWP ;   // 30   Bearing (course) to current Target
  INT8  iGpsNumWaypoint ; // 32   Index of current Target (0= home position, 1,2,3... = waypoints)

  INT8  iTemperature ;    // 33   Current temperature

  INT8  iDebugIndex ;     // 34   Index of low frequency telemetry value
  INT16 iDebugValue ;     // 35   Low frequency telemetry value

  UINT8  uiCRC ;          // 37   Checksum of packet
} TRM_TELEMETRY ;
// #pragma pack()


// Flags indicating which type of information was returned

#define SDK_NOTHING			0
#define SDK_DEBUG			1
#define SDK_TELEMETRY		2
#define SDK_SETTINGS		4

// AXIS

#define PITCH 0
#define ROLL  1
#define YAW   2

// Serial Command definitions

// Serial Channels

#define CHANNEL_SERGAS    0			// Used for throttle override command (this is added to the RC control stick value, range -127 to 127)
#define CHANNEL_SERYAW    1			// Used for yaw override command (this is added to the RC control stick value, range -127 to 127)
#define CHANNEL_SERROLL   2			// Used for roll override command (this is added to the RC control stick value, range -127 to 127)
#define CHANNEL_SERPITCH  3			// Used for pitch override command (this is added to the RC control stick value, range -127 to 127)
#define CHANNEL_SEREXT1   4			// Used for switch override commands ext1-ext4 (this is added to the RC control stick value, range -127 to 127)
#define CHANNEL_SEREXT2   5
#define CHANNEL_SEREXT3   6
#define CHANNEL_SEREXT4   7
#define CHANNEL_SERROLLD  8			// Used to roll copter to a certain absolute angle (degrees, this is added to the RC control stick values and SERROLL, range -127 to 127)
#define CHANNEL_SERNICKD  9			// Used to pitch copter to a certain absolute angle (degrees, this is added to the RC control stick values and SERPITCH, range -127 to 127)
#define CHANNEL_SERYAWD  10			// Used to turn copter to an absolute heading 		NOT YET IMPLEMENTED!!!
#define CHANNEL_SERVARIO 11			// Used to control height over ground 				NOT YET IMPLEMENTED!!!

// Internal channel numbers
#define CHANNEL_GAS		0
#define CHANNEL_YAW		1
#define CHANNEL_ROLL	2
#define CHANNEL_PITCH	3
#define CHANNEL_EXT1	4
#define CHANNEL_EXT2	5
#define CHANNEL_EXT3	6
#define CHANNEL_EXT4	7

// Low speed telemtry indexes
#define STD_TEL_CHANNEL_09    1
#define STD_TEL_CHANNEL_10    2
#define STD_TEL_CHANNEL_11    3
#define STD_TEL_CHANNEL_12    4
#define STD_TEL_MAG_X         5
#define STD_TEL_MAG_Y         6
#define STD_TEL_MAG_Z         7
#define STD_TEL_MAG_HOVERGAS  8



// Serial command interface (command occupies 3 lsbs of opcode, rest is index)
#define CMD_ENGINES          1
#define CMD_CONTROL          2
#define CMD_MODE             3
#define CMD_ACTION           4

uint8_t ComputeCRC(uint8_t uiSeed, int16_t iStart, uint8_t* pBuffer, int16_t iSize);

SDKRESULT Connect() ;
SDKRESULT EnableTelemetry() ;
SDKRESULT ProcessTelemetry(int16_t* piInfoAvailable) ;

SDKRESULT SendCommand(uint8_t uiCommand, uint8_t uiOpcode, int8_t iParam) ;
SDKRESULT RequestSettings() ;


TRM_DEBUG* DebugData() ;
TRM_TELEMETRY* TelemetryData() ;
UINT8* SettingsData();

int16_t	  SettingsSize() ;
int16_t	  DebugSize() ;
int16_t	  SoftwareVersion() ;
int16_t	  SettingsVersion() ;

void Disconnect() ;

extern TRM_DEBUG* pDebugData ;
extern TRM_TELEMETRY* pTelemetryData ;

#endif	/* SDK_H */

