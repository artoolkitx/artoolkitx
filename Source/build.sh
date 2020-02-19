#! /bin/bash

#
# artoolkitX master build script.
#
# This script builds the core libraries, utilities, and examples.
# Parameters control target platform(s) and options.
#
# Copyright 2018, artoolkitX Contributors.
# Author(s): Philip Lamb, Thorsten Bux, John Wolf, Dan Bell.
#

# Get our location.
OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function usage {
    echo "Usage: $(basename $0) [--debug] (macos | windows | ios | linux | android | linux-raspbian | docs)... [tests] [examples] [unity] [cmake \"<generator>\"]"
    exit 1
}

if [ $# -eq 0 ]; then
    usage
fi

# -e = exit on errors
set -e

# -x = debug
#set -x

# Parse parameters
while test $# -gt 0
do
    case "$1" in
        macos) BUILD_MACOS=1
            ;;
        ios) BUILD_IOS=1
            ;;
        linux) BUILD_LINUX=1
            ;;
        linux-raspbian) BUILD_LINUX_RASPBIAN=1
            ;;
        android) BUILD_ANDROID=1
            ;;
        windows) BUILD_WINDOWS=1
            ;;
		examples) BUILD_EXAMPLES=1
		    ;;
        docs) BUILD_DOCS=1
            ;;
        --debug) DEBUG=
            ;;
        cmake) CMAKE_GENERATOR="$2"
            shift
            ;;
        --*) echo "bad option $1"
            usage
            ;;
        *) echo "bad argument $1"
            usage
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
    # Identify Linux OS. Sets useful variables: ID, ID_LIKE, VERSION, NAME, PRETTY_NAME.
    source /etc/os-release
    # Windows Subsystem for Linux identifies itself as 'Linux'. Additional test required.
    if grep -qE "(Microsoft|WSL)" /proc/version &> /dev/null ; then
        OS='Windows'
    fi
elif [ "$OS" = "Darwin" ]
then
    CPUS=`/usr/sbin/sysctl -n hw.ncpu`
elif [ "$OS" = "CYGWIN_NT-6.1" ]
then
    # bash on Cygwin.
    CPUS=`/usr/bin/nproc`
    OS='Windows'
elif [ "$OS" = "MINGW64_NT-6.1" ]
then
    # git-bash on Windows 7
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

# Set default CMake generator for Windows.
echo "$CMAKE_GENERATOR"
if [ $OS = "Windows" ]  && test -z "$CMAKE_GENERATOR"; then
    CMAKE_GENERATOR="Visual Studio 15 2017 Win64"
fi

# Function to allow check for required packages.
function check_package {
	# Variant for distros that use debian packaging.
	if (type dpkg-query >/dev/null 2>&1) ; then
		if ! $(dpkg-query -W -f='${Status}' $1 | grep -q '^install ok installed$') ; then
			echo "Warning: required package '$1' does not appear to be installed. To install it use 'sudo apt-get install $1'."
		fi
	# Variant for distros that use rpm packaging.
	elif (type rpm >/dev/null 2>&1) ; then
		if ! $(rpm -qa | grep -q $1) ; then
			echo "Warning: required package '$1' does not appear to be installed. To install it use 'sudo dnf install $1'."
		fi
	fi
}

if [ "$OS" = "Darwin" ] ; then
# ======================================================================
#  Build platforms hosted by macOS
# ======================================================================

# macOS
if [ $BUILD_MACOS ] ; then
    if [ ! -d "depends/macos/Frameworks/opencv2.framework" ] ; then
        curl --location "https://github.com/artoolkitx/opencv/releases/download/4.0.0-pre-artoolkitx/opencv-4.0.0-pre-artoolkitx-macos.zip" -o opencv2.zip
        unzip opencv2.zip -d depends/macos/Frameworks
        rm opencv2.zip
    fi

    if [ ! -d "build-macos" ] ; then
        mkdir build-macos
    fi
    cd build-macos
    rm -f CMakeCache.txt
    cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE:FILEPATH=../cmake/macos.toolchain.cmake ${BUILD_TESTS+-DBUILD_TESTS:BOOL=ON}
    xcodebuild -target ALL_BUILD -configuration ${DEBUG+Debug}${DEBUG-Release}
    xcodebuild -target install -configuration ${DEBUG+Debug}${DEBUG-Release}
    cd $OURDIR

	if [ $BUILD_EXAMPLES ] ; then
    	(cd "../Examples/Square tracking example/macOS"
    	xcodebuild -target "artoolkitX Square Tracking Example" -configuration ${DEBUG+Debug}${DEBUG-Release}
    	)
    	(cd "../Examples/2d tracking example/macOS"
    	xcodebuild -target "artoolkitX 2d Tracking Example" -configuration ${DEBUG+Debug}${DEBUG-Release}
    	)
    fi
