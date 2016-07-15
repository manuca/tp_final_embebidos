#include "signal_data.h"

#define SIGNAL_OFFSET (512)
#define PI            (3.14159265358979323846)

void calculateSignal(uint16_t * buffer, size_t length)
{
  float delta = 2*PI/length;

  for(int i = 0; i < length; i++) {
    buffer[i] = (uint16_t) (SIGNAL_OFFSET + SIGNAL_OFFSET * sin(delta*i));
  }
}
