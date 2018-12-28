cmake_minimum_required(VERSION 2.6)
project(ukui-biometric-auth)

find_package(Qt5 COMPONENTS Core Widgets DBus)
find_package(Qt5LinguistTools)

set(UKUI_BIOMETRIC_DIR /usr/share/ukui-biometric)
set(CMAKE_INSTALL_PREFIX /usr)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set (CONDOR_SCRIPT_PERMS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

add_compile_options(-fPIC)
add_definitions(-DUKUI_BIOMETRIC=${UKUI_BIOMETRIC_DIR})
add_definitions(-DCONFIG_FILE=/etc/biometric-auth/ukui-biometric.conf)


add_subdirectory(bioauth)
add_subdirectory(bioauth-bin)
add_subdirectory(pam-biometric)
add_subdirectory(polkit-agent)
add_subdirectory(images)

add_dependencies(bioauth BioAuth)
add_dependencies(polkit-ukui-authentication-agent-1 BioAuthWidgets)
