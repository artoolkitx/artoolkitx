set(PUBLIC_HEADERS
    include/ARX/AR/ar.h
    include/ARX/AR/arConfig.h
    include/ARX/AR/arFilterTransMat.h
    include/ARX/AR/arImageProc.h
    include/ARX/AR/arMulti.h
    include/ARX/AR/icp.h
    include/ARX/AR/icpCalib.h
    include/ARX/AR/icpCore.h
    include/ARX/AR/matrix.h
    include/ARX/AR/param.h
    include/ARX/AR/paramGL.h
)

set(SOURCE
    include/ARX/AR/config.h
    ar3DCreateHandle.c
    ar3DUtil.c
    arCreateHandle.c
    arDetectMarker.c
    arDetectMarker2.c
    arFilterTransMat.c
    arGetLine.c
    arGetMarkerInfo.c
    arGetTransMat.c
    arGetTransMatStereo.c
    arImageProc.c
    arLabeling.c
    arLabelingSub/arLabelingPrivate.h
    arLabelingSub/arLabelingSub.h
    arLabelingSub/arLabelingSubDBIC.c
    arLabelingSub/arLabelingSubDBRC.c
    arLabelingSub/arLabelingSubDBZ.c
    arLabelingSub/arLabelingSubDWIC.c
    arLabelingSub/arLabelingSubDWRC.c
    arLabelingSub/arLabelingSubDWZ.c
    arLabelingSub/arLabelingSubEBIC.c
    arLabelingSub/arLabelingSubEBRC.c
    arLabelingSub/arLabelingSubEBZ.c
    arLabelingSub/arLabelingSubEWIC.c
    arLabelingSub/arLabelingSubEWRC.c
    arLabelingSub/arLabelingSubEWZ.c
    arMultiEditConfig.c
    arMultiFreeConfig.c
    arMultiGetTransMat.c
    arMultiGetTransMatStereo.c
    arMultiReadConfigFile.c
    arPattAttach.c
    arPattCreateHandle.c
    arPattGetID.c
    arPattLoad.c
    arPattSave.c
    arRefineCorners.cpp
    arRefineCorners.h
    arUtil.c
    icpCalibStereo.c
    icpCore.c
    icpHandle.c
    icpPoint.c
    icpPointRobust.c
    icpStereoHandle.c
    icpStereoPoint.c
    icpStereoPointRobust.c
    icpUtil.c
    mAlloc.c
    mAllocDup.c
    mAllocInv.c
    mAllocMul.c
    mAllocTrans.c
    mAllocUnit.c
    mDet.c
    mDisp.c
    mDup.c
    mFree.c
    mInv.c
    mMul.c
    mPCA.c
    mSelfInv.c
    mTrans.c
    mUnit.c
    paramChangeSize.c
    paramClear.c
    paramDecomp.c
    paramDisp.c
    paramDistortion.c
    paramFile.c
    paramGetPerspective.c
    paramGL.c
    paramLT.c
    vAlloc.c
    vDisp.c
    vFree.c
    vHouse.c
    vInnerP.c
    vTridiag.c
)

if (ARX_TARGET_PLATFORM_ANDROID)
    set(LIBS ${LIBS}
        log
    )
endif()

if (ARX_TARGET_PLATFORM_IOS OR ARX_TARGET_PLATFORM_MACOS)
    set(LIBS ${LIBS}
        "-framework CoreFoundation"
        "-framework Accelerate"
    )
endif()

if (ARX_TARGET_PLATFORM_IOS OR ARX_TARGET_PLATFORM_MACOS)
    set_source_files_properties(arUtil.c PROPERTIES
        COMPILE_FLAGS "-x objective-c"
        XCODE_EXPLICIT_FILE_TYPE "sourcecode.c.objc"
    )
endif()

add_library(AR STATIC
    ${PUBLIC_HEADERS} ${SOURCE}
)

target_include_directories(AR
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ARUtil/include
)

target_link_libraries(AR
    INTERFACE ${LIBS}
)

if(HAVE_OPENCV)
    target_include_directories(AR
        PRIVATE ${OpenCV_INCLUDE_DIR}
    )

    if(NOT ARX_TARGET_PLATFORM_WINDOWS)
        target_link_libraries(AR
            PRIVATE ${OpenCV_LIBS}
        )
    else()
        set (LINK_LIST "")
        foreach(x ${OpenCV_LIBS})
            string(REPLACE ".lib" "341d.lib" x_debug ${x})
            list (APPEND LINK_LIST debug ${x_debug} optimized ${x})
        endforeach()
        target_link_libraries(AR
            ${LINK_LIST}
        )
    endif()
endif()

# Pass on headers to parent.
string(REGEX REPLACE "([^;]+)" "AR/\\1" hprefixed "${PUBLIC_HEADERS}")
set(FRAMEWORK_HEADERS
   ${FRAMEWORK_HEADERS}
   ${hprefixed}
   PARENT_SCOPE
)

