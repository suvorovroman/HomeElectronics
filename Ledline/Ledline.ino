/*
 *	0.1	Switch on/off with dimming effect.
 *	1.0	Brightness adjustment (NEC:0 only)
 *	1.1	Exponential brightness adjustment, constant speed.
 *	1.2	Shortened brightness transition time (1024 ms).
 *		4 brightness transition levels.
 *		Integral (integer)  gamma correction. 
 *		Toggle maximizes brightness.
 *	1.3	Task scheduler with normal and ISR queue.
 *		Interrupt to start IrReceiver decoding loop.
 *	1.4	Toggle button task started by PCINT (no polling).
 *		Idle mode when schedulers queues are empty.
 *		8 brightness levels .
 *	1.5	Pull up RXD to avoid noise when USB disconnected.
 *		Serial port is connected only in tracing tasks.
 *		IR receiving process is stoped until pin interrupt.
 *		Plus and Minus adjust brightness by 1.
 *		4 preset brightness levels with Next/Prev buttons
 */
#define VERSION 0x0105

#ifdef PICSIMLAB
#define	SLEEP	false
#warning "Disable MCU sleep mode at idle time. PICSimLab incorrectly simulates PWM in sleep mode."
#else
#define SLEEP	true
#warning "Enable MCU sleep mode at idle time."
#endif

#include "task.h"

task::scheduler Scheduler;

void loop() {
  // put your main code here, to run repeatedly:
  Scheduler.execute(SLEEP);
}

/** Task which can trace information into serial port. */
class traceable:public task
{
public:

  /** Defines traceable routine. */
  virtual void trace() noexcept = 0;

private:

  void operator()()
  {
    Serial.begin(9600);
    trace();
    Serial.end();
  }
  
};

#include <IRremote.hpp>

// IR remote control CAR MP3
// Protocol = NEC Address = 0x0
#define CAR_ADDRESS		0x0000
#define CAR_BUTTON_CH_PREV	0x45     
#define CAR_BUTTON_CH		0x46    
#define CAR_BUTTON_CH_NEXT	0x47    
#define CAR_BUTTON_PREV		0x44
#define CAR_BUTTON_NEXT		0x40
#define CAR_BUTTON_PLAY		0x43
#define CAR_BUTTON_MINUS	0x7
#define CAR_BUTTON_PLUS		0x15
#define CAR_BUTTON_EQ		0x9
#define CAR_BUTTON_0		0x16
#define CAR_BUTTON_100_PLUS	0x19
#define CAR_BUTTON_200_PLUS	0xD
#define CAR_BUTTON_1		0xC
#define CAR_BUTTON_2		0x18
#define CAR_BUTTON_3		0x5E
#define CAR_BUTTON_4		0x8
#define CAR_BUTTON_5		0x1C
#define CAR_BUTTON_6		0x5A
#define CAR_BUTTON_7		0x42
#define CAR_BUTTON_8		0x52
#define CAR_BUTTON_9		0x4A

// IR remote control NEBULA
// Protocol = NEC Address = 0xFD07
#define NEB_ADDRESS		0xFD07
#define NEB_BUTTON_ONOFF	0xA
#define NEB_BUTTON_SCAFF	0xB
#define NEB_BUTTON_NEBULA	0x3
#define NEB_BUTTON_B_PLUS	0x1
#define NEB_BUTTON_B_MINUS	0x5
#define NEB_BUTTON_S_PLUS	0x4
#define NEB_BUTTON_S_MINUS	0x2
#define NEB_BUTTON_LA_MINUS	0xC
#define NEB_BUTTON_LA_PLUS	0xD
#define NEB_BUTTON_LA_MODE	0xE
#define NEB_BUTTON_LA_ONOFF	0xF
#define NEB_BUTTON_RESET	0X1A

// IR remote control Polaris
// Protocol = PulseDistance
// RawData
#define POLARIS_BUTTON_ON_OFF 0x3B2 

#define PIN_CONTROL	3	// PD3(PCINT19/OC2B/INT1)
#define PIN_LEDLINE	5	// PD5(PCINT21/OC0B/T1)   
#define PIN_TOGGLE	6	// PD6(PCINT22/OC0A/AIN0)
#define PIN_RXD		0	// PD0(RXD/PCINT16)

