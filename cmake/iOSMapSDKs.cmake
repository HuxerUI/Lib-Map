# Supply directories containing the extracted device/simulator .framework slices.
# These directories must also contain each SDK's dependent frameworks.
set(HUXERUI_MAP_IOS_FRAMEWORK_DIRS "" CACHE STRING "Semicolon-separated map SDK framework directories")
set(HUXERUI_MAP_IOS_EXTRA_LIBRARIES "" CACHE STRING "Additional vendor static libraries, such as Baidu thirdlibs")
target_compile_options(huxerui_map PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
set_target_properties(huxerui_map PROPERTIES OBJCXX_STANDARD 20 OBJCXX_STANDARD_REQUIRED ON)
target_link_libraries(huxerui_map PRIVATE
    "-framework UIKit" "-framework CoreLocation" "-framework QuartzCore"
    "-framework SystemConfiguration" "-framework CoreTelephony" "-framework Security"
    "-framework CoreGraphics" "-framework CoreText" "-framework AVFAudio" "-framework OpenGLES" "-framework Metal" "-framework MetalKit"
    "-framework GLKit" "-framework CoreMotion" "-framework Accelerate" z sqlite3)
target_link_options(huxerui_map PUBLIC "LINKER:-ObjC")
if(HUXERUI_MAP_IOS_EXTRA_LIBRARIES)
    target_link_libraries(huxerui_map PRIVATE ${HUXERUI_MAP_IOS_EXTRA_LIBRARIES})
endif()
foreach(_dir IN LISTS HUXERUI_MAP_IOS_FRAMEWORK_DIRS)
    target_compile_options(huxerui_map PRIVATE "-F${_dir}")
    target_link_options(huxerui_map PUBLIC "SHELL:-F\"${_dir}\"")
endforeach()
function(_map_framework name)
    find_library(_map_${name} NAMES ${name} PATHS ${HUXERUI_MAP_IOS_FRAMEWORK_DIRS} REQUIRED)
    target_link_libraries(huxerui_map PRIVATE "${_map_${name}}")
endfunction()
if(HUXERUI_MAP_ENABLE_AMAP)
    target_compile_definitions(huxerui_map PRIVATE HUXERUI_MAP_ENABLE_AMAP=1)
    _map_framework(MAMapKit)
    _map_framework(AMapFoundationKit)
endif()
if(HUXERUI_MAP_ENABLE_BAIDU)
    target_compile_definitions(huxerui_map PRIVATE HUXERUI_MAP_ENABLE_BAIDU=1)
    _map_framework(BaiduMapAPI_Base)
    _map_framework(BaiduMapAPI_Map)
endif()
