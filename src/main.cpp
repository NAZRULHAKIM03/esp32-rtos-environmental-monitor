#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_BME280.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme;

void TaskDisplay(void *pvParameters);
void TaskEnvironment(void *pvParameters);

SemaphoreHandle_t i2cMutex;
float temperature = 0, humidity = 0, pressure = 0;

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("Fail to initiliaze Oled Display!!!");
        while(true);
    }

    if(!bme.begin(0x76, &Wire))
    {
        Serial.println("Fail to initialize BME280!!!");
        while(true);
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    Serial.println("Finish Initialization");

    i2cMutex = xSemaphoreCreateMutex();

    xTaskCreate(TaskEnvironment, "Environment", 2048, NULL, 1, NULL);
    xTaskCreate(TaskDisplay, "Display", 2048, NULL, 1, NULL);
}

void loop()
{

}

void TaskEnvironment(void *pvParameneters)
{
    while(1)
    {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
        {
            temperature = bme.readTemperature();
            humidity = bme.readHumidity();
            pressure = bme.readPressure() / 100.0F; // hPa

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

            xSemaphoreGive(i2cMutex);
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void TaskDisplay(void *pvParameters)
{
    while(1)
    {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
        {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("Environment Dashboard\n");

            display.print("Temperature : ");
            display.print(temperature, 1);
            display.println("C");

            display.print("Humidity : ");
            display.print(humidity, 1);
            display.println("%");

            display.print("Pressure : ");
            display.print(pressure, 0);
            display.println("hPa");
            display.display();
            
            xSemaphoreGive(i2cMutex);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}