#include "udiv.h"

/** Ledline. */
class ledline
{
public:

  /** Brightness level. */
  typedef uint8_t brightness;
  
  static const uint8_t MinLevel	= 0x00;	//< Minimum brightness level.
  static const uint8_t MaxLevel	= 0xFF;	//< Maximum brightness level.
  
  /**  Preset brightness. */
  enum class preset
    {
      No,	//< No illumination.
      Night,	//< To find a way.
      Twilight,	//< To see everythin in general.
      Day,	//< To read.
      Bound	//< Upper bound
    };

  ledline():Level(MinLevel), Dimm(*this)
  {
    (*this)[preset::No]		= 0x00;
    (*this)[preset::Night]	= 0x01;
    (*this)[preset::Twilight]	= (MaxLevel - MinLevel) >> 1;
    (*this)[preset::Day]	= MaxLevel;
  }

  /** Mount ledline on specified PWM pin. */
  void mount(uint8_t _pin)
  {
    Pin = _pin;
    pinMode(Pin, OUTPUT);
  }

  /** Get current brightness level.
      \return Current brightness level.
  */
  operator uint8_t() const {return Level;}

  /** Set current brightness level
      \param[in] _level		Brightness level.
      \return Modified ledline object reference.
  */
  ledline& operator =(uint8_t _level)
  {
    Level = _level;
    analogWrite(Pin, gamma(Level));
    return *this;
  }

  /** Increase current ledline brightness level by 1.
      If current level becomes higher than MaxLevel it rounds to MinLevel.
  */
  void operator ++()
  {
    *this = ++Level;
  }
  
  /** Decrese current brightness level by 1.
      If current level becomes lower than MinLevel it rouns to MaxLevel.
  */
  void operator--()
  {
    *this = --Level;
  }

  /** Sets/gets preset mode brightness.
      \param[in] _mode	Preset mode.
      \return Preset mode brightness.
  */
  brightness& operator[](preset _mode)
  {
    return Preset[static_cast<uint8_t>(_mode)];
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
     *    g(z)=\lfloor\frac{v(z)}{255}\rfloor-1+\begin{cases}1 & v(z)\pmod{255} > n \\0 & \text{otherwise}\end{cases}
     *  \f]
     *
     Here \f$n\f$ is a rounding mode parameter. If \f$n = 0\f$ then \f$g(z)\f$ is rounded up, if \f$n = 255\f$ then \f$g(z)\f$
     is rounded down and any value \f$0 < n < 255\f$ means intermediate round up mode - for example \f$n = 127\f$ may be considered
     as a rounding half up mode.
     The following table shows complete duty cycle calculation for different \f$n\f$.

