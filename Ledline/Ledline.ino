/*
 *  0.1  Switch on/off with dimming effect.
 *  1.0  Brightness adjustment (NEC:0 only)
 *  1.1  Exponential brightness adjustment, constant speed.
 */
#define VERSION 0x0101

class task
{
  public:

  task():Next(NULL){}

  virtual void operator()() = 0;

  class queue
  { 
    public:

      queue():Tail(NULL){}

      bool populated() const
      {
        return Tail != NULL;
      }
      
      task* get()
      {
        auto t = Tail->Next;
        if(t == Tail)
          Tail = NULL;
        else
          Tail->Next = t->Next;
        t->Next = NULL;
        return t;
      }

      void put(task *t)
      {
        if(t->Next == NULL)
        {
          /* t is not in any queue */
          if(Tail)
          {
            t->Next = Tail->Next;
            Tail->Next = t;
          }
          else
            t->Next = t;
          Tail = t;
        }
      }
      
    private:

    /* 
     *  Task queue as circular linked list.
     *              Tail  
     *                | 
     *                v
     *  T1->T2->...->Tn
     *  ^             |
     *  +-------------+
     */

    task *Tail;
    
  };

  private:

  task *Next;
  
};

task::queue Queue;

#include <IRremote.hpp>

// IR remote control CAR MP3
// Protocol = NEC Address = 0x0
#define CAR_BUTTON_CH_PREV  0x45     
#define CAR_BUTTON_CH       0x46    
#define CAR_BUTTON_CH_NEXT  0x47    
#define CAR_BUTTON_PREV     0x44
#define CAR_BUTTON_NEXT     0x40
#define CAR_BUTTON_PLAY     0x43
#define CAR_BUTTON_MINUS    0x7
#define CAR_BUTTON_PLUS     0x15
#define CAR_BUTTON_EQ       0x9
#define CAR_BUTTON_0        0x16
#define CAR_BUTTON_100_PLUS 0x19
#define CAR_BUTTON_200_PLUS 0xD
#define CAR_BUTTON_1        0xC
#define CAR_BUTTON_2        0x18
#define CAR_BUTTON_3        0x5E
#define CAR_BUTTON_4        0x8
#define CAR_BUTTON_5        0x1C
#define CAR_BUTTON_6        0x5A
#define CAR_BUTTON_7        0x42
#define CAR_BUTTON_8        0x52
#define CAR_BUTTON_9        0x4A

// IR remote control Polaris
// Protocol = PulseDistance
// RawData
#define POLARIS_BUTTON_ON_OFF 0x3B2 

#define CONTROL_PIN       3
#define LEDLINE_PIN       5   
#define TEST_BUTTON_PIN   6

#include <math.h>

class ledline
{
  public:

    static const  uint8_t MinLevel = 0;       //< Minimum brightness level.
    static const  uint8_t MaxLevel = 0xFF;    //< Maximum brightness level.

    ledline():SwitchTask(*this), LevelTask(*this), Level(MinLevel), AdjustedMaxLevel(MaxLevel){}

    void begin(uint8_t _pin)
    {
       Pin = _pin;
       pinMode(Pin, OUTPUT);
    }

    void toggle_discrete()
    {
      Queue.put(&SwitchTask);
    }

    void fill_up_to(uint8_t _level, bool _adjust = false)
    {
      if(_level != Level)
      {
        LevelTask.begin(_level, _adjust);
        Queue.put(&LevelTask);
      }
    }

    void toggle()
    {
      fill_up_to(Level ? MinLevel:AdjustedMaxLevel);
    }

    void adjust(int _step)
    {
      fill_up_to(constrain(Level + _step, MinLevel, MaxLevel), true); 
    }

    uint8_t get() const
    {
      return Level;
    }
    
  private:

                  uint8_t Level;              //< Current ledline control pin PWМ level.
                  uint8_t Pin;                //< Ledline control pin.
                  uint8_t AdjustedMaxLevel;   //< Adjusted brightnes maximum

    uint8_t correct(uint8_t _level) const
    {
      return ceil(exp((log(256)/255)*_level) - 1);
    }
    
    void set(uint8_t _level)
    {
      Level = _level;
      analogWrite(Pin, correct(Level));
    }

    class task:public ::task
    {
      public:

        task(ledline &_ledline):Ledline(_ledline){}
        
      protected:

        ledline &Ledline;
        
    };
    
    class switch_task:public task
    {
      public:

        switch_task(ledline &_ledline):task(_ledline){}

