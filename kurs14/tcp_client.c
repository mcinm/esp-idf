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

#define SERVER_IP "192.168.X.X"
#define SERVER_PORT 9999
#define MSG "MCINM - esp32 TCP client"

static const char *TAG = "mcinm_tcp";


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
    printf("Test esp32 TCP client - MCINM\n");
    sleep(5); // Dajmy czas na nawiązanie połączenia z wifi

    // Bufor
    char rx_buffer[128];

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    
    // Tworzenie gniazda
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0 ) {
        ESP_LOGE(TAG, "Nie mozna stworzyc socketa: %d", errno);
        esp_restart();
    }
    
    ESP_LOGI(TAG, "Socket utworzony, łączenie z %s:%d", SERVER_IP, SERVER_PORT);
    

    // Łączenie z serwerem
    int err = connect(s, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
            ESP_LOGE(TAG, "Socket nie może się połączyć: errno %d", errno);
            esp_restart();
        }

    while (true) {
        sleep(5);
        printf("Kolejna pętla\n");
        int err = send(s, MSG, strlen(MSG), 0);
		if (err < 0) {
            ESP_LOGE(TAG, "Błąd podczas wysyłania: errno %d", errno);
            ESP_LOGE(TAG, "Zamykanie gniazda.");
            shutdown(s, 0);
            close(s);
            break;
        }

		int len = recv(s, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Błąd podczase odbioru odpowiedzi: errno %d", errno);
            break;
        }

        else {
            rx_buffer[len] = 0;
            ESP_LOGI(TAG, "Odebrano %d bajtów od %s", len, SERVER_IP);
            ESP_LOGI(TAG, "Wiadomość zwrotna: %s", rx_buffer);
		}
    }
}
