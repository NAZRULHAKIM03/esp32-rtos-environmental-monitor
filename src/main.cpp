#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>

typedef struct
{
    float temperature;
    float humidity;
    float pressure;
    float lux;
} SensorData;

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

QueueHandle_t sensorQueue;
SemaphoreHandle_t i2cMutex, button_semaphore;

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

    sensorQueue = xQueueCreate(5, sizeof(SensorData));

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
    SensorData currentData;

    while(1)
    {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
        {
            currentData.temperature = bme.readTemperature();
            currentData.humidity = bme.readHumidity();
            currentData.pressure = bme.readPressure() / 100.0F; // hPa
            currentData.lux = lightmeter.readLightLevel();

            xSemaphoreGive(i2cMutex);
        }

        if(isnan(currentData.temperature) || isnan(currentData.humidity) || isnan(currentData.pressure))
        {
            Serial.println("Failed to read from BME280");
        }
        else if(isnan(currentData.lux))
        {
            Serial.println("Failed to read from BH1750");
        }
        else
        {
            Serial.print("Temperature : ");
            Serial.print(currentData.temperature, 1);
            Serial.println("C");

            Serial.print("Humidity : ");
            Serial.print(currentData.humidity, 1);
            Serial.println("%");

            Serial.print("Pressure : ");
            Serial.print(currentData.pressure, 0);
            Serial.println("hPa");

            Serial.print("Lux : ");
            Serial.println(currentData.lux, 1);
        }

        xQueueSend(sensorQueue, &currentData, portMAX_DELAY);   // Send package to Display Task

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void TaskDisplay(void *pvParameters)
{
    SensorData receivedData;

    while(1)
    {
        if(xQueueReceive(sensorQueue, &receivedData, portMAX_DELAY) == pdTRUE)  // sleep until a package arrives
        {
            if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
            {
                display.clearDisplay();
                display.setCursor(0, 0);
                display.println("Environment Dashboard\n");

                display.print("Temperature : ");
                display.print(receivedData.temperature, 1);
                display.println(" C");

                display.print("Humidity : ");
                display.print(receivedData.humidity, 1);
                display.println(" %");

                display.print("Pressure : ");
                display.print(receivedData.pressure, 0);
                display.println(" hPa");

                display.print("Lux : ");
                display.println(receivedData.lux, 1);
                display.display();
                
                xSemaphoreGive(i2cMutex);
            }
        }
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