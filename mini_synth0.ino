// mozzi ---------------------------------------------------------------
#include <MozziConfigValues.h>
#define MOZZI_AUDIO_INPUT MOZZI_AUDIO_INPUT_STANDARD
#define MOZZI_AUDIO_INPUT_PIN 0
#define MOZZI_ANALOG_READ_RESOLUTION 10
//#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_2PIN_PWM

#include <Mozzi.h>

#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator

#include <ADSR.h>

#include <ResonantFilter.h>

#define KNOB_PIN 1

LowPassFilter lpf;
#define AUDIO_BUFF_SIZE 256
int audiobuff[AUDIO_BUFF_SIZE];

Oscil <SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);


// fft ---------------------------------------------------------------
//#define FFT_SQRT_APPROXIMATION
#define FFT_WINDOW_SIZE 256
#include "arduinoFFT.h"
float vReal[FFT_WINDOW_SIZE];
float vImag[FFT_WINDOW_SIZE];

ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, FFT_WINDOW_SIZE, MOZZI_AUDIO_RATE, true);


// u8g ---------------------------------------------------------------
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_2_3W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* reset=*/ 8);



// ---------------------------------------------------------------
void setup() {
  randomSeed(42);

  /* U8g2 Project: SSD1306 Test Board */
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  digitalWrite(10, 0);
  digitalWrite(9, 0);
  u8g2.begin();

  startMozzi();
}

void loop() {
  audioHook();
}


void updateControl(){
  byte cutoff_freq = mozziAnalogRead<8>(KNOB_PIN); // range 0-255
  lpf.setCutoffFreq(cutoff_freq);
  lpf.setResonance(220);

  u8g2.firstPage();  
  do {
    draw();
  } while( u8g2.nextPage() );
}


AudioOutput updateAudio(){
  // subtracting 512 moves the unsigned audio data into 0-centred,
  // signed range required by all Mozzi units
  int asig = mozziAnalogRead(KNOB_PIN)-512;
  asig = lpf.next(asig>>1);
  return MonoOutput::fromAlmostNBit(9, asig);
}




void fft_proc(int adata)
{
  static int pos = 0;

  vReal[pos] = adata;
  pos++;
  if (pos >= FFT_WINDOW_SIZE) {
    pos = 0;

    FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);	/* Weigh data */
    FFT.compute(FFTDirection::Forward); /* Compute FFT */
    FFT.complexToMagnitude(); /* Compute magnitudes */  
    // float x = FFT.majorPeak();

    //vReal[] に結果が入る。サイズは FFT_WINDOW_SIZE
  }
}



void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}


void draw(void) {
  u8g2_prepare();
  u8g2_line(0);
}

void u8g2_line(uint8_t a) {
  u8g2.drawStr( 0, 0, "drawLine");
  u8g2.drawLine(7+a, 10, 40, 55);
  u8g2.drawLine(7+a*2, 10, 60, 55);
  u8g2.drawLine(7+a*3, 10, 80, 55);
  u8g2.drawLine(7+a*4, 10, 100, 55);
}


// todos 
// スペアナ描画
// 波形描画
// パラメータいじったとき、その波形を表示（発声を待たない）



#if false
// adsr こう使う
ADSR <MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE> envelope;

void HandleNoteOn(byte channel, byte note, byte velocity) {
  aSin.setFreq(mtof(float(note)));
  envelope.noteOn();
}


void HandleNoteOff(byte channel, byte note, byte velocity) {
  envelope.noteOff();
}

  envelope.setADLevels(255,64);
  envelope.setTimes(50,200,10000,200); // 10000 is so the note will sustain 10 seconds unless a noteOff comes


void updateControl(){
  MIDI.read();
  envelope.update();
}

AudioOutput updateAudio(){
  return MonoOutput::from16Bit(envelope.next() * aSin.next());
}


#endif

