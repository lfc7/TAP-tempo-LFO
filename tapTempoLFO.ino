/*v1.0 tested 

   TAP-tempo LFO

   ( designed for arduino nano ATmega328P/16MHz/5v and I2C LCD16*2 )

   - Pedal & button Tap-IN

   - Analog Sync IN

   - Sync OUT (analog & Midi)

   - LFO out with 7 waveForms

   - Divided analog Clock OUT

   - Tap tempo can be constrain in settable up & dw limits, avoiding bad taps

   -- Config at start 
   
   Press TAP-button and plug power on , keep TAP-button press , adjust preset with rotactors and knobs.
   
   - "Clock-OUT lenght" can be set with DIVIDER rotactor from 1/48 beat to 10/48 beat.
   - "Clock-OUT polarity" can be set with LIMIT knob.
   - "running Beats-Until-Chain-Reset" can be set with BPM knob from 2 to 8
   - "Total-Tap to compute" can be set with WAVEFORM rotactor from 2 to 9

   Release TAP button to save.

   

     ____     __          ______              _       ____ ___  ___  ___
    / __/_ __/ /___ _____/_  __/______  ___  (_)___  |_  // _ \/ _ \/ _ \
   / _// // / __/ // / __// / / __/ _ \/ _ \/ / __/ _/_ </ // / // / // /
  /_/  \_,_/\__/\_,_/_/  /_/ /_/  \___/_//_/_/\__/ /____/\___/\___/\___/

  with love.

*/

/*** includes ***/
#include <EEPROM.h>
#include  <EEWrap.h>

#include <TimerOne.h>


// LCD lib
#include <LiquidCrystal_I2C.h>

//include custom characteres
#include "customChar.h"

// include the modified ArduinoTapTempo library
#include "ArduinoTapTempo_mod.h"

/* include lfo waveform looking-up table */
#include "lut48x256.h" 


/*** CONFIG ***/
#define TAP_DEBOUNCE_MS 200UL //tap button debounce time in millis


/*** defines ***/
#define WAVE_SEL    A3 //waveform selector

#define BPM_POT     A1 //manual bpm pot

#define LIMIT_POT   A2 //limit bpm pot

#define DIVIDER_SEL A0 //clock divider selector

#define TAP_PIN       3 // the one with int1 !!


#define SYNC_OUT_PIN   8 // 
#define SYNC_OUT      PORTB , 0

#define CLK_OUT_PIN   9 //  divided clock out
#define CLK_OUT       PORTB , 1

#define PWM_OUT_PIN   11 //must be timer2 OC2A pin
#define PWM_OUT       PORTB , 3

#define LED_ONBOARD_PIN   13 // led
#define LED_ONBOARD       PORTB , 5

#define CLOCK         0xF8 // midi clock message

//lfo stuff ******************************************
#define TIMER2_TIME   16UL //in microsecond
#define PWM11         OCR2A //shortcut


/** some MACROS **/
#define SET_CLK_OUT (PORTB |=(1<<1))
#define CLR_CLK_OUT (PORTB &= (~(1<<1)))

#define SET_SYNC_OUT (PORTB |=(1<<0))
#define CLR_SYNC_OUT (PORTB &= (~(1<<0)))

#define SET_LED (PORTB |=(1<<5))
#define CLR_LED (PORTB &= (~(1<<5)))

#define SET(x,y) (x |=(1<<y))                //-Bit set/clear macros
#define CLR(x,y) (x &= (~(1<<y)))             // |
#define CHK(x,y) (x & (1<<y))                 // |
#define TOG(x,y) (x^=(1<<y))                  //-+



/*** Vars ***/

unsigned int divider = 2;
const uint8_t clk_dividers[10]={ 96, 64, 48, 32, 24, 16, 12, 8, 6, 1 }; // clock divider setting

volatile unsigned int tick_counter = 0;
volatile unsigned int sync_counter = 0;
volatile unsigned int max_tick = 48;

volatile bool	flag_inhibit_tap = false;
volatile bool flag_tempo_change = false;

volatile unsigned long  _q; // timer1 time in micros

volatile unsigned char wave_sel = 0;
volatile float low_limit, high_limit;

unsigned long TS_last_run = 0;
unsigned long TS_last_int1 = 0;

unsigned int bpm_pot;
unsigned int old_limit_pot;

/* config vars */
uint8_t clockOutLenght = 4;
bool clockOutPolarity = false;

uint8_t beatsUntilChainRst = 4;
uint8_t totalTapVal = 4;


/*lfo ( timer2 interrupt stuff ) */
volatile unsigned long totIteToNextTableStep=5000;
volatile unsigned long  timer2acc = 0; //
volatile unsigned int  lfoTableStep = 0; //

