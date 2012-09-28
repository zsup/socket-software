#ifndef FADER_h
#define FADER_h

class Fader
{
  bool          fading;
  unsigned char start_level, target_level;
  unsigned long started_at, will_end_at;
  float         level_delta_per_ms;

  public:

    Fader();

    void start(unsigned char device_start_level, unsigned char user_target_level,
               unsigned long duration, unsigned long now);

    bool is_fading();

    unsigned char current_level(unsigned long now);
  
  private:

    unsigned char mapped_device_level(unsigned char user_level);
};

#endif
