# artoolkitX v1.x Quick Start
1. [macOS](#macos)
2. [iOS](#ios)
3. [Linux](#linux)
4. [Android](#android)
5. [Windows](#windows)
6. [Next Steps](#next-steps)

## macOS
* For development, Xcode 14 on Mac OS 12.5 or later is required (destination Macs running mac OS 10.13 are supported).
* Open `Examples` for double-clickable example apps.
* Xcode project for each example is in `Examples/*example name*/macOS`.
* To rebuild the SDK and examples for macOS, in Terminal navigate to `Source` and execute the command `./build.sh macos examples`

## iOS 
* For development, Xcode 14 on Mac OS 12.5 or later, and an iOS device running iOS 11.0 or later is required.
* Open `Examples`. Because apps must be signed for your iOS device, no prebuilt app is provided.
* Xcode project for each example is in `Examples/*example name*/iOS`.
    1. Open the Xcode project.
    2. Navigate to the project settings
    3. Select the settings for the example app (under "Targets")
    4. Select the "General" tab.
    5. Set the development team to one of your registered Apple Developer accounts
    6. Choose a bundle ID that your account is authorized to sign. 
* Connect your device, select it as the target for building and running, and build and run.
* To rebuild the SDK and examples for iOS, in Terminal navigate to `Source` and execute the command `./build.sh ios examples`

## Linux
* A Linux distro supporting either Video4Linux2 and/or gstreamer 1.0, and an x86_64 or ARM 32-bit or ARM 64-bit CPU is required.
* Install the packages 'artoolkitx-lib', 'artoolkitx-dev', and 'artoolkitx-examples' using your package manager:
    
    For Debian-based systems, this will be via `sudo dpkg -i artoolkitx-lib_VERSION_PLATFORM.deb`.
    
    For Redhat-based systems this will be via `yum -y install artoolkitx-lib_VERSION_PLATFORM.rpm`
    
    where VERSION is the artoolkitx version number triplet, e.g. 1.0.2, and PLATFORM is the target platform, e.g. 'amd64'.
* The executable for the examples will be in /usr/bin, and the source in /usr/src.
* To rebuild the SDK and examples for Linux, in a terminal navigate to `Source` and execute the command `./build.sh linux examples`

## Android
* Android Studio 2021 or later with NDK release 21 or later, and an Android device running Android 7.0 or later is required.
* Open `Examples` for prebuilt .apks. These may be installed on your device via `adb install *example name*.apk` where EXAMPLE is the example name.
* An Android Studio project for each example is in `Examples/*example name*/Android`.
* To rebuild the SDK and examples for Android, in a terminal navigate to `Source` and execute the command `./build.sh android examples`

## Windows
* For development Visual Studio 2019 or later, Community Editor or Professional, and a 64-bit PC running Windows 10 or later is required.
* Open `Examples` for double-clickable example apps.
* Visual Studio project for each example is in `Examples/*example name*/Windows`. 
* To rebuild the SDK and examples for Windows, using a bash shell provided by either git-bash, Windows Subsystem for Linux, or Cygwin, navigate to `Source` and execute the command `./build.sh windows examples`

## Next steps

For more documentation, see the wiki at <https://github.com/artoolkitx/artoolkitx/wiki>.

----

