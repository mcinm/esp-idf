#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

gptimer_handle_t gptimer = NULL;


typedef struct {
	uint8_t liczba;
	char *tekst;
} dane;


static bool IRAM_ATTR przerwanie(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
	
	dane *dane1 = (dane *)user_ctx;
	
	ets_printf("dane_timera: liczba: %d, tekst: %s\n", (uint8_t)dane1->liczba, (char*)dane1->tekst);
	ets_printf("Aktualny stan licznika: %d, stan licznika po relaodzie: %d\n\n", (uint64_t)edata->count_value, (uint64_t)edata->alarm_value);

	return true;
}


void app_main(void)
{
	
	gptimer_config_t timer_conf = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = 1 * 1000 * 1000,
	};
	gptimer_new_timer(&timer_conf, &gptimer);
	
	gptimer_alarm_config_t alarm_config = {
    .alarm_count = 1000000,
    .flags.auto_reload_on_alarm = true,
    .reload_count = 500,
};
	gptimer_set_alarm_action(gptimer, &alarm_config);
	
	gptimer_event_callbacks_t cbs = {
		.on_alarm = przerwanie,
	};
	
	dane dane_timera;
	dane_timera.liczba = 30;
	dane_timera.tekst = "przykladowy tekst";
	
	gptimer_register_event_callbacks(gptimer, &cbs, &dane_timera);
	gptimer_enable(gptimer);
	gptimer_start(gptimer);
	

    while (true) {
		
		dane_timera.liczba ++;
        vTaskDelay( 1000 / portTICK_PERIOD_MS);
    }
}
