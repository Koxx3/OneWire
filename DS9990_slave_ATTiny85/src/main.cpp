/*
 *    Example-Code that emulates a DS18B20
 *
 *    Tested with:
 *    - https://github.com/PaulStoffregen/OneWire --> DS18x20-Example, atmega328@16MHz as Slave
 *    - DS9490R-Master, atmega328@16MHz and teensy3.2@96MHz as Slave
 */

#include "OneWireHub.h"
#include "DS9990.h" // Custom SE_EB
#include "DHT.h"

#define DEBUG_DISPLAY_DHT 1
#define DEBUG 0

constexpr uint8_t pin_led{2};
constexpr uint8_t pin_onewire{3};

uint32_t i_loop = 0;
uint32_t lastDhtReading = -4000;

auto hub = OneWireHub(pin_onewire);

auto ds9990 = DS9990(DS9990::family_code, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14); // DS2450
//DHT_nonblocking dht_sensor(D2, DHT_TYPE_22);

#define DHTPIN 3 // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// Reading temperature or humidity takes about 250 milliseconds!
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
float h;
// Read temperature as Celsius (the default)
float t;

bool blinking(void);

#if 0
void readDhtNonBlocking()
{
      /* Measure once every four seconds. */
    float temperature;
    float humidity;

    if (dht_sensor.measure(&temperature, &humidity) == true)
    {
      /*
#if DEBUG_DISPLAY_DHT
    Serial.print("T = ");
    Serial.print(temperature, 1);
    Serial.print(" deg. C, H = ");
    Serial.print(humidity, 1);
    Serial.println("%");
#endif
*/
    }

}

#endif

void readDhtBlocking()
{

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();

#if DEBUG
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print("\n");
#endif
}

void setValues()
{
  uint8_t val[8];
  memset(val, 0, 8);
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
  pinMode(4, OUTPUT);

  hub.attach(ds9990);
  setValues();

  dht.begin();

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

    // read received data from memory
    uint8_t val[8];
    ds9990.readMemory(val, 7, 0);

    // apply brake output to LED and D6
    digitalWrite(pin_led, val[0]);
    digitalWrite(4, val[0]);

    // read temperature and humidity from DHT
    if (millis() > lastDhtReading + 4000)
    {
      //readDhtNonBlocking();
      readDhtBlocking();
      lastDhtReading = millis();
    }

    int16_t temp_int = t * 10;
    int16_t hum_int = h * 10;

    val[1] = temp_int & 0xff;
    val[2] = (temp_int >> 8) & 0xff;
    val[3] = hum_int & 0xff;
    val[4] = (hum_int >> 8) & 0xff;
#if DEBUG
    Serial.printf("millis = %d / A val[0] = %02x\n", millis(), val[0]);
#endif

    // read current from A0
    uint16_t current = analogRead(7);
    val[5] = ((uint16_t)current) & 0xff;
    val[6] = (((uint16_t)current) >> 8) & 0xff;
#if DEBUG
    //Serial.printf(" Current = %d\n", current);
#endif

    // write data in memory for next request
    ds9990.writeMemory(&val[1], 6, 1);

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
