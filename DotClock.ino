#include <SPI.h>
#include "MAX7219_Dot_Matrix.h"
#include <PCF8563.h>

//#define DEBUG
#define BT_SET  2
#define BT_UP   3
#define BT_DOWN 4
#define LDR     A3

const byte chips = 4;
const byte BOUNC_TIME = 80;
const String MONTH[] = {"JAN", "FEB", "MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC" };

char Tm[16]; char setbuffer[4];

unsigned long lastMoved = 0;
unsigned long MOVE_INTERVAL = 30;  // mS

int  messageOffset, timeCount = 0, set_counter = 0;

byte Count = 0, SetCounter = 0, tmt =1, state = 0;

uint8_t Hours;

bool timeSet = false, isBootUp = true, is24Hour = false, userinput = false, TimeScroll = true, IsTick = true;


enum Button
{
  UP,
  Down,
  Set,
  None,
};

enum ActionType
{
  SET_Hour,
  SET_Minutes,
  SET_Seconds,
  SET_Day,
  SET_Months,
  SET_Year,
  SET_24Hour,
  Completed,
};
ActionType Action = ActionType::Completed;

MAX7219_Dot_Matrix display (chips, 10);  // Chips / LOAD 
PCF8563 pcf;
Time nowTime; 



void setup ()
  {
      pinMode(BT_SET,INPUT_PULLUP);
      pinMode(BT_UP,INPUT_PULLUP);
      pinMode(BT_DOWN,INPUT_PULLUP);
      display.begin ();
      //Serial.begin(9600);
      display.setIntensity (0);
      pcf.init();//initialize the clock     
  }  // end of setup


/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/

void updateTimeDisplay (char* message, byte LenStart, byte LenStop)
  {
    byte ln = (LenStop - LenStart)+2;
    char Buff[ln];
        
    for(byte i=LenStart; i< ln; i++)Buff[i] = message[i];
    Buff[ln-1] = 0;
   
    if(Buff[4]== '0' && Buff[5]=='0')TimeScroll = true;
    if(TimeScroll){
      display.sendSmooth (Tm, messageOffset);
      // next time show one pixel onwards
      if (messageOffset++ >= (int) (strlen(Tm) * 8))messageOffset = - chips * 8;        
       if(messageOffset == 0)TimeScroll = false;      
    }
    else display.sendString(Buff, IsTick);

    if(Count==23)IsTick = true;        
    else if(Count >=46)
    {
      IsTick = false;
      Count=0;
    }
    Count++;
}  // end of updateTimeDisplay


/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/

void updateDisplay (char* message)
  {    
      display.sendSmooth (message, messageOffset);
      // next time show one pixel onwards
      if (messageOffset++ >= (int) (strlen(message) * 8))messageOffset = - chips * 8;     
  }


/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/
void loop () 
  { 
    int intancity = analogRead(LDR);
    intancity = (intancity *.018);
    if(intancity > 15)intancity = 15;

    display.setIntensity(intancity);
    //Serial.println(intancity);
    if(isBootUp)BootDisplay();
    else
    {
       ShowTime();  
       SetTime();     
       if(!timeSet)
       {
        UserInput();
        if(!userinput)
        {
         if (millis () - lastMoved >= MOVE_INTERVAL)
         {    
             updateTimeDisplay (Tm,0,6);            
             lastMoved = millis ();
         }
       }
       }
    }     
  }  // end of loop


/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/
void BootDisplay()
{
   if (millis () - lastMoved >= MOVE_INTERVAL)
   {    
       updateDisplay((char*)"WELCOME TO CHIP & CODE");            
       lastMoved = millis ();
       Count++;       
   }     
   if(Count >176){
    isBootUp = false;
    Count = 0;
    display.sendString("CHIP",false);
    delay(1000);
    display.sendString(" & ",false);
    delay(1000);
    display.sendString("CODE",false);
    delay(1000);
   }
}


