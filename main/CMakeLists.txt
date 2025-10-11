idf_component_register(SRCS
    "main.cpp"
    "i2c_driver.c"
    "amoled_driver.c"
    "initSequence.c"
    "power_driver.cpp"
    "display_s3.c"
    "touch_driver.cpp"
    "display_s3_pro.c"
    "display_s3_qt.c"
    "display_esp32.c"
    "display_long.c"
    "display_hmi.c"
    "display_rgb.cpp"
    "display_watch_s3.c"
    INCLUDE_DIRS ".")

set(Rust_TOOLCHAIN "esp")
set(Rust_CARGO_TARGET "xtensa-esp32s3-none-elf")

include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5.2
)
FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(
    MANIFEST_PATH "../rusty/Cargo.toml"
    NO_STD
)

# Optimize Rust code for size like we're doing it for C.
corrosion_add_target_rustflags(rusty
    "-C embed-bitcode=yes"
    "-C lto=true"
    "-C opt-level=s"
    "-g"
)
