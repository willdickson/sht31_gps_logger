#include "SHT31.h"
#include "SSD1306.h"
#include "gps_monitor.h"
#include "openlog.h"
#include "ArduinoJson.h"

const int DisplayDC = 5;
const int DisplayCS = 3;
const int DisplayReset = 4;
const int DisplayDecimals = 1;
const SPISettings DisplaySPISettings(4000000,MSBFIRST,SPI_MODE0);
const uint32_t loop_dt = 100;

const OpenlogParam OpenlogParamVals = {115200, 500, 0, 6, &Serial3} ;
const uint32_t log_update_dt = 5000;
const uint32_t JsonBufferSize = 1000;

void setupDisplay();
void setupTempHumdSensor();
void setupGPSMonitor();
void setupOpenlog();

SHT31 tempHumdSensor =  SHT31();    
SSD1306 display(DisplayDC, DisplayReset, DisplayCS);
GPSMonitor gpsMonitor;
Openlog openlog(OpenlogParamVals);

void setup()
{
    Serial.begin(115200);
    setupDisplay();
    setupTempHumdSensor();
    setupGPSMonitor();
    setupOpenlog();
    delay(1000);

}

void loop()
{
    static uint32_t cnt = 0;
    static unsigned long last_log_t = millis();

    SHT31_Reading reading = tempHumdSensor.getReading();
    Serial.print(cnt);
    Serial.print(", temp =  ");
    Serial.print(reading.temperature);
    Serial.print(" (C), humd = ");
    Serial.print(reading.humidity);
    Serial.println(" (%)");

    String temperatureStr = String(reading.temperature,DisplayDecimals);
    String humidityStr = String(reading.humidity,DisplayDecimals);

    GPSData gpsData;
    bool gpsDataOk = false;
    if (gpsMonitor.haveData())
    {
        gpsData = gpsMonitor.getData(&gpsDataOk);
    }
    String gpsFix = String("false");
    String dateTimeStr = String("--");
    String latitudeStr = String("--");
    String longitudeStr = String("--");
    String altitudeStr = String("--");
    String speedStr = String("--");
    if (gpsDataOk) 
    {
        if (gpsData.fix) 
        {
            gpsFix = String("true");
        }
        dateTimeStr = gpsData.getDateTimeString();
        latitudeStr = gpsData.getLatitudeString();
        longitudeStr = gpsData.getLongitudeString();
        altitudeStr = String(gpsData.getAltitudeInMeter(),DisplayDecimals);
        speedStr = String(gpsData.getSpeedInMeterPerSec(),DisplayDecimals);
    }

    // Update display
    // -----------------------------------------------------------------------
    SPI.beginTransaction(DisplaySPISettings);
    display.clearDisplay();   
    display.setCursor(0,0);

    display.println(dateTimeStr);
    display.print("temp:  ");
    display.print(temperatureStr);
    display.println(" C");

    display.print("humd:  ");
    display.print(humidityStr);
    display.println(" %");

    display.print("fix:   ");
    display.println(gpsFix);

    display.print("lat:   ");
    display.println(latitudeStr);

    display.print("lon:   ");
    display.println(longitudeStr);

    display.print("alt:   ");
    display.println(altitudeStr);

    display.print("spd:   ");
    display.println(speedStr);

    display.display();
    SPI.endTransaction();
    // -----------------------------------------------------------------------

    // Update SD card log file
    // -----------------------------------------------------------------------
    if ((millis() - last_log_t) > log_update_dt)
    {
        StaticJsonBuffer<JsonBufferSize> jsonBuffer;
        JsonObject &jsonMsg = jsonBuffer.createObject();

        jsonMsg["date"] = dateTimeStr;
        jsonMsg["temp"] = temperatureStr;
        jsonMsg["humd"] = humidityStr;
        jsonMsg["fix"] = gpsFix;
        jsonMsg["lat"] = latitudeStr;
        jsonMsg["lon"] = longitudeStr;
        jsonMsg["alt"] = altitudeStr;
        jsonMsg["spd"] = speedStr;

        //jsonMsg.printTo(Serial);
        //Serial.println();

        // Write json object to open log
        jsonMsg.printTo(*openlog.getSerialPtr());
        openlog.println();

        last_log_t = millis();
    }
    // -----------------------------------------------------------------------

    delay(loop_dt);
    cnt++;
}


void setupDisplay()
{
    delay(500);
    display.begin(SSD1306_SWITCHCAPVCC);
    SPI.beginTransaction(DisplaySPISettings);
    display.clearDisplay();   
    display.display();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    SPI.endTransaction();

    SPI.beginTransaction(DisplaySPISettings);
    display.clearDisplay();   
    display.setCursor(0,0);
    display.println("Initializing Device");
    display.display();
    SPI.endTransaction();
    delay(500);

}


void setupTempHumdSensor()
{
    tempHumdSensor.begin();
    SPI.beginTransaction(DisplaySPISettings);
    display.println("* SHT31 Sensor");
    display.display();
    SPI.endTransaction();
}


void setupGPSMonitor()
{
    gpsMonitor.initialize();
    gpsMonitor.setTimerCallback( []() {gpsMonitor.readData(); });
    gpsMonitor.start();

    SPI.beginTransaction(DisplaySPISettings);
    display.println("* GPS");
    display.display();
    SPI.endTransaction();
}


void setupOpenlog()
{
    openlog.initialize();
    bool loggerOk = openlog.openNewLogFile();
    SPI.beginTransaction(DisplaySPISettings);
    display.print("* openlog: ");
    display.println(loggerOk);
    display.display();
    SPI.endTransaction();
}
