/*
   TAP-tempo LFO 
   
   (designed for arduino nano ATmega328P/16MHz/5v)
   
   -Pedal & button Tap-IN
   -Analog Sync IN

   -Sync OUT (analog & Midi)
   -LFO out with 7 waveForms 
   -Divided analog Clock OUT 
   
   -Tap tempo can be constrain in up & dw limits, avoiding bad taps
   
   
   ____     __          ______              _       ____ ___  ___  ___
  / __/_ __/ /___ _____/_  __/______  ___  (_)___  |_  // _ \/ _ \/ _ \
 / _// // / __/ // / __// / / __/ _ \/ _ \/ / __/ _/_ </ // / // / // /
/_/  \_,_/\__/\_,_/_/  /_/ /_/  \___/_//_/_/\__/ /____/\___/\___/\___/ 
                                                                       
with love.

*/

/*** includes ***/

#include <TimerOne.h>

// LCD lib
#include <LiquidCrystal_I2C.h>

//include custom characteres
#include "customChar.h"

// include the ArduinoTapTempo library
#include "ArduinoTapTempo.h"



/*** defines ***/
#define WAVE_SEL    A3

#define BPM_POT     A2

#define LIMIT_POT   A1

#define DIVIDER_SEL A0

#define TAP_PIN     3 // the one with int1


#define SYNC_OUT_PIN   8 // 
#define SYNC_OUT      PORTB , 0

#define CLK_OUT_PIN   9 // 
#define CLK_OUT       PORTB , 1

#define PWM_OUT_PIN   10 //ou 9
#define PWM_OUT       PORTB , 2

#define CLOCK         0xF8

#define SET(x,y) (x |=(1<<y))                //-Bit set/clear macros
#define CLR(x,y) (x &= (~(1<<y)))             // |
#define CHK(x,y) (x & (1<<y))                 // |
#define TOG(x,y) (x^=(1<<y))                  //-+

#define SET_CLK_OUT (PORTB |=(1<<1))
#define CLR_CLK_OUT (PORTB &= (~(1<<1)))

#define SET_SYNC_OUT (PORTB |=(1<<0))
#define CLR_SYNC_OUT (PORTB &= (~(1<<0)))


/*** Vars ***/

unsigned int divider = 2;

volatile unsigned int tick_counter = 0;
volatile unsigned int sync_counter = 0;
volatile unsigned int max_tick = 48;

volatile char	seq_inhibit_tap = 0;

volatile boolean flag_tempo_change = false;
volatile boolean flag_divider_change = false;

volatile unsigned long  _q;

unsigned long TS_last_run = 0;
unsigned long TS_last_int1 = 0;

unsigned int bpm_pot;
unsigned int old_limit_pot;

//include lfo waveform looking-up table
#include "lut48.h"


volatile unsigned char wave_sel = 0;

volatile float low_limit, high_limit;


// make an ArduinoTapTempo object
ArduinoTapTempo tapTempo;

//make an lcd
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

/*** interrupts routines ***/

/* timer1 int ***/
void beatClockInt()
{
  //  /
  tick_counter++;
  sync_counter++;

  if (  flag_tempo_change )
  {
    flag_tempo_change = false;
    Timer1.setPeriod( _q );
  }

  // divided clock out
  if ( tick_counter >= max_tick )
  {
    tick_counter = 0;
    SET_CLK_OUT;
  }
  else if ( tick_counter >= 2 )
  {
    CLR_CLK_OUT;
  }

  //analog sync out
  if ( sync_counter >= 48 )
  {
    sync_counter = 0;
    SET_SYNC_OUT;

    // update ArduinoTapTempo if in manual (knob) bpm
    if (seq_inhibit_tap > 0)
    {
      tapTempo.setBeatLength( _q * 1000 );
      tapTempo.resetTapChain();
      seq_inhibit_tap = 0;
    }
  }
  else if ( sync_counter >= 8 )
  {
    CLR_SYNC_OUT;
    tapTempo.update(false);
  }

  // midi clock
  if ( ! CHK(sync_counter, 0) ) //
  {
    Serial.write((char)CLOCK);
  }

  //pseudo LFO
  unsigned int dduty = *(waveform[wave_sel] + sync_counter);
  Timer1.setPwmDuty(PWM_OUT_PIN, dduty << 2);


}

/* INT1 interrupt ***/
void button_int()
{
  bool flagNewTap;
  unsigned long new_q;
  unsigned long diff_TS;

  diff_TS = (millis() - TS_last_int1);

  if ( seq_inhibit_tap == 0)
  {
    if ( diff_TS > 200UL) //sort of 10ms debounce
    {

      flagNewTap = ! tapTempo.isChainActive();

      // update ArduinoTapTempo
      if (flagNewTap)
      {

        tapTempo.update(true);
        TS_last_int1 = millis();

        tick_counter = 0;
        SET_CLK_OUT;
        sync_counter = 0;
        SET_SYNC_OUT;
        Serial.write((char)CLOCK);
        Timer1.start();

      } else {
        if (diff_TS < (60000UL / uint16_t(low_limit)) && diff_TS > (60000UL / uint16_t(high_limit)))
        {
          tapTempo.update(true);
          TS_last_int1 = millis();

          //chk if tempo has changed
          new_q = (tapTempo.getBeatLength() * 1000UL) / 48UL;
          if (  _q != new_q && tapTempo.getTapsInChain() > 2 )
          {
            _q = new_q;
            flag_tempo_change = true;
          }

        } else {
          tapTempo.update(false);
          //tapTempo.resetTapChain();
        }

      }

    }

  }

}