	|Level	|n=255	|n=127	|n=0	|
	|:---	|:---	|:---	|:---	|
	|0	|0	|0	|0	|    
	|1	|0	|0  	|1	|
	|2	|0	|0  	|1	|  
	|3	|0	|0  	|1	|  
	|4	|0	|0  	|1	|  
	|5	|0	|0  	|1	|
	|6	|0	|0  	|1	|
	|7	|0	|0	|1	|  
	|8	|0	|0	|1	|  
	|9	|0	|0	|1	|  
	|10	|0	|0	|1	|  
	|11	|0	|0	|1	|  
	|12	|0	|0	|1	|  
	|13	|0	|0	|1	|  
	|14	|0	|0	|1	|  
	|15	|0	|0	|1	|  
	|16	|0	|1	|1	|  
	|17	|0	|1	|1	|  
	|18	|0	|1	|1	|  
	|19	|0	|1	|1	|  
	|20	|0	|1	|1	|  
	|21	|0	|1	|1	|  
	|22	|0	|1	|1	|  
	|23	|0	|1	|1	|  
	|24	|0	|1	|1	|  
	|25	|0	|1	|1	|  
	|26	|0	|1	|1	|  
	|27	|0	|1	|1	|  
	|28	|0	|1	|1	|  
	|29	|0	|1	|1	|  
	|30	|0	|1	|1	|  
	|31	|0	|1	|1	|  
	|32	|1	|1	|2	|  
	|33	|1	|1	|2	|  
	|34	|1	|1	|2	|  
	|35	|1	|1	|2	|  
	|36	|1	|1	|2	|  
	|37	|1	|1	|2	|  
	|38	|1	|1	|2	|  
	|39	|1	|1	|2	|  
	|40	|1	|2	|2	|  
	|41	|1	|2	|2	|  
	|42	|1	|2	|2	|  
	|43	|1	|2	|2	|  
	|44	|1	|2	|2	|  
	|45	|1	|2	|2	|  
	|46	|1	|2	|2	|  
	|47	|1	|2	|2	|  
	|48	|3	|2	|3	|  
	|49	|3	|2	|3	|  
	|50	|3	|2	|3	|  
	|51	|3	|2	|3	|  
	|52	|3	|2	|3	|  
	|53	|3	|2	|3	|  
	|54	|3	|2	|3	|  
	|55	|3	|2	|3	|  
	|56	|3	|3	|3	|  
	|57	|3	|3	|3	|  
	|58	|3	|3	|3	|  
	|59	|3	|3	|3	|  
	|60	|3	|3	|3	|  
	|61	|3	|3	|3	|  
	|62	|3	|3	|3	|  
	|63	|3	|3	|3	|  
	|64	|3	|3	|4	|  
	|65	|3	|3	|4	|  
	|66	|3	|3	|4	|  
	|67	|3	|3	|4	|  
	|68	|3	|4	|4	|  
	|69	|3	|4	|4	|  
	|70	|3	|4	|4	|  
	|71	|3	|4	|4	|  
	|72	|5	|4	|5	|  
	|73	|5	|4	|5	|  
	|74	|5	|4	|5	|  
	|75	|5	|4	|5	|  
	|76	|5	|5	|5	|  
	|77	|5	|5	|5	|  
	|78	|5	|5	|5	|  
	|79	|5	|5	|5	|  
	|80	|5	|5	|6	|  
	|81	|5	|5	|6	|  
	|82	|5	|5	|6	|  
	|83	|5	|5	|6	|  
	|84	|5	|6	|6	|  
	|85	|5	|6	|6	|  
	|86	|5	|6	|6	|  
	|87	|5	|6	|6	|  
	|88	|7	|6	|7	|  
	|89	|7	|6	|7	|  
	|90	|7	|6	|7	|  
	|91	|7	|6	|7	|  
	|92	|7	|7	|7	|  
	|93	|7	|7	|7	|  
	|94	|7	|7	|7	|  
	|95	|7	|7	|7	|  
	|96	|7	|7	|8	|  
	|97	|7	|7	|8	|  
	|98	|7	|8	|8	|  
	|99	|7	|8	|8	|  
	|100	|9	|8	|9	|  
	|101	|9	|8	|9	|  
	|102	|9	|9	|9	|  
	|103	|9	|9	|9	|  
	|104	|9	|9	|10	| 
	|105	|9	|9	|10	| 
	|106	|9	|10	|10	| 
	|107	|9	|10	|10	| 
	|108	|11	|10	|11	| 
	|109	|11	|10	|11	| 
	|110	|11	|11	|11	| 
	|111	|11	|11	|11	| 
	|112	|11	|11	|12	| 
	|113	|11	|11	|12	| 
	|114	|11	|12	|12	| 
	|115	|11	|12	|12	| 
	|116	|13	|12	|13	| 
	|117	|13	|12	|13	| 
	|118	|13	|13	|13	| 
	|119	|13	|13	|13	| 
	|120	|13	|13	|14	| 
	|121	|13	|13	|14	| 
	|122	|13	|14	|14	| 
	|123	|13	|14	|14	| 
	|124	|15	|14	|15	| 
	|125	|15	|14	|15	| 
	|126	|15	|15	|15	| 
	|127	|15	|15	|15	| 
	|128	|15	|15	|16	| 
	|129	|15	|16	|16	| 
	|130	|17	|16	|17	| 
	|131	|17	|17	|17	| 
	|132	|17	|17	|18	| 
	|133	|17	|18	|18	| 
	|134	|19	|18	|19	| 
	|135	|19	|19	|19	| 
	|136	|19	|19	|20	| 
	|137	|19	|20	|20	| 
	|138	|21	|20	|21	| 
	|139	|21	|21	|21	| 
	|140	|21	|21	|22	| 
	|141	|21	|22	|22	| 
	|142	|23	|22	|23	| 
	|143	|23	|23	|23	| 
	|144	|23	|23	|24	| 
	|145	|23	|24	|24	| 
	|146	|25	|24	|25	| 
	|147	|25	|25	|25	| 
	|148	|25	|25	|26	| 
	|149	|25	|26	|26	| 
	|150	|27	|26	|27	| 
	|151	|27	|27	|27	| 
	|152	|27	|27	|28	| 
	|153	|27	|28	|28	| 
	|154	|29	|28	|29	| 
	|155	|29	|29	|29	| 
	|156	|29	|29	|30	| 
	|157	|29	|30	|30	| 
	|158	|31	|30	|31	| 
	|159	|31	|31	|31	| 
	|160	|31	|32	|32	| 
	|161	|33	|33	|33	| 
	|162	|33	|34	|34	| 
	|163	|35	|35	|35	| 
	|164	|35	|36	|36	| 
	|165	|37	|37	|37	| 
	|166	|37	|38	|38	| 
	|167	|39	|39	|39	| 
	|168	|39	|40	|40	| 
	|169	|41	|41	|41	| 
	|170	|41	|42	|42	| 
	|171	|43	|43	|43	| 
	|172	|43	|44	|44	| 
	|173	|45	|45	|45	| 
	|174	|45	|46	|46	| 
	|175	|47	|47	|47	| 
	|176	|47	|48	|48	| 
	|177	|49	|49	|49	| 
	|178	|49	|50	|50	| 
	|179	|51	|51	|51	| 
	|180	|51	|52	|52	| 
	|181	|53	|53	|53	| 
	|182	|53	|54	|54	| 
	|183	|55	|55	|55	| 
	|184	|55	|56	|56	| 
	|185	|57	|57	|57	| 
	|186	|57	|58	|58	| 
	|187	|59	|59	|59	| 
	|188	|59	|60	|60	| 
	|189	|61	|61	|61	| 
	|190	|61	|62	|62	| 
	|191	|63	|63	|63	| 
	|192	|65	|65	|65	| 
	|193	|67	|67	|67	| 
	|194	|69	|69	|69	| 
	|195	|71	|71	|71	| 
	|196	|73	|73	|73	| 
	|197	|75	|75	|75	| 
	|198	|77	|77	|77	| 
	|199	|79	|79	|79	| 
	|200	|81	|81	|81	| 
	|201	|83	|83	|83	| 
	|202	|85	|85	|85	| 
	|203	|87	|87	|87	| 
	|204	|89	|89	|89	| 
	|205	|91	|91	|91	| 
	|206	|93	|93	|93	| 
	|207	|95	|95	|95	| 
	|208	|97	|97	|97	| 
	|209	|99	|99	|99	| 
	|210	|101	|101	|101	|
	|211	|103	|103	|103	|
	|212	|105	|105	|105	|
	|213	|107	|107	|107	|
	|214	|109	|109	|109	|
	|215	|111	|111	|111	|
	|216	|113	|113	|113	|
	|217	|115	|115	|115	|
	|218	|117	|117	|117	|
	|219	|119	|119	|119	|
	|220	|121	|121	|121	|
	|221	|123	|123	|123	|
	|222	|125	|125	|125	|
	|223	|127	|127	|127	|
	|224	|131	|131	|131	|
	|225	|135	|135	|135	|
	|226	|139	|139	|139	|
	|227	|143	|143	|143	|
	|228	|147	|147	|147	|
	|229	|151	|151	|151	|
	|230	|155	|155	|155	|
	|231	|159	|159	|159	|
	|232	|163	|163	|163	|
	|233	|167	|167	|167	|
	|234	|171	|171	|171	|
	|235	|175	|175	|175	|
	|236	|179	|179	|179	|
	|237	|183	|183	|183	|
	|238	|187	|187	|187	|
	|239	|191	|191	|191	|
	|240	|195	|195	|195	|
	|241	|199	|199	|199	|
	|242	|203	|203	|203	|
	|243	|207	|207	|207	|
	|244	|211	|211	|211	|
	|245	|215	|215	|215	|
	|246	|219	|219	|219	|
	|247	|223	|223	|223	|
	|248	|227	|227	|227	|
	|249	|231	|231	|231	|
	|250	|235	|235	|235	|
	|251	|239	|239	|239	|
	|252	|243	|243	|243	|
	|253	|247	|247	|247	|
	|254	|251	|251	|251	|
	|255	|255	|255	|255	|

