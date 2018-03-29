#Package control file is required and has some important fields that need to be filled.

set(PACKAGE_SUMMARY "artoolkitX-SDK development package")

set(PACKAGE_DESCRIPTION "This package contains all necessary libraries and header files to \n start developing with artoolkitX. Also the API documentation is included.\n For more information visit www.artoolkitx.org. \n Fork us on Github https://github.com/artoolkitx/artoolkitx \n .\n Follow us on Twitter: twitter.com/artoolkitx")

if(${CPACK_GENERATOR} STREQUAL "DEB")
    string(CONCAT summary "${PACKAGE_SUMMARY}" "\n " "${PACKAGE_DESCRIPTION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${summary})
elseif (${CPACK_GENERATOR} STREQUAL "RPM")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PACKAGE_SUMMARY})
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${PACKAGE_DESCRIPTION})
endif()

##Add the include files
INSTALL(DIRECTORY ${ARTK_HOME}/SDK/include/ARX DESTINATION include)

##Add the documentation
INSTALL(DIRECTORY ${ARTK_HOME}/Documentation/ DESTINATION /usr/share/doc/${CMAKE_PROJECT_NAME} 
DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE 
PATTERN "*.gitignore" EXCLUDE)
