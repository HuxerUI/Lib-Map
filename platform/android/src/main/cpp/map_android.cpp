#include "map_internal.h"
#include <huxerui/android/platform_registry.h>

namespace huxerui::map {
bool IsSupported(Provider provider) noexcept { return provider == Provider::AMap || provider == Provider::Baidu; }
void detail::InstallPlatformMaps(ApplicationContext& context) {
  context.RegisterPlatformView<MapProperties>(amap_type,
      android::JavaPlatformViewFactory<MapProperties>{.class_name = "org.huxerui.lib.map.AMapFactory"});
  context.RegisterPlatformView<MapProperties>(baidu_type,
      android::JavaPlatformViewFactory<MapProperties>{.class_name = "org.huxerui.lib.map.BaiduMapFactory"});
}
} // namespace huxerui::map
