#pragma once
#include <huxerui/map.h>

namespace huxerui::map::detail {
inline constexpr char amap_type[] = "huxerui.map/AMap";
inline constexpr char baidu_type[] = "huxerui.map/Baidu";
struct MapProperties {
  MapOptions options;
  [[nodiscard]] static PlatformPayload Encode(const MapProperties& properties);
  bool operator==(const MapProperties&) const = default;
};
void InstallPlatformMaps(ApplicationContext& context);
} // namespace huxerui::map::detail
