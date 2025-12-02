use core::ffi::{CStr, c_char};
use qrcode::{Color, QrCode};

const DARK_BIT: u8 = 1;
const LIGHT_BIT: u8 = 0;

/// Packs a slice of colors into a `u8` value for LVGL 1 bit indexed images.
///
/// The first item from `items` defines the most-significant bit and the remaining values will
/// populate the bits towards the least-significant one.
fn pack_colors<'a, I: Iterator<Item = &'a Color>>(items: I) -> u8 {
    items
        .map(|color| color.select(DARK_BIT, LIGHT_BIT))
        .enumerate()
        .fold(0, |acc, (index, bit)| acc | bit << (7 - index))
}

/// Generates a QR code image for the given message. Image date gets written to `data` and
/// `data_len`, `width`, and `height` will be updated accordingly.
///
/// # Safety
///
/// The behavior is undefined if any of the following conditions is violated:
///
/// * `message`, `data`, `data_len`, `width`, and `height` must be non-null and properly aligned
/// * `message` must point to a valid null-terminated C string and allow reading this data
/// * `data` must be valid for writing `data_len` bytes to it
/// * `data_len` must point to a valid value not larger than `isize::MAX` and adding `data_len` to
///   `data` must not wrap around
/// * `data_len` must be valid for reading and writing a `usize` value
/// * `width` and `height` must be valid for writing `u16` values to them
/// * The memory referenced by any of the raw pointers must not be mutated during the call to this
///   function
///
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rusty_generate_qr_lv_img_data(
    message: *const c_char,
    module_size: usize,
    data: *mut u8,
    data_len: *mut usize,
    width: *mut u16,
    height: *mut u16,
) -> bool {
    // SAFETY: `data_len` is required to be valid for reading and writing
    let data_len = unsafe { data_len.as_mut() };
    let mut result = false;

    if let Some(data_len) = data_len
        && !message.is_null()
        && module_size >= 1
        && !width.is_null()
        && !height.is_null()
    {
        // SAFETY: `message` is required to point to a valid null-terminated C string
        let message = unsafe { CStr::from_ptr(message) };

        if let Ok(code) = QrCode::new(message.to_bytes()) {
            let data_capacity = *data_len;
            let qr_modules = code.width();
            let img_width = qr_modules * module_size;
            let img_width_u16: Result<u16, _> = img_width.try_into();

            if let Ok(img_width_u16) = img_width_u16 {
                let colors = code.into_colors();
                let mut write_pos = data;
                // SAFETY: `write_pos` starts as a copy of `data` which is required to be valid for
                // writing up to `data_capacity` (copied from `data_len`) bytes to it. This raw
                // pointer points just behind the last valid data byte which could be written. It
                // will actually never be written and is used for comparing `write_pos` against.
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
                        let byte = pack_colors(chunk.into_iter());

                        if write_pos >= write_end {
                            return false;
                        }
                        // SAFETY: We checked that `write_pos` is not outside the range of the
                        // output buffer just above. So we are safe to write and the range between
                        // `data` and the updated `write_pos` (non-inclusive) is completely within
                        // the data buffer.
                        unsafe {
                            write_pos.write(byte);
                            write_pos = write_pos.add(1);
                        }
                    }

                    if let Some(chunk) = byte_colors_iterator.into_remainder() {
                        let byte = pack_colors(chunk.into_iter());

                        if write_pos >= write_end {
                            return false;
                        }
                        // SAFETY: We checked that `write_pos` is not outside the range of the
                        // output buffer just above. So we are safe to write and the range between
                        // `data` and the updated `write_pos` (non-inclusive) is completely within
                        // the data buffer.
                        unsafe { write_pos.write(byte) };
                        write_pos = unsafe { write_pos.add(1) };
                    }
                }

                // SAFETY: `with` and `height` are required to be non-null and valid for writing
                // `u16` values to.
                unsafe {
                    width.write(img_width_u16);
                    height.write(img_width_u16);
                }
                // SAFETY: `write_pos` was derived from `data` and incremented only if it was
                // smaller than `write_end`. So the value is at maximum equal to `write_end ==
                // data.add(data_capacity)` where `data_capacity` is the input value of `data_len`
                // and we are safe to use this offset.
                *data_len = unsafe { write_pos.offset_from_unsigned(data) };

                result = true;
            }
        }
    }

    result
}
