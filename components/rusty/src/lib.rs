#![feature(iter_array_chunks)]
#![no_std]

mod qr;
mod rt;

use core::ffi::c_int;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rusty_add(left: c_int, right: c_int) -> c_int {
    left + right
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
