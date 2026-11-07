#include "map_ios_internal.h"
#import <CoreLocation/CoreLocation.h>

namespace huxerui::map {
namespace detail {
namespace {
class FailedMap final : public PlatformMap {
public:
  FailedMap(std::shared_ptr<EventSink> sink, std::string message) : sink_(std::move(sink)) {
    label_ = [UILabel new];
    label_.numberOfLines = 0;
    label_.text = String("HuxerUI Map: " + message);
    sink_->Emit("failed", PlatformPayload::Object{{"code", "initialization-failed"}, {"message", message}});
  }
  UIView* View() override { return label_; }
  void Update(const MapOptions&) override {}
  void Dispose() override { sink_->active = false; }
private:
  UILabel* label_;
  std::shared_ptr<EventSink> sink_;
};

using Creator = std::shared_ptr<PlatformMap>(const MapOptions&, std::shared_ptr<EventSink>);
void Register(ApplicationContext& context, const char* type, Creator* create) {
  ios::PlatformViewFactory<MapProperties, PlatformMap> factory{
      .create = [create](UIViewController*, const MapProperties& properties, PlatformEventEmitter events) -> std::shared_ptr<PlatformMap> {
        auto sink = std::make_shared<EventSink>(std::move(events));
        if (!properties.options.privacy.Granted()) return std::make_shared<FailedMap>(sink, "Provider privacy consent is required");
        @try {
          try { return create(properties.options, sink); }
          catch (const std::exception& error) { return std::make_shared<FailedMap>(sink, error.what()); }
        } @catch (NSException* error) {
          return std::make_shared<FailedMap>(sink, error.reason.UTF8String ?: "Map initialization failed");
        }
      },
      .view = [](const std::shared_ptr<PlatformMap>& instance) { return instance->View(); },
      .update = [](PlatformMap& instance, const MapProperties& properties) { instance.Update(properties.options); },
      .dispose = [](PlatformMap& instance) { instance.Dispose(); },
  };
  context.RegisterPlatformView<MapProperties>(type, std::move(factory));
}
} // namespace
void InstallPlatformMaps(ApplicationContext& context) {
#if !HUXERUI_MAP_ENABLE_AMAP && !HUXERUI_MAP_ENABLE_BAIDU
  HUXERUI_MAP_UNUSED(context);
#endif
#if HUXERUI_MAP_ENABLE_AMAP
  Register(context, amap_type, CreateAMap);
#endif
#if HUXERUI_MAP_ENABLE_BAIDU
  Register(context, baidu_type, CreateBaiduMap);
#endif
}
} // namespace detail
bool IsSupported(Provider provider) noexcept {
  switch (provider) {
#if HUXERUI_MAP_ENABLE_AMAP
    case Provider::AMap: return true;
#endif
#if HUXERUI_MAP_ENABLE_BAIDU
    case Provider::Baidu: return true;
#endif
    default: return false;
  }
}
} // namespace huxerui::map
