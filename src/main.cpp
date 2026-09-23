#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void TaskEnvironment(void *pvParameters);

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    if(!bme.begin(0x76, &Wire))
    {
        Serial.println("Fail to initialize BME280!!!");
        while(true);
    }

    Serial.println("Finish Initialization");

    xTaskCreate(TaskEnvironment, "Environment", 2048, NULL, 1, NULL);
}

void loop()
{

}

void TaskEnvironment(void *pvParameneters)
{
    while(1)
    {
        float temperature = bme.readTemperature();
        float humidity = bme.readHumidity();
        float pressure = bme.readPressure() / 100.0F; // hPa

        if(isnan(temperature) || isnan(humidity) || isnan(pressure))
        {
            Serial.println("Failed to read from BME280");
        }
        else
        {
            Serial.print("Temperature : ");
            Serial.print(temperature, 1);
            Serial.println("C");

            Serial.print("Humidity : ");
            Serial.print(humidity, 1);
            Serial.println("%");

            Serial.print("Pressure : ");
            Serial.print(pressure, 0);
            Serial.println("hPa");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}