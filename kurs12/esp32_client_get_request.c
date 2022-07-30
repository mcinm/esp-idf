#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"

#define SSID "nazwa_sieci"
#define PASSWD "hasło"


void wifi_init(void)
{
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *netif_handle = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif_handle, "MCINM");
    
    wifi_init_congif_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_config_t conf = {
        .sta = {
            .ssid = SSID,
            .password = PASSWD,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    uint8_t mac[6] = {0x70, 0xB3, 0xD5, 0x6A, 0x02, 0x01};
    esp_wifi_set_mac(WIFI_IF_STA, mac);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &conf));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t event_get(esp_http_client_event_handle_t get)
{
    switch (get->event_id)
    {
            case HTTP_EVENT_ON_DATA:
                printf("%.*s\n", get->data_len, (char*)get->data);
                break;
            default:
                break;
    }
    return ESP_OK;
}
void get_req(void)
{
    esp_http_client_config_t config = {
        .url = "http://piteusz.ovh",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = event_get
    };
    
    esp_http_client_handle_t req = esp_http_client__init(&config);
    esp_http_client_perform(req);
    esp_http_client_cleanup(req);
}
void app_main(void)
{
    wifi_init(();
    vTaskDelay( 5000 / portTICK_PERIOD_MS);
    get_req();
}