//EEPROM *******************************************
uint8_e savedClockOutLenght EEMEM;
bool_e  savedClockOutPolarity EEMEM;
uint8_e savedBeatsUntilChainRst EEMEM;
uint8_e savedTotalTapVal EEMEM;

// make an ArduinoTapTempo object
ArduinoTapTempo tapTempo;

//make an lcd
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display


/*** interrupts routines **************************************************************************************/

/* timer2 overflow interrupt service routine --> lfo pwm */
ISR(TIMER2_OVF_vect)          
{
   timer2acc ++; // 
 
   if( timer2acc >= totIteToNextTableStep  )
   {
    
    timer2acc = 0; //
    
    //go next lfo table step
    
    lfoTableStep++;
    
    if(lfoTableStep >= WAVEFORM_MAX_STEP)lfoTableStep= 0; 

    PWM11=*(waveform[wave_sel] + lfoTableStep);
    
   }
     
}

/* timer1 int ***/
void beatClockInt()
{
  //
  tick_counter++;
  sync_counter++;

  if (  flag_tempo_change )
  {
    flag_tempo_change = false;
    Timer1.setPeriod( _q );
    setLFO_Period( _q * max_tick );
  }

  // divided clock out
  if ( tick_counter >= max_tick )
  {
    tick_counter = 0;
    if (clockOutPolarity)
    {
      SET_CLK_OUT;
    }
    else
    {
      CLR_CLK_OUT;
    }

    resetLFO_Period( _q * max_tick );

  }
  else if ( tick_counter >= clockOutLenght || tick_counter >= (max_tick - 1))
  {
    if (clockOutPolarity)
    {
      CLR_CLK_OUT;
    }
    else
    {
      SET_CLK_OUT;
    }
  }

  // analog sync out
  if ( sync_counter >= 48 )
  {
    sync_counter = 0;
    SET_SYNC_OUT;
    SET_LED;

    // update ArduinoTapTempo if in manual (knob) bpm
    if (flag_inhibit_tap)
    {
      tapTempo.setBeatLength( ( _q / 1000UL ) * 48UL ); // _q * 1000
      tapTempo.resetTapChain();
      flag_inhibit_tap = false;
    }
  }
  else if ( sync_counter >= 8 )
  {
    CLR_SYNC_OUT;
    CLR_LED;
    tapTempo.update(false);
  }

  // midi clock output
  if ( ! CHK(sync_counter, 0) ) // fire on even number 
  {
    Serial.write((char)CLOCK);
  }

}

/* INT1 interrupt (TAP)***/
void button_int()
{
  bool flagNewTap;
  unsigned long new_q;
  unsigned long diff_TS;

  diff_TS = (millis() - TS_last_int1);

  if ( ! flag_inhibit_tap )
  {
    if ( diff_TS > TAP_DEBOUNCE_MS) //sort of 10ms debounce
    {

      flagNewTap = ! tapTempo.isChainActive();

      // update ArduinoTapTempo
      if (flagNewTap)
      {

        tapTempo.update(true);
        TS_last_int1 = millis();

        tick_counter = 0;
        if (clockOutPolarity)
        {
          SET_CLK_OUT;
        }
        else
        {
          CLR_CLK_OUT;
        }
        
        resetLFO_Period( _q * max_tick );
        
        sync_counter = 0;
        SET_SYNC_OUT;
        SET_LED;

        Serial.write((char)CLOCK);
        Timer1.start();

      } else {
        
        if (diff_TS < (60000UL / uint16_t(low_limit)) && diff_TS > (60000UL / uint16_t(high_limit)))
        {
          tapTempo.update(true);
          TS_last_int1 = millis();

          //chk if tempo has changed
          new_q = (tapTempo.getBeatLength() * 1000UL) / 48UL;
          if (  _q != new_q && tapTempo.getTapsInChain() > 2 ) //totalTapVal
          {
            _q = new_q;
            flag_tempo_change = true;
          }

        } else {
          tapTempo.update(false);
          tapTempo.resetTapChain();
        }

      }

    }

  }

}

/** routines ****************************************************/
unsigned long get_int1_BPM()
{
  return 60000000UL / ( _q * 48UL );
}

