#Package control file is required and has some important fields that need to be filled.

set(PACKAGE_SUMMARY "artoolkitX-SDK runtime")

set(PACKAGE_DESCRIPTION "This package contains all necessary libraries for artoolkitX to run.\n For more information visit http://www.artoolkitx.org. \n Fork us on Github https://github.com/artoolkitx/artoolkitx \n .\n Follow us on Twitter: http://www.twitter.com/artoolkitx")

if(${CPACK_GENERATOR} STREQUAL "DEB")
    string(CONCAT summary "${PACKAGE_SUMMARY}" "\n " "${PACKAGE_DESCRIPTION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${summary})
elseif (${CPACK_GENERATOR} STREQUAL "RPM")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PACKAGE_SUMMARY})
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${PACKAGE_DESCRIPTION})
endif()