fi
# /BUILD_MACOS

# iOS
if [ $BUILD_IOS ] ; then

    
    if [ ! -d "depends/ios/Frameworks/opencv2.framework" ] ; then
        curl --location "https://github.com/artoolkitx/opencv/releases/download/4.0.0-pre-artoolkitx/opencv-4.0.0-pre-artoolkitx-ios.zip" -o opencv2.zip
        unzip opencv2.zip -d depends/ios/Frameworks
        rm opencv2.zip
    fi

    if [ ! -d "build-ios" ] ; then
        mkdir build-ios
    fi
    cd build-ios
    rm -f CMakeCache.txt
    cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE:FILEPATH=../cmake/ios.toolchain.cmake
    xcodebuild -target ALL_BUILD -configuration ${DEBUG+Debug}${DEBUG-Release} -destination generic/platform=iOS
    xcodebuild -target install -configuration ${DEBUG+Debug}${DEBUG-Release}
    cd $OURDIR


    if [ $BUILD_EXAMPLES ] ; then
        (cd "../Examples/Square tracking example/iOS"
        xcodebuild -target "artoolkitX Square Tracking Example" -configuration ${DEBUG+Debug}${DEBUG-Release} -destination generic/platform=iOS
        )
        # (cd "../Examples/Square tracking example with OSG/iOS"
        # xcodebuild -target "artoolkitX Square Tracking Example with OSG" -configuration ${DEBUG+Debug}${DEBUG-Release} -destination generic/platform=iOS
        # )
        cp -rf "../Examples/Square tracking example/iOS/build/Release-iphoneos/artoolkitX Square Tracking Example.app" ../Examples/
        # cp -v "../Examples/Square tracking example with OSG/iOS/build/Release-iphoneos/"artoolkitX Square Tracking Example with OSG.app ../Examples/
    fi
fi
# /BUILD_IOS

fi
# /Darwin

if [ "$OS" = "Darwin" ] || [ "$OS" = "Linux" ] || [ "$OS" = "Windows" ] ; then
# ======================================================================
#  Build platforms hosted by macOS/Linux/Windows
# ======================================================================

# Android
if [ $BUILD_ANDROID ] ; then

# Use the standard path to the Android NDK.
#export ANDROID_NDK=${ANDROID_NDK_ROOT}

if [ "$OS" = "Linux" ] ; then
	check_package cmake
fi

if [ ! -d "depends/android/include/opencv2" ] ; then
    curl --location "https://github.com/artoolkitx/opencv/releases/download/3.4.1-dev-artoolkitx/opencv-3.4.1-dev-artoolkitx-android.tgz" -o opencv2.tgz
    tar xzf opencv2.tgz --strip-components=1 -C depends/android
    rm opencv2.tgz
fi

if [ ! -d "build-android" ] ; then
	mkdir build-android
fi
cd build-android

if [ -z "$ANDROID_HOME" ] ; then
    echo "    *****
    You need to set ANDROID_HOME to the root of your Android SDK installation to build the artoolkitX Android Java Library (ARXJ).
    (On macOS the default is ~/Library/Android/sdk/).
    Skipping ARXJ build.
    *****"
else
    echo "Building ARXJ library as AAR"
    cd $OURDIR
    cd ARXJ/ARXJProj; ./gradlew assembleRelease;
    cd $OURDIR
    mkdir -p ../SDK/lib/ARXJ/
    cp ARXJ/ARXJProj/arxj/build/outputs/aar/arxj-release.aar ../SDK/lib/ARXJ/

    if [ $BUILD_EXAMPLES ] ; then
        echo "Building example ARSquareTracking as APK"
        cd $OURDIR
        cd "../Examples/Square tracking example/Android/ARSquareTracking"; ./gradlew assembleRelease;
        cd $OURDIR
        cd "../Examples/Square tracking example with OSG/Android/ARSquareTracking"; ./gradlew assembleRelease;
        cd $OURDIR
        cp -v "../Examples/Square tracking example/Android/ARSquareTracking/ARSquareTrackingExample/build/outputs/apk/release/"ARSquareTrackingExample-release-unsigned.apk ../Examples/
        cp -v "../Examples/Square tracking example with OSG/Android/ARSquareTracking/ARSquareTrackingExample/build/outputs/apk/release/"ARSquareTrackingExample-release-unsigned.apk ../Examples/ARSquareTrackingExampleOSG-release-unsigned.apk
        
        echo "Building example AR2dTracking as APK"
        cd $OURDIR
        cd "../Examples/2d tracking example/Android/AR2DTracking_Proj"; ./gradlew assembleRelease;
        cd $OURDIR
        cp -v "../Examples/2d tracking example/Android/AR2DTracking_Proj/AR2DTrackingExample/build/outputs/apk/release/"AR2DTrackingExample-release-unsigned.apk ../Examples/

    fi
