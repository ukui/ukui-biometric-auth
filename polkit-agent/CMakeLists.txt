cmake_minimum_required(VERSION 2.6)
project(ukui-polkit-agent)


find_package(PolkitQt5-1 REQUIRED 0.103.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(LIB_ARCH_PATH /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE})

configure_file(
    ${PROJECT_SOURCE_DIR}/data/polkit-ukui-authentication-agent-1.desktop.in
    ${PROJECT_BINARY_DIR}/data/polkit-ukui-authentication-agent-1.desktop
    )


include_directories(
    ${POLKITQT-1_INCLUDE_DIR}
    ../common
    ../bioauth/include
    )

qt5_add_resources(polkit_SRCS assets.qrc)

set(polkit_SRCS
    ${polkit_SRCS}
    src/PolkitAgent.cpp
    src/mainwindow.cpp
    src/PolkitListener.cpp
    src/sessionmanager.cpp
    ../common/generic.cpp
    )

add_executable(polkit-ukui-authentication-agent-1 ${polkit_SRCS})
target_link_libraries(polkit-ukui-authentication-agent-1
    Qt5::Core Qt5::Widgets Qt5::DBus
    ${POLKITQT-1_LIBRARIES}
    BioAuthWidgets
    )
install(TARGETS polkit-ukui-authentication-agent-1
    DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/ukui-polkit)

# 翻译文件
file(GLOB ts_files i18n_ts/*.ts)
qt5_add_translation(qm_files ${ts_files})
add_custom_target(polkit_i18n DEPENDS ${qm_files} SOURCES ${ts_files})
add_dependencies(polkit-ukui-authentication-agent-1 polkit_i18n)

install(FILES ${qm_files} DESTINATION ${UKUI_BIOMETRIC_DIR}/i18n_qm/polkit)
install(FILES ${PROJECT_BINARY_DIR}/data/polkit-ukui-authentication-agent-1.desktop
    DESTINATION /etc/xdg/autostart)
