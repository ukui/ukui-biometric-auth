project(bioauth-bin)

set(CMAKE_AUTOMOC ON)

include_directories(
    ../common
    ../bioauth/include
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5DBus_INCLUDE_DIRS}
    )

set(bin_SRCS
    ${bin_SRCS}
    src/main.cpp
    src/keywatcher.cpp
    ../common/generic.cpp
    )

add_executable(bioauth ${bin_SRCS})
target_link_libraries(bioauth Qt5::Core BioAuth)

install(TARGETS bioauth DESTINATION bin)

file(GLOB ts_files i18n_ts/*.ts)
qt5_add_translation(qm_files ${ts_files})
add_custom_target(bioauth-bin_i18n DEPENDS ${qm_files} SOURCES ${ts_files})
add_dependencies(bioauth bioauth-bin_i18n)

install(FILES ${qm_files} DESTINATION ${UKUI_BIOMETRIC_DIR}/i18n_qm/bioauth-bin)
