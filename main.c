#include <stdint.h>

#include <nordic_common.h>
#include <bsp.h>
#include <bsp_btn_ble.h>
#include <app_timer.h>
#include <peer_manager.h>

#pragma GCC diagnostic ignored "-Wint-conversion"
#define NRF_LOG_MODULE_NAME "BLE_DEVICE_INFORMATION"
#include <nrf_log.h>
#include <nrf_log_ctrl.h>
#undef ASSERT
#include <log/log.h>

#include <softdevice_handler.h>
#include <ble_advdata.h>
#include <ble_advertising.h>
#include <ble_db_discovery.h>
#include <ble_conn_params.h>
#include <ble_conn_state.h>
#include <ble_dis.h>

static void ble_event_handler(ble_evt_t *event) {
	log_enter("%d", event->header.evt_id);
	ble_conn_params_on_ble_evt(event);
	ble_conn_state_on_ble_evt(event);
	pm_on_ble_evt(event);
	bsp_btn_ble_on_ble_evt(event);

	switch(event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		info("connected");
		bsp_indication_set(BSP_INDICATE_CONNECTED);
		break;
	case BLE_GAP_EVT_DISCONNECTED:
		info("disconnected");
		break;
	default:
		dbg("unwanaged ble event type");
		break;
	}

	ble_advertising_on_ble_evt(event);
}

static void db_discovery_handler(ble_db_discovery_evt_t *event) {
	log_enter("%p", event);
}

static void adv_event_handler(ble_adv_evt_t event) {
	log_enter("%d", event);
	bsp_indication_set(BSP_INDICATE_ADVERTISING);
}

int main(void) {
	int prescaler = 0;
	uint32_t err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	APP_TIMER_INIT(prescaler, 4, false);

	err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS,
			APP_TIMER_TICKS(100, prescaler),
			NULL);
	APP_ERROR_CHECK(err_code);

	bsp_event_t startup_event;
	err_code = bsp_btn_ble_init(NULL, &startup_event);
	APP_ERROR_CHECK(err_code);

	nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
	SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

	ble_enable_params_t ble_enable_params;
	err_code = softdevice_enable_get_default_config(0, 1,
			&ble_enable_params);
	APP_ERROR_CHECK(err_code);

	CHECK_RAM_START_ADDR(0, 1);

	err_code = softdevice_enable(&ble_enable_params);
	APP_ERROR_CHECK(err_code);

	err_code = softdevice_ble_evt_handler_set(ble_event_handler);
	APP_ERROR_CHECK(err_code);

	err_code = pm_init();
	APP_ERROR_CHECK(err_code);

	ble_gap_sec_params_t sec_params = {
		.bond = 1,
		.mitm = 0,
		.lesc = 0,
		.keypress = 0,
		.io_caps = BLE_GAP_IO_CAPS_NONE,
		.oob = 0,
		.min_key_size = 7,
		.max_key_size = 16,
		.kdist_own = {
			.enc = 1,
			.id = 1
		},
		.kdist_peer = {
			.enc = 1,
			.id = 1
		}
	};

	err_code = pm_sec_params_set(&sec_params);
	APP_ERROR_CHECK(err_code);

	const char *device_name = "Device Informations";
	ble_gap_conn_sec_mode_t sec_mode;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
	err_code = sd_ble_gap_device_name_set(&sec_mode,
			(const uint8_t *)device_name,
			strlen(device_name));
	APP_ERROR_CHECK(err_code);

	ble_gap_conn_params_t gap_conn_params = {
		.min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),
		.max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),
		.slave_latency = 0,
		.conn_sup_timeout = MSEC_TO_UNITS(4000, UNIT_1_25_MS)
	};
	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);

	ble_advdata_t advdata = {
		.name_type = BLE_ADVDATA_FULL_NAME,
		.include_appearance = true,
		.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
		.uuids_complete = {
			.uuid_cnt = 0,
			.p_uuids = NULL
		}
	};
	ble_adv_modes_config_t options = {
		.ble_adv_fast_enabled = true,
		.ble_adv_fast_interval = 40,
		.ble_adv_fast_timeout = 30,

		.ble_adv_slow_enabled = true,
		.ble_adv_slow_interval = 3200,
		.ble_adv_slow_timeout = 180
	};
	err_code = ble_advertising_init(&advdata, NULL, &options, adv_event_handler, NULL);
	APP_ERROR_CHECK(err_code);

	err_code = ble_db_discovery_init(db_discovery_handler);
	APP_ERROR_CHECK(err_code);

	ble_dis_init_t dis_init = {};
	ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, "Leo G.");
	ble_srv_ascii_to_utf8(&dis_init.model_num_str, "coiot-dk");
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);
	err_code = ble_dis_init(&dis_init);
	APP_ERROR_CHECK(err_code);

	ble_conn_params_init_t cp_init = {
		.p_conn_params = NULL,
		.first_conn_params_update_delay = APP_TIMER_TICKS(5000, prescaler),
		.next_conn_params_update_delay = APP_TIMER_TICKS(30000, prescaler),
		.max_conn_params_update_count = 3,
		.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID,
		.disconnect_on_fail = true,
	};
	err_code = ble_conn_params_init(&cp_init);
	APP_ERROR_CHECK(err_code);

	err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(err_code);

	while(true) {
		if(!NRF_LOG_PROCESS()) {
			err_code = sd_app_evt_wait();
			APP_ERROR_CHECK(err_code);
		}
	}
}
