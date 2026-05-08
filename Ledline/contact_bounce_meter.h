/** \file
    Contact bounce meter measures voltage stabilization time on PB0(PCINT0/CLKO/ICP1).

    Used interrupts vectors:
	- PCINT0
	- TIMER1 OVF
	- TIMER1 CAPT

    \subsection MEASUREMENTS	Measurements

    The following table summarizes measurement results for the different mechanical switches provided by this meter program.

    Board column contains measuring board name where
    - Nano - Arduino Nano board with 16 MHz Atmega328p MCU.

    Switch columns contains nominal switch identifier where
    - BB - button from arduino set (blue button).
    - OS - Old wall switch
    - NS - New wall switch
    - NL - New wall switch on long wire

    Offset column contains number of TCNT1 overflows occured before latest captured value. Capture column contains latest captured TCNT1 value. Overall measured bounce time in timer clocks (Offset*0xFFFF+Capture) is in the Time column. Counter column contains number of measuring timer cycles (full cycles from 0 to 0xFFFF). Two last empty sampling cycles stops measurement process. N column contains measurement number for corresponding switch (usually 11 measurement per switch). Column \f$t\f$ contains bounce time in seconds calculated according to frequency of MCU clock.
    
    \f{tabular} {|l|r|r|r|r|l|r|r|}
    \hline
    \multicolumn{1}{|c|}{Board}&\multicolumn{1}{|c|}{Offset}&\multicolumn{1}{|c|}{Capture}&\multicolumn{1}{|c|}{Time}&\multicolumn{1}{|c|}{Counter}&\multicolumn{1}{|c|}{Switch}&\multicolumn{1}{|c|}{N}&\multicolumn{1}{|c|}{$t$}\\
    \hline
    Nano	& 1	 & 3699		& 69234		& 4		 & BB	 & 1	& 0.0043271	\\
    Nano	& 1	 & 3798		& 69333		& 4		 & BB	 & 2	& 0.0043333	\\
    Nano	& 0	 & 8924		& 8924		& 3		 & BB	 & 3	& 0.0005578	\\
    Nano	& 1	 & 1408		& 66943		& 3		 & BB	 & 4	& 0.0041839	\\
    Nano	& 0	 & 16811	& 16811		& 3		 & BB	 & 5	& 0.0010507	\\
    Nano	& 1	 & 3812		& 69347		& 4		 & BB	 & 6	& 0.0043342	\\
    Nano	& 0	 & 56078	& 56078		& 3		 & BB	 & 7	& 0.0035049	\\
    Nano	& 0	 & 27587	& 27587		& 3		 & BB	 & 8	& 0.0017242	\\
    Nano	& 1	 & 3463		& 68998		& 4		 & BB	 & 9	& 0.0043124	\\
    Nano	& 1	 & 3453		& 68988		& 4		 & BB	 & 10	& 0.0043118	\\
    Nano	& 0	 & 8611		& 8611		& 3		 & BB	 & 11	& 0.0005382	\\
    \hline
    Nano	& 1	 & 42724	& 108259	& 4		 & OS	 & 1	& 0.0067662	\\
    Nano	& 1	 & 30771	& 96306		& 4		 & OS	 & 2	& 0.0060191	\\
    Nano	& 1	 & 36352	& 101887	& 4		 & OS	 & 3	& 0.0063679	\\
    Nano	& 1	 & 49801	& 115336	& 4		 & OS	 & 4	& 0.0072085	\\
    Nano	& 1	 & 37598	& 103133	& 4		 & OS	 & 5	& 0.0064458	\\
    Nano	& 3	 & 31336	& 227941	& 5		 & OS	 & 6	& 0.0142463	\\
    Nano	& 2	 & 26641	& 157711	& 5		 & OS	 & 7	& 0.0098569	\\
    Nano	& 7	 & 43357	& 502102	& 9		 & OS	 & 8	& 0.0313814	\\
    Nano	& 3	 & 12365	& 208970	& 5		 & OS	 & 9	& 0.0130606	\\
    Nano	& 1	 & 7693		& 73228		& 4		 & OS	 & 10	& 0.0045768	\\
    Nano	& 1	 & 25969	& 91504		& 4		 & OS	 & 11	& 0.0057190	\\
    \hline
    Nano	& 1	 & 24964	& 90499		& 4		 & NS	 & 1	& 0.0056562	\\
    Nano	& 1	 & 439		& 65974		& 3		 & NS	 & 2	& 0.0041234	\\
    Nano	& 1	 & 22928	& 88463		& 4		 & NS	 & 3	& 0.0055289	\\
    Nano	& 1	 & 590		& 66125		& 4		 & NS	 & 4	& 0.0041328	\\
    Nano	& 1	 & 22486	& 88021		& 4		 & NS	 & 5	& 0.0055013	\\
    Nano	& 1	 & 169		& 65704		& 4		 & NS	 & 6	& 0.0041065	\\
    Nano	& 1	 & 21315	& 86850		& 4		 & NS	 & 7	& 0.0054281	\\
    Nano	& 1	 & 166		& 65701		& 4		 & NS	 & 8	& 0.0041063	\\
    Nano	& 1	 & 17127	& 82662		& 4		 & NS	 & 9	& 0.0051664	\\
    Nano	& 0	 & 23309	& 23309		& 3		 & NS	 & 10	& 0.0014568	\\
    Nano	& 1	 & 20379	& 85914		& 4		 & NS	 & 11	& 0.0053696	\\
    \hline
    Nano	& 1	 & 6098		& 71633		& 4		 & NL	 & 1	& 0.0044771	\\
    Nano	& 1	 & 11572	& 77107		& 4		 & NL	 & 2	& 0.0048192	\\
    Nano	& 1	 & 11193	& 76728		& 4		 & NL	 & 3	& 0.0047955	\\
    Nano	& 1	 & 11391	& 76926		& 4		 & NL	 & 4	& 0.0048079	\\
    Nano	& 1	 & 11904	& 77439		& 4		 & NL	 & 5	& 0.0048399	\\
    Nano	& 1	 & 12592	& 78127		& 4		 & NL	 & 6	& 0.0048829	\\
    Nano	& 1	 & 31		& 65566		& 3		 & NL	 & 7	& 0.0040979	\\
    Nano	& 1	 & 4465		& 70000		& 4		 & NL	 & 8	& 0.0043750	\\
    Nano	& 1	 & 644		& 66179		& 3		 & NL	 & 9	& 0.0041362	\\
    Nano	& 1	 & 5040		& 70575		& 4		 & NL	 & 10	& 0.0044109	\\
    Nano	& 1	 & 11074	& 76609		& 4		 & NL	 & 11	& 0.0047881	\\
    \hline
    \f}

    Visualization of bounce time distribution around its mean value is shown here.

    \f{tikzpicture}
	\begin{axis}[
		ytick = {1, 2, 3, 4, 5, 6},
		yticklabels = {BB+OS+NS+NS, OS+NS+NL, NL, NS, OS, BB},
	]
		\addplot[
			boxplot prepared = {
			  median = 0.0055871,
			},
		]
		table [y index=0] {
			0.0043271
			0.0043333
			0.0005578
			0.0041839
			0.0010507
			0.0043342
			0.0035049
			0.0017242
			0.0043124
			0.0043118
			0.0005382
			0.0067662
			0.0060191
			0.0063679
			0.0072085
			0.0064458
			0.0142463
			0.0098569
			0.0313814
			0.0130606
			0.0045768
			0.0057190
			0.0056562
			0.0041234
			0.0055289
			0.0041328
			0.0055013
			0.0041065
			0.0054281
			0.0041063
			0.0051664
			0.0014568
			0.0053696
			0.0044771
			0.0048192
			0.0047955
			0.0048079
			0.0048399
			0.0048829
			0.0040979
			0.0043750
			0.0041362
			0.0044109
			0.0047881
		};
			
		\addplot[
			boxplot prepared = {
			  median = 0.0064441
			},
		]
		table [y index=0] {
			0.0067662
			0.0060191
			0.0063679
			0.0072085
			0.0064458
			0.0142463
			0.0098569
			0.0313814
			0.0130606
			0.0045768
			0.0057190
			0.0056562
			0.0041234
			0.0055289
			0.0041328
			0.0055013
			0.0041065
			0.0054281
			0.0041063
			0.0051664
			0.0014568
			0.0053696
			0.0044771
			0.0048192
			0.0047955
			0.0048079
			0.0048399
			0.0048829
			0.0040979
			0.0043750
			0.0041362
			0.0044109
			0.0047881
		};
			
		\addplot[
			boxplot prepared = {
			  median = 0.0045846
			},
		]
		table [y index=0] {
			0.0044771
			0.0048192
			0.0047955
			0.0048079
			0.0048399
			0.0048829
			0.0040979
			0.0043750
			0.0041362
			0.0044109
			0.0047881			
		};
		
		\addplot[
			boxplot prepared = {
			  median = 0.0045979
			},
		]
		table [y index=0] {
			0.0056562
			0.0041234
			0.0055289
			0.0041328
			0.0055013
			0.0041065
			0.0054281
			0.0041063
			0.0051664
			0.0014568
			0.0053696			
		};
		
		\addplot[
			boxplot prepared = {
			  median = 0.0101499
			},
		]
		table [y index=0] {
			0.0067662
			0.0060191
			0.0063679
			0.0072085
			0.0064458
			0.0142463
			0.0098569
			0.0313814
			0.0130606
			0.0045768
			0.0057190			
		};
		
		\addplot[
			boxplot prepared = {
				median = 0.0030162
			},
		]
		table [y index=0] {
			0.0043271
			0.0043333
			0.0005578
			0.0041839
			0.0010507
			0.0043342
			0.0035049
			0.0017242
			0.0043124
			0.0043118
			0.0005382			
		};
	\end{axis}
    \f}

    Calculation of some statistics summarized on the following table \anchor tau3sigma.

    \f{tabular} {|l|r|r|r|r|r|r|r|}
	\hline
	\multicolumn{1}{|c|}{Switch}&\multicolumn{1}{|c|}{$n$}&\multicolumn{1}{|c|}{$\bar{t}$}&\multicolumn{1}{|c|}{$t_{max}$}&\multicolumn{1}{|c|}{$D_c$}&\multicolumn{1}{|c|}{$\sigma_c$}&\multicolumn{1}{|c|}{$3\sigma_c$}&\multicolumn{1}{|c|}{$\bar{t}+3\sigma_c$} \\
	\hline
	BB		& 11	& 0.0030162	& 0.0043342	& 0.0000027868	& 0.0016694	& 0.0050081	& 0.0080244	\\
	OS		& 11	& 0.0101499	& 0.0313814	& 0.0000591995	& 0.0076941	& 0.0230824	& 0.0332322	\\
	NS		& 11	& 0.0045979	& 0.0056562	& 0.0000015202	& 0.0012330	& 0.0036989	& 0.0082967	\\
	NL		& 11	& 0.0045846	& 0.0048829	& 0.0000000869	& 0.0002947	& 0.0008842	& 0.0054688	\\
	OS+NS+NL	& 33	& 0.0064441	& 0.0313814	& 0.0000260830	& 0.0051072	& 0.0153215	& 0.0217656	\\
	BB+OS+NS+NL	& 44	& 0.0055871	& 0.0313814	& 0.0000223132	& 0.0047237	& 0.0141710	& 0.0197582	\\
	\hline
    \f}

Where 
    \f[
	\bar{t} = \frac{1}{n}\sum_{i=1}^{n}{t_i}
    \f]

    \f[
	t_{max} = \max_{1 \leq i \leq n} t_i
    \f]
    
    \f{equation}
	D_s = \frac{1}{n}\sum_{i=1}^{n}{(t_i - \bar{t})^2} = \frac{1}{n}\sum_{i=1}^{n}{(t_i^2 - 2t_i\bar{t} + \bar{t}^2)} = \\
	\frac{1}{n}\sum_{i=1}^{n}{t_i^2} - 2\bar{t}\frac{1}{n}\sum_{i=1}^{n}{t_i} + \bar{t}^2 = \\
	\frac{1}{n}\sum_{i=1}^{n}{t_i^2} - 2\bar{t}^2 + \bar{t}^2 = \\
	\frac{1}{n}\sum_{i=1}^{n}{t_i^2} -\bar{t}^2
    \f}

    \f{equation}
	D_c = \frac{1}{n-1}\sum_{i=1}^{n}{(t_i-\bar{t})^2} = \\
		\frac{1}{n-1}\sum_{i=1}^{n}{(t_i^2-2t_i\bar{t}+\bar{t}^2)} = \\
		\frac{1}{n-1}(\sum_{i=1}^{n}{t_i^2}-2\bar{t}\sum_{i=1}^{n}{t_i}+n\bar{t}^2) = \\
		\frac{n}{n-1}(\frac{1}{n}\sum_{i=1}^{n}{t_i^2}-\bar{t}^2)
    \f}

    \f[
	 D_c = \frac{n}{n-1}D_s
    \f]

    \f[
	  \sigma_c = \sqrt{D_c}
    \f]

Two simpliest circuits will be considered to connect switch to MCU. The first circuit pulls MCU pin down to ground and the second one pulls MCU pin to high voltage when switch is off. If the second circuit is used with builtin pin pullup resistor then voltage drop exists across resistor \f$R_2\f$ which depends on its resistance (high enough \f$R_2\f$ resitance leads to permanent hight level on pin). That's why all circuits are considered without any external pull resistors (external pull resistor is connected to pin before switch RC-filter). 

\image html PULLDOWN_SWITCH_CIRCUIT.png "Pulldown switch circuit"
\image latex PULLDOWN_SWITCH_CIRCUIT.pdf "Pulldown switch circuit"
\image html PULLUP_SWITCH_CIRCUIT.png "Pullup switch circuit"
\image latex PULLUP_SWITCH_CIRCUIT.pdf "Pullup switch circuit"

In the case of pull down circuit capacitor \f$C\f$ is charging through \f$R_2\f$ resistor and discharging through \f$R_1+R_2\f$ resistors. In the case of pull up circuit capacitor \f$C\f$ is charging through \f$R_1+R_2\f$ and discharging through \f$R_2\f$ resistors. So the shortest time of charging/discharging is defined by \f$R_2\f$ resistence and \f$C\f$ capacity. Longest time of charging/discharging is defined by \f$R_1+R_2\f$ resistence and \f$C\f$ capacity.
Time constant of capacitor (time of 63,8% charging or 38,2% discharging) is used to estimate charging/discharging time.

\f[
	\tau_s = CR_2
\f]

\f[
	\tau_l = C(R_1+R_2)
\f]

\f$\tau_s\f$ should by large enough to cover switch bounce time and in opposite \f$\tau_l\f$ should be small enough to avoid significant switch delay.
\f$R_1\f$ should be chosen to avoid high current consumed by the circuit when switch is closed.

Two main configurations of these circuits will be estimated. The first one is with relatively high resistance and low capacity and the second one is with low resistence and high capacity. In both cases only configurations where \f$R1>R2\f$ will be considered because if \f$R2>R1\f$ the capacitor does not discharge completely (capacitor is a second voltage source competing for the same ground as main voltage source - in the case of higher \f$R2\f$ resistance it loses).

Futher calculations are based on maximum estimated bounce time of 0.033 for OS switch (\ref tau3sigma "Bounce statistics").

Following table contains calculated by maximum estimated bounce time \f$\tau_{max}\f$ value \f$\bar{R_2}\f$, existing nearest standard resistance value \f$R_2\f$ and corresponding resistance capacitor time constant (\f$\tau\f$), 

\f{tabular} {|r|r|r|r|r|r|}
	\hline
	\multicolumn{1}{|c|}{$C$} & \multicolumn{1}{|c|}{$F$} & \multicolumn{1}{|c|}{$\tau_{max}$} & \multicolumn{1}{|c|}{$\bar{R_2}$} & \multicolumn{1}{|c|}{$R_2$} & \multicolumn{1}{|c|}{$\tau_s$}\\
	\hline
	10 nF		& 0.00000001	& 0.033		& 3323223.9	& 1000000	& 0.0100\\
	100 nF		& 0.0000001	& 0.033		& 332322.4	& 220000	& 0.0220\\
	1 $\mu F$	& 0.000001	& 0.033		& 33232.2	& 10000		& 0.0100\\
	3.3 $\mu F$	& 0.0000033	& 0.033		& 10070.4	& 10000		& 0.0330\\
	10 $\mu F$	& 0.00001	& 0.033		& 3323.2	& 2200		& 0.0220\\
	33 $\mu F$	& 0.000033	& 0.033		& 1007.0	& 1000		& 0.0330\\
	100 $\mu F$	& 0.0001	& 0.033		& 332.3		& 220		& 0.0220\\
	220 $\mu F$	& 0.00022	& 0.033		& 151.1		& 220		& 0.0484\\
	1000 $\mu F$	& 0.001		& 0.033		& 33.2		& 220		& 0.2200\\
	\hline
\f}

where \f$\bar{R_2} = \tau_{max}/F\f$ and \f$\tau = FR_2\f$.

Following combinations of \f$C\f$ and \f$R_2\f$ can be used to provide time constant not less than maximim estimated bounce time

\f{tabular} {|r|r|r|r|}
	\hline
	\multicolumn{1}{|c|}{$C$} & \multicolumn{1}{|c|}{$F$} & \multicolumn{1}{|c|}{$R_2$} & \multicolumn{1}{|c|}{$\tau_s$}\\
	\hline
	3.3 $\mu F$	& 0.0000033	& 10000		& 0.0330\\
	33 $\mu F$	& 0.000033	& 1000		& 0.0330\\
	220 $\mu F$	& 0.00022	& 220		& 0.0484\\
	\hline
\f}

For these combinations and \f$R_1\f$ value greater then 10 kOhm (for pull resister) following \f$\tau_l\f$ are calculted

\f{tabular} {|r|r|r|r|r|}
	\hline
	\multicolumn{1}{|c|}{$C$} & \multicolumn{1}{|c|}{$F$} & \multicolumn{1}{|c|}{$R_2$} & \multicolumn{1}{|c|}{$R_1$} & \multicolumn{1}{|c|}{$\tau_l$} \\
	\hline
	3.3 $\mu F$	& 0.0000033	& 10000	& 10000		& 0.066		\\
	3.3 $\mu F$	& 0.0000033	& 10000	& 100000	& 0.363		\\
	33 $\mu F$	& 0.000033	& 1000	& 10000		& 0.363		\\
	3.3 $\mu F$	& 0.0000033	& 10000	& 220000	& 0.759		\\
	3.3 $\mu F$	& 0.0000033	& 10000	& 470000	& 1.584		\\
	220 $\mu F$	& 0.00022	& 220	& 10000		& 2.2484	\\
	3.3 $\mu F$	& 0.0000033	& 10000	& 1000000	& 3.333		\\
	33 $\mu F$	& 0.000033	& 1000	& 100000	& 3.333		\\
	33 $\mu F$	& 0.000033	& 1000	& 220000	& 7.293		\\
	33 $\mu F$	& 0.000033	& 1000	& 470000	& 15.543	\\
	220 $\mu F$	& 0.00022	& 220	& 100000	& 22.0484	\\
	33 $\mu F$	& 0.000033	& 1000	& 1000000	& 33.033	\\
	220 $\mu F$	& 0.00022	& 220	& 220000	& 48.4484	\\
	220 $\mu F$	& 0.00022	& 220	& 470000	& 103.4484	\\
	220 $\mu F$	& 0.00022	& 220	& 1000000	& 220.0484	\\
	\hline
\f}

So if \f$C = 3.3\,\mu F\f$, \f$R_1 = 100\,kOhm\f$ and \f$R_2 = 10\,kOhm\f$ are used circuits will  provide \f$\tau_s=0.033\,s\f$ and \f$\tau_l=0.363\,s\f$.


*/
#include "task.h"

/** Contact bounce meter. */
class contact_bounce_meter:public task
{
  static const uint8_t Pin = 8;	///< Arduino Nano Atmega328p input capture pin number - PB0(PCINT0/CLKO/ICP1)
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
    pinMode(Pin, INPUT_PULLUP); // Pullup input catpture pin.
    
    PCICR |= _BV(PCIE0);	// Enable PCIE0 interrupts.

    TCCR1A = 0x00;		// Timer operating mode normal (non-PWM).
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


