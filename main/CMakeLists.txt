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

# FIXME: Find a way to properly set this as a link-time dependency from the
# rusty component.
target_link_libraries(${COMPONENT_LIB} rusty)
