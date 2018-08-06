include_directories(${PROJECT_SOURCE_DIR}/ARX/include)
include_directories(${PROJECT_SOURCE_DIR}/ARX)
include_directories(${PROJECT_SOURCE_DIR}/ARX/AR/include)
include_directories(${PROJECT_SOURCE_DIR}/ARX/AR2/include)
include_directories(${PROJECT_SOURCE_DIR}/ARX/ARUtil/include)
include_directories(${PROJECT_SOURCE_DIR}/ARX/ARVideo/include)
include_directories(${PROJECT_SOURCE_DIR}/ARX/ARG/include)
include_directories(${PROJECT_SOURCE_DIR}/ARX/KPM/include)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../ARX/AR/include)

message("CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

add_definitions("-std=c++11")

link_directories(${CMAKE_CURRENT_BINARY_DIR}/../ARX ${CMAKE_CURRENT_BINARY_DIR}/../ARX/AR ${CMAKE_CURRENT_BINARY_DIR}/../ARX/ARUtil ${CMAKE_CURRENT_BINARY_DIR}/../ARX/AR2 ${CMAKE_CURRENT_BINARY_DIR}/../ARX/KPM ${CMAKE_CURRENT_BINARY_DIR}/../ARX/ARVideo ${CMAKE_CURRENT_BINARY_DIR}/../ARX/ARG)
link_libraries(libARX libARVideo libAR libARG libARUtil)

add_executable(artoolkitx  ${PROJECT_SOURCE_DIR}/artoolkitx.js/ARX_js.cpp ${PROJECT_SOURCE_DIR}/artoolkitx.js/ARX_bindings.cpp)

set(EXPORT_FUNCTIONS "'_arwUpdateAR', '_arwCapture', '_arwGetProjectionMatrix', '_arwQueryTrackableVisibilityAndTransformation', '_arwGetTrackablePatternConfig', '_arwGetTrackablePatternImage', '_arwLoadOpticalParams'")
set(EXPORTED_RUNTIME_FUNCTIONS "'ccall', 'cwrap', 'FS', 'setValue'")
set(MODULE_OPTIONS "-s MODULARIZE_INSTANCE=1 -s EXPORT_NAME='artoolkitXjs' -s EXPORT_ES6=1" )

math(EXPR MEM 256*1024*1024)
message('CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}')
set(MEMORY "-s TOTAL_MEMORY=${MEM}")

#To generate source maps the -g4 needs to be passed to the LINK_FLAGS
if(${CMAKE_BUILD_TYPE} EQUAL 'Debug')
    set(DEBUG_EMCC "-g4 -s SAFE_HEAP=1 -s ASSERTIONS=1 --js-opts 0 -s DEMANGLE_SUPPORT=0" )
    # set(NO_WASM "-s WASM=0")
else()
    set(OPTIMIZE_EMCC "-Oz")
    # set(NO_WASM "-s WASM=0")
    # If WASM add a pre-js to register the wasm loader
    set(WASM_LOADER "--pre-js ${PROJECT_SOURCE_DIR}/artoolkitx.js/ARX_wasmLoader.js")
endif()

set_target_properties(artoolkitx PROPERTIES LINK_FLAGS "${MODULE_OPTIONS} ${NO_WASM} ${WASM_LOADER} ${MEMORY} ${DEBUG_EMCC} ${OPTIMIZE_EMCC} --bind --post-js ${PROJECT_SOURCE_DIR}/artoolkitx.js/ARX_additions.js -s EXPORTED_FUNCTIONS='[${EXPORT_FUNCTIONS}]' -s EXTRA_EXPORTED_RUNTIME_METHODS='[${EXPORTED_RUNTIME_FUNCTIONS}]'")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/artoolkitx.js ${CMAKE_CURRENT_BINARY_DIR}/artoolkitx.wasm
    DESTINATION ${PROJECT_SOURCE_DIR}/../SDK/lib
)