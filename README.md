# ESP8266-FeldHell
ESP8266 and Si5351 FeldHell Beacon

Simple Feld Hell beacon for ESP8266, with the Etherkit Si5351a breakout board, by Jason Milldrum NT7S, but works flawlessly with the cheaps chinese Si5351 breakout modules too.

This release is adapted for ESP8266, and uses the timer1 hardware interrupt for the required precise timing.
For the moment i've powered off the WiFi, maybe in a future release i'll use it for something like an interactive webpage to change frequency and text of the beacon, or something else, who knows. This little piece of code is born as an exercise in using timer1, the only one that you can safely use in ESP8266 without worry about hangs the system (timer0 is shared with the whole wifi and tcp/ip system).

Original Feld Hell code for Arduino by Mark Vandewettering K6HX, adapted for the Si5351A by Robert Liesenfeld AK6L <ak6l@ak6l.org>.  Arduino Timer setup code by Thomas Knutsen LA3PNA, ESP8266 Timer setup by Marco Campinoti IU5HKU.

Si5351A is connected via I2C on pin D1 (SCL) and D2 (SDA) as marked on Wemos D1 mini Lite and chinese clone boards.
Works without troubles in 144Mhz too, FeldHell doesn't require the zero drift as WSPR does :-)
