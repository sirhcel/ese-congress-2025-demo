#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "regex.h"

#include "wifi_scanner.h"


#define BACKGROUND_SCAN_STACK_SIZE 4096
#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE

#ifdef CONFIG_EXAMPLE_USE_SCAN_CHANNEL_BITMAP
#define USE_CHANNEL_BITMAP 1
#define CHANNEL_LIST_SIZE 3
static uint8_t channel_list[CHANNEL_LIST_SIZE] = {1, 6, 11};
#endif /*CONFIG_EXAMPLE_USE_SCAN_CHANNEL_BITMAP*/


static const char *TAG = "scan";


#ifdef USE_CHANNEL_BITMAP
static void array_2_channel_bitmap(const uint8_t channel_list[], const uint8_t channel_list_size, wifi_scan_config_t *scan_config) {

    for(uint8_t i = 0; i < channel_list_size; i++) {
        uint8_t channel = channel_list[i];
        scan_config->channel_bitmap.ghz_2_channels |= (1 << channel);
    }
}
#endif /*USE_CHANNEL_BITMAP*/


static void init_wifi(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}


static SemaphoreHandle_t background_scan_semaphore = NULL;
static SemaphoreHandle_t scan_data_semaphore = NULL;
static wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE] = {0, };
static uint16_t ap_count = 0;
static uint16_t ap_display_index = 0;


static void scan_networks(void *parameters) {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;

    ESP_LOGI(TAG, "WiFi background scan task running");

    for (;;) {
        xSemaphoreTake(background_scan_semaphore, portMAX_DELAY);

        ESP_LOGI(TAG, "WiFi background scan started");

        ap_count = 0;
        ap_display_index = 0;
        memset(ap_info, 0, sizeof(ap_info));

#ifdef USE_CHANNEL_BITMAP
        wifi_scan_config_t *scan_config = (wifi_scan_config_t *)calloc(1,sizeof(wifi_scan_config_t));
        if (!scan_config) {
            ESP_LOGE(TAG, "Memory Allocation for scan config failed!");
            return;
        }
        array_2_channel_bitmap(channel_list, CHANNEL_LIST_SIZE, scan_config);
        esp_wifi_scan_start(scan_config, true);
        free(scan_config);

#else
        esp_wifi_scan_start(NULL, true);
#endif /*USE_CHANNEL_BITMAP*/

        ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
        for (int i = 0; i < number; i++) {
            ESP_LOGI(TAG,
                "%d: ssid: %s, rssi: %d, channel: %d",
                i,
                ap_info[i].ssid,
                ap_info[i].rssi,
                ap_info[i].primary
            );
        }

        ESP_LOGI(TAG, "WiFi background scan done");

        xSemaphoreGive(scan_data_semaphore);
    }
}


typedef struct {
    lv_obj_t *screen;
    lv_obj_t *title;
} main_screen_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *title;
    lv_obj_t *ssid;
    lv_obj_t *rssi;
    lv_obj_t *auth;
} details_screen_t;

static main_screen_t main_screen = {0, };
static details_screen_t details_screen_1 = {0, };
static details_screen_t details_screen_2 = {0, };

static lv_timer_t *cycle_timer = NULL;


void create_details_screen(details_screen_t *screen, const char *title) {
    screen->screen = lv_obj_create(NULL);
    lv_obj_t *view = lv_obj_create(screen->screen);
    lv_obj_set_size(view, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);

    screen->title = lv_label_create(view);
    screen->ssid = lv_label_create(view);
    screen->rssi = lv_label_create(view);
    screen->auth = lv_label_create(view);
}


void create_main_screen(main_screen_t *screen, const char *title) {
    screen->screen = lv_obj_create(NULL);
    lv_obj_t *view = lv_obj_create(screen->screen);
    lv_obj_set_size(view, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);

    screen->title = lv_label_create(view);
    // lv_obj_add_style(label_title, &style_label, 0);
    lv_label_set_text(screen->title, title);
}


static const char *pretty_authmode(int authmode) {
    switch (authmode) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_OWE: return "One";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2 PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2 PSK";
        case WIFI_AUTH_ENTERPRISE: return "Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3 PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3 PSK";
        case WIFI_AUTH_WPA3_ENT_192: return "WPA3 Enterprise 192";
        default: return "Unknown";
    }
}


static void cycle_timer_cb(lv_timer_t *timer) {
    lv_obj_t *current_screen = lv_scr_act();
    lv_obj_t *new_screen = NULL;

    if (ap_display_index < ap_count) {
        const wifi_ap_record_t *info = &ap_info[ap_display_index];
        details_screen_t *new_details = NULL;

        ESP_LOGI(TAG, "about to display details %d/%d", ap_display_index, ap_count);

        // Determine next screen object to prepare.
        if (current_screen == details_screen_1.screen) {
            new_details = &details_screen_2;
            new_screen = new_details->screen;
        } else {
            new_details = &details_screen_1;
            new_screen = new_details->screen;
        }

        // Setup information for next screen.
        lv_label_set_text_fmt(
            new_details->title,
            "Network %" PRIu16 "/%" PRIu16,
            ap_display_index + 1,
            ap_count
        );
        lv_label_set_text(new_details->ssid, (const char *)info->ssid);
        lv_label_set_text_fmt(new_details->rssi, "RSSI: %d", info->rssi);
        lv_label_set_text_fmt(new_details->auth, "Auth: %s", pretty_authmode(info->authmode));

        ap_display_index += 1;
    } else {
        new_screen = main_screen.screen;
    }

    if (current_screen == main_screen.screen && new_screen != main_screen.screen) {
        xSemaphoreTake(scan_data_semaphore, portMAX_DELAY);
    } else if (current_screen != main_screen.screen && new_screen == main_screen.screen) {
        xSemaphoreGive(background_scan_semaphore);
    }

    if (new_screen != NULL) {
        lv_scr_load_anim(
            new_screen,
            LV_SCR_LOAD_ANIM_OVER_LEFT,
            // LV_SCR_LOAD_ANIM_FADE_IN,
            500,
            0,
            false
        );
    }
}


void wifi_scanner(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    background_scan_semaphore = xSemaphoreCreateBinary();
    assert(background_scan_semaphore);
    xSemaphoreGive(background_scan_semaphore);

    scan_data_semaphore = xSemaphoreCreateBinary();
    assert(scan_data_semaphore);

    create_main_screen(&main_screen, "WiFi Scanner");
    assert(main_screen.screen);
    create_details_screen(&details_screen_1, "Network 1");
    assert(details_screen_1.screen);
    create_details_screen(&details_screen_2, "Network 2");
    assert(details_screen_2.screen);

    lv_scr_load(main_screen.screen);

    init_wifi();

    xTaskCreate(
        scan_networks,
        "WiFi Scan",
        BACKGROUND_SCAN_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY,
        NULL
    );

    cycle_timer = lv_timer_create(cycle_timer_cb, 5000, NULL);
    assert(cycle_timer);
}
