cmake_minimum_required(VERSION 3.20)
project(ThermalControl C RC)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)


# Raylib-eigene Beispiele/Tests nicht mitbauen, spart Zeit
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

add_executable(ThermalControl
    # UI
    UI/src/ui_constants.c
    UI/src/lang/lang.c
    UI/src/lang/font_manager.c
    UI/src/elements/dropdown.c
    UI/src/elements/topbar.c
    UI/src/elements/console.c
    UI/src/elements/temp_graph.c
    UI/src/elements/button.c
    UI/src/elements/control_panel.c
    UI/src/elements/error_window.c
    UI/src/elements/flash_window.c
    UI/src/elements/checkbox.c
    UI/src/elements/numeric_input.c
    UI/src/elements/warning_import.c

    # Resources
    UI/resources/app.rc

    # Bridge between UI & backend
    BRIDGE/src/bridge_connection.c
    BRIDGE/src/bridge_graph_queue.c
    BRIDGE/src/bridge_error.c
    BRIDGE/src/bridge_log_queue.c
    BRIDGE/src/bridge_flash.c
    BRIDGE/src/bridge_control.c

    # Main application (UI) 
    UI/src/main.c

    # Backend
    CORE/src/worker_flash.c
    CORE/src/worker_com_list.c
    CORE/src/worker_sensor.c
    CORE/src/serial_port.c

    # Data
    DATA/src/firmware_files.c
    DATA/src/file_dialog.c
    DATA/src/data_export.c
    DATA/src/data_import.c

    # Tests
    TESTS/src/test_console.c
    TESTS/src/test_graph.c
)

add_custom_command(TARGET ThermalControl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/resources/fonts
        $<TARGET_FILE_DIR:ThermalControl>/resources/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/resources/images
        $<TARGET_FILE_DIR:ThermalControl>/resources/images
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/DATA/resources/firmware
        $<TARGET_FILE_DIR:ThermalControl>/resources/firmware
)

target_include_directories(ThermalControl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/header
    ${CMAKE_CURRENT_SOURCE_DIR}/BRIDGE/header
    ${CMAKE_CURRENT_SOURCE_DIR}/CORE/header
    ${CMAKE_CURRENT_SOURCE_DIR}/DATA/header
    ${CMAKE_CURRENT_SOURCE_DIR}/TESTS/header
)

target_link_libraries(ThermalControl PRIVATE raylib)

if (WIN32)
    target_link_libraries(ThermalControl PRIVATE winmm)
endif()