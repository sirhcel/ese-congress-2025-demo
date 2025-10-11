#![no_std]

use core::ffi::c_int;
use core::panic::PanicInfo;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rusty_add(left: c_int, right: c_int) -> c_int {
    left + right
}

#[panic_handler]
fn panic(_: &PanicInfo) -> ! {
    loop {}
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = unsafe { rusty_add(2, 2) };
        assert_eq!(result, 4);
    }
}
