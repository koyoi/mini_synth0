
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

