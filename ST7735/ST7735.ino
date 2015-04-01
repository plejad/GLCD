#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "sdk.h"


#define PIN_BACKLIGHT 9
#define SPI_CSB 0x04 
#define SPI_PORT PORTB  // sets Pins 10-13 for SPI communication

#define sclk 13
#define mosi 11

#define CS 10
#define DC 8
#define RST 7

Adafruit_ST7735 tft = Adafruit_ST7735(CS, DC, RST);   

int cx =0 ;
int cy= 0 ;
unsigned char lastchr ;

TRM_DEBUG* pDebugData=NULL ;
TRM_TELEMETRY* pTelemetryData=NULL ;


void SPI_Init()
{
  // pinMode(7, OUTPUT) ; // CS pin for the pressure sensor
  
  pinMode(PIN_BACKLIGHT,OUTPUT) ;  // Turn backlight on
  digitalWrite(PIN_BACKLIGHT,HIGH) ;

  /*  
  SPI_PORT = SPI_PORT | SPI_CSB;     // Set CSB of SCA3000 to one
 
  Spi.mode(0);    // This requires the SPI library, but also initializes ports. 
  //  SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0); //Set SPI comm to slowest speed, or 250kHz
                                                    // Good speed to initially use for diagnostics
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1); //Set SPI comm to slower speed of 1MHz to stay within
                                        // specifications of the SCA3000 chip (1.6 MHz max)
                                        // Top SPI speed = 4MHz;  this is 4MHz / 2^2, or 1MHz
    */
    
    tft.initR(INITR_BLACKTAB) ;
   
    tft.fillScreen(ST7735_BLACK);
    
    delay(50);  // To allow SPI communications to initialize and LCD to boot up & stabilize    
}


void testlines(uint16_t color) {
  tft.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, 0, x, tft.height()-1, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, 0, tft.width()-1, y, color);
  }

  tft.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, 0, x, tft.height()-1, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, 0, 0, y, color);
  }

  tft.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, tft.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, tft.height()-1, tft.width()-1, y, color);
  }

  tft.fillScreen(ST7735_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, 0, y, color);
  }
}


void testdrawtext(char *text, uint16_t color) {
  tft.setRotation(1) ;
  tft.setCursor(0, 0);
  tft.setTextSize(2) ;
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}


void DrawGauge(INT8 x, INT8 y, INT8 w, INT8 h, INT8 from, INT8 to, UINT16 color, UINT16 border)
{
  tft.drawRect(x,y,w,h, border) ;
  tft.fillRect(x+1,y+1,from,h-2, ST7735_BLACK) ;
  tft.fillRect(x+1+from,y+1,to-from+1,h-2, color) ;
  tft.fillRect(x+1+to+1,y+1,w-3-to,h-2, ST7735_BLACK) ;
}

void DrawInstrument(UINT8 x, UINT8 y, UINT8 r, INT16 needle, INT16 oldneedle, UINT16 color, UINT16 border)
{
  float x1 = x + cos(radians((float)needle))*(r-2) ;
  float y1 = y + sin(radians((float)needle))*(r-2) ;
  float x1o = x + cos(radians((float)oldneedle))*(r-2) ;
  float y1o = y + sin(radians((float)oldneedle))*(r-2) ;
  
  tft.drawCircle(x,y,r,border) ;
  
  if (needle != oldneedle)
  {
    tft.drawLine(x,y,x1o,y1o, ST7735_BLACK) ;
  }
  
  tft.drawLine(x,y,x1,y1, color) ;
  tft.fillCircle(x,y,2,border) ;
}

void Plot(UINT8 x, UINT8 y, UINT8 ly, UINT16 color)
{
  UINT8 lx =x ;
  
  if (lx>0)
  {
    lx-- ;
  }
  
  tft.drawLine(lx,ly,x,y,color) ;
}

