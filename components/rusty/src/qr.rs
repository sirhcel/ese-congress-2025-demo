use core::ffi::{CStr, c_char};
use qrcode::QrCode;

const DARK_BIT: u8 = 1;
const LIGHT_BIT: u8 = 0;

/// Generates a QR code image for the given message. Image date gets written to `data` and
/// `data_len`, `width`, and `height` will be updated accordingly.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rusty_generate_qr_lv_img_data(
    message: *const c_char,
    data: *mut u8,
    data_len: *mut usize,
    width: *mut u16,
    height: *mut u16,
) -> bool {
    let data_len = unsafe { data_len.as_mut() };
    let mut result = false;

    if let Some(data_len) = data_len
        && !message.is_null()
        && !width.is_null()
        && !height.is_null()
    {
        let message = unsafe { CStr::from_ptr(message) };

        if let Ok(code) = QrCode::new(message.to_bytes()) {
            let data_capacity = *data_len;
            let img_width = code.width();
            let img_width_u16: Result<u16, _> = code.width().try_into();

            if let Ok(img_width_u16) = img_width_u16 {
                let colors = code.into_colors();
                let mut write_pos = data;
                let write_end = unsafe { write_pos.add(data_capacity) };

                for line in colors.chunks(img_width) {
                    for byte_colors in line.chunks(u8::BITS as usize) {
                        if write_pos >= write_end {
                            return false;
                        }

                        let byte = byte_colors
                            .iter()
                            .map(|color| color.select(DARK_BIT, LIGHT_BIT))
                            .enumerate()
                            .fold(0, |acc, (index, bit)| acc | bit << (7 - index));
                        unsafe { write_pos.write(byte) };
                        write_pos = unsafe { write_pos.add(1) };
                    }
                }

                unsafe { width.write(img_width_u16) };
                unsafe { height.write(img_width_u16) };
                *data_len = img_width * img_width * 1 / 8 + 1;
                result = true;
            }
        }
    }

    result
}
