#include "arduino/Arduino.h"

extern "C" void Serial1_begin(unsigned long rate)
{
  Serial1.begin(rate);
}

extern "C" void Serial1_end(void)
{
  Serial1.end();
}

extern "C" int Serial1_available(void)
{
  return Serial1.available();
}

extern "C" int Serial1_peek(void)
{
  return Serial1.peek();
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

extern "C" void Serial1_println(char *str)
{
  Serial1.println(str);
}
