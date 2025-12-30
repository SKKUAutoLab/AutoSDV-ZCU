cmake_minimum_required(VERSION 3.12.4)
project(zcu)

# Find required packages
find_package(FastRTPS REQUIRED)
find_package(fastcdr REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

# Set include directories for all targets
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/camera_module
    ${PROJECT_SOURCE_DIR}/include/control_module
    ${PROJECT_SOURCE_DIR}/include/status_module
    ${PROJECT_SOURCE_DIR}/include/update_module
    ${FastRTPS_INCLUDE_DIR}
    ${fastcdr_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# CAN Setting source file (common)
set(CAN_SETTING_SOURCE
    src/control_module/s32g3_skku_can_setting.c
)

# Add source files for camera module
set(CAMERA_SOURCES
    src/camera_module/camera_server.cpp
    src/camera_module/Image.cxx
    src/camera_module/ImagePubSubTypes.cxx
)

# Add source files for control module
set(CONTROL_SOURCES
    src/control_module/control_subscriber.cpp
    src/control_module/ControlCommand.cxx
    src/control_module/ControlCommandPubSubTypes.cxx
)

set(STATUS_SOURCES
    src/status_module/status_publisher.cpp
    src/status_module/Status.cxx
    src/status_module/StatusPubSubTypes.cxx
)

set(UPDATE_SOURCES
    src/update_module/update_subscriber.cpp
    src/update_module/UpdateNotification.cxx
    src/update_module/UpdateNotificationPubSubTypes.cxx
)

# Create executable
add_executable(zcu 
    src/main.cpp
    ${CAMERA_SOURCES}
    ${CONTROL_SOURCES}
    ${STATUS_SOURCES}
    ${UPDATE_SOURCES}
    ${CAN_SETTING_SOURCE}
)

# Link libraries
target_link_libraries(zcu
    fastrtps 
    fastcdr
    pthread
    ${Boot_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)