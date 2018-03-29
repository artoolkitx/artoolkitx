#! /bin/bash

#
# Package artoolkitX for all platforms.
#
# Copyright 2017-2018, Realmax Inc. and artoolkitX Contributors.
# Author(s): Philip Lamb, Thorsten Bux
#

# -e = exit on errors; -x = debug
set -e -x

# Get our location.
OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function usage {
    echo "Usage: $(basename $0) (macos | ios | linux | android)... [rpm] [deb]"
    exit 1
}

if [ $# -eq 0 ]; then
    usage
fi
ARTOOLKITX_HOME=$OURDIR/../..

function verifyPackageType {
    if [ ! $PACKAGE_LINUX ] 
    then
        echo "You can only specify a type for linux packaging"
    else
        PACKAGE_type+=($1)
    fi
}

# Parse parameters
while test $# -gt 0
do
    case "$1" in
        osx) PACKAGE_MACOS=1
            ;;
        macos) PACKAGE_MACOS=1
            ;;
        ios) PACKAGE_IOS=1
            ;;
        android) PACKAGE_ANDROID=1
            ;;
        linux) PACKAGE_LINUX=1
            ;;
        windows) PACKAGE_WINDOWS=1
            ;;
        rpm) verifyPackageType $1
            ;;
        deb) verifyPackageType $1
            ;;        
        --*) echo "bad option $1"
            ;;
        *) echo "bad argument $1"
            ;;
    esac
    shift
done

# Set OS-dependent variables.
OS=`uname -s`
ARCH=`uname -m`
TAR='/usr/bin/tar'
if [ "$OS" = "Linux" ]
then
    CPUS=`/usr/bin/nproc`
    TAR='/bin/tar'
elif [ "$OS" = "Darwin" ]
then
    CPUS=`/usr/sbin/sysctl -n hw.ncpu`
elif [ "$OS" = "CYGWIN_NT-6.1" ]
then
    # bash on Cygwin.
    CPUS=`/usr/bin/nproc`
    OS='Windows'
elif [ "$OS" = "MINGW64_NT-10.0" ]
then
    # git-bash on Windows.
    CPUS=`/usr/bin/nproc`
    OS='Windows'
else
    CPUS=1
fi

