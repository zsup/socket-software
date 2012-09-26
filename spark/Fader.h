class Fader
{
  bool          fading;
  unsigned char start_level;
  unsigned char target_level;
  unsigned long started_at;
  unsigned long will_end_at;
  float         level_delta_per_ms;

  public:

  Fader();
  
  /**
   * Start fading toward the target over the given duration.
   * @param start_level   Level from which to start fading
   * @param target_level  Level to fade toward
   * @param duration      Fade duration in milliseconds
   * @param now           Number of milliseconds representing current time
   */
  void start(unsigned char start_level, unsigned char target_level,
             unsigned long duration, unsigned long now);
  
  bool is_fading();

  unsigned char current_level(unsigned long now);
};
