#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_config.h"
#include "esp_http_server.h"
#include "wifi_config.h"
#include "blink.c"

#include <esp_log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define WIFI_SSID wifi_name
#define WIFI_PASS wifi_password

static const char *TAG = "WiFiStation";
static EventGroupHandle_t wifi_event_group;
static int device_state = 0;

#define WIFI_CONNECTED_BIT BIT0 // Event bits

// Notes:
// Use NVS to store wifi details at run time (more secure)
// Investigate using WPA3 rather than WPA2
// Use HTTPS rather than http

// Whitelist handle
// static bool is_whitelisted(const char *client_ip)
// {
//     const char *whitelist[] = WHITELIST;
//     for (int i = 0; whitelist[i] != NULL; i++) {
//         if (strcmp(client_ip, whitelist[i]) == 0) {
//             return true;
//         }
//     }
//     return false;
// }

// Event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static bool previous_retry;
    static int8_t attempt;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (attempt < max_retries)
        {
            if (!previous_retry)
            {
                ESP_LOGW(TAG, "Disconnected. Reconnecting (attempt %d/%d)...", attempt + 1, max_retries);
                esp_wifi_connect();
                previous_retry = true;
                attempt++;
            }
            else
            {
                previous_retry = false;
                esp_wifi_connect();
            }
        }
        else 
        {
            ESP_LOGE(TAG, "Disconnected. Max retries exceeded!");
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        attempt = 0;
    }
}

// Wi-Fi initialization
void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialized.");
}

// Send command to Device
// curl -X POST http://<ESP32_IP>/ -d "[COMMAND]"

// AFTER THIS MAKE A WAY FOR EXTERNAL DEVICE CAN READ ESP32's KNOWLEDGE OF PC

static void update_device_state(const char *command, int *device_state)
{
    // Turn ON only if currently OFF
    if (strcmp(command, "DEVICE ON") == 0) {
        if (*device_state == 0 || *device_state == 2) {
            ESP_LOGI("DEVICE_STATUS", "Turning On");
            *device_state = 1;
            led_loop(*device_state);
        } else {
            ESP_LOGI("DEVICE_STATUS", "Device already ON");
        }
    }
    // Turn OFF only if currently ON
    else if (strcmp(command, "DEVICE OFF") == 0) {
        if (*device_state == 1) {
            ESP_LOGI("DEVICE_STATUS", "Turning Off");
            *device_state = 2;
            led_loop(*device_state);
        } else {
            ESP_LOGI("DEVICE_STATUS", "Device already OFF");
        }
    }
    else {
        ESP_LOGW("DEVICE_STATUS", "UNKNOWN COMMAND: %s", command);
    }
}

void post_handle(httpd_req_t *req, int length)
{
    char buffer[128];
    
    int ret = httpd_req_recv(req, buffer, length);
    if (ret > 0) {
        buffer[ret] = '\0';
        update_device_state(buffer, &device_state);
    } else {
        ESP_LOGW("POST_HANDLE", "No data received.");
    }
}

static esp_err_t post_handler(httpd_req_t *req)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    httpd_resp_send(req, "POST successfully sent to ESP32", HTTPD_RESP_USE_STRLEN);
    post_handle(req, req->content_len);
    return ESP_OK;
}

void server_initiation()
{
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server_handle = NULL;
    httpd_start(&server_handle, &server_config);
    httpd_uri_t uri_post = {
        .uri = "/",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server_handle, &uri_post);
}

// Read PC Power State
void led_monitor_task(void *pvParameters)  {
    int *device_state = (int *)pvParameters; 
    while (1) {
        *device_state = led_loop(0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    led_setup();
    wifi_init_sta();
    server_initiation();
    xTaskCreate(led_monitor_task, "led_monitor_task", 2048, &device_state, 5, NULL);
}