if [ "$OS" = "Darwin" ] ; then
# ======================================================================
#  Package platforms hosted by macOS
# ======================================================================

    # macOS
    if [ $PACKAGE_MACOS ] ; then

        # Get version from header `Source>ARX>AR>include>ARX.AR>ar.h`
        VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' ${ARTOOLKITX_HOME}/Source/build-macos/ARX/AR/include/ARX/AR/config.h`
        # If the tiny version number is 0, drop it.
        VERSION=`echo -n "${VERSION}" | sed -E -e 's/([0-9]+\.[0-9]+)\.0/\1/'`
        VERSION_DEV=`sed -En -e 's/#define AR_HEADER_VERSION_DEV[[:space:]]+([0-9]+).*/\1/p' ${ARTOOLKITX_HOME}/Source/build-macos/ARX/AR/include/ARX/AR/config.h`
        if [ "${VERSION_DEV}" = "0" ] ; then unset VERSION_DEV ; fi
        
        TARGET_DIR="${OURDIR}/macos/package/artoolkitx"
        if [ -d ${TARGET_DIR} ] ; then
            rm -rf ${TARGET_DIR}
        fi
        rsync -ar --files-from=${OURDIR}/macos/bom --exclude-from=${OURDIR}/macos/excludes ${ARTOOLKITX_HOME} ${TARGET_DIR}
        (cd "${OURDIR}/macos";export DIST_NAME="artoolkitX for macOS";export DIST_VERSION="v${VERSION}${VERSION_DEV+-dev}";export SOURCE_DIR="package";export SOURCE_FILES="artoolkitX";make -e dist;rm "wc.dmg";rm "template.dmg")
        rm -rf ${TARGET_DIR}
        mv $OURDIR/macos/*.dmg $OURDIR/macos/package
        PACKAGE_NAME="artoolkitX for macOS v${VERSION}${VERSION_DEV+-dev}.dmg"
    fi
    # /macOS

    # iOS
    if [ $PACKAGE_IOS ] ; then

        # Get version from header `Source>ARX>AR>include>ARX.AR>ar.h`
        VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' ${ARTOOLKITX_HOME}/Source/build-ios/ARX/AR/include/ARX/AR/config.h`
        # If the tiny version number is 0, drop it.
        VERSION=`echo -n "${VERSION}" | sed -E -e 's/([0-9]+\.[0-9]+)\.0/\1/'`
        VERSION_DEV=`sed -En -e 's/#define AR_HEADER_VERSION_DEV[[:space:]]+([0-9]+).*/\1/p' ${ARTOOLKITX_HOME}/Source/build-ios/ARX/AR/include/ARX/AR/config.h`
        if [ "${VERSION_DEV}" = "0" ] ; then unset VERSION_DEV ; fi

        TARGET_DIR="${OURDIR}/ios/package/artoolkitx"
        if [ -d ${TARGET_DIR} ] ; then
            rm -rf ${TARGET_DIR}
        fi
        rsync -ar --files-from=${OURDIR}/ios/bom --exclude-from=${OURDIR}/ios/excludes ${ARTOOLKITX_HOME} ${TARGET_DIR}
        (cd "${OURDIR}/ios";export DIST_NAME="artoolkitX for iOS";export DIST_VERSION="v${VERSION}${VERSION_DEV+-dev}";export SOURCE_DIR="package";export SOURCE_FILES="artoolkitX";make -e dist;rm "wc.dmg";rm "template.dmg")
        rm -rf ${TARGET_DIR}
        mv $OURDIR/ios/*.dmg $OURDIR/ios/package
        PACKAGE_NAME="artoolkitX for iOS v${VERSION}${VERSION_DEV+-dev}.dmg"
    fi
    # /iOS
    echo $PACKAGE_NAME
    export PACKAGE_NAME
fi
# /$OS = Darwin

# IDEA: windows as platform should be possible for Android too
if [ "$OS" = "Darwin" ] || [ "$OS" = "Linux" ] ; then
    # ======================================================================
    #  Package platforms hosted by macOS/Linux
    # ======================================================================

    # Android
    if [ $PACKAGE_ANDROID ] ; then

        # Get version from header `SDK>include>ARX.AR>ar.h`
        VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' ${ARTOOLKITX_HOME}/Source/ARXJ/ARXJProj/arxj/.externalNativeBuild/cmake/release/arm64-v8a/ARX/AR/include/ARX/AR/config.h`
        # If the tiny version number is 0, drop it.
        VERSION=`echo -n "${VERSION}" | sed -E -e 's/([0-9]+\.[0-9]+)\.0/\1/'`
        VERSION_DEV=`sed -En -e 's/#define AR_HEADER_VERSION_DEV[[:space:]]+([0-9]+).*/\1/p' ${ARTOOLKITX_HOME}/Source/ARXJ/ARXJProj/arxj/.externalNativeBuild/cmake/release/arm64-v8a/ARX/AR/include/ARX/AR/config.h`
        if [ "${VERSION_DEV}" = "0" ] ; then unset VERSION_DEV ; fi

        TARGET_DIR="${OURDIR}/android/package/artoolkitX"
        if [ -d ${TARGET_DIR} ] ; then
            rm -rf ${TARGET_DIR}
        fi

        rsync -ar --files-from=${OURDIR}/android/bom --exclude-from=${OURDIR}/android/excludes ${ARTOOLKITX_HOME} ${TARGET_DIR}

        #Package all into a zip file
        cd ./android/package/
        zip --filesync -r "artoolkitx-${VERSION}${VERSION_DEV+-dev}-Android.zip" ./artoolkitX/
        PACKAGE_NAME="artoolkitx-${VERSION}${VERSION_DEV+-dev}-Android.zip"
        #Clean up
        cd $OURDIR
        rm -rf ${TARGET_DIR}
        export PACKAGE_NAME
    fi
    # /Android
fi
# /"$OS" = "Darwin" || "$OS" = "Linux"

if [ "$OS" = "Linux" ] ; then
# ======================================================================
#  Package platforms hosted by Linux
# ======================================================================

