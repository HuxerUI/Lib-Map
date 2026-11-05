if(HUXERUI_PROJECT_PLAN_OUTPUT)
    get_filename_component(_map_plan_dir "${HUXERUI_PROJECT_PLAN_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${_map_plan_dir}")
    file(WRITE "${HUXERUI_PROJECT_PLAN_OUTPUT}" [=[
{
  "schema": 1,
  "kind": "library",
  "name": "Lib-Map",
  "id": "org.huxerui.lib.map",
  "target": "huxerui_map",
  "namespace": "huxerui::map",
  "publicTarget": "HuxerUI::Map"
}
]=])
endif()
if(HUXERUI_PROJECT_PLAN_ONLY)
    return()
endif()
if(NOT HUXERUI_LIBRARY_GRAPH_ONLY)
    enable_language(CXX)
endif()
if(HUXERUI_MAP_BUILD_VIEW)
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/HuxerUIDependency.cmake")
endif()