	Rounding down and rounding half up mode will lead to zero brightness level (swiched off led line) in spite of positive dimming level near to
	its zero values (from 0 to 31 dimming level). It is not convinient when brightness is corrected manually (correction continues
	when led line is off). So more suitable is rounding up mode with \f$n=0\f$. It makes non-zero duty cycle for all non-zero dimming levels.
	Here graphs of gamma function for differen \f$n\f$ near to zero level.
	
	\f{tikzpicture}
	\begin{axis}
         [
           enlargelimits=true,
	   legend entries = {{$n=255$}, {$n=127$}, {$n=0$}},
	   legend pos = north west
         ]
           \addplot[const plot, dotted]
	   table
	   {
		x	y
		0	0      
		1      	0      
		2      	0      
		3      	0      
		4      	0      
		5      	0      
		6      	0      
		7      	0      
		8      	0      
		9      	0      
		10     	0      
		11     	0      
		12     	0      
		13     	0      
		14     	0      
		15     	0      
		16     	0      
		17     	0      
		18     	0      
		19     	0      
		20     	0      
		21     	0      
		22     	0      
		23     	0      
		24     	0      
		25     	0      
		26     	0      
		27     	0      
		28     	0      
		29     	0      
		30     	0      
		31     	0      
		32     	1      
	   };
           \addplot[const plot, dashed]
	   table
	   {
		x	y
		0	0            
		1      	0        
		2      	0        
		3      	0        
		4      	0        
		5      	0        
		6      	0        
		7      	0            
		8      	0            
		9      	0            
		10     	0            
		11     	0            
		12     	0            
		13     	0            
		14     	0            
		15     	0            
		16     	1            
		17     	1            
		18     	1            
		19     	1            
		20     	1            
		21     	1            
		22     	1            
		23     	1            
		24     	1            
		25     	1            
		26     	1            
		27     	1            
		28     	1            
		29     	1            
		30     	1            
		31     	1            
		32     	1            
	   };   
           \addplot[const plot]
	   table
	   {
		x	y
		0	0	            
		1      	1        
		2      	1        
		3      	1        
		4      	1        
		5      	1        
		6      	1        
		7      	1            
		8      	1            
		9      	1            
		10     	1            
		11     	1            
		12     	1            
		13     	1            
		14     	1            
		15     	1            
		16     	1            
		17     	1            
		18     	1            
		19     	1            
		20     	1            
		21     	1            
		22     	1            
		23     	1            
		24     	1            
		25     	1            
		26     	1            
		27     	1            
		28     	1            
		29     	1            
		30     	1            
		31     	1            
		32     	2            
	   };   
	   \end{axis}
       \f}

       

       \param[in]	_z	Dimming level (linear dimming control).
       \param[in]	_r	Rounding parameter
       \return Corrected duty cycle corresponding to specified dimming level.
 */
  uint8_t gamma(uint8_t _z, uint8_t _r = 0)
  {
    udiv8_t d;

    d = udiv(_z << 3, 255);
    d = udiv((1 << d.quot)*(d.rem + 255), 255);
    return d.quot - 1 + (d.rem > _r ? 1:0);
  }

