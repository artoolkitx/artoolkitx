#Define package description
SET(PACKAGE_SUMMARY "Complete artoolkitX-SDK")
SET(PACKAGE_DESCRIPTION "Metapackage for artoolkitx. This package will install the artoolkitX libraries,\n header files, utilities  and binaries. artoolkitx is a free and open source\n software development kit for augmented reality applications.\n This package depends on the library, \n development, utilities and example packages of artoolkitx.\n Fork us on Github https://github.com/artoolkitx/artoolkitx \n .\n Follow us on Twitter: twitter.com/artoolkitx")

if(${CPACK_GENERATOR} STREQUAL "DEB")
    string(CONCAT summary "${PACKAGE_SUMMARY}" "\n " "${PACKAGE_DESCRIPTION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${summary})
elseif (${CPACK_GENERATOR} STREQUAL "RPM")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PACKAGE_SUMMARY})
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${PACKAGE_DESCRIPTION})
endif()

