#include "Tlc5940.h"
#include <avr/wdt.h>
#include <EEPROM.h>

int x;

uint16_t dmx_address = 0;
uint32_t level;

uint16_t strobe[16]; // Stores milliseconds between toggles
uint32_t timers[16]; // Strobe timers
uint16_t levels[16]; // stores levels for strobing
uint8_t need_update; // Need to call Tlc.update();

uint8_t mode = 0; 
/*
Modes:
 
 0 = 8-bit DMX values ISL
 1 = 8-bit DMX values ISL + 8-bit strobe
 2 = 16-bit DMX values, linear
 
 */

void setup()
{
  wdt_reset();
  wdt_disable(); 

  delay(1000);  // 1000ms to stabilize voltage

  if(EEPROM.read(0) == 0x55 || EEPROM.read(0) == 0x55) // We've been programmed, but read twice to be sure.
  {
    mode = EEPROM.read(1);
    if(mode > 2)
    {
      mode = 0; 
    }
  }
  else  // We've never been programmed
  {
    EEPROM.write(1,mode);
    EEPROM.write(0,0x55);    
  }



  Tlc.init();
  Serial.begin(250000);
  pinMode(A0,INPUT_PULLUP);
  pinMode(A1,INPUT_PULLUP);
  pinMode(A2,INPUT_PULLUP);
  pinMode(A3,INPUT_PULLUP);
  pinMode(A4,INPUT_PULLUP);
  pinMode(A5,INPUT_PULLUP);

  pinMode(5,INPUT_PULLUP);
  pinMode(6,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);

  if(digitalRead(A0) == LOW)
  {
    dmx_address +=1; 
  }
  if(digitalRead(A1) == LOW)
  {
    dmx_address +=2; 
  }
  if(digitalRead(A2) == LOW)
  {
    dmx_address +=4; 
  }
  if(digitalRead(A3) == LOW)
  {
    dmx_address +=8; 
  }
  if(digitalRead(A4) == LOW)
  {
    dmx_address +=16; 
  }
  if(digitalRead(A5) == LOW)
  {
    dmx_address +=32; 
  }
  if(digitalRead(5) == LOW)
  {
    dmx_address +=64; 
  }
  if(digitalRead(6) == LOW)
  {
    dmx_address +=128; 
  }
  if(digitalRead(7) == LOW)
  {
    dmx_address +=256; 
  }

  DMXinit(dmx_address);
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);
  wdt_reset(); // Reset the WDT
  wdt_enable(WDTO_120MS); // Enable WDT at 120ms -- arduino will reset within 120ms of a lockup
}

void loop()
{
  wdt_reset(); // Must call every 120ms at most.
  if(dmx_address == 0)
  {

  }
  else if(dmx_address == 511)
  {
    wdt_disable(); // we're going to be stuck in some loops, so disable.


    while(digitalRead(7) == HIGH)
    {
      if(digitalRead(A0) == LOW && digitalRead(A1) == HIGH)
      {
        mode = 1; 
      }
      if(digitalRead(A1) == LOW && digitalRead(A0) == LOW)
      {
        mode = 2;
      }
    }

    EEPROM.write(1,mode);
  
    for(;;)
    {
       // Do nothing! 
    }


  }
  else
  {

    if(mode == 0) // 8-bit straight
    {
      if(DMXupdated())
      {   
        for(x = 0; x < 16; x++)
        {
          level = map(DMXget(dmx_address+x), 0, 255, 0, 4095);
          level = level*level/4095;
          Tlc.set(x, level);
        }
        Tlc.update();
      }
    }
    else if(mode == 2) // 16-bit straight
    {
      if(DMXupdated())
      {   
        for(x = 0; x < 16; x++)
        {
          level = map(word(DMXget(dmx_address+x*2),DMXget(dmx_address+1+x*2)), 0, 65535, 0, 4095);
          Tlc.set(x, level);
        }
        Tlc.update();
      }
    }
    else if(mode == 1) // 8-bit + strobe
    {
      if(DMXupdated())
      {   
        for(x = 0; x < 16; x++)
        {
          level = map(DMXget(dmx_address+x*2), 0, 255, 0, 4095);
          strobe[x] = map(DMXget(dmx_address+1+x*2), 0, 255, 500, 100);
          if(DMXget(dmx_address+1+x*2) == 0)
          {
            strobe[x] = 0; 
          }
          levels[x] = level*level/4095;
          if(strobe[x] == 0)
          {
            Tlc.set(x,levels[x]);
            need_update = 1; 
          }
        }     
      }

      for(x = 0; x < 16; x++)
      {

        if(timers[x] + strobe[x] < millis() && strobe[x] > 0)
        {
          timers[x] = millis();
          if(Tlc.get(x) > 0)
          {
            Tlc.set(x,0);
            need_update = 1;
          }
          else
          {
            Tlc.set(x,levels[x]);
            need_update = 1;
          } 
        }
      }

      if(need_update == 1)
      {
        Tlc.update();
        need_update = 0; 
      }

    }


  }

}





