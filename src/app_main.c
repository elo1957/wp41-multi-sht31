#include <Arduino.h>
// #include <WiFi.h>
#include <Wire.h>
#include "SHTSensor.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "mqtt_client.h"

#define NB_CAPTEURS 4
#define TCAADDR 0x70
static const char *TAG = "4_SHT31";
SHTSensor sht;
int msg_id = 0;
const char *ssid = "";
const char *password = "";
static EventGroupHandle_t mqtt_event_group;
static esp_mqtt_client_handle_t client = NULL;
const static int CONNECTED_BIT = BIT0;
char payl[256];
esp_mqtt_event_id_t k;
void tcaselect(uint8_t i)
{
    if (i > 7)
        return;
    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://mqtt3.thingspeak.com",
        .port = 1883,
        .client_id = "JR4YHzYXKBgwNTIrOBMMLT0",
        .username = "JR4YHzYXKBgwNTIrOBMMLT0",
        .password = ""};
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t start_result = esp_mqtt_client_start(client);
    if (start_result != ESP_OK)
    {
        ESP_LOGE(TAG, "MQTT client failed to start: %s", esp_err_to_name(start_result));
        return;
    }
    ESP_LOGI(TAG, "MQTT client started successfully");
}

void setup()
{
    Wire.begin();
    Serial.begin(9600);
    delay(1000);

    if (sht.init())
    {
        Serial.println("SHT sensor initialized successfully");
    }
    else
    {
        Serial.println("SHT sensor initialization failed");
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    mqtt_event_group = xEventGroupCreate();
    mqtt_app_start();
}

float moistures[NB_CAPTEURS];
float temperatures[NB_CAPTEURS];

void app_main()
{
    setup();
    while (1)
    {
        for (int i = 0; i < NB_CAPTEURS; ++i)
        {
            tcaselect(i);
            if (sht.readSample())
            {
                moistures[i] = sht.getHumidity();
                temperatures[i] = sht.getTemperature();
                Serial.printf("Sensor %d: Humidity: %.2f, Temperature: %.2f\n", i, moistures[i], temperatures[i]);
            }
            else
            {
                Serial.printf("Error reading from sensor %d\n", i);
            }
        }

        snprintf(payl, sizeof(payl), "field1=%.2f&field2=%.2f&field3=%.2f&field4=%.2f&field5=%.2f&field6=%.2f&field7=%.2f&field8=%.2f&status=MQTTPUBLISH",
                 moistures[0], temperatures[0], moistures[1], temperatures[1], moistures[2], temperatures[2], moistures[3], temperatures[3]);
        if (xEventGroupWaitBits(mqtt_event_group, CONNECTED_BIT, false, true, 1000 / portTICK_PERIOD_MS))
        {
            msg_id = esp_mqtt_client_publish(client, "channels/myChannelNumber/publish", payl, 0, 0, 0);
            ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
        }
        else
        {
            ESP_LOGE(TAG, "MQTT client not connected - attempting to reconnect");
            esp_mqtt_client_stop(client);
            mqtt_app_start(); // Re-initialize and start MQTT client
        }
    }

    delay(5000);
}