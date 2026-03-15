/** \file
    Contact bounce meter measures voltage stabilization time on PB0(PCINT0/CLKO/ICP1).

    Used interrupts vectors:
	- PCINT0
	- TIMER1 OVF
	- TIMER1 CAPT


    Measurements
    ------------
    BB - button from arduino set (blue button).
    OS - Old wall switch
    NS - New wall switch
    NL - New wall switch on long wire

    |Board	|Offset	|Capture	|Time	|Counter	|Button	|No	|
    |:---	|:---	|:---		|:---	|:---		|:---	|:---	|
    |Nano	|1	|3699		|69234	|4		|BB	|1	|
    |Nano	|1	|3798		|69333	|4		|BB	|2	|
    |Nano	|0	|8924		|8924	|3		|BB	|3	|
    |Nano	|1	|1408		|66943	|3		|BB	|4	|
    |Nano	|0	|16811		|16811	|3		|BB	|5	|
    |Nano	|1	|3812		|69347	|4		|BB	|6	|
    |Nano	|0	|56078		|56078	|3		|BB	|7	|
    |Nano	|0	|27587		|27587	|3		|BB	|8	|
    |Nano	|1	|3463		|68998	|4		|BB	|9	|
    |Nano	|1	|3453		|68988	|4		|BB	|10	|
    |Nano	|0	|8611		|8611	|3		|BB	|11	|
    |Nano	|1	|42724		|108259	|4		|OS	|1	|
    |Nano	|1	|30771		|96306	|4		|OS	|2	|
    |Nano	|1	|36352		|101887	|4		|OS	|3	|
    |Nano	|1	|49801		|115336	|4		|OS	|4	|
    |Nano	|1	|37598		|103133	|4		|OS	|5	|
    |Nano	|3	|31336		|227941	|5		|OS	|6	|
    |Nano	|2	|26641		|157711	|5		|OS	|7	|
    |Nano	|7	|43357		|502102	|9		|OS	|8	|
    |Nano	|3	|12365		|208970	|5		|OS	|9	|
    |Nano	|1	|7693		|73228	|4		|OS	|10	|
    |Nano	|1	|25969		|91504	|4		|OS	|11	|
    |Nano	|1	|24964		|90499	|4		|NS	|1	|
    |Nano	|1	|439		|65974	|3		|NS	|2	|
    |Nano	|1	|22928		|88463	|4		|NS	|3	|
    |Nano	|1	|590		|66125	|4		|NS	|4	|
    |Nano	|1	|22486		|88021	|4		|NS	|5	|
    |Nano	|1	|169		|65704	|4		|NS	|6	|
    |Nano	|1	|21315		|86850	|4		|NS	|7	|
    |Nano	|1	|166		|65701	|4		|NS	|8	|
    |Nano	|1	|17127		|82662	|4		|NS	|9	|
    |Nano	|0	|23309		|23309	|3		|NS	|10	|
    |Nano	|1	|20379		|85914	|4		|NS	|11	|
    |Nano	|1	|6098		|71633	|4		|NL	|1	|
    |Nano	|1	|11572		|77107	|4		|NL	|2	|
    |Nano	|1	|11193		|76728	|4		|NL	|3	|
    |Nano	|1	|11391		|76926	|4		|NL	|4	|
    |Nano	|1	|11904		|77439	|4		|NL	|5	|
    |Nano	|1	|12592		|78127	|4		|NL	|6	|
    |Nano	|1	|31		|65566	|3		|NL	|7	|
    |Nano	|1	|4465		|70000	|4		|NL	|8	|
    |Nano	|1	|644		|66179	|3		|NL	|9	|
    |Nano	|1	|5040		|70575	|4		|NL	|10	|
    |Nano	|1	|11074		|76609	|4		|NL	|11	|
 */
#include "task.h"

/** Contact bounce meter. */
class contact_bounce_meter:public task
{
  static const uint8_t MaxIdleCounter = 2;
  