/********* RUN ONCE *********************************************/
void setup()
{
  //  Set MIDI baud rate: *************************************
  Serial.begin(31250);  // set midi baudrate
  Serial.write(0xFF); // "is alive" midi msg

  // setup tap button
  pinMode(TAP_PIN, INPUT_PULLUP);
  digitalWrite(TAP_PIN, HIGH);
  delay(100);

  //setup analog inputs
  pinMode(BPM_POT, INPUT);
  pinMode(LIMIT_POT, INPUT);
  pinMode(DIVIDER_SEL, INPUT);

  //setup outputs
  pinMode (SYNC_OUT_PIN, OUTPUT);
  pinMode (CLK_OUT_PIN, OUTPUT);
  pinMode (PWM_OUT_PIN, OUTPUT);
  pinMode (LED_ONBOARD_PIN, OUTPUT);

  //init LCD
  initLCD();

  //get config at start
  if (! digitalRead(TAP_PIN))
  {
    ConfigMenu();
  }

  //load config from eeprom
  clockOutLenght = savedClockOutLenght; //load from eeprom
  clockOutPolarity = savedClockOutPolarity;

  totalTapVal = savedTotalTapVal;
  beatsUntilChainRst = savedBeatsUntilChainRst;

  /*   tapTempo config ********** */
  tapTempo.disableSkippedTapDetection();
  // tapTempo.enableSkippedTapDetection();
  //tapTempo.setSkippedTapThresholdLow(1.8);
  // tapTempo.setSkippedTapThresholdHigh(2.2);
  tapTempo.setBeatsUntilChainReset(beatsUntilChainRst);
  tapTempo.setTotalTapValues(totalTapVal);

  // init interrupt on TAP button
  attachInterrupt(INT1, button_int, FALLING);

  //init timer1
  Timer1.initialize(float(( 60000.0 / 120.0 ) * 1000.00 / 48.00));
  Timer1.attachInterrupt(beatClockInt);

  //init timer2 (lfo pwm generator)
  initTimer2_PWM();
  enable_Timer2_intr();

  //last inits
  TS_last_run = millis();
  flag_inhibit_tap = false;
  tapTempo.update(false);

}

/********* LOOP forever ******************************************/
void loop()
{
  tapTempo.update(false);

  if ( true || millis() - TS_last_run > 20UL )
  {

    TS_last_run = millis();

    float bpm;

    //first chk if manual bpm has changed
    unsigned int new_bpm_pot =  (analogRead(BPM_POT) >> 2) + 30;
    
    if ( new_bpm_pot != bpm_pot )
    {
      flag_inhibit_tap = true;
      tapTempo.resetTapChain();
      bpm_pot = new_bpm_pot;
      bpm = new_bpm_pot;
      //tapTempo.setMinBPM(30.0);
      //tapTempo.setMaxBPM(255.0);
      _q = 1250000UL / bpm;
      flag_tempo_change = true;
    }

    /* chk limit pot */
    unsigned int limit_pot = analogRead(LIMIT_POT) >> 3;
    if ( true || limit_pot != old_limit_pot || flag_tempo_change)
    {
      old_limit_pot = limit_pot;
      bpm = get_int1_BPM();//tapTempo.getBPM();
      low_limit = float(constrain((bpm - limit_pot), 30.0, bpm));
      high_limit = float(constrain((bpm + limit_pot), bpm, 284.0));
    }

    /* chk if divider rotary has changed */
    unsigned int new_divider = map( analogRead(DIVIDER_SEL), 0, 1024, 0, 10 );
    if ( divider != new_divider)
    {
      divider = new_divider;
      max_tick = clk_dividers[divider];

      //flag_divider_change = true;
    }

    /* chk lfo wave_sel_pot */
    wave_sel = map( analogRead(WAVE_SEL), 0, 1024, 0, 7 );

    /* update LCD */
    RefreshLcd( int(get_int1_BPM()), low_limit, high_limit, tapTempo.isChainActive(), max_tick, wave_sel);

  }


}

/** LFO stuff *****************/
void initTimer2_PWM()
{
  pinMode(PWM_OUT_PIN, OUTPUT);
  TCCR2A = 0; 
  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20); // fastPWM
  TCCR2B = 0;
  TCCR2B = _BV(CS20); //no prescaler 
  PWM11= 127; // 
}

void enable_Timer2_intr()
{
    TIMSK2 = _BV(TOIE2);
}
 
void disable_Timer2_intr()
{
    TIMSK2 = 0;
}

void setPWM11( uint8_t duty )
{
  PWM11=duty; 
}

void  setLFO_Period( unsigned long timeMicros )
{
  totIteToNextTableStep = ( timeMicros / ( WAVEFORM_MAX_STEP * TIMER2_TIME ) );// nb of iteration to go next step
}

void resetLFO_Period( unsigned long timeMicros ) //put in beatclock
{
  //set LFO speed
  setLFO_Period( timeMicros );
  
  //reset lfo to start
  
  //disable_Timer2_intr(); // not needed since resetLFO_Period() is call in interupt
  
  timer2acc=totIteToNextTableStep ;// will reset next timer2ovf
  lfoTableStep = WAVEFORM_MAX_STEP; //will reset next time timer2ovf
  
  //enable_Timer2_intr();
  
}


/** LCD stuff *****************/

