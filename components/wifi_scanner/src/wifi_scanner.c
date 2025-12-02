#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "regex.h"

#include "rusty.h"
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
static uint16_t ap_info_length = 0;
static uint16_t ap_info_index = 0;
static uint16_t ap_count = 0;


static void scan_networks(void *parameters) {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;

    ESP_LOGI(TAG, "WiFi background scan task running");

    for (;;) {
        xSemaphoreTake(background_scan_semaphore, portMAX_DELAY);

        ESP_LOGI(TAG, "WiFi background scan started");

        ap_count = 0;
        ap_info_index = 0;
        ap_info_length = 0;
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
        ap_info_length = number;

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


#define QR_IMG_DATA_HEADER_LENGTH 8
#define QR_IMG_DATA_DATA_MAX_LENGTH 1024
#define QR_IMG_DATA_CAPACITY (QR_IMG_DATA_HEADER_LENGTH + QR_IMG_DATA_DATA_MAX_LENGTH)
#define QR_IMG_MODULE_SIZE 2


typedef struct {
    lv_obj_t *screen;
    lv_obj_t *title;
    lv_obj_t *status;
} main_screen_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *title;
    lv_obj_t *ssid;
    lv_obj_t *rssi;
    lv_obj_t *auth;
    lv_obj_t *qr;
    lv_img_dsc_t *qr_img_dsc;
} details_screen_t;

static main_screen_t main_screen = {0, };
static details_screen_t details_screen_1 = {0, };
static details_screen_t details_screen_2 = {0, };

static lv_style_t label_style;

static lv_timer_t *cycle_timer = NULL;


static uint8_t empty_image_data[QR_IMG_DATA_HEADER_LENGTH] = {
    0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0xff,
};

static lv_img_dsc_t empty_img_dsc = {
    .header.always_zero = 0,
    .header.w = 0,
    .header.h = 0,
    .data_size = QR_IMG_DATA_HEADER_LENGTH,
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .data = empty_image_data,
};

static uint8_t image_data_1[QR_IMG_DATA_CAPACITY] = {
    0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0xff,
    0x00,
};

static lv_img_dsc_t qr_img_dsc_1 = {
    .header.always_zero = 0,
    .header.w = 0,
    .header.h = 0,
    .data_size = QR_IMG_DATA_HEADER_LENGTH,
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .data = image_data_1,
};

static uint8_t image_data_2[QR_IMG_DATA_CAPACITY] = {
    0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0xff,
    0x00,
};

static lv_img_dsc_t qr_img_dsc_2 = {
    .header.always_zero = 0,
    .header.w = 0,
    .header.h = 0,
    .data_size = QR_IMG_DATA_HEADER_LENGTH,
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .data = image_data_2,
};


static void init_styles(void) {
    lv_style_init(&label_style);
    lv_style_set_text_color(&label_style, lv_color_hex(0x657377));
    lv_style_set_text_font(&label_style, &lv_font_montserrat_18);
}


void init_details_screen(
    details_screen_t *screen,
    const char *title,
    lv_img_dsc_t *qr_img_dsc
) {
    screen->screen = lv_obj_create(NULL);
    lv_obj_t *view = lv_obj_create(screen->screen);
    lv_obj_set_size(view, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);

    screen->title = lv_label_create(view);
    lv_obj_add_style(screen->title, &label_style, 0);

    screen->ssid = lv_label_create(view);
    lv_obj_set_style_text_font(screen->ssid, &lv_font_montserrat_32, 0);
    screen->rssi = lv_label_create(view);
    lv_obj_set_style_text_font(screen->rssi, &lv_font_montserrat_18, 0);
    lv_label_set_recolor(screen->rssi, true);
    screen->auth = lv_label_create(view);
    lv_obj_set_style_text_font(screen->auth, &lv_font_montserrat_18, 0);
    lv_label_set_recolor(screen->auth, true);

    screen->qr = lv_img_create(screen->screen);
    lv_img_set_src(screen->qr, &empty_img_dsc);
    screen->qr_img_dsc = qr_img_dsc;
}


void init_main_screen(main_screen_t *screen, const char *title) {
    screen->screen = lv_obj_create(NULL);
    lv_obj_t *view = lv_obj_create(screen->screen);
    lv_obj_set_size(view, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);

    screen->title = lv_label_create(view);
    lv_obj_set_style_text_font(screen->title, &lv_font_montserrat_32, 0);
    lv_label_set_text(screen->title, title);

    screen->status = lv_label_create(view);
    lv_obj_add_style(screen->status, &label_style, 0);
    lv_obj_set_style_text_font(screen->status, &lv_font_montserrat_18, 0);
    lv_label_set_text(screen->status, "Scanning ...");
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

    if (ap_info_index < ap_info_length) {
        const wifi_ap_record_t *info = &ap_info[ap_info_index];
        details_screen_t *new_details = NULL;

        ESP_LOGI(TAG, "about to display details %d/%d", ap_info_index + 1, ap_info_length);

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
            "Network %" PRIu16 "/%" PRIu16 " (%" PRIu16 ")",
            ap_info_index + 1,
            ap_info_length,
            ap_count
        );
        lv_label_set_text(new_details->ssid, (const char *)info->ssid);
        lv_label_set_text_fmt(new_details->rssi, "#657377 RSSI:# %d", info->rssi);
        lv_label_set_text_fmt(new_details->auth, "#657377 Auth:# %s", pretty_authmode(info->authmode));

        lv_img_dsc_t *img_dsc = new_details->qr_img_dsc;
        // Safety: The data backing img_dsc->data is actually writeable and
        // that's what we need for updating the image.
        uint8_t *pixel_data = (uint8_t *)img_dsc->data + QR_IMG_DATA_HEADER_LENGTH;
        size_t pixel_data_length = QR_IMG_DATA_DATA_MAX_LENGTH;
        uint16_t width = 0;
        uint16_t height = 0;

        // ESP_LOGI(TAG, "before rusty_generate_qr: pixel_data: %p, pixel_data_length: %u",
        //     pixel_data, pixel_data_length);

        char message[256] = {0, };
        const size_t message_capacity = sizeof(message)/sizeof(char);
        const int printed = snprintf(
            message,
            message_capacity,
            // FIXME: Use the right scheme and add more information.
            "WIFI:T:foo;S:%s;H:false;;",
            (const char *)info->ssid
        );

        if (printed < message_capacity
            && rusty_generate_qr_lv_img_data(message, QR_IMG_MODULE_SIZE, pixel_data, &pixel_data_length, &width, &height))
        {
            // ESP_LOGI(TAG, "rusty_generate_qr(%s, %p, %u, %hu, %hu)",
            //     message,
            //     pixel_data,
            //     pixel_data_length,
            //     width,
            //     height
            // );

            img_dsc->data_size = QR_IMG_DATA_HEADER_LENGTH + pixel_data_length;
            img_dsc->header.w = width;
            img_dsc->header.h = height;

            // ESP_LOG_BUFFER_HEXDUMP(TAG, img_dsc->data, img_dsc->data_size, ESP_LOG_INFO);

            lv_img_set_src(new_details->qr, img_dsc);
            lv_obj_align(new_details->qr, LV_ALIGN_BOTTOM_RIGHT, -15, -25);
        } else {
            lv_img_set_src(new_details->qr, &empty_img_dsc);
        }

        ap_info_index += 1;
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
            300,
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

    init_styles();
    init_main_screen(&main_screen, "WiFi Scanner");
    assert(main_screen.screen);
    init_details_screen(&details_screen_1, "Template 1", &qr_img_dsc_1);
    assert(details_screen_1.screen);
    init_details_screen(&details_screen_2, "Template 2", &qr_img_dsc_2);
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