  /** Output measurement results. */
  void operator()()
  {
    Serial.begin(9600);
    Serial.println();
    Serial.print("Offset:"); Serial.println(Offset);
    Serial.print("Capture:"); Serial.println(Capture);
    Serial.print("Time:"); Serial.println(Capture + Offset*0x0000FFFFL);
    Serial.print("Counter:"); Serial.println(Counter);
    Serial.end();
  }
  
  /** Enable PCINT0. */
  void enablePCINT0()
  {
    PCMSK0 |= _BV(PCINT0);     
  }

  /** Disable PCINT0. */
  void disablePCINT0()
  {
    PCMSK0 &= ~_BV(PCINT0);    
  }

  /** Enable TIMER1 OVF and CAPT. */
  void enableTIMER1_OVF_CAPT()
  {
    TIMSK1 |= _BV(ICIE1)|_BV(TOIE1);
  }

  /** Disable TIMER1 OVF and CAPT. */
  void disableTIMER1_OVF_CAPT()
  {
    TIMSK1 &= ~(_BV(ICIE1)|_BV(TOIE1));
  }

  /** Clear TCNT1. */
  void clearTCNT1()
  {
    TCNT1 = 0;
  }

  uint16_t	Capture;	///< ICR1 value saved in ICI ISR
  uint8_t	Offset;		///< TOV counter value saved at ICI ISR
  uint8_t	Counter;	///< TOV overflow counter 
  bool		Idle;		///< Idle flag is used to check if any ICI has been occurs since previous TOV
  uint8_t	IdleCounter;	///< Number of sequential idle TOV intervals
  
public:

  /** Singleton object. */
  static contact_bounce_meter Self;
  
  /** Create meter in idle state. */
  contact_bounce_meter()
  {
  }

  /** Begin measuring process.

      Unconditionally enables pin change interrupt 0 to start measuring process on
      PCINT0 pin.
   */
  void begin()
  {
    PCICR |= _BV(PCIE0);	// Enable PCIE0 interrupts.

    TCCR1A = 0x00;		// Timer operatin mode normal (non-PWM).
    TCCR1B = _BV(CS10);		// Timer prescaler mode - no prescaler, falling edge to trigger input capture.
    TCCR1C = 0x00;		// No compare mode forced.

    enablePCINT0();		// enable PCINT0
  }

  /** ISR to start measuring process. */
  void PCINT()
  {
    Counter = Offset = 0;
    Capture = 0;
    Idle = true;
    IdleCounter = MaxIdleCounter;
    
    disablePCINT0();
    enableTIMER1_OVF_CAPT();
    clearTCNT1();
  }

  /** TIMER1_OVF interrupt.

      Increases overflow counter and stops measuring process if number of sequential idle overflow intervals exceed limit.
   */
  void OVF()
  {
    Counter++;
    if(Idle && --IdleCounter == 0)
      {
	disableTIMER1_OVF_CAPT();
	enablePCINT0();
	task::scheduler::Self.putISR(this);
      }
    Idle = true;
  }

  /** TIMER1_CAPT interrupt.

      Save captured and overflow counter values. Reset idle flat to continue measuring.

      \todo Reset IdleCounter after latest measurement. Now idle overflows is countet after the first measure - an error.
  */
  void CAPT()
  {
    Capture = ICR1;
    Offset = Counter;
    Idle = false;
    IdleCounter = MaxIdleCounter;
  }

};

contact_bounce_meter contact_bounce_meter::Self;

/** Pin Change Interrupt on ICP1 to start measure.

    All other PCINT of PCI0 except ICP1 should be masked to avoid unintended start.
 */
ISR(PCINT0_vect)
{
  contact_bounce_meter::Self.PCINT();
}

/** TOV1 ISR. */
ISR(TIMER1_OVF_vect)
{
  contact_bounce_meter::Self.OVF();
}

/** Input Capture Interript ISR. */
ISR(TIMER1_CAPT_vect)
{
  contact_bounce_meter::Self.CAPT();
}
