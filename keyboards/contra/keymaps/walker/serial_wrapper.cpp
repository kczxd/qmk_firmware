#include "arduino/Arduino.h"

extern "C" void Serial1_begin(unsigned long rate)
{
  Serial1.begin(rate);
}

extern "C" int Serial1_read(void)
{
  return Serial1.read();
}

extern "C" void Serial1_flush(void)
{
  Serial1.flush();
}

extern "C" size_t Serial1_write(uint8_t data)
{
  return Serial1.write(data);
}

/*
extern "C" void Serial1_println(char *str)
{
  Serial1.println(str);
}
*/
