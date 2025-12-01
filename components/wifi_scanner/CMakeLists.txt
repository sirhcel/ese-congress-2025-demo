idf_component_register(
    SRCS "src/wifi_scanner.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_wifi lvgl nvs_flash
)
