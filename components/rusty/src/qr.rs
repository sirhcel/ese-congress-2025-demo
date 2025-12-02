use core::ffi::{CStr, c_char};
use qrcode::{Color, QrCode};

const DARK_BIT: u8 = 1;
const LIGHT_BIT: u8 = 0;

fn pack_colors<'a, I: Iterator<Item = &'a Color>>(items: I) -> u8 {
    items
        .map(|color| color.select(DARK_BIT, LIGHT_BIT))
        .enumerate()
        .fold(0, |acc, (index, bit)| acc | bit << (7 - index))
}

/// Generates a QR code image for the given message. Image date gets written to `data` and
/// `data_len`, `width`, and `height` will be updated accordingly.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rusty_generate_qr_lv_img_data(
    message: *const c_char,
    module_size: usize,
    data: *mut u8,
    data_len: *mut usize,
    width: *mut u16,
    height: *mut u16,
) -> bool {
    let data_len = unsafe { data_len.as_mut() };
    let mut result = false;

    if let Some(data_len) = data_len
        && !message.is_null()
        && module_size >= 1
        && !width.is_null()
        && !height.is_null()
    {
        let message = unsafe { CStr::from_ptr(message) };

        if let Ok(code) = QrCode::new(message.to_bytes()) {
            let data_capacity = *data_len;
            let qr_modules = code.width();
            let img_width = qr_modules * module_size;
            let img_width_u16: Result<u16, _> = img_width.try_into();

            if let Ok(img_width_u16) = img_width_u16 {
                let colors = code.into_colors();
                let mut write_pos = data;
                let write_end = unsafe { write_pos.add(data_capacity) };

                for line in colors
                    .chunks(qr_modules)
                    .flat_map(|item| core::iter::repeat_n(item, module_size))
                {
                    let mut byte_colors_iterator = line
                        .iter()
                        .flat_map(|item| core::iter::repeat_n(item, module_size))
                        .array_chunks::<8>();

                    for chunk in byte_colors_iterator.by_ref() {
                        if write_pos >= write_end {
                            return false;
                        }

                        let byte = pack_colors(chunk.into_iter());
                        unsafe { write_pos.write(byte) };
                        write_pos = unsafe { write_pos.add(1) };
                    }

                    if let Some(chunk) = byte_colors_iterator.into_remainder() {
                        if write_pos >= write_end {
                            return false;
                        }

                        let byte = pack_colors(chunk.into_iter());
                        unsafe { write_pos.write(byte) };
                        write_pos = unsafe { write_pos.add(1) };
                    }
                }

                unsafe { width.write(img_width_u16) };
                unsafe { height.write(img_width_u16) };
                *data_len = unsafe { write_pos.offset_from_unsigned(data) };
                result = true;
            }
        }
    }

    result
}
