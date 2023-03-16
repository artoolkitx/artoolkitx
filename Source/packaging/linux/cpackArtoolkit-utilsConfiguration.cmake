#Define package description
set(PACKAGE_SUMMARY "artoolkitx-SDK utilities")
set(PACKAGE_DESCRIPTION "This package contains all artoolkitX utilities. These utilities include \n apps for camera calibration, trackable creation and multi-marker creation. \n For more information visit www.artoolkitx.org. \n Fork us on Github https://github.com/artoolkitx/artoolkitx \n .\n Follow us on Twitter: twitter.com/artoolkitx")

if(${CPACK_GENERATOR} STREQUAL "DEB")
    string(CONCAT summary "${PACKAGE_SUMMARY}" "\n " "${PACKAGE_DESCRIPTION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${summary})
elseif (${CPACK_GENERATOR} STREQUAL "RPM")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PACKAGE_SUMMARY})
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${PACKAGE_DESCRIPTION})
endif()

#Set all util apps
set(AR_utils "${ARTK_HOME}/SDK/bin/artoolkitx_check_id" "${ARTK_HOME}/SDK/bin/artoolkitx_checkResolution" "${ARTK_HOME}/SDK/bin/artoolkitx_dispTexData" "${ARTK_HOME}/SDK/bin/artoolkitx_genMarkerSet" "${ARTK_HOME}/SDK/bin/artoolkitx_genTexData" "${ARTK_HOME}/SDK/bin/artoolkitx_mk_patt"  "${ARTK_HOME}/SDK/bin/artoolkitx_check_image_2d_tracking" "${ARTK_HOME}/SDK/bin/artoolkitx_image_database2d")

##Add the util binary files
install(FILES ${AR_utils} DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)