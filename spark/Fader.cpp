#ifndef FADER_cpp
#define FADER_cpp

#include "Fader.h"

Fader::Fader()
{
  fading = false;
}

void Fader::start(unsigned char start_level, unsigned char target_level,
                  unsigned long duration, unsigned long now)
{
  this->start_level = start_level;
  this->target_level = target_level;
  started_at = now;
  will_end_at = now + duration;
  level_delta_per_ms = (target_level - start_level) / (float) duration;
  fading = true;
}

bool Fader::is_fading()
{
  return fading;
}

unsigned char Fader::current_level(unsigned long now)
{
  if (now > will_end_at)
  {
    fading = false;
    return target_level;
  }
  unsigned char level_delta = (unsigned char) ((now - started_at) * level_delta_per_ms);
  return start_level + level_delta;
}

#endif