unsigned long getIntBPM()
{
  return 60000000UL / (_q * 48UL);
}

/********* RUN ONCE *********************************************/
void setup()
{
  //  Set MIDI baud rate: *************************************
  Serial.begin(31250);  // set midi baudrate
  Serial.write(0xFF); // "is alive" midi msg

  //init LCD
  initLCD();

  // setup tap button
  pinMode(TAP_PIN, INPUT_PULLUP);
  digitalWrite(TAP_PIN, HIGH);
  delay(100);
  attachInterrupt(INT1, button_int, FALLING);

  //setup analog inputs
  pinMode(BPM_POT, INPUT);
  pinMode(LIMIT_POT, INPUT);
  pinMode(DIVIDER_SEL, INPUT);

  //init timer1
  Timer1.initialize(float(( 60000.0 / 120.0 ) * 1000.00 / 48.00));
  Timer1.attachInterrupt(beatClockInt);
  Timer1.pwm(PWM_OUT_PIN, 128);

  //tapTempo config
  
  tapTempo.disableSkippedTapDetection();
  // tapTempo.enableSkippedTapDetection();
  //tapTempo.setSkippedTapThresholdLow(1.8);
  // tapTempo.setSkippedTapThresholdHigh(2.2);
  tapTempo.setBeatsUntilChainReset(4);
  tapTempo.setTotalTapValues(3);


  //last inits
  TS_last_run = millis();
  seq_inhibit_tap = 0;
  tapTempo.update(false);

}

/********* LOOP forever *********************************************/
void loop()
{
  tapTempo.update(false);

  if ( true || millis() - TS_last_run > 10UL )
  {

    TS_last_run = millis();

    float bpm;

    //first chk if manual bpm has changed
    unsigned int new_bpm_pot =  (analogRead(BPM_POT) >> 2) + 30;
    if ( new_bpm_pot != bpm_pot )
    {
      seq_inhibit_tap = 1;
      tapTempo.resetTapChain();
      bpm_pot = new_bpm_pot;
      bpm = new_bpm_pot;
      //tapTempo.setMinBPM(30.0);
      //tapTempo.setMaxBPM(255.0);
      //_q = float(( 60000.0 / bpm ) * 1000.00 / 48.00);
      _q = 1250000UL / bpm;
      flag_tempo_change = true;
    }

    //chk limit pot


    unsigned int limit_pot = analogRead(LIMIT_POT) >> 3;
    if ( true || limit_pot != old_limit_pot || flag_tempo_change)
    {
      old_limit_pot = limit_pot;
      bpm = getIntBPM();//tapTempo.getBPM();
      low_limit = float(constrain((bpm - limit_pot), 30.0, bpm));
      high_limit = float(constrain((bpm + limit_pot), bpm, 284.0));
    }



    //chk if divider rotary has changed
    unsigned int new_divider = map( analogRead(DIVIDER_SEL), 0, 1024, 0, 10 );
    if ( divider != new_divider)
    {
      divider = new_divider;
      switch (divider)
      {
        case 0:    //
          max_tick = 96;
          break;

        case 1:
          // triolet blanche
          max_tick = 64;
          break;

        case 2:    //
          //noire
          max_tick = 48;
          break;

        case 3:    //
          //noire triolet
          max_tick = 32;
          break;

        case 4:    //
          //croche
          max_tick = 24;
          break;

        case 5:    //
          //croche triolet
          max_tick = 16;
          break;

        case 6:    //
          //dbl croche
          max_tick = 12;
          break;

        case 7:    //
          // dble triolet
          max_tick = 8;
          break;

        case 8:    //
          //triple
          max_tick = 6;
          break;

        case 9:
          // hold
          max_tick = 0;  //HOLD
          break;

        default:
          //
          max_tick = 24;
          break;

      }

      flag_divider_change = true;
    }

    //chk lfo wave_sel_pot
    wave_sel = map( analogRead(WAVE_SEL), 0, 1024, 0, 7 );


    //update LCD
    RefreshLcd( int(getIntBPM()), low_limit, high_limit, tapTempo.isChainActive(), max_tick, wave_sel);

  }


}

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

  //hello lcd screen
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Tap LFO ver0.1b");
  lcd.setCursor(0, 1);
  lcd.print("FuturTronic3000");
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
  //lcd.clear();

  /* first line : "low < bpm < hgh" */
  lcd.setCursor(0, 0);
  lcd.print(_low_limit);
  lcd.print("  ");

  lcd.setCursor(3, 0);
  if ( _bpm == _low_limit )
  {
    lcd.print(" = ");
  }
  else
  {
    lcd.print(" < ");
  }

  lcd.setCursor(6, 0);
  lcd.print(_bpm);
  lcd.print(" ");

  lcd.setCursor(9, 0);
  if ( _bpm == _high_limit )
  {
    lcd.print(" = ");
  }
  else
  {
    lcd.print(" < ");
  }
  lcd.print(_high_limit);
  lcd.print("  ");

  /*second line : "[TAP][DIV][WAVF]" */
  lcd.setCursor(0, 1);
  if (tap_again)
  {
    lcd.print("[TAP]");
  }
  else
  {
    lcd.print("[   ]");
  }

  if ( ! _div )
  {
    lcd.print("[HLD]");
  }
  else
  {
    lcd.print("[ ");
    lcd.write((uint8_t)noteChar[divider][0]);
    lcd.write((uint8_t)noteChar[divider][1]);
    lcd.print("]");
  }

  lcd.setCursor(10, 1);
  lcd.print("[");
  lcd.print(waveformName[_waveform]);
  lcd.print("]");

}


