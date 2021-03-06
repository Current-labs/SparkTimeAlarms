/*
  TimeAlarms.cpp - Arduino Time alarms for use with Time library   
  Copyright (c) 208-2011 Michael Margolis. 
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
 */
 
 /*
  2 July 2011 - replaced alarm types implied from alarm value with enums to make trigger logic more robust
              - this fixes bug in repeating weekly alarms - thanks to Vincent Valdy and draythomp for testing
*/

extern "C" {
#include <string.h> // for memset
}



#include "TimeAlarms.h"
//#include "Time.h"

#define IS_ONESHOT  true   // constants used in arguments to create method
#define IS_REPEAT   false 


//**************************************************************
//* Alarm Class Constructor

AlarmClass::AlarmClass()
{
  Mode.isEnabled = Mode.isOneShot = 0;
  Mode.alarmType = dtNotAllocated;
  value = nextTrigger = 0;
  onTickHandler = NULL;  // prevent a callback until this pointer is explicitly set 
}

//**************************************************************
//* Private Methods

 
void AlarmClass::updateNextTrigger()
{  
  if( (value != 0) && Mode.isEnabled )
  {
    time_t time = Time.local();
    if( dtIsAlarm(Mode.alarmType) && nextTrigger <= time )   // update alarm if next trigger is not yet in the future
    {      
      if(Mode.alarmType == dtExplicitAlarm ) // is the value a specific date and time in the future
      {
        nextTrigger = value;  // yes, trigger on this value   
      }
      else if(Mode.alarmType == dtDailyAlarm)  //if this is a daily alarm
      {
        if( value + previousMidnight(Time.local()) <= time)
        {
          nextTrigger = value + nextMidnight(time); // if time has passed then set for tomorrow
        }
        else
        {
          nextTrigger = value + previousMidnight(time);  // set the date to today and add the time given in value   
        }
      }
      else if(Mode.alarmType == dtWeeklyAlarm)  // if this is a weekly alarm
      {
        if( (value + previousSunday(Time.local())) <= time)
        {
          nextTrigger = value + nextSunday(time); // if day has passed then set for the next week.
        }
        else
        {
          nextTrigger = value + previousSunday(time);  // set the date to this week today and add the time given in value 
        } 
      }
      else  // its not a recognized alarm type - this should not happen 
      {
        Mode.isEnabled = 0;  // Disable the alarm
      }	  
    }
    if( Mode.alarmType == dtTimer)
    {
      // its a timer
      nextTrigger = time + value;  // add the value to previous time (this ensures delay always at least Value seconds)
    }
  }
  else
  {
    Mode.isEnabled = 0;  // Disable if the value is 0
  }
}

//**************************************************************
//* Time Alarms Public Methods

TimeAlarmsClass::TimeAlarmsClass()
{
  isServicing = false;
  for(uint8_t id = 0; id < dtNBR_ALARMS; id++)
     free(id);   // ensure  all Alarms are cleared and available for allocation  
}

// this method creates a trigger at the given absolute time_t
// it replaces the call to alarmOnce with values greater than a week   
AlarmID_t TimeAlarmsClass::triggerOnce(time_t value, OnTick_t onTickHandler, void *onTickArgs){   // trigger once at the given time_t
     if( value > 0)
        return create( value, onTickHandler, onTickArgs, IS_ONESHOT, dtExplicitAlarm );
     else
        return dtINVALID_ALARM_ID; // dont't allocate if the time is greater than one day 	  
}

// this method will now return an error if the value is greater than one day - use DOW methods for weekly alarms   
AlarmID_t TimeAlarmsClass::alarmOnce(time_t value, OnTick_t onTickHandler, void *onTickArgs){   // trigger once at the given time of day
     if( value <= SECS_PER_DAY)
        return create( value, onTickHandler, onTickArgs, IS_ONESHOT, dtDailyAlarm );
     else
        return dtINVALID_ALARM_ID; // dont't allocate if the time is greater than one day 	  
}

AlarmID_t TimeAlarmsClass::alarmOnce(const int H,  const int M,  const int S,OnTick_t onTickHandler, void *onTickArgs){   // as above with HMS arguments
   return create( AlarmHMS(H,M,S), onTickHandler, onTickArgs, IS_ONESHOT, dtDailyAlarm );
}

AlarmID_t TimeAlarmsClass::alarmOnce(const timeDayOfWeek_t DOW, const int H,  const int M,  const int S, OnTick_t onTickHandler, void *onTickArgs){  // as above, with day of week 
   return create( (DOW-1) * SECS_PER_DAY + AlarmHMS(H,M,S), onTickHandler, onTickArgs, IS_ONESHOT, dtWeeklyAlarm );   
}
   
