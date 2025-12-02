// FIXME: Replace with generated header

#ifdef __cplusplus
extern "C" {
#endif

int rusty_add(int left, int right);
bool rusty_generate_qr_lv_img_data(
    const char *message,
    size_t module_size,
    uint8_t *data,
    size_t *data_len,
    uint16_t *width,
    uint16_t *height
);

#ifdef __cplusplus
}
#endif
