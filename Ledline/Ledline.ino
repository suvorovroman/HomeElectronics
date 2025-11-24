/*
 *  0.1   Switch on/off with dimming effect.
 *  1.0   Brightness adjustment (NEC:0 only)
 *  1.1   Exponential brightness adjustment, constant speed.
 *  1.2   Shortened brightness transition time (1024 ms).
 *        4 brightness transition levels.
 *        Integral gamma correction. 
 *        Toggle maximizes brightness.
 *  1.3   Task scheduler with normal and ISR queue.
 *        Interrupt to start IrReceiver decoding loop.
 */
#define VERSION 0x0103

/** Task. */
class task
{
  public:

    task():Next(NULL){}

    virtual void operator()() = 0;

    /** Task queue. */
    class queue
    { 
      public:

        /** Create empty queue. */
        queue():Tail(NULL){}

        task* head()
        {
          return Tail;
        }

        task* tail()
        {
          return Tail->Next;
        }

        /** Put task in queue.
         *  
         *  Puts task at the queue if it is not yet in any queue and returns pointer to the task. Does nothing and return NULL
         *  if task already in a queue.
         *  
         *  \param[in]  _t  Task to put into the queue.
         *  \return Pointer to the task or NULL if task already is in queue.
         */
        task* put(task *t)
        {
          if(t->Next == NULL)
          {
            if(Tail)
            {
              t->Next = Tail->Next;
              Tail->Next = t;
            }
            else
              t->Next = t;
            Tail = t;
            return t;     // t is scheduled for execution
          }
          else
            return NULL;  // t is already in a queue and is not scheduled
        }

        /** Removes head task from queue and returns pointer to it.
         *  
         *  Does not check if queue is not empty and in the case result is unpredictable.
         *  
         *  \return Pointer to removed task;
         */
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

    /** Task scheduler. */
    class scheduler
    {
      public:

        scheduler():ISRFlag(false){}

        task* put(task* t)
        {
          return Queue.put(t);
        }

        task* putISR(task* t)
        {
          t = ISRQueue.put(t);
          if(t)
            ISRFlag = true;
          return t;
        }

        void execute()
        {
          if(ISRFlag)
          {
            (*ISRQueue.tail())();
            noInterrupts();
            ISRQueue.get();
            if(ISRQueue.head() == NULL)
              ISRFlag = false;
            interrupts();
          }
          else
          if(Queue.head())
            (*Queue.get())();
        }

      private:

        queue Queue;
        queue ISRQueue;

        public:
        byte  ISRFlag;
        
    };

  private:

