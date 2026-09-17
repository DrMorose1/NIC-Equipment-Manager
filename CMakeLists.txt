cmake_minimum_required(VERSION 3.20)
project(NICEquipmentManager VERSION 1.3.3 LANGUAGES C RC)

add_executable(NICEquipmentManager WIN32
    app.rc
    src/main.c
    src/config.c
    src/validation.c
    src/shell_tools.c
    src/ui_layout.c
    src/diagnostics.c
    src/network_commands.c
)

target_compile_features(NICEquipmentManager PRIVATE c_std_11)
target_include_directories(NICEquipmentManager PRIVATE src)
target_link_libraries(NICEquipmentManager PRIVATE
    iphlpapi
    ws2_32
    comctl32
    shell32
    ole32
    advapi32
    gdi32
    uxtheme
    dwmapi
)

if(MSVC)
    target_compile_options(NICEquipmentManager PRIVATE /W4 /O2)
else()
    target_compile_options(NICEquipmentManager PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(NICEquipmentManager PROPERTIES
    OUTPUT_NAME "NICEquipmentManager"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
