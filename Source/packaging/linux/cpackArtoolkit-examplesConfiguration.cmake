#Package control file is required and has some important fields that need to be filled.

set(PACKAGE_SUMMARY "artoolkitx-SDK examples")

set(PACKAGE_DESCRIPTION "This package contains all artoolkitX examples. These examples include apps that \n demonstrate simple augmented reality applications. \n For more information visit www.artoolkitx.org. \n Fork us on Github https://github.com/artoolkitx/artoolkitx \n .\n Follow us on Twitter: twitter.com/artoolkitx")

if(${CPACK_GENERATOR} STREQUAL "DEB")
    string(CONCAT summary "${PACKAGE_SUMMARY}" "\n " "${PACKAGE_DESCRIPTION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${summary})
elseif (${CPACK_GENERATOR} STREQUAL "RPM")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PACKAGE_SUMMARY})
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${PACKAGE_DESCRIPTION})
endif()

#Read all example files
file(GLOB AR_examples ${BUILD_ARTEFACTS_DIR}/../bin/*_example)

#Set all data files from that we need to include
set(AR_examples_data "${ARTK_HOME}/SDK/share/artoolkitx_square_tracking_example/hiro.patt" "${ARTK_HOME}/SDK/share/artoolkitx_square_tracking_example/kanji.patt")

##Add the example files
INSTALL(FILES ${AR_examples} DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

##Add data files
#Data/
install(FILES ${AR_examples_data} DESTINATION share/artoolkitx_square_tracking_example)

##End add data