private:

  uint8_t	Level;	//< Current ledline control pin PWМ level.
  uint8_t	Pin:4;	//< Ledline control pin number.
  
  brightness	Preset[(uint8_t)preset::Bound];	//< Preset level brightness (intermediate only).
  
  /** Ledline driver task.
      Task with a reference to specific ledline.
  */
  class task:public ::task
  {
  public:

    task(ledline &_ledline):Ledline(_ledline){}

  protected:

    ledline &Ledline;
  };

public:
  
  /** Dimming task. */
  class dimm:public task
  {
  public:

    dimm(ledline &_ledline):task(_ledline){}

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
	      Ledline = l;
	    }
	  else
	    if(dl < 0)
	      {
		if(l > Level)
		  Scheduler.put(this);
		else
		  if(l < Level)
		    l = Level;
		Ledline = l;
	      }

	  Time += dt;
	  Stat.Time += dt;

	  if(Level == l)
	    {
	      Stat.print();
	    }
	}
    }

    void invoke(uint8_t _level)
    {
      if(_level != Ledline.Level && Scheduler.put(this))
	{
	  Level = _level;
	  Distance = Level - Ledline.Level;
	  Time = millis();
	  Stat.Time = Stat.Count = 0;
	}
    }

    /** \brief Toggle ledline. */
    void toggle()
    {
      invoke(Ledline.Level ? Ledline.MinLevel:Ledline.MaxLevel);
    }

    /** Dimm to preset mode brightness.
	\param[in] _mode	Preset mode.
    */
    void set(preset _mode)
    {
       invoke(Ledline[_mode]);
    }

    /** Switch to the next higher preset brightness level.
	\return Next higher preset level.
    */
    preset next()
    {
      preset mode = (preset)((uint8_t)preset::No + 1);
      while((uint8_t)mode < (uint8_t)(preset::Bound) - 1 && Ledline[mode] <= Ledline)
	mode = (preset)((uint8_t)mode + 1);
      invoke(Ledline[mode]);
      return mode;
    }

    /** Switch to the previous lower preset briughtness level.
	\return Previous lower preset mode.
    */
    preset prev()
    {
      preset mode = (preset)((uint8_t)preset::Bound - 2);
      while((uint8_t)mode > (uint8_t)(preset::No) && Ledline[mode] >= Ledline)
	mode = (preset)((uint8_t)mode - 1);
      invoke(Ledline[mode]);
      return mode;
    }

    private:

      int Level;            //< Target level.
      int Distance;         //< Distance between target and source levels.
      unsigned long Time;   //< Previouse timestamp.

      static const int LevelingTime = 1024; //< Transition time between to different brightness levels in milliseconds.

      struct stat:public traceable
      {
	uint16_t Count;
	uint16_t Time;

	void trace() noexcept
	{
	  Serial.print(F("Count:\t")); Serial.print(Count); Serial.print('\t');
	  Serial.print(F("Time:\t")); Serial.print(Time); Serial.print('\t');
	  Serial.print(F("Rate:\t")); Serial.print(Count/Time); Serial.println();
	}

	/** Print dimming statistics. */
	void print()
	{
	  static stat t;
	  /*
	    This task scheduled only from one another task.
	    So it can not interfere with other own instances.
	  */
	  Scheduler.put(&t);
	  t.Count = Count;
	  t.Time = Time;
	}
	
      } Stat;
  } Dimm;

} Ledline;