# Linux
if [ $PACKAGE_LINUX ] ; then

    if [ ${#PACKAGE_type[@]} -eq 0 ] 
        then
        echo "You need to specify a package type: deb/rpm. Like: ./package.sh linux [deb rpm]"
        exit
    fi

    if [ ! -d "build-linux-x86_64" ] ; then
        TARGET_DIR=linux
        #Create target directory
        mkdir -p ${TARGET_DIR}

        cd $TARGET_DIR

        #Specify working directoy
        WORKING_DIR=cpack
        mkdir -p $WORKING_DIR

        cd $WORKING_DIR

        for i in "${PACKAGE_type[@]}"
        do
            echo "package_type: $i"
            echo "ARTOOLKITX_HOME: ${ARTOOLKITX_HOME}"

            if [ "$i" = "deb" ]
            then
                packageGenerator="DEB"
            elif [ "$i" = "rpm" ]
            then
                packageGenerator="RPM"
            fi

            rm -rf -- *
            cmake -DCPACK_GENERATOR=$packageGenerator -DARTK_HOME=${ARTOOLKITX_HOME} ../cpack"$packageGenerator"Artoolkit/lib/
            cpack
            mv *.${packageGenerator,,} ../package/

            rm -rf -- *
            cmake -DCPACK_GENERATOR=$packageGenerator -DARTK_HOME=${ARTOOLKITX_HOME} ../cpack"$packageGenerator"Artoolkit/dev/
            cpack
            mv *.${packageGenerator,,} ../package/
            
            rm -rf -- *
            cmake -DCPACK_GENERATOR=$packageGenerator -DARTK_HOME=${ARTOOLKITX_HOME} ../cpack"$packageGenerator"Artoolkit/examples/
            cpack
            mv *.${packageGenerator,,} ../package/

            rm -rf -- *
            cmake -DCPACK_GENERATOR=$packageGenerator -DARTK_HOME=${ARTOOLKITX_HOME} ../cpack"$packageGenerator"Artoolkit/utils/
            cpack
            mv *.${packageGenerator,,} ../package/

            rm -rf -- *
            cmake -DCPACK_GENERATOR=$packageGenerator -DARTK_HOME=${ARTOOLKITX_HOME} ../cpack"$packageGenerator"Artoolkit/complete/
            cpack
            mv *.${packageGenerator,,} ../package/

        done

    else
        echo "Please run the Linux build first. \n To do so navigate to the Source directory and run './build.sh linux'"
    fi

fi
# /Linux

fi
# /"$OS" = "Linux"

# IDEA: windows as platform should be possible for Android too
if [ "$OS" = "Windows" ] ; then
    # ======================================================================
    #  Package platforms hosted by macOS/Linux
    # ======================================================================

    # Android
    if [ $PACKAGE_WINDOWS ] ; then

        # Get version from header `SDK>include>ARX.AR>ar.h`
        VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' ${ARTOOLKITX_HOME}/SDK/include/ARX/AR/config.h`
        # If the tiny version number is 0, drop it.
        VERSION=`echo -n "${VERSION}" | sed -E -e 's/([0-9]+\.[0-9]+)\.0/\1/'`
        VERSION_DEV=`sed -En -e 's/#define AR_HEADER_VERSION_DEV[[:space:]]+([0-9]+).*/\1/p' ${ARTOOLKITX_HOME}/SDK/include/ARX/AR/config.h`
        if [ "${VERSION_DEV}" = "0" ] ; then unset VERSION_DEV ; fi

        TARGET_DIR="${OURDIR}/windows/package"
        if [ -d ${TARGET_DIR} ] ; then
            rm -rf ${TARGET_DIR}
        fi

        PACKAGE_NAME="artoolkitx-${VERSION}${VERSION_DEV+-dev}-Windows.zip"

        tar czvf "$PACKAGE_NAME" \
            -T "./windows/bom" \
#            --exclude-ignore "${OURDIR}/windows/excludes" \

#        tar -czv "$PACKAGE_NAME" \
#            -add-file="${OURDIR}/windows/bom" \
#            --exclude-ignore-recursive="${OURDIR}/windows/excludes"

        #Package all into a zip file
        #cd ./windows/package/
        #zip --filesync -r "artoolkitx-${VERSION}${VERSION_DEV+-dev}-Windows.zip" ./artoolkitX/
        #Clean up
        cd $OURDIR
        rm -rf ${TARGET_DIR}
        export PACKAGE_NAME
    fi
    # /Android
fi
# /"$OS" = "Darwin" || "$OS" = "Linux"