    task *Next;
  
};

task::scheduler Scheduler;

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

struct udiv8_t
{
  uint8_t quot;
  uint8_t rem;
  udiv8_t(){}
  udiv8_t(uint8_t _q, uint8_t _r):quot(_q), rem(_r){}
};


struct udiv8_t udiv(uint16_t _n, uint8_t _d)
{
  return udiv8_t(_n/_d, _n%_d);
}

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
      Scheduler.put(&SwitchTask);
    }

    void fill_up_to(uint8_t _level, bool _adjust = false)
    {
      if(_level != Level)
      {
        if(Scheduler.put(&LevelTask))
          LevelTask.begin(_level, _adjust);
      }
    }

    void toggle(uint8_t _level = 0)
    {
      fill_up_to(Level ? MinLevel:(_level ? _level:AdjustedMaxLevel));
    }

    void adjust(int _step)
    {
      fill_up_to(constrain(Level + _step, MinLevel, MaxLevel), true); 
    }

    uint8_t get() const
    {
      return Level;
    }
    
    /** Brightness correction (gamma correction).
     *  
     *  It is observed that visual brightness of the led line has no linear dependence on PWM duty cycle. While duty cycle value runs from 0 up to 255, 
     *  visual brightness increases faster on lower values and slower for higher values of duty cycle. This makes to suppose logarithmic dependence between
     *  duty cycle and its visual brightness something as a follows
     *  
     *  \f{tikzpicture}
     *    \begin{axis}
     *    [
     *      enlargelimits=false,
     *      ylabel=Visual brightness (y),
     *      xlabel=Duty cycle (x),
     *    ]
     *      \addplot[domain=0:255,]{255/ln(256)*ln(x+1)};
     *    \end{axis}
     *  \f}
     *  
     *  This curve may be represented by binary logarithmic function, shifted and scaled to be defined on domain [0,255] with the same values range 
     *  
     *  \f[
     *    y = f(x) = k\log_2(x+1)
     *  \f]
     *  
     *  Scaling factor can be expressed from the following equation
     *  
     *  \f[
     *    255 = k\log_2(255+1)
     *  \f]
     *  
     *  So supposed brightness function can be expressed as
     *  
     *  \f[
     *    y = f(x) = \frac{255}{\log_2(255+1)}\log_2(x+1) =\frac{255}{8}\log_2(x+1)
     *  \f]
     *  
     *  The main goail is to acheve linear dependence of visual brightness on dimming level - integer control parameter of brightness in the range from 0 to 255. 
     *  For this gamma correction function \f$x=g(z)\f$ should be provided to convert dimming level to duty cycle so that \f$y = f(g(z)) = z\f$, 
     *  where z - is a dimming level and y - visual brightness. 
     *  Obviously such a gamma correction should be an inverse function of brightness function
     *  
     *  \f[
     *    y = z = f(g(z)) = \frac{255}{8}\log_2(g(z)+1) \Rightarrow \frac{8}{255}z = \log_2(g(z)+1) \Rightarrow 2^{\frac{8}{255}z} = g(z)+1
     *  \f]
     *  
     *  Thus the gamma correction function will be following
     *  
     *  \f[
     *    x = g(z) = 2^{\frac{8}{255}z}-1
     *  \f]
     *  
     *  \f{tikzpicture}
     *    \begin{axis}
     *    [
     *      enlargelimits = false,
     *      xlabel = Dimming level (z),
     *      ylabel = Duty cycle (x),
     *    ]
     *      \addplot[domain=0:255]{2^(8*x/255)-1};
     *    \end{axis}
     *  \f}
     * 
     *   Whereas integer power of two can be calculated effectivly as a bitwise shift then its picewise linear approximation can be used
     *   be used to calclulate gamma correction without floating point arithmetic. \f$8z\f$ can be represented as \f$255m+r\f$, where
     *   \f$m=\lfloor 8z/255 \rfloor\f$ and \f$r=8z\pmod{255}\f$. Then
     *   
     *   \f[
     *      g(z)=2^{\frac{8z}{255}}-1=2^{\frac{255m+r}{255}}-1=2^m*2^{\frac{r}{255}}-1
     *   \f]
     *   
     *  Integer part \f$2^m\f$ of gamma correction function can be calculated directly by bitwise shift operation. Fractional part \f$2^{r/255}\f$
     *  can be subsituted by linear approximation \f$r/255+1\f$.    
     *  
     *  \f{tikzpicture}
     *    \begin{axis}
     *    [
     *      enlargelimits=false,
     *      legend entries={{$2^{\frac{x}{255}}$},{$\frac{r}{255}+1$}},
     *      legend pos=south east,
     *      xlabel=$r$
     *    ]
     *      \addplot[domain=0:255,dashed]{2^(x/255)};
     *      \addplot[domain=0:255]{x/255+1};
     *    \end{axis}
     *  \f}
     *  
     *  Thus picewise linearly approximated gamma correction will be as follows
     *  
     *  \f[
     *    g(z)=2^m\left(\frac{r}{255}+1\right)-1=2^{\lfloor 8z/255 \rfloor}\left(\frac{8z\pmod{255}}{255}+1\right)-1=\frac{2^{\lfloor 8z/255 \rfloor}(8z\pmod{255}+255)}{255}-1
     *  \f]
     *  
     *  Or 
     *  
     *  \f[
     *    g(z)=\frac{2^{\lfloor 8z/255 \rfloor}(8z\pmod{255}+255)}{255}-1
     *  \f]
     *    
     *  Whereas integer division provides truncated result (rounded toward zero) remainder may be analyzed to increase precision. Let
     *  
     *  \f[
     *    v(z)= 2^{\lfloor 8z/255 \rfloor}(8z\pmod{255}+255)
     *  \f]
     *  
     *  Then
     *  
     *  \f[
     *    g(z)=\lfloor\frac{v(z)}{255}\rfloor-1+\begin{cases}1 & v(z)\pmod{255} >=128 \\0 & \text{otherwise}\end{cases}
     *  \f]
     *  
     *  \param[in]  _z  Dimming level (linear dimming control).
     *  \return Corrected duty cycle corresponding to specified dimming level.
     */
    uint8_t gamma(uint8_t _z)
    {
      udiv8_t d;
      
      d = udiv(_z << 3, 255);
      d = udiv((1 << d.quot)*(d.rem + 255), 255);
      return d.quot - 1 + (d.rem&0x80 >> 7);
    }
    
  private:

                  uint8_t Level;              //< Current ledline control pin PWМ level.
                  uint8_t Pin;                //< Ledline control pin.
                  uint8_t AdjustedMaxLevel;   //< Adjusted brightness maximum

    
    void set(uint8_t _level)
    {
      Level = _level;
      analogWrite(Pin, gamma(Level));
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
              Scheduler.put(this);
            else
            {
              int l = Ledline.Level + dl;
              
              if(dl > 0)
              {
                if(l < Level)
                  Scheduler.put(this);
                else
                if(l > Level)
                  l = Level;
                Ledline.set(l);
              }
              else
              if(dl < 0)
              {
                if(l > Level)
                  Scheduler.put(this);
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

        static const int LevelingTime = 1024; //< Transition time between to different brightness levels in milliseconds.

        struct
        {
          unsigned long Count;
          unsigned long Time;
        } Stat;
        
    } LevelTask;
    

} Ledline;

class remote_control_input_task:public task
{
  private:

    class invoker:public task
    {
      public:

          void operator()()
          {
            static remote_control_input_task t;
            Scheduler.put(&t);
          }
          
    };

  public:

    void adjust(int _sign)
    {
      Ledline.adjust(_sign*((Ledline.MaxLevel - Ledline.MinLevel + 1)>>2));
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
              Ledline.toggle(Ledline.MaxLevel);
          }
        }
        else
        if(IrReceiver.decodedIRData.protocol != UNKNOWN)
          Ledline.toggle(Ledline.MaxLevel);
      }
      else      
        Scheduler.put(this); 
    }

    static void ISRoutine()
    {
      static invoker invoker;
      Scheduler.putISR(&invoker);
    }
  
};

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
          Ledline.toggle(Ledline.MaxLevel);
        else
        {
          remote_control_input_task t;
          t.adjust(-1);
        }
    }
    Scheduler.put(this);
  }

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

  Scheduler.put(&ToggleButtonTask);

  attachInterrupt(digitalPinToInterrupt(3), remote_control_input_task::ISRoutine, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:
  Scheduler.execute();
}
