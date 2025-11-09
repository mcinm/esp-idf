#include <stdio.h>
#include <inttypes.h>
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

#define SERVER_IP "192.168.XXX"
#define SERVER_PORT 9999
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
    printf("MCINM - esp32 UDP Client\n");
    
    
    struct sockaddr_in conn_config;
    char bufor_recv[128]; // bufor na odpowiedź z serwera
    
    conn_config.sin_addr.s_addr = inet_addr(SERVER_IP);
    conn_config.sin_family = AF_INET;
    conn_config.sin_port = htons(SERVER_PORT);

   
       int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
       if (s < 0 ) {
        ESP_LOGE(TAG, "Nie mozna stworzyc socketa: %d", errno);
        esp_restart();
    }
    
   // Ustawiamy timeout 2.5 sekundy
   struct timeval timeout;
   timeout.tv_sec = 2;
   timeout.tv_usec = 500000;
   
   if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        ESP_LOGE(TAG, "Nie mozna ustawic timeout'u: %d", errno);
    }
   
  while(1) {
    int err = sendto(s, MSG, strlen(MSG), 0, (struct sockaddr *)&conn_config, sizeof(conn_config));
      if (err < 0) {
            ESP_LOGE(TAG, "Problem z wysłaniem: %d", errno);
            // No break, client might try again
        } else {
            ESP_LOGI(TAG, "Wysłano: %s", MSG);
        }

    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);
    int len = recvfrom(s, bufor_recv, sizeof(bufor_recv) - 1, 0, (struct sockaddr *)&source_addr, &socklen); // Oczekiwanie na dane i zapis ich w buforze "bufor_recv"
    if (len < 0) {
        ESP_LOGE(TAG, "Nie odebrano danych: errno %d", errno);
    }
    else {
        bufor_recv[len] = 0;
        ESP_LOGI(TAG, "Odebrano %d bajty(ów) od %s", len, (char*)SERVER_IP);
        ESP_LOGI(TAG, "%s", bufor_recv);
        if (strncmp(bufor_recv, "OK", 2) == 0) { // Sprawdzamy czy pierwsze dwa znaki to "OK"
            ESP_LOGI(TAG, "Odbiór poprawny");
        }
    }
      
      vTaskDelay(3000 / portTICK_PERIOD_MS);
    printf("Kolejna pętla\n");
  }
}
