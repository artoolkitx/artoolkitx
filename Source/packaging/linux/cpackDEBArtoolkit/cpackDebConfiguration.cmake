set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})

##Add changelog file
# Generate deb change log file
execute_process(COMMAND gzip -9 -n -c ${CMAKE_CURRENT_SOURCE_DIR}/../changelog.Debian
                WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                OUTPUT_FILE "${PROJECT_BINARY_DIR}/changelog.Debian.gz")

##Preparing the copyright file
#Read license file
FILE(STRINGS ${ARTK_HOME}/LICENSE.txt LICENSE_IN NEWLINE_CONSUME)

#Prepare the DEBIAN copyright header
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../copyright_template.txt COPYRIGHT_HEADER NEWLINE_CONSUME)
#Get copyright header and our license text together
string(CONCAT COPYRIGHT_OUT "${COPYRIGHT_HEADER}" "${LICENSE_IN}" "\n\n  .\n On Debian systems, the complete text of the GNU Lesser General Public License can be found in '/usr/share/common-licenses/LGPL-3'")
#Write to file
FILE(WRITE ${PROJECT_BINARY_DIR}/copyright "${COPYRIGHT_OUT}")
#Read file
install(FILES ${PROJECT_BINARY_DIR}/copyright DESTINATION /usr/share/doc/${CPACK_DEBIAN_PACKAGE_NAME}/ RENAME copyright PERMISSIONS
        OWNER_WRITE OWNER_READ
        GROUP_READ
        WORLD_READ)
##End copyright file

##control file settings for all deb packages 
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://www.artoolkitx.org")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "artoolkitX.org <info@artoolkitx.org>") #required
## End control file settings

#Zip changelog file
execute_process(COMMAND gzip -9 -n -c ${PROJECT_BINARY_DIR}/ChangeLog.txt
                WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                OUTPUT_FILE "${PROJECT_BINARY_DIR}/changelog.gz")

#SETUP doc-directory structure with correct permissions
INSTALL(DIRECTORY DESTINATION /usr DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(DIRECTORY DESTINATION /usr/share DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(DIRECTORY DESTINATION /usr/share/doc DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(DIRECTORY DESTINATION /usr/share/doc/${PROJECT_NAME} DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

##Include the changelog files
#Two changelog files are shipped. One that comes with the SDK and one that is in the DEB format to fullfill the DEB package needs.
install(FILES "${PROJECT_BINARY_DIR}/changelog.gz"
              "${PROJECT_BINARY_DIR}/changelog.Debian.gz"
              DESTINATION "/usr/share/doc/${CPACK_DEBIAN_PACKAGE_NAME}")
#INFO: In a regular deb file the changelog and the copyright file go directly into the DEBIAN directory. But that is not supported by CPACK. 
#As we like to provide them we ship them using the /usr/share/doc/ProjectName/ directory.
