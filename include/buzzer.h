class Buzzer
{
public:
    void init();
#ifndef DESKTOPSIM
    const int timebase = 4000000;
    void onImpl(int period);
    inline void on(float freq) { onImpl((int)(4000000.0 / freq)); }
#else
    void on(float) {}
#endif
    void off();
#ifndef DESKTOPSIM
    void beepImpl(int period, int dur);
    inline void beep(float freq, int dur) { beepImpl((int)(4000000.0 / freq), dur); }
#else
    void beep(float freq, int dur) {}
#endif
    void click();
    void rickroll();
};

extern Buzzer buzzer;
