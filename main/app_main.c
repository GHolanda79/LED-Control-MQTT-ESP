#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_led_state_event_group;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - led on
 * - led off */

#define LED_OFF_BIT    BIT0
#define LED_ON_BIT     BIT1

static const char *TAG_APP = "app_event";
static const char *TAG_MQTT = "mqtt_event";
static const char *TAG_LED = "led_event";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/led/state", 2);
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);    
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        ESP_LOGI(TAG_LED, "SET BITS");
        if(strncmp(event->topic, "/led/state", event->topic_len) == 0)
        {
            if(strncmp(strupr(event->data), "ON", event->data_len) == 0)
            {
                xEventGroupSetBits(s_led_state_event_group, LED_ON_BIT);
                ESP_LOGI(TAG_LED, "SET LED_ON_BIT");
            } else if(strncmp(strupr(event->data), "OFF", event->data_len) == 0)
            {
                xEventGroupSetBits(s_led_state_event_group, LED_OFF_BIT);
                ESP_LOGI(TAG_LED, "SET LED_OFF_BIT");
            }
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }

        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
}

// Start MQTT service
static void mqtt_app_start(void)
{
    // Configure mttq URI
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URI,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// Start LED
static void led_start(void)
{
    // Create event group for LED
    s_led_state_event_group = xEventGroupCreate();
    ESP_LOGI(TAG_LED, "Group Event s_led_state_event_group created");

    // Configure LED
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),          // configure pins
        .mode = GPIO_MODE_OUTPUT,                   // operation mode
        .pull_up_en = GPIO_PULLUP_DISABLE,          // internal pull-up resistance enable or desable
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      // internal pull-down resistance enable or desable
        .intr_type = GPIO_INTR_DISABLE              // interruption type
    };

    // Configure LED in GPIO port
    gpio_config(&led_conf);
    ESP_LOGI(TAG_LED, "LED configured in port GPIO%d", LED_PIN);
}


static void led_task(void *param)
{
    while (1) {
        // Configure bits for each specific event 
        EventBits_t bits = xEventGroupWaitBits(
            s_led_state_event_group,        // xEventGroup: the event group handler
            LED_ON_BIT | LED_OFF_BIT,       // uxBitsToWaitFor: wait for one of the bits
            pdTRUE,                         // xClearOnExit: Defines whether event bits should be cleared after the task is unlocked.
            pdFALSE,                        // xWaitForAllBits: Set whether the task should expect all bits of the mask or just one
            portMAX_DELAY);                 // xTicksToWait: Set whether the task should expect all bits of the mask or just one

        if (bits & LED_ON_BIT) {
            gpio_set_level(LED_PIN, 1);
            ESP_LOGI(TAG_LED, "LED ON");        // Set LED state to ON
        } else if (bits & LED_OFF_BIT) {
            gpio_set_level(LED_PIN, 0);         // Set LED state to OFF
            ESP_LOGI(TAG_LED, "LED OFF");
        }
        vTaskDelay(pdMS_TO_TICKS(100));         // check the status of the led every 100 ms 
    }
}

void app_main(void)
{
    ESP_LOGI(TAG_APP, "[APP] Startup..");
    ESP_LOGI(TAG_APP, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG_APP, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_ws", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
    led_start();
    xTaskCreate(led_task, "LED Task", 2048, NULL, 1, NULL);
}
