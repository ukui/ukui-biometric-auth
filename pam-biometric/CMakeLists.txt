project(pam_biometric)

find_path(PAM_INCLUDE_DIR security/pam_modules.h)
find_library(PAM_LIB NAMES pam)

if(NOT PAM_INCLUDE_DIR OR NOT PAM_LIB)
    message(FATAL_ERROR "pam_modules.h not found")
endif()

set(pam_SRCS
    pam_biometric.c
    logger.c
    )

include_directories(
    ../common
    ${PAM_INCLUDE_DIR}
    )


add_library(pam_biometric SHARED ${pam_SRCS})
target_link_libraries(pam_biometric ${PAM_LIB})
#去除lib前缀
set_target_properties(pam_biometric PROPERTIES PREFIX "")

install(TARGETS pam_biometric DESTINATION /lib/security)
install(FILES pam-configs/pam-biometric DESTINATION /usr/share/pam-configs)
install(FILES data/ukui-biometric.conf DESTINATION /etc/biometric-auth)
install(FILES data/org.freedesktop.plicykit.pkexec.bioctl.policy
    data/org.freedesktop.plicykit.pkexec.biodrvctl.policy
    DESTINATION /usr/share/polkit-1/actions
    )
install(FILES utils/bioctl utils/biodrvctl DESTINATION bin PERMISSIONS ${CONDOR_SCRIPT_PERMS})