/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/
void ShowTime()
  {
      nowTime = pcf.getTime();//get current time
      Hours = nowTime.hour;
      if(!is24Hour && Hours > 12)Hours = Hours - 12; 
      Tm[0] = 48+(Hours/10)%10;
      Tm[1] = 48+Hours%10;   
      Tm[2] = 48+(nowTime.minute/10)%10;
      Tm[3] = 48+nowTime.minute%10;
      Tm[4] = 48+(nowTime.second/10)%10;
      Tm[5] = 48+nowTime.second%10;
      Tm[6] = ' ';

      Tm[7] = 48+(nowTime.day/10)%10;
      Tm[8] = 48+nowTime.day%10;
      Tm[9] = '/';
      Tm[10] = 48+(nowTime.month/10)%10;
      Tm[11] = 48+nowTime.month%10;
      Tm[12] = '/';
      Tm[13] = 48+(nowTime.year/10)%10;
      Tm[14] = 48+nowTime.year%10;
      Tm[15] = 0;
      
}


/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/
Button GetButton()
{
  if(!digitalRead(BT_UP) || !digitalRead(BT_DOWN))
        {
          timeCount++;
          if(timeCount >=BOUNC_TIME)
          {
            timeCount = 0;                         
            if(!digitalRead(BT_UP))return Button::UP;
            else if (!digitalRead(BT_DOWN))return Button::Down;
          }
     }
     else if(!digitalRead(BT_SET))
        {
            timeCount++;
            if(timeCount >=(BOUNC_TIME ))
            {
              timeCount = 0;    
              return Button::Set;              
            }
        }
        else timeCount =0;
        return None;
}


/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/
void SetTime()
{
  if(!timeSet)
  {    
    if(!digitalRead(BT_SET))SetCounter++;
    if(SetCounter >=(BOUNC_TIME *2 )){
      SetCounter = 0;
      timeSet = true;   
      Action=ActionType::SET_Hour;
      delay(200);
    }
  }

  if(timeSet)
  {
    IsTick = false;
    switch(Action)
    {
      case ActionType::SET_Hour:
            switch( GetButton())
            {
              case Button::UP:
                tmt++;
                if(tmt > 23)tmt=23;
              break;
        
              case Button::Down:        
                tmt--;
                if(tmt == 255)tmt=0;
              break;
        
              case Button::Set:
              Action=ActionType::SET_Minutes;
              break;
            }
            pcf.setHour(tmt);            
            set_counter++;
            setbuffer[0] = Tm[0];
            setbuffer[1] = Tm[1];
            setbuffer[2] = ' ';
            setbuffer[3] = 'H';
            Blink();          
      break;

      case ActionType::SET_Minutes:
            tmt = nowTime.minute;
            switch( GetButton())
            {
              case Button::UP:
                tmt++;
               if(tmt > 59)tmt=0; 
              break;
        
              case Button::Down:        
                tmt--;
                if(tmt == 255)tmt=0;
              break;
        
              case Button::Set:
              Action=ActionType::SET_Seconds;
              break;
            }
            pcf.setMinut(tmt);
            set_counter++;
            setbuffer[0] = Tm[2];
            setbuffer[1] = Tm[3];
            setbuffer[2] = ' ';
            setbuffer[3] = 'M';
            Blink();
      break;

      case ActionType::SET_Seconds:
            tmt = nowTime.second;
            switch( GetButton())
            {
              case Button::UP:
                tmt++;
               if(tmt > 59)tmt=0; 
              break;
        
              case Button::Down:        
                tmt--;
                if(tmt == 255)tmt=0;
              break;
        
              case Button::Set:
              Action=ActionType::SET_Day;
              break;
            }
            pcf.setSecond(tmt);
            set_counter++;
            setbuffer[0] = Tm[4];
            setbuffer[1] = Tm[5];
            setbuffer[2] = ' ';
            setbuffer[3] = 'S';
            Blink();
      break;

      case ActionType::SET_Day:
            tmt = nowTime.day;
            switch( GetButton())
            {
              case Button::UP:
                tmt++;
               if(tmt > 31)tmt=31; 
              break;
        
              case Button::Down:        
                tmt--;
                if(tmt == 255 || tmt == 0)tmt=1;
              break;
        
              case Button::Set:
              Action=ActionType::SET_Months;
              break;
            }
            pcf.setDay(tmt);
            set_counter++;
            setbuffer[0] = Tm[7];
            setbuffer[1] = Tm[8];
            setbuffer[2] = ' ';
            setbuffer[3] = 'D';
            Blink();
      break;

      case ActionType::SET_Months:
            tmt = nowTime.month;
            switch( GetButton())
            {
              case Button::UP:
                tmt++;
               if(tmt > 12)tmt=12; 
              break;
        
              case Button::Down:        
                tmt--;
                if(tmt == 255 || tmt == 0)tmt=1;
              break;
        
              case Button::Set:
              Action=ActionType::SET_Year;
              break;
            }
            pcf.setMonth(tmt);
            set_counter++;
            setbuffer[0] = Tm[10];
            setbuffer[1] = Tm[11];
            setbuffer[2] = ' ';
            setbuffer[3] = 'M';
            Blink();
      break;

      case ActionType::SET_Year:
            tmt = nowTime.year;
            switch( GetButton())
            {
              case Button::UP:
                tmt++;
               if(tmt > 99)tmt=99; 
              break;
        
              case Button::Down:        
                tmt--;
                if(tmt == 255)tmt=1;
              break;
        
              case Button::Set:
              Action=ActionType::SET_24Hour;
              break;
            }
            pcf.setYear(tmt);
            set_counter++;
            setbuffer[0] = Tm[13];
            setbuffer[1] = Tm[14];
            setbuffer[2] = ' ';
            setbuffer[3] = 'Y';                                    
            Blink();
      break;

      case SET_24Hour:
        if(is24Hour){
                setbuffer[0] = '2';
                setbuffer[1] = '4';
        }
        else
        {
          setbuffer[0] = '1';
                setbuffer[1] = '2';
          }
      switch( GetButton())
            {
              case Button::UP:
                is24Hour = true;                 
              break;
        
              case Button::Down:        
                is24Hour = false;                
              break;
              
              case Button::Set:
                Action=ActionType::Completed;
              break;
            }              
              set_counter++;            
              setbuffer[2] = 'H';
              setbuffer[3] = 'r';                                    
              Blink();
      
      break;
      
      case Completed:
            timeSet = false;      
            set_counter = 0;
            display.sendString("Done", IsTick);
            IsTick = true;
            delay(1000);
      break;
    }

  }

  
  }