/** IR remote control. */
class ir_remote_control
{
public:

  /** Read control data.
      Iterative task which tests IR control until meaningfull data occurs.
   */
  class input:public traceable
  {
  private:
  
    void trace() noexcept
    {
      if(IrReceiver.decode())
	{
	  if(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW)
	    Serial.println(F("Overflow detected"));
	  else
	    {
	      IrReceiver.printIRResultShort(&Serial);
	      IrReceiver.printIRSendUsage(&Serial);
	      if(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)
		Serial.println(F("Repeat received"));
	    }

	  /* Commend has been received.
	     Stop receiving process until next receiver pin interrupt. */
	  IrReceiver.stop();
  
	  if(IrReceiver.decodedIRData.protocol == NEC)
	    {
	      if(IrReceiver.decodedIRData.address == CAR_ADDRESS)
		switch(IrReceiver.decodedIRData.command)
		  {
		  case CAR_BUTTON_MINUS:
		    --Ledline;
		    break;
		  case CAR_BUTTON_PLUS:
		    ++Ledline;
		    break;
		  case CAR_BUTTON_0:
		    Ledline.Dimm.set(ledline::preset::No);
		    break;
		  case CAR_BUTTON_1:
		    Ledline.Dimm.set(ledline::preset::Night);
		    break;
		  case CAR_BUTTON_2:
		    Ledline.Dimm.set(ledline::preset::Twilight);
		    break;
		  case CAR_BUTTON_3:
		    Ledline.Dimm.set(ledline::preset::Day);
		    break;
		  case CAR_BUTTON_NEXT:
		    Ledline.Dimm.next();
		    break;
		  case CAR_BUTTON_PREV:
		    Ledline.Dimm.prev();
		    break;
		  default:
		    Ledline.Dimm.toggle();
		  }
	      else
		if(IrReceiver.decodedIRData.address == NEB_ADDRESS)
		  switch(IrReceiver.decodedIRData.command)
		    {
		    default:
		    case NEB_BUTTON_ONOFF:
		      Ledline.Dimm.toggle();
		      break;
		    case NEB_BUTTON_B_PLUS:
		      ++Ledline;
		      break;
		    case NEB_BUTTON_B_MINUS:
		      --Ledline;
		    case NEB_BUTTON_S_PLUS:
		      Ledline.Dimm.next();
		      break;
		    case NEB_BUTTON_S_MINUS:
		      Ledline.Dimm.prev();
		      break;
		    }
	    }
	  else
	    if(IrReceiver.decodedIRData.protocol != UNKNOWN)
	      Ledline.Dimm.toggle();
	}
      else      
        Scheduler.put(this); 
    }

  };