// this method will now return an error if the value is greater than one day - use DOW methods for weekly alarms   
AlarmID_t TimeAlarmsClass::alarmRepeat(time_t value, OnTick_t onTickHandler, void *onTickArgs){ // trigger daily at the given time
    if( value <= SECS_PER_DAY)
       return create( value, onTickHandler, onTickArgs, IS_REPEAT, dtDailyAlarm );
    else
       return dtINVALID_ALARM_ID; // dont't allocate if the time is greater than one day 	  
    }
    
    AlarmID_t TimeAlarmsClass::alarmRepeat(const int H,  const int M,  const int S, OnTick_t onTickHandler, void *onTickArgs){ // as above with HMS arguments
         return create( AlarmHMS(H,M,S), onTickHandler, onTickArgs, IS_REPEAT, dtDailyAlarm );
    }
    
    AlarmID_t TimeAlarmsClass::alarmRepeat(const timeDayOfWeek_t DOW, const int H,  const int M,  const int S, OnTick_t onTickHandler, void *onTickArgs){  // as above, with day of week 
       return create( (DOW-1) * SECS_PER_DAY + AlarmHMS(H,M,S), onTickHandler, onTickArgs, IS_REPEAT, dtWeeklyAlarm );      
    }
      
    AlarmID_t TimeAlarmsClass::timerOnce(time_t value, OnTick_t onTickHandler, void *onTickArgs){   // trigger once after the given number of seconds 
         return create( value, onTickHandler, onTickArgs, IS_ONESHOT, dtTimer );
    }
    
    AlarmID_t TimeAlarmsClass::timerOnce(const int H,  const int M,  const int S, OnTick_t onTickHandler, void *onTickArgs){   // As above with HMS arguments
      return create( AlarmHMS(H,M,S), onTickHandler, onTickArgs, IS_ONESHOT, dtTimer );
    }
      
    AlarmID_t TimeAlarmsClass::timerRepeat(time_t value, OnTick_t onTickHandler, void *onTickArgs){ // trigger after the given number of seconds continuously
         return create( value, onTickHandler, onTickArgs, IS_REPEAT, dtTimer);
    }
    
    AlarmID_t TimeAlarmsClass::timerRepeat(const int H,  const int M,  const int S, OnTick_t onTickHandler, void *onTickArgs){ // trigger after the given number of seconds continuously
         return create( AlarmHMS(H,M,S), onTickHandler, onTickArgs, IS_REPEAT, dtTimer);
    }
    
    void TimeAlarmsClass::enable(AlarmID_t ID)
    {
      if(isAllocated(ID)) {
        Alarm[ID].Mode.isEnabled = (Alarm[ID].value != 0) && (Alarm[ID].onTickHandler != 0) ;  // only enable if value is non zero and a tick handler has been set
        Alarm[ID].updateNextTrigger(); // trigger is updated whenever  this is called, even if already enabled	 
      }
    }
    
    void TimeAlarmsClass::disable(AlarmID_t ID)
    {
      if(isAllocated(ID))
        Alarm[ID].Mode.isEnabled = false;
    }
      
    // write the given value to the given alarm
    void TimeAlarmsClass::write(AlarmID_t ID, time_t value)
    {
      if(isAllocated(ID))
      {
        Alarm[ID].value = value;
        enable(ID);  // update trigger time
      }
    }
    
    // return the value for the given alarm ID
    time_t TimeAlarmsClass::read(AlarmID_t ID)
    {
      if(isAllocated(ID))
        return Alarm[ID].value ;
      else 	
        return dtINVALID_TIME;  
    }
    
    // return the alarm type for the given alarm ID
    dtAlarmPeriod_t TimeAlarmsClass::readType(AlarmID_t ID)
    {
      if(isAllocated(ID))
        return (dtAlarmPeriod_t)Alarm[ID].Mode.alarmType ;
      else 	
        return dtNotAllocated;  
    }

    void TimeAlarmsClass::free(AlarmID_t ID)
    {
      if(isAllocated(ID))
      {
        Alarm[ID].Mode.isEnabled = false;
    	Alarm[ID].Mode.alarmType = dtNotAllocated;
        Alarm[ID].onTickHandler = 0;
    	Alarm[ID].value = 0;
    	Alarm[ID].nextTrigger = 0;   	
      }
    }
    
    // returns the number of allocated timers
    uint8_t TimeAlarmsClass::count()
    {
       uint8_t c = 0; 
       for(uint8_t id = 0; id < dtNBR_ALARMS; id++)
       {
          if(isAllocated(id))
            c++;
       }
       return c;
    }
    
    // returns true only if id is allocated and the type is a time based alarm, returns false if not allocated or if its a timer
     bool TimeAlarmsClass::isAlarm(AlarmID_t ID)
     {
        return( isAllocated(ID) && dtIsAlarm(Alarm[ID].Mode.alarmType) );
     }
     
     // returns true if this id is allocated
     bool TimeAlarmsClass::isAllocated(AlarmID_t ID)
     {
        return( ID < dtNBR_ALARMS && Alarm[ID].Mode.alarmType != dtNotAllocated );
     }
     
    
    AlarmID_t TimeAlarmsClass::getTriggeredAlarmId()  //returns the currently triggered  alarm id
    // returns  dtINVALID_ALARM_ID if not invoked from within an alarm handler
    {
      if(isServicing)
           return  servicedAlarmId;  // new private data member used instead of local loop variable i in serviceAlarms();
      else
         return dtINVALID_ALARM_ID; // valid ids only available when servicing a callback
    }
     
    // following functions are not Alarm ID specific.
    void TimeAlarmsClass::delay(unsigned long ms)
    {
      unsigned long start = millis();
      while( millis() - start  <= ms)
        serviceAlarms();
    }
    		
    void TimeAlarmsClass::waitForDigits( uint8_t Digits, dtUnits_t Units)
    {
      while(Digits != getDigitsNow(Units) )
      {
        serviceAlarms();
      }
    }
    
    void TimeAlarmsClass::waitForRollover( dtUnits_t Units)
    {
      while(getDigitsNow(Units) == 0  ) // if its just rolled over than wait for another rollover	                            
        serviceAlarms();
      waitForDigits(0, Units);
    }
    
    uint8_t TimeAlarmsClass::getDigitsNow( dtUnits_t Units)
    {
      time_t time = Time.local();
      if(Units == dtSecond) return numberOfSeconds(time);
      if(Units == dtMinute) return numberOfMinutes(time); 
      if(Units == dtHour) return numberOfHours(time);
      if(Units == dtDay) return dayOfWeek(time);
      return 255;  // This should never happen 
    }
    
    //***********************************************************
    //* Private Methods
    
    void TimeAlarmsClass::serviceAlarms()
    {
      if(! isServicing)
      {
        isServicing = true;
        for( servicedAlarmId = 0; servicedAlarmId < dtNBR_ALARMS; servicedAlarmId++)
        {
          if( Alarm[servicedAlarmId].Mode.isEnabled && (Time.local() >= Alarm[servicedAlarmId].nextTrigger)  )
          {
            OnTick_t TickHandler = Alarm[servicedAlarmId].onTickHandler;
            void *TickArgs = Alarm[servicedAlarmId].onTickArgs;
            if(Alarm[servicedAlarmId].Mode.isOneShot)
               free(servicedAlarmId);  // free the ID if mode is OnShot		
            else   
               Alarm[servicedAlarmId].updateNextTrigger();
            if( TickHandler != NULL) {        
              (*TickHandler)(TickArgs);     // call the handler  
            }
          }
        }
        isServicing = false;
      }
    }
    
    // returns the absolute time of the next scheduled alarm, or 0 if none
    time_t TimeAlarmsClass::getNextTrigger()
    {
      time_t nextTrigger = 0xffffffff;  // the max time value

      for(uint8_t id = 0; id < dtNBR_ALARMS; id++)
      {
        if(isAllocated(id) )
        {
      	  // if(Alarm[id].nextTrigger <  nextTrigger) {
          if(uint32_t(Alarm[id].nextTrigger) <  uint32_t(nextTrigger)) {
  		      nextTrigger = Alarm[id].nextTrigger;
          }
        }      
    	}

      return (uint32_t(nextTrigger) == 0xffffffff) ? 0 : nextTrigger;
    }

    void TimeAlarmsClass::captureLastNextTriggerData()
    {
      time_t timeNow = Time.local();
      // Serial.println("timeNow      : " + String(numberOfHours  (timeNow)));
      // Serial.println("timeNow      : " + String(numberOfMinutes(timeNow)));
      // Serial.println("timeNow      : " + String(numberOfSeconds(timeNow)));
      // Serial.println("timeNow      : " + String(dayOfWeek      (timeNow)));

      time_t  lastTrigger = 0x00000000;
      uint8_t lastTriggerId = 0;
      for(uint8_t id = 0; id < dtNBR_ALARMS; id++)
      {
        if(isAllocated(id) )
        {
          time_t testValue = Alarm[id].nextTrigger;
          // Serial.println("-- testValue  : " + String(numberOfHours  (testValue)));
          // Serial.println("-- testValue  : " + String(numberOfMinutes(testValue)));
          // Serial.println("-- testValue  : " + String(numberOfSeconds(testValue)));
          // Serial.println("-- testValue  : " + String(dayOfWeek      (testValue)));
          if(uint32_t(Alarm[id].nextTrigger) > uint32_t(lastTrigger)) {
            lastTrigger = Alarm[id].nextTrigger;
            lastTriggerId = id;
          }
        }
      }
      // Serial.println("lastTriggerId: " + String(lastTriggerId));
      // Serial.println("lastTrigger  : " + String(numberOfHours  (lastTrigger)));
      // Serial.println("lastTrigger  : " + String(numberOfMinutes(lastTrigger)));
      // Serial.println("lastTrigger  : " + String(numberOfSeconds(lastTrigger)));
      // Serial.println("lastTrigger  : " + String(dayOfWeek      (lastTrigger)));

      time_t nextTrigger = 0xffffffff;  // the max time value
      uint8_t nextTriggerId = 0;
      for(uint8_t id = 0; id < dtNBR_ALARMS; id++)
      {
        if(isAllocated(id) )
        {
          // if(Alarm[id].nextTrigger <  nextTrigger) {
          if(uint32_t(Alarm[id].nextTrigger) <  uint32_t(nextTrigger)) {
            nextTrigger = Alarm[id].nextTrigger;
            nextTriggerId = id;
          }
        }      
      }
      // Serial.println("nextTriggerId: " + String(nextTriggerId));
      // Serial.println("nextTrigger  : " + String(numberOfHours  (nextTrigger)));
      // Serial.println("nextTrigger  : " + String(numberOfMinutes(nextTrigger)));
      // Serial.println("nextTrigger  : " + String(numberOfSeconds(nextTrigger)));
      // Serial.println("nextTrigger  : " + String(dayOfWeek      (nextTrigger)));

      uint32_t lastTriggerSeconds = uint8_t(numberOfHours(lastTrigger))*3600 + uint8_t(numberOfMinutes(lastTrigger))*60 + uint8_t(numberOfSeconds(lastTrigger));
      uint32_t nextTriggerSeconds = uint8_t(numberOfHours(nextTrigger))*3600 + uint8_t(numberOfMinutes(nextTrigger))*60 + uint8_t(numberOfSeconds(nextTrigger));
      uint32_t timeNowSeconds     = uint8_t(numberOfHours(timeNow    ))*3600 + uint8_t(numberOfMinutes(timeNow    ))*60 + uint8_t(numberOfSeconds(timeNow    ));

      this->ltId = lastTriggerId;
      this->ntId = nextTriggerId;
      this->ltOnTickArgs = Alarm[lastTriggerId].onTickArgs;
      this->ntOnTickArgs = Alarm[nextTriggerId].onTickArgs;
      this->ltTimeSeconds = lastTriggerSeconds;
      this->ntTimeSeconds = nextTriggerSeconds;
      this->nowTimeSeconds = timeNowSeconds;

      // Serial.println("");
    }

    void *TimeAlarmsClass::getOnTickArgs(AlarmID_t ID)
    {
      // if(isAllocated(ID)) {
      return Alarm[ID].onTickArgs;
      // }
    }

    // attempt to create an alarm and return true if successful
    AlarmID_t TimeAlarmsClass::create( time_t value, OnTick_t onTickHandler, void *onTickArgs, uint8_t isOneShot, dtAlarmPeriod_t alarmType, uint8_t isEnabled) 
    {
      if( ! (dtIsAlarm(alarmType) && Time.local() < SECS_PER_YEAR)) // only create alarm ids if the time is at least Jan 1 1971
      {  
    	for(uint8_t id = 0; id < dtNBR_ALARMS; id++)
        {
          if( Alarm[id].Mode.alarmType == dtNotAllocated )
    	  {
    	  // here if there is an Alarm id that is not allocated
      	    Alarm[id].onTickHandler = onTickHandler;
            Alarm[id].onTickArgs = onTickArgs;
    	    Alarm[id].Mode.isOneShot = isOneShot;
    	    Alarm[id].Mode.alarmType = alarmType;
    	    Alarm[id].value = value;
    	    isEnabled ?  enable(id) : disable(id);   
            return id;  // alarm created ok
    	  }  
        }
      }
      return dtINVALID_ALARM_ID; // no IDs available or time is invalid
    }
    
    // make one instance for the user to use
    TimeAlarmsClass Alarm = TimeAlarmsClass() ;