fi
    
fi
# /BUILD_ANDROID

# Documentation
if [ $BUILD_DOCS ] ; then
    if [ "$OS" = "Linux" ] ; then
        check_package doxygen
    fi

    (cd "../Documentation"
    rm -rf APIreference/ARX/html APIreference/ARX/xml
    cd doxygen
    doxygen Doxyfile
    )
fi
# /BUILD_DOCS

fi
# /Darwin||Linux||Windows

if [ "$OS" = "Linux" ] ; then
# ======================================================================
#  Build platforms hosted by Linux
# ======================================================================

# Linux
if [ $BUILD_LINUX ] ; then
	if (type dpkg-query >/dev/null 2>&1) ; then
		check_package build-essential
		check_package cmake
		check_package libjpeg-dev
		check_package libgl1-mesa-dev
		check_package libsdl2-dev
		check_package libudev-dev
		check_package libv4l-dev
		check_package libdc1394-22-dev
		check_package libgstreamer1.0-dev
		check_package libsqlite3-dev
		check_package libcurl4-openssl-dev
		check_package libssl-dev
	elif (type rpm >/dev/null 2>&1) ; then
		check_package gcc
		check_package gcc-c++
		check_package make
		check_package cmake
		check_package libjpeg-turbo-devel
		check_package mesa-libGL-devel
		check_package libSDL2-devel
		check_package systemd-devel
		check_package libv4l-devel
		check_package libdc1394-devel
		check_package gstreamer1-devel
		check_package libsqlite3x-devel
		check_package libcurl-devel
		check_package libopenssl-devel
	fi

    # Check if a suitable version of OpenCV is installed. If not, but its available, install it.
    # If neither, try our precompiled version.
    if (type dpkg-query >/dev/null 2>&1) ; then
        if (apt-cache --quiet=1 policy libopencv-dev | grep -E 'Installed: 3\.') ; then
            echo "Using installed OpenCV"
        else
            if (apt-cache --quiet=1 policy libopencv-dev | grep -E 'Candidate: 3\.') ; then
                echo "Installing OpenCV"
                sudo apt-get install libopencv-dev
            else
                echo "Downloading prebuilt OpenCV"
                if [ ! -d "depends/linux/include/opencv2" ] ; then
                    curl --location "https://github.com/artoolkitx/opencv/releases/download/3.4.1-dev-artoolkitx/opencv-3.4.1-dev-artoolkitx-linux-x86_64.tgz" -o opencv2.tgz
                    tar xzf opencv2.tgz --strip-components=1 -C depends/linux
                    rm opencv2.tgz
                fi
            fi
        fi
    fi    
    

	if [ ! -d "build-linux-x86_64" ] ; then
		mkdir build-linux-x86_64
	fi
	cd build-linux-x86_64
	rm -f CMakeCache.txt
	cmake .. -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
	make -j $CPUS
    make install${DEBUG-/strip}
	cd ..

 	if [ $BUILD_EXAMPLES ] ; then
    	(cd "../Examples/Square tracking example/Linux"
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
        make install
    	)
#    	(cd "../Examples/Square tracking example with OSG/Linux"
#        mkdir -p build
#        cd build
#        cmake .. -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
#        make
#        make install
#    	)
 	fi

fi
# /BUILD_LINUX

