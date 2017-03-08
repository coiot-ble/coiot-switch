#include <stdint.h>

#include <app_timer.h>
#include <peer_manager.h>
#define NRF_LOG_MODULE_NAME "BLE_DEVICE_INFORMATION"
#include <nrf_log.h>

static void *seq_req_timeout_handler_arg = NULL;
void sec_req_timeout_handler(void *ctx) {
	uint16_t *cur_handle = seq_req_timeout_handler_arg;
	if(*cur_handle != BLE_CONN_HANDLE_INVALID) {
		pm_conn_sec_status_t status;
		uint32_t err_code = pm_conn_sec_status_get(*cur_handle, &status);
		APP_ERROR_CHECK(err_code);

		if(!status.encrypted) {
			NRF_LOG_INFO("Start encryption\r\n");
			err_code = pm_conn_secure(*cur_handle, false);
			APP_ERROR_CHECK(err_code);
		}
	}
}

int main(void) {
	uint32_t err_code;

	APP_TIMER_INIT(0, 4, false);
	APP_TIMER_DEF(sec_req_timer_id);

	err_code = app_timer_create(&sec_req_timer_id, APP_TIMER_MODE_SINGLE_SHOT, sec_req_timeout_handler);
	APP_ERROR_CHECK(err_code);
}