void initLCD()
{
  // init Transmission
  Wire.begin();
  Wire.beginTransmission(0x27);

  //create custom char
  delay(100);
  lcd.createChar(0, blanche);
  delay(10);
  lcd.createChar(1, noire);
  delay(10);
  lcd.createChar(2, croche);
  delay(10);
  lcd.createChar(3, dblcroche);
  delay(10);
  lcd.createChar(4, triolet);
  delay(10);

  lcd.begin(16, 2, LCD_5x8DOTS); // initialize the lcd
  delay(100);
  //hello lcd screen
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print(F("Tap-tempo LFO"));
  lcd.setCursor(0, 1);
  lcd.print(F("FuturTronic 3000"));
  delay(1500);

  //re-create custom char sometimes CGRam hangs
  lcd.createChar(0, blanche);
  delay(10);
  lcd.createChar(1, noire);
  delay(10);
  lcd.createChar(2, croche);
  delay(10);
  lcd.createChar(3, dblcroche);
  delay(10);
  lcd.createChar(4, triolet);
  delay(10);

  return;
}

void RefreshLcd(unsigned int _bpm, unsigned int _low_limit, unsigned int _high_limit, bool tap_again, int _div, int _waveform)
{

  /* first line : "low < bpm < hgh " **************/

  lcd.setCursor(0, 0);
  lcd.print(_low_limit);
  lcd.print(F("  "));

  lcd.setCursor(3, 0);
  if ( _bpm == _low_limit )
  {
    lcd.print(F(" = "));
  }
  else
  {
    lcd.print(F(" < "));
  }

  lcd.setCursor(6, 0);
  lcd.print(_bpm);
  lcd.print(F(" "));

  lcd.setCursor(9, 0);
  if ( _bpm == _high_limit )
  {
    lcd.print(F(" = "));
  }
  else
  {
    lcd.print(F(" < "));
  }
  lcd.print(_high_limit);
  lcd.print(F("  "));
  

  /*second line : "[DIV][TAP][WAVF]" *****************/

  lcd.setCursor(0, 1);

  //Divider
  if ( _div == 1 )
  {
    lcd.print(F("[HLD]"));
  }
  else
  {
    lcd.print(F("[ "));
    lcd.write((uint8_t)noteChar[divider][0]);
    lcd.write((uint8_t)noteChar[divider][1]);
    lcd.print(F("]"));
  }

  //TAP chain active or not
  if (tap_again)
  {
    lcd.print(F("[TAP]"));
  }
  else
  {
    lcd.print(F("[   ]"));
  }

  //Waveform
  lcd.setCursor(10, 1);
  lcd.print(F("["));
  lcd.print(waveformName[_waveform]);
  lcd.print(F("]"));

  
  return;
}

void ConfigMenu()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(F(" Keep TAP press,"));

  lcd.setCursor(0, 1);
  lcd.print(F("release to save."));

  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("CLK Lgt:   Pol: "));
  lcd.setCursor(0, 1);
  lcd.print(F("TAP Tot:  Rst:  "));

  while ( ! digitalRead(TAP_PIN))
  {
    clockOutLenght = map( analogRead(DIVIDER_SEL), 0, 1024, 1, 11 );
    lcd.setCursor(8, 0);
    lcd.print(clockOutLenght);
    if (clockOutLenght < 10)lcd.print(F(" "));

    clockOutPolarity = analogRead(LIMIT_POT)>>9; //map( analogRead(LIMIT_POT), 0, 1024, 0, 1 );
    
    lcd.setCursor(15, 0);
    if (clockOutPolarity)
    {
      lcd.print(F("+"));
    } else {
      lcd.print(F("-"));
    }

    totalTapVal = map( analogRead(WAVE_SEL), 0, 1024, 2, 10 );
    lcd.setCursor(8, 1);
    lcd.print(totalTapVal);

    beatsUntilChainRst = map( analogRead(BPM_POT), 0, 1024, 2, 10 );
    lcd.setCursor(14, 1);
    lcd.print(beatsUntilChainRst);

    delay(100); //slow down
  }

  //button was released; save values in EEPROM
  savedClockOutLenght  = clockOutLenght;  //save in eeprom
  savedClockOutPolarity  = clockOutPolarity;  //save in eeprom

  savedTotalTapVal = totalTapVal;
  savedBeatsUntilChainRst = beatsUntilChainRst;

  //display config
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("config saved"));
  lcd.setCursor(0, 1);
  lcd.print(F("L:   P:  T:  R: "));

  lcd.setCursor(2, 1);
  lcd.print(savedClockOutLenght);

  lcd.setCursor(7, 1);
  if (savedClockOutPolarity)
  {
    lcd.print(F("+"));
  } else {
    lcd.print(F("-"));
  }

  lcd.setCursor(11, 1);
  lcd.print(savedTotalTapVal);

  lcd.setCursor(15, 1);
  lcd.print(savedBeatsUntilChainRst);

  delay(2000);
}

// END OF FILE