        void operator()()
        {
          Ledline.set(Ledline.Level ? Ledline.MinLevel:Ledline.MaxLevel);
        }
        
    } SwitchTask;

    class level_task:public task
    {
      public:

          level_task(ledline &_ledline):task(_ledline){}

          void operator()()
          {
            int dt = millis() - Time;
            int dl = (((long)(Distance < 0 ? -1:+1)*(MaxLevel - MinLevel))*dt)/LevelingTime;

            Stat.Count++;

            if(dl == 0)
              Queue.put(this);
            else
            {
              int l = Ledline.Level + dl;
              
              if(dl > 0)
              {
                if(l < Level)
                  Queue.put(this);
                else
                if(l > Level)
                  l = Level;
                Ledline.set(l);
              }
              else
              if(dl < 0)
              {
                if(l > Level)
                  Queue.put(this);
                else
                if(l < Level)
                  l = Level;
                Ledline.set(l);
              }
              
              Time += dt;

              if(Level == l)
              {
                if(AdjustMaxLevel)
                  Ledline.AdjustedMaxLevel = l;
                print_stat();
              }
            }
          }

          void begin(uint8_t _level, bool _adjust_max_level = false)
          {
            Level = _level;
            Distance = Level - Ledline.Level;
            AdjustMaxLevel = _adjust_max_level;
            Stat.Time = Time = millis();
            Stat.Count = 0;              
          }

          void print_stat()
          {
            Serial.print(F("Count:\t")); Serial.print(Stat.Count); Serial.println();
            Serial.print(F("Time:\t")); Serial.print(Time - Stat.Time); Serial.println();
          }

      private:

        int Level;            //< Target level.
        int Distance;         //< Distance between target and source levels.
        unsigned long Time;   //< Previouse timestamp.
        bool AdjustMaxLevel;  //< Flag to adjust max brightness level by targed one.

        static const int LevelingTime = 2000; //< Transition time between to different brightness levels in milliseconds.

        struct
        {
          unsigned long Count;
          unsigned long Time;
        } Stat;
        
    } LevelTask;
    

} Ledline;

class remote_control_input_task:public task
{
  public:

  void adjust(int _sign)
  {
    Ledline.adjust(_sign*((Ledline.MaxLevel - Ledline.MinLevel + 1)>>3));
  }

  void operator()()
  {
    if(IrReceiver.decode())
    {
      if(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW)
        Serial.println(F("Overflow detected"));
      else
      {
        IrReceiver.printIRResultShort(&Serial);
        IrReceiver.printIRSendUsage(&Serial);  
      }
      
      IrReceiver.resume();

      if(IrReceiver.decodedIRData.protocol == NEC && IrReceiver.decodedIRData.address == 0)
      {
        switch(IrReceiver.decodedIRData.command)
        {
          case CAR_BUTTON_MINUS:
            adjust(-1);
          break;
          case CAR_BUTTON_PLUS:
            adjust(+1);
          break;
          default:
            Ledline.toggle();
        }
      }
      else
      if(IrReceiver.decodedIRData.protocol != UNKNOWN)
        Ledline.toggle();
    }
    
    /* Loop back to remote control input check.
     *  This is equal simple loop processing. In the future
     *  this task should be initiated from external interrupt.
     */
    Queue.put(this); 
  }

} RemoteControlInputTask;

class toggle_button_task:public task
{
  public:

  toggle_button_task(){}

  void operator()()
  {
    if(digitalRead(TEST_BUTTON_PIN) == LOW)
    {
        uint8_t l = Ledline.get();
        
        if(l == Ledline.MinLevel)
          Sign = +1;
        else
        if(l == Ledline.MaxLevel)
          Sign = -1;
          
        RemoteControlInputTask.adjust(Sign);
    }
    Queue.put(this);
  }

private:

  int Sign;
  
} ToggleButtonTask;

static void print_version()
{
  Serial.print('V'); 
  Serial.print(':');
  Serial.print(VERSION >> 8);
  Serial.print('.');
  Serial.println(VERSION&0x0F);
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  print_version();
  /* Control pin here is set in intput mode without pullup. 
   *  If its neccessary external pullup register sholbd used or
   *  different or self made reciever library.
   */
  IrReceiver.begin(CONTROL_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("Ready to receive IR signals of protocols:"));
  printActiveIRProtocols(&Serial);
  Serial.println();

  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  Ledline.begin(LEDLINE_PIN);

  Queue.put(&RemoteControlInputTask);
  Queue.put(&ToggleButtonTask);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Queue.populated())
    (*Queue.get())();
}
