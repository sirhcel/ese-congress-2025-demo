use core::panic::PanicInfo;
use libc_alloc::LibcAlloc;

#[global_allocator]
static ALLOCATOR: LibcAlloc = LibcAlloc;

#[panic_handler]
fn panic(_: &PanicInfo) -> ! {
    loop {}
}
