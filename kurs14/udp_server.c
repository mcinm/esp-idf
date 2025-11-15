#include <stdio.h>
#include <inttypes.h>
#include "lwip/inet.h"
#include "lwip/timeouts.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <sys/_timeval.h>


#define SSID "SSID"
#define PASSWD "HASŁO"


#define SERVER_PORT 12345
#define MSG "MCINM - esp32 udp client"

static const char *TAG = "mcinm_udp";


void wifi_init(void)
{
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *netif_handle = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif_handle, "MCINM");
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
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
    ESP_ERROR_CHECK(esp_wifi_connect());
}



void app_main(void)
{
	wifi_init();
vTaskDelay(4000 / portTICK_PERIOD_MS);
    printf("Hello world!\n");

    char bufor_rx[128];
	char client_addr[128];
    struct sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0 ) {
        ESP_LOGE(TAG, "Nie mozna stworzyc socketa: errno %d", errno);
        esp_restart();
    }

    // Ustawiamy timeout 15 sekund
    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    int err = bind(s, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (err < 0) {
        ESP_LOGE(TAG, "Nie udało się zbindować socketa: errno %d", errno);
	}
    ESP_LOGI(TAG, "Socket zbindowany na porcie: %d", SERVER_PORT);

    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);


    while (1) {
        ESP_LOGI(TAG, "Oczekiwanie na dane...");

        int len = recvfrom(s, bufor_rx, sizeof(bufor_rx) - 1, 0, (struct sockaddr*)&source_addr,&socklen);
        if (len < 0) {
            ESP_LOGE(TAG, "Timeout: errno %d", errno);
        }

        else {
			inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, client_addr, sizeof(client_addr)-1);
            bufor_rx[len] = 0;
			
            ESP_LOGI(TAG, "Odebrane %d bajtów od: %s.", len, client_addr);
            ESP_LOGI(TAG, "Odebrana wiadomość: %s", bufor_rx);
		}
	}
}
