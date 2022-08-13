#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#define SSID "ssid_sieci"
#define PASSWD "hasło"


void wifi_init(void)
{
    nvs_flash_init();
    tcpip_adapter_init();
    esp_event_loop_create_default();
    
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    
    wifi_config_t net_conf = {
        .sta = {
            .ssid = SSID,
            .password = PASSWD
        },
    };
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &net_conf);
    esp_wifi_start();
    esp_wifi_connect();
}
void app_main()
{
    wifi_init();
}
