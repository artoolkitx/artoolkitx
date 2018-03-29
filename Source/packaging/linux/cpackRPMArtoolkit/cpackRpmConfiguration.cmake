set(CPACK_RPM_PACKAGE_LICENSE "LGPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_PACKAGE_URL "http://www.artoolkitx.org")

install(FILES ${ARTK_HOME}/ChangeLog.txt DESTINATION share/doc/${PROJECT_NAME} )
install(FILES ${ARTK_HOME}/README.md DESTINATION share/doc/${PROJECT_NAME})
install(FILES ${ARTK_HOME}/LICENSE.txt DESTINATION share/doc/${PROJECT_NAME})

set(CPACK_RPM_USER_FILELIST "%doc /usr/share/doc/${PROJECT_NAME}/ChangeLog.txt" "%license /usr/share/doc/${PROJECT_NAME}/LICENSE.txt" "%doc /usr/share/doc/${PROJECT_NAME}/README.md" "%doc /usr/share/doc/${PROJECT_NAME}/")

