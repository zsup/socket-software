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

    /**
     * Start fading toward the target over the given duration.
     * @param start_level   Device level from which to start fading (0-255)
     * @param target_level  User facing level to fade toward (0-12)
     * @param duration      Fade duration in milliseconds
     * @param now           Number of milliseconds representing current time
     */
    void start(unsigned char start_level, unsigned char target_level,
               unsigned long duration, unsigned long now);

    bool is_fading();

    unsigned char current_level(unsigned long now);
  
  private:

    unsigned char mapped_device_level(unsigned char user_level);
};

#endif
