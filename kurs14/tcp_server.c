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

#define SERVER_IP "192.168.XX.XX"
#define SERVER_PORT 80
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



static void do_retransmit(const int s)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(s, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Nie udało się odebrać wiadomości: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Połączenie zakończone");
        } else {
            rx_buffer[len] = 0;
            ESP_LOGI(TAG, "Odebrano %d bajtów: %s", len, rx_buffer);
        }
    } while (len > 0);
    static char payload[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 19\r\n\r\nMCINM - Hello World";
    send(s, payload, sizeof(payload), 0);
}




void app_main(void)
{
    wifi_init();
    printf("Test esp32 TCP client - MCINM\n");
    sleep(5); // Dajmy czas na nawiązanie połączenia z wifi

    // Bufor na adres klienta
    char addr_str[128];
    struct sockaddr_storage dest_addr;
    int keepAlive = 1; // true - włączamy procedurę keepalive
    int keepIdle = 5; // Czas bezczynności w sekundach po jakim server zacznie wysyłać okresowe pakiety sprawdzające czy client jest osiągalny 
    int keepInterval = 5; // Czas w sekundach między pakietami sprawdzającymi
    int keepCount = 3; // Ilość pakietów sprawdzających po których (bez odpowiedzi) server porzuci połączenie
	
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(SERVER_PORT);
    
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Nie udało się utworzyć socketa: errno %d", errno);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Nie udało się zbindować socketa: errno %d", errno);
    }

    err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Wystąpił problem przy próbie nasłuchu socketa: errno %d", errno);
         }


    while (true) {
        sleep(5);
        printf("Kolejna pętla\n");

        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int s = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (s < 0) {
            ESP_LOGE(TAG, "Nie udało się zaakceptować połączenia: errno %d", errno);
        }

        setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(s, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        do_retransmit(s);

        shutdown(s, 0);
        close(s);
    }
}
