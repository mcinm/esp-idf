/*
 * MCINM.PL - I2S MICROPHONE (ICS-43434) UDP TRANSFER
 * https://mcinm.pl/kurs-esp32-11-i2s
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "driver/i2s_types.h"
#include "esp_err.h"
#include "hal/i2s_types.h"
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

#include "driver/i2s_std.h"
#include "driver/gpio.h"



#define SSID "SSID"
#define PASSWD "PASSWD"

#define SERVER_IP "192.168.xx.xx"
#define SERVER_PORT 8876

#define SAMPLE_BUFFER_SIZE 128
#define SAMPLE_RATE 12000
#define BIT_RATE 32


#define GPIO_BCLK GPIO_NUM_3
#define GPIO_DIN GPIO_NUM_2
#define GPIO_WS GPIO_NUM_1
#define GPIO_DOUT GPIO_NUM_5


static i2s_chan_handle_t                rx_chan;

static const char *TAG = "mcinm_mikro_udp";


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


static void i2s_init(void) {

	i2s_chan_config_t i2s_channel_cfg;
	i2s_channel_cfg.id = I2S_NUM_0;
	i2s_channel_cfg.role = I2S_ROLE_MASTER;
	i2s_channel_cfg.dma_desc_num = 3;
	i2s_channel_cfg.dma_frame_num = 1020,
	i2s_channel_cfg.allow_pd = false;
	i2s_channel_cfg.intr_priority = 0;

	i2s_new_channel(&i2s_channel_cfg, NULL, &rx_chan);

	i2s_std_config_t i2s_cfg;
	i2s_std_clk_config_t clk_config = {
		.clk_src = I2S_CLK_SRC_DEFAULT,
		.sample_rate_hz = SAMPLE_RATE,
		//.ext_clk_freq_hz = 0,
		.mclk_multiple = I2S_MCLK_MULTIPLE_192,
		//.bclk_div = 8,
	};

	i2s_std_slot_config_t slot_config = {
		.data_bit_width = 32,
		.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
		.slot_mode = I2S_SLOT_MODE_MONO,
		.slot_mask = I2S_STD_SLOT_LEFT,
		.ws_width = 32,
		.ws_pol = false,
		.bit_shift = true,
	};

	i2s_std_gpio_config_t i2s_gpio_config = {
		.mclk = I2S_GPIO_UNUSED,
		.bclk = GPIO_BCLK,
		.ws = GPIO_WS,
		.dout = GPIO_DOUT,
		.din = GPIO_DIN,
		.invert_flags = {
			.bclk_inv = false,
			.mclk_inv = false,
			.ws_inv = false,
		},
	};

	i2s_cfg.clk_cfg = clk_config;
	i2s_cfg.slot_cfg = slot_config;
	i2s_cfg.gpio_cfg = i2s_gpio_config;

	ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &i2s_cfg));
	ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}



void app_main(void)
{
    wifi_init();
    printf("MCINM - esp32 UDP I2S Client\n");
    sleep(5); // Dajmy czas na nawiązanie połączenia z wifi
    

	  i2s_init();

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
    
	int32_t raw_samples[SAMPLE_BUFFER_SIZE];

   
  while(1) {
	    size_t bytes_read = 0;
	    i2s_channel_read(rx_chan, raw_samples, 128, &bytes_read, 100);

    	int samples_read = bytes_read / sizeof(int32_t);

    	if (samples_read > 0) {
		     sendto(s, (uint8_t*)raw_samples, samples_read * sizeof(int32_t), 0, (struct sockaddr *)&conn_config, sizeof(conn_config));
	    }
  }
}
