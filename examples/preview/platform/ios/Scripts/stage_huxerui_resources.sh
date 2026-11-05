set -eu

HUXERUI_RESOURCE_SOURCE="$HUXERUI_CORE_BUILD_DIR/huxerui-ios/example_map/resources/package"
HUXERUI_RESOURCE_DESTINATION="$TARGET_BUILD_DIR/$UNLOCALIZED_RESOURCES_FOLDER_PATH/HuxerUI"
if [ ! -f "$HUXERUI_RESOURCE_SOURCE/huxerui/resources.bin" ]; then
  echo "error: HuxerUI resource package is missing: $HUXERUI_RESOURCE_SOURCE" >&2
  exit 1
fi

cmake -E remove_directory "$HUXERUI_RESOURCE_DESTINATION"
cmake -E make_directory "$HUXERUI_RESOURCE_DESTINATION"
cmake -E copy_directory "$HUXERUI_RESOURCE_SOURCE" "$HUXERUI_RESOURCE_DESTINATION"

# Keep provider bundles beside the app executable, as required by their SDKs.
map_old_ifs="$IFS"
IFS=';'
for map_resource_dir in ${HUXERUI_MAP_IOS_RESOURCE_DIRS:-}; do
  for map_bundle in "$map_resource_dir"/*.bundle; do
    if [ -d "$map_bundle" ]; then
      cmake -E copy_directory "$map_bundle" "$TARGET_BUILD_DIR/$UNLOCALIZED_RESOURCES_FOLDER_PATH/$(basename "$map_bundle")"
    fi
  done
done
IFS="$map_old_ifs"

if [ -n "${HUXERUI_INTEGRATION_PLAN:-}" ]; then
  mkdir -p "$(dirname "$HUXERUI_INTEGRATION_PLAN")"
  {
    printf '{\n'
    printf '  "schema": 1,\n'
    printf '  "target": "%s",\n' "example_map"
    printf '  "artifact": "%s",\n' "$TARGET_BUILD_DIR/$EXECUTABLE_PATH"
    printf '  "bundle": "%s",\n' "$TARGET_BUILD_DIR/$WRAPPER_NAME"
    printf '  "bundleIdentifier": "%s"\n' "$PRODUCT_BUNDLE_IDENTIFIER"
    printf '}\n'
  } > "$HUXERUI_INTEGRATION_PLAN"
fi
