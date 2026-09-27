#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON 25
#define LED 32
#define BUZZER 33

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme;
BH1750 lightmeter;

void TaskDisplay(void *pvParameters);
void TaskEnvironment(void *pvParameters);
void TaskAlarm(void *pvParameters);

SemaphoreHandle_t i2cMutex, button_semaphore;
float temperature = 0, humidity = 0, pressure = 0, lux = 0;

void IRAM_ATTR button_isr()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;              // Special Variable to let RTOS know if waking up task require context switch
    xSemaphoreGiveFromISR(button_semaphore, &xHigherPriorityTaskWoken);   // if Task waken higher priority than previous running task pdTRUE

    if(xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();                                   // After isr finish execute, instead of continue previous running task the RTOS will immediately switch to higher priority task(no unnecessary delay)
    }
}

void setup()
{
    Serial.begin(115200);
    Wire.begin();
    pinMode(BUTTON, INPUT_PULLUP);
    pinMode(LED, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("Fail to initiliaze Oled Display!!!");
        while(true);
    }

    if(!lightmeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire))
    {
        Serial.println("Failed to Initialize BH1750!!!");
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
    button_semaphore = xSemaphoreCreateBinary();

    xTaskCreate(TaskEnvironment, "Environment", 2048, NULL, 1, NULL);
    xTaskCreate(TaskDisplay, "Display", 2048, NULL, 1, NULL);
    xTaskCreate(TaskAlarm, "Interrupt", 1024, NULL, 2, NULL);

    attachInterrupt(BUTTON, button_isr, FALLING);
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
            lux = lightmeter.readLightLevel();

            if(isnan(temperature) || isnan(humidity) || isnan(pressure))
            {
                Serial.println("Failed to read from BME280");
            }
            else if(isnan(lux))
            {
                Serial.println("Failed to read from BH1750");
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

                Serial.print("Lux : ");
                Serial.println(lux, 1);
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
            display.println(" C");

            display.print("Humidity : ");
            display.print(humidity, 1);
            display.println(" %");

            display.print("Pressure : ");
            display.print(pressure, 0);
            display.println(" hPa");

            display.print("Lux : ");
            display.println(lux, 1);
            display.display();
            
            xSemaphoreGive(i2cMutex);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void TaskAlarm(void *pvParameters)
{
    while(1)
    {
        if(xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) // portMAX_DELAY means "sleep forever and use 0% CPU until semaphore given"
        {
            digitalWrite(LED, HIGH);
            digitalWrite(BUZZER, HIGH);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            digitalWrite(LED, LOW);
            digitalWrite(BUZZER, LOW);
        }
    }
}