void snprint(char* buf, INT8 len, INT16 n)
{
  UINT8 fNeg = ( n < 0) ;
  INT16 iTmp ;

  if (fNeg)
  {
    n=-n ;
  }

  if (0==len)
  {
    iTmp = n ;

    if (iTmp==0)
    {
      len=1 ;
    }
    else
    {
      while (iTmp > 0)
      {
        iTmp /=10 ;
        len++ ;
      }
    }

    if (fNeg)
    {
      len++ ;
    }
  }


  buf[len]=0 ;
  len-- ;

  if (n==0)
  {
    buf[len]='0' ;
    len-- ;
  }
  else
  {
    while ((n>0) && (len>=0))
    {
      buf[len] = '0' + (n % 10) ;
      n /= 10 ;
      len -- ;
    }
  }

  if ((fNeg) && (len>=0))
  {
    buf[len] = '-' ;
    len-- ;
  }

  while (len >=0)
  {
    buf[len] = ' ' ;
    len-- ;
  }
}

void setup()
{
  SPI_Init() ;
  SDKRESULT res = SDKRESULT_OK;
  int16_t iReturnedInfo=SDK_NOTHING ;

  INT16 iOldRoll = 999 ;
  INT16 iOldPitch = 999 ;
  INT16 iOldCmps = 999 ;
  INT16 iRoll ;
  INT16 iPitch ;
  INT16 iCmps ;
  INT8  i ;
  UINT8 x ;
  UINT8 y ;
  char buf[8] ;
  INT8 chan[12] ;
  UINT8 cx=0 ;
  INT8 lgr=100 ;
  INT8 lgn=100 ;
  INT8 lgy=100 ;
  
  
  Serial.begin(9600) ;  
  
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(1) ;
  tft.setCursor(0, 0);
  tft.setTextSize(1) ;
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  tft.setTextWrap(true);

 
  pDebugData = DebugData() ;
  pTelemetryData = TelemetryData();

  tft.println(" www.plejad.net") ;
  delay(2000) ;
  tft.println("Connecting...") ;
  
  // Try to retrieve version information from the pcc
  res = Connect();
  if (res != SDKRESULT_OK)
  {
    goto cleanup ;
  }

  tft.println("Waiting for data...") ;

  // Finally, switch the PCC into debug mode
  res = EnableTelemetry();
  LEDON ;

  if (res != SDKRESULT_OK)
  {
    goto cleanup ;
  }
  
  tft.fillScreen(ST7735_BLACK);
 
  
  

  // Output all labels
  tft.setCursor(0, 0);
  tft.setTextSize(0) ;
  tft.println("T") ;
  tft.println("Y") ;
  tft.println("N") ;
  tft.println("R") ;
 
  for (i=0 ; i< 4 ; i++)
  {
    tft.setCursor(52, i*8);
    tft.write(i+49) ;
    tft.setCursor(104,i*8);
    tft.write(i+53) ;
  }
  
  tft.setCursor(95, 45);
  tft.print("SAT:") ;
  tft.setCursor(95, 53);
  tft.print("BAT:") ;
  
  
  while (true)
  {
    ProcessTelemetry(&iReturnedInfo) ;
    
    if ((iReturnedInfo & SDK_DEBUG)==SDK_DEBUG)
    {
    
      pDebugData = DebugData() ;
      pTelemetryData = TelemetryData();    
      
      for (i=0 ; i < 8 ; i++)
      {
        chan[i] = pDebugData->iChannels[i] ;
      }
      
      if (chan[0]>100)
      {
        break ;
      }
      
      x=8 ;
      y=0 ;
      
      for (i=0 ; i< 12 ; i++)
      {
        if (chan[i]>0)
        {
          iRoll = (chan[i]+3)/7 + 19 ;
          DrawGauge(x,y,40,6,19,iRoll, RGB(0,255,0), RGB(255,255,255)) ;
        }
        else
        {
          iRoll = (chan[i]-3)/7 + 19 ;
          DrawGauge(x,y,40,6,iRoll, 19, RGB(0,255,0), RGB(255,255,255)) ;
        }
        
        y=y+8 ;
        if (y>=32)
        {
          x=x+52 ;
          y=0 ;
        }
      }
      
      /*
      DrawGauge(8,0,40,6,19,19, RGB(0,255,0), RGB(255,255,255)) ;
      DrawGauge(8,8,40,6,0,19, RGB(0,255,0), RGB(255,255,255)) ;
      DrawGauge(8,16,40,6,19,38, RGB(0,255,0), RGB(255,255,255)) ;

      DrawGauge(60,0,40,6,19,19, RGB(0,255,0), RGB(255,255,255)) ;
      DrawGauge(112,0,40,6,19,19, RGB(0,255,0), RGB(255,255,255)) ;
      */
      
      iRoll = pDebugData->iAngle[ROLL] >> 5 ;
      iPitch = pDebugData->iAngle[PITCH] >> 5 ;
      iCmps = pDebugData->iHeading-90 ;
      
      DrawInstrument(14,52,14,iRoll,iOldRoll , RGB(255,0,0), RGB(255,255,255)) ;
      DrawInstrument(45,52,14,iPitch,iOldPitch , RGB(255,255,0), RGB(255,255,255)) ;
      DrawInstrument(76,52,14,iCmps,iOldCmps , RGB(0,100,255), RGB(255,255,255)) ;
      
      
      iOldRoll = iRoll ;
      iOldPitch = iPitch ;
      iOldCmps = iCmps ;
      
      
      
      tft.drawFastVLine(cx,68,61, RGB(0,0,0)) ;

      iRoll= 100+(pDebugData->iGyro[ROLL] >> 2);
      Plot(cx, iRoll, lgr, RGB(255,255,0));
      lgr=iRoll ;
      
      iRoll= 100+(pDebugData->iGyro[PITCH] >> 2);
      Plot(cx, iRoll, lgn, RGB(0,255,255));
      lgn=iRoll ;

      iRoll= 100+(pDebugData->iGyro[YAW] >> 2);
      Plot(cx, iRoll, lgy, RGB(255,0,255));
      lgy=iRoll ;

      cx++ ;

      if (cx>159)
      {
        cx=0 ;
      }
    
  
    }

    if ((iReturnedInfo & SDK_TELEMETRY)== SDK_TELEMETRY)
    {

      tft.setCursor(122, 53);
      iRoll = pDebugData->iVoltage/10 ;
      snprint(buf,2,iRoll/10) ;
      tft.print(buf) ;
      tft.write('.') ;
      snprint(buf,1,iRoll % 10) ;
      tft.print(buf) ;
      tft.write('V') ;
      
      tft.setCursor(122, 45);
      snprint(buf,2,pTelemetryData->iGpsNumSat) ;
      tft.print(buf) ;
      
      switch (pTelemetryData->iDebugIndex)
      {
        case STD_TEL_CHANNEL_09:
        case STD_TEL_CHANNEL_10:
        case STD_TEL_CHANNEL_11:
        case STD_TEL_CHANNEL_12:
        {
          chan[8+pTelemetryData->iDebugIndex-STD_TEL_CHANNEL_09]=pTelemetryData->iDebugValue ;
        }
        break;
      }
      
      
    }
    
    // tft.print() ;
    
  }
 
cleanup:  
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1) ;  
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);  
  tft.setCursor(0, 60);
  tft.print("Please wait...") ;
  delay(3000) ;
  Serial.end() ;
  Serial.begin(9600) ;  
  
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 60);
  
  
}


void loop()
{
  unsigned char chr ;
  
  if (Serial.available())
  {
    chr = Serial.read() ;
  
    if (lastchr==0xfe)
    {
      if (chr==1)
      {
          // tft.fillScreen(ST7735_BLACK);
          tft.fillRect(0,60,tft.width(),32,ST7735_BLACK);
          tft.setCursor(0, 60);
          cx =0 ;
          cy =0 ;
      }
      else if (chr==192)
      {
          tft.setCursor(0, 60);
          cx=0 ;
          tft.write('\n') ;
      }
    }
    else
    {
      if (chr != 0xfe)
      {
        tft.write(chr) ;
      }
    }

    lastchr = chr ;  
  }
}



