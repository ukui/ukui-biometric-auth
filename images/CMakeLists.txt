file(GLOB images LIST_DIRECTORIES false *.png *.gif)

file(GLOB children *)
set(dirlist "")
foreach(child ${children})
    if(IS_DIRECTORY ${child})
        list(APPEND dirlist ${child})
    endif()
endforeach()

install(FILES ${images} DESTINATION ${UKUI_BIOMETRIC_DIR}/images)
install(DIRECTORY ${dirlist} DESTINATION ${UKUI_BIOMETRIC_DIR}/images)