if [ $BUILD_LINUX_RASPBIAN ] ; then
    if [ "$ID" = "raspbian" ]; then
    	# Building on Raspbian.
        if (type dpkg-query >/dev/null 2>&1) ; then
            check_package build-essential
            check_package cmake
            check_package libjpeg-dev
            check_package libraspberrypi-dev
            check_package libudev-dev
            check_package libv4l-dev
            check_package libdc1394-22-dev
            check_package libsqlite3-dev
            check_package libcurl4-openssl-dev
        elif (type rpm >/dev/null 2>&1) ; then
            check_package gcc
            check_package gcc-c++
            check_package make
            check_package cmake
            check_package libjpeg-turbo-devel
            check_package libraspberrypi-devel
            check_package systemd-devel
            check_package libv4l-devel
            check_package libdc1394-devel
            check_package libsqlite3x-devel
            check_package libcurl-devel
        fi

        if [ ! -d "depends/linux-raspbian/include/opencv2" ] ; then
            curl --location "https://github.com/artoolkitx/opencv/releases/download/3.4.1-dev-artoolkitx/opencv-3.4.1-dev-artoolkitx-linux-raspbian-armhf.tgz" -o opencv2.tgz
            tar xzf opencv2.tgz --strip-components=1 -C depends/linux-raspbian
            rm opencv2.tgz
        fi

        if [ ! -d "build-linux-raspbian" ] ; then
            mkdir build-linux-raspbian
        fi
        cd build-linux-raspbian
        rm -f CMakeCache.txt
        cmake .. -DARX_TARGET_PLATFORM_VARIANT=raspbian -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
        make -j $CPUS
        make install
        cd ..

        if [ $BUILD_EXAMPLES ] ; then
            (cd "../Examples/Square tracking example/Linux"
            mkdir -p build-raspbian
            cd build-raspbian
            cmake .. -DARX_TARGET_PLATFORM_VARIANT=raspbian -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
            make
            make install
            )
#    	    (cd "../Examples/Square tracking example with OSG/Linux"
#           mkdir -p build-raspbian
#           cd build-raspbian
#           cmake .. -DARX_TARGET_PLATFORM_VARIANT=raspbian -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
#           make
#           make install
#    	    )
        fi
    else
        # Cross-compiling.
        if (type dpkg-query >/dev/null 2>&1) ; then
            check_package build-essential
            check_package cmake
            check_package g++-5-arm-linux-gnueabihf
        elif (type rpm >/dev/null 2>&1) ; then
            check_package gcc
            check_package gcc-c++
            check_package make
            check_package cmake
            check_package gcc-c++-arm-linux-gnueabihf
        fi
        echo "Cross compiling not currently supported."
    fi

fi
# /BUILD_LINUX_RASPBIAN

fi
# /Linux

if [ "$OS" = "Windows" ] ; then
# ======================================================================
#  Build platforms hosted by Windows
# ======================================================================

# Windows
if [ $BUILD_WINDOWS ] ; then

    if [ ! -d "build-windows" ] ; then
        mkdir build-windows
    fi

    if [ ! -d "depends/windows/include/opencv2" ] ; then
        curl --location "https://github.com/artoolkitx/opencv/releases/download/3.4.1-dev-artoolkitx/opencv-3.4.1-dev-artoolkitx-windows.tgz" -o opencv2.tgz
        tar xzf opencv2.tgz --strip-components=1 -C depends/windows
        rm opencv2.tgz
    fi

    cd build-windows
    rm -f CMakeCache.txt
    cmake.exe .. -G "$CMAKE_GENERATOR" -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE
    cmake.exe --build . --config ${DEBUG+Debug}${DEBUG-Release} --target install
    cp $OURDIR/depends/windows/lib/x64/opencv* $OURDIR/../SDK/bin
    cp $OURDIR/depends/windows/lib/x64/SDL2.dll $OURDIR/../SDK/bin

    if [ $BUILD_EXAMPLES ] ; then
    cd $OURDIR

    (cd "../Examples/Square tracking example/Windows"
        mkdir -p build-windows
        cd build-windows
        cmake.exe .. -DCMAKE_CONFIGURATION_TYPES=${DEBUG+Debug}${DEBUG-Release} "-G$CMAKE_GENERATOR"
        cmake.exe --build . --config ${DEBUG+Debug}${DEBUG-Release}  --target install
        #Copy needed dlls into the corresponding Visual Studio directory to allow running examples from inside the Visual Studio GUI
        mkdir -p ${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/opencv*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/SDL2.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/ARX*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/*.patt ./${DEBUG+Debug}${DEBUG-Release}
    )
    (cd "../Examples/2d tracking example/Windows"
        mkdir -p build-windows
        cd build-windows
        cmake.exe .. -DCMAKE_CONFIGURATION_TYPES=${DEBUG+Debug}${DEBUG-Release} "-G$CMAKE_GENERATOR"
        cmake.exe --build . --config ${DEBUG+Debug}${DEBUG-Release}  --target install
        #Copy needed dlls into the corresponding Visual Studio directory to allow running examples from inside the Visual Studio GUI
        mkdir -p ${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/opencv*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/SDL2.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/ARX*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/pinball.jpg ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/database.xml.gz ./${DEBUG+Debug}${DEBUG-Release}
    )
    fi
fi
# /BUILD_WINDOWS

fi
# /Windows
