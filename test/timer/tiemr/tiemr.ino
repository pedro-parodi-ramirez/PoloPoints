#define SECOND_IN_MICROS 1000000
hw_timer_t *Timer0_Cfg = NULL;
 
void IRAM_ATTR Timer0_ISR()
{
  Serial.println("Paso un segundo ...");
}
void setup()
{
  Serial.begin(9600);
  Timer0_Cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);
  timerAlarmWrite(Timer0_Cfg, SECOND_IN_MICROS, true);
  timerAlarmEnable(Timer0_Cfg);
  timerStart(Timer0_Cfg);
  delay(100);
}

void loop()
{
    // Do Nothing!
}