#include "TTS/TTS.h"

TTS mainTTS(TTS_PIN);
TTS tts(TTS_PIN);

extern "C" void TTSInit(void)
{
}

extern "C" void sayPhonemes(const char *phonemes)
{
  mainTTS.sayPhonemes(phonemes);
}

extern "C" void sayText(const char *phonemes)
{
  mainTTS.sayText(phonemes);
}

extern "C" void setPitch(byte pitch)
{
  mainTTS.setPitch(pitch);
}