/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/
void UserInput()
{
switch( GetButton())
            {
              case UP:
              userinput = true;
              if(Action == ActionType::Completed)
              {
              setbuffer[0] = Tm[7];
              setbuffer[1] = Tm[8];
              setbuffer[2] = ' ';
              setbuffer[3] = ' ';
              set_counter = 1;
              Action = ActionType::SET_Months;
              }
              else if(Action == ActionType::SET_Months)
              {
              setbuffer[0] = MONTH[nowTime.month-1][0];
              setbuffer[1] = MONTH[nowTime.month-1][1];
              setbuffer[2] = MONTH[nowTime.month-1][2];
              setbuffer[3] = ' ';
              set_counter = 1;              
              Action = ActionType::SET_Year;
                }
                else if(Action == ActionType::SET_Year)
                {                
              setbuffer[0] = '2';
              setbuffer[1] = '0';
              setbuffer[2] = Tm[13];
              setbuffer[3] = Tm[14];
              set_counter = 1;              
              Action = ActionType::Completed;
                }
              break;
              }
              
              if(set_counter !=0){
                set_counter++;
              }
              if(set_counter > 700){
                set_counter = 0;                
                Action = ActionType::Completed;
                userinput = false;
              }
              if(userinput)display.sendString(setbuffer, false);
}

/*******************************************************************************************/
/*                                                                                         */
/*******************************************************************************************/

  void Blink(){
                            
            if (set_counter >100 && set_counter<130)
            {
            setbuffer[0] = ' ';
            setbuffer[1] = ' ';       
            }
            else if (set_counter > 130)set_counter = 0;
            
            updateTimeDisplay (setbuffer,0,4);
  }
