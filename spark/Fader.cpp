#ifndef FADER_cpp
#define FADER_cpp

#include "Fader.h"
#include <math.h>

Fader::Fader()
{
  fading = false;
}

void Fader::start(unsigned char user_start, unsigned char user_target,
                  unsigned long duration, unsigned long now)
{
  this->start_level = user_start;
  this->target_level = this->mapped_device_level(user_target);
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

unsigned char Fader::mapped_device_level(unsigned char user_level)
{
  if (0 < user_level && 12 >= user_level)
  {
    double slope = (255.0 - 40.0) / (12.0 - 1.0);
    double intercept = 40.0 - slope;
    return (unsigned char) lround(user_level * slope + intercept);
  }
  return 0;
}

#endif
