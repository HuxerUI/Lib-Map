#pragma once
#include "map_internal.h"
#include <huxerui/ios/platform_registry.h>
#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#include <optional>

#define HUXERUI_MAP_UNUSED(value) static_cast<void>(value)

namespace huxerui::map::detail {
// Queue platform callbacks until the mounted PlatformView has its event bindings.
// A shared active flag also cancels callbacks queued before unmount.
struct EventSink : std::enable_shared_from_this<EventSink> {
  PlatformEventEmitter events;
  bool active = true;
  explicit EventSink(PlatformEventEmitter value) : events(std::move(value)) {}
  void Emit(std::string name, PlatformPayload payload = {}) {
    auto self = shared_from_this();
    dispatch_async(dispatch_get_main_queue(), ^{
      if (self->active) self->events.Emit(name, payload);
    });
  }
  void CoordinateEvent(std::string name, CLLocationCoordinate2D coordinate) {
    Emit(std::move(name), PlatformPayload::Object{{"latitude", coordinate.latitude}, {"longitude", coordinate.longitude}});
  }
  void CameraIdle(CLLocationCoordinate2D center, double zoom) {
    Emit("cameraIdle", PlatformPayload::Object{
        {"center", PlatformPayload::Object{{"latitude", center.latitude}, {"longitude", center.longitude}}}, {"zoom", zoom}});
  }
};
inline NSString* String(const std::string& value) {
  return [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding];
}
inline CLLocationCoordinate2D Coordinate2D(Coordinate value) {
  return CLLocationCoordinate2DMake(value.latitude, value.longitude);
}
inline UIColor* Color(std::uint32_t argb) {
  return [UIColor colorWithRed:((argb >> 16) & 255) / 255.0 green:((argb >> 8) & 255) / 255.0
                         blue:(argb & 255) / 255.0 alpha:((argb >> 24) & 255) / 255.0];
}
struct PlatformMap {
  virtual ~PlatformMap() = default;
  virtual UIView* View() = 0;
  virtual void Update(const MapOptions& options) = 0;
  virtual void Dispose() = 0;
};
std::shared_ptr<PlatformMap> CreateAMap(const MapOptions&, std::shared_ptr<EventSink>);
std::shared_ptr<PlatformMap> CreateBaiduMap(const MapOptions&, std::shared_ptr<EventSink>);
} // namespace huxerui::map::detail
