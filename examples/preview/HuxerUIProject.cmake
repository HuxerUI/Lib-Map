if(HUXERUI_PROJECT_PLAN_OUTPUT)
    get_filename_component(_map_preview_plan_dir "${HUXERUI_PROJECT_PLAN_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${_map_preview_plan_dir}")
    file(WRITE "${HUXERUI_PROJECT_PLAN_OUTPUT}" [=[
{
  "schema": 1,
  "kind": "app",
  "name": "Map Preview",
  "id": "org.huxerui.lib.map.preview",
  "target": "example_map"
}
]=])
endif()
if(HUXERUI_PROJECT_PLAN_ONLY)
    return()
endif()
if(NOT HUXERUI_LIBRARY_GRAPH_ONLY)
    enable_language(CXX)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/HuxerUIDependency.cmake")
set(HUXERUI_LIBRARY_GRAPH_OUTPUT "${CMAKE_CURRENT_LIST_DIR}/.huxerui/generated/libraries.json")
