/*
 *    Example-Code that emulates a DS18B20
 *
 *    Tested with:
 *    - https://github.com/PaulStoffregen/OneWire --> DS18x20-Example, atmega328@16MHz as Slave
 *    - DS9490R-Master, atmega328@16MHz and teensy3.2@96MHz as Slave
 */

#include "OneWireHub.h"
#include "DS9990.h" // Custom SE_EB
#include "esp8266_peri.h"

constexpr uint8_t pin_led{2};
constexpr uint8_t pin_onewire{D1};

uint32_t i_loop = 0;

auto hub = OneWireHub(pin_onewire);

auto ds9990 = DS9990(DS9990::family_code, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14); // DS2450

bool blinking(void);

void setValues()
{

  uint8_t val[8];
  val[0] = 0x11;
  val[1] = 0x22;
  val[2] = 0x33;
  val[3] = 0x44;
  val[4] = 0x55;
  val[5] = 0x66;
  val[6] = 0x77;
  val[7] = 0x88;
  ds9990.writeMemory(val, 8, 0);
}

void setup()
{

#if DEBUG    
  Serial.begin(921600);
  Serial.println("OneWire-Hub");
  Serial.flush();
#endif

  pinMode(pin_led, OUTPUT);
  pinMode(D6, OUTPUT);

  hub.attach(ds9990);
  setValues();

  //Serial.println("config done");
}

void loop()
{
  boolean hasProcessed = false;
  // following function must be called periodically
  hub.poll(&hasProcessed);
  if (hasProcessed)
  {
    //Serial.printf("hasProcessed = %d / millis = %d\n", hasProcessed, millis());

    /*
  // this part is just for debugging (USE_SERIAL_DEBUG in OneWire.h must be enabled for output)
  if (hub.hasError())
    hub.printError();
*/

    /*
  // Blink triggers the state-change
  if (blinking())
  {
    uint8_t val[8];
    //ds9990.readMemory(val, 8, 0);


    //Serial.println();

    i_loop++;
  }
  */

    uint8_t val[8];
    ds9990.readMemory(val, 7, 0);

    digitalWrite(pin_led, val[0]);

    val[1] = i_loop % 255;
#if DEBUG    
    Serial.printf("milles = %d / A val[0] = %02x / val[1] = %02x\n", millis(), val[0], val[1]);
#endif

    ds9990.writeMemory(&val[1], 1, 1);

    i_loop++;
  }
}

bool blinking(void)
{
  constexpr uint32_t interval = 1000;    // interval at which to blink (milliseconds)
  static uint32_t nextMillis = millis(); // will store next time LED will updated

  if (millis() > nextMillis)
  {
    nextMillis += interval;        // save the next time you blinked the LED
    static uint8_t ledState = LOW; // ledState used to set the LED
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
    digitalWrite(pin_led, ledState);

    //uint8_t val[8];
    //val[0] = rand() % 255;
    //val[1] = rand() % 2;
    //ds9990.writeMemory(val, 8, 0);

    return 1;
  }
  return 0;
}