  /** Start IrReceive listening from ISR. */
  class listen:public task
  {
  public:

    /** ISR for IR control pin interrupt. */
    static void ISRoutine()
    {
      static listen t;
      Scheduler.putISR(&t);
    }
    
  private:

    /** Start listening from main process. */
    void operator()()
    {
      static input t;
      Scheduler.put(&t);
      /* Start IR receiving process before input task begins. */
      IrReceiver.start();
    }

  };

};

/** Test task.

    Test task is used for different testing purposes. It should be modified for each specific case.
*/
class test:public traceable
{
public:

  static void invokeISR()
  {
    static test t;
    Scheduler.putISR(&t);
  }

private:

  void trace() noexcept
  {
    if(digitalRead(PIN_TOGGLE) == LOW)
      {
	if(Ledline == ledline::MinLevel)
	  Ledline.Dimm.toggle();
	else
	 Ledline.Dimm.prev();
      }
   }
  
};

/** Entry point. */
class begin:public traceable
{
public:

  static void invoke()
  {
    static begin t;
    Scheduler.put(&t);
  }
  
private:

  void trace() noexcept
  {
    version();

    /*
      Configure IR receiver pin and stop receiving process until pin interrupt.
      Control pin here is set in intput mode without pullup. 
      If its neccessary external pullup resister should be used or
      different or self made reciever library.
     */
    IrReceiver.begin(PIN_CONTROL, DISABLE_LED_FEEDBACK);
    IrReceiver.stop();
    /* Attach IR receiver pin interrupt to start receiving process.*/
    attachInterrupt(digitalPinToInterrupt(PIN_CONTROL), ir_remote_control::listen::ISRoutine, FALLING);

    Serial.println(F("Ready to receive IR signals of protocols:"));
    printActiveIRProtocols(&Serial);
    Serial.println();

    Ledline.mount(PIN_LEDLINE);
   
    pinMode(PIN_TOGGLE, INPUT_PULLUP);
    PCICR = _BV(PCIE2);
    PCMSK2 = _BV(PCINT22);

    /* Pullup serial receive data pin to avoid noise wher it is disconnected. */
    pinMode(PIN_RXD, INPUT_PULLUP);
  }

  void version()
  {
    Serial.print('V'); 
    Serial.print(':');
    Serial.print(VERSION >> 8);
    Serial.print('.');
    Serial.println(VERSION&0x0F);
  }
};

void setup() {
  // put your setup code here, to run once:
  begin::invoke();
}

/** Test button interrupt processing.

    This interrupt is used for starting different testing routines. Test task should be
    modified for each specific test.
*/
ISR(PCINT2_vect)
{
  test::invokeISR();
}
