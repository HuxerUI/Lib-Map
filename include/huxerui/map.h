#pragma once

#include <huxerui/app.h>
#include <huxerui/map/types.h>
#include <huxerui/platform_registry.h>
#include <huxerui/view.h>

namespace huxerui::map {

struct CoordinateEvent {
  Coordinate coordinate;
  [[nodiscard]] static CoordinateEvent Decode(const PlatformPayload& payload);
};
struct CameraEvent {
  CameraPosition camera;
  [[nodiscard]] static CameraEvent Decode(const PlatformPayload& payload);
};
struct MapError {
  std::string code;
  std::string message;
  [[nodiscard]] static MapError Decode(const PlatformPayload& payload);
};
struct MapEvents {
  struct Loaded : Event<void()> { static constexpr char Name[] = "loaded"; };
  struct Clicked : Event<void(CoordinateEvent)> { static constexpr char Name[] = "clicked"; };
  struct LongPressed : Event<void(CoordinateEvent)> { static constexpr char Name[] = "longPressed"; };
  struct CameraIdle : Event<void(CameraEvent)> { static constexpr char Name[] = "cameraIdle"; };
  struct MarkerClicked : Event<void(std::string)> { static constexpr char Name[] = "markerClicked"; };
  struct Error : Event<void(MapError)> { static constexpr char Name[] = "error"; };
};

/// Register once through AppOptions::application_hooks, before mounting maps.
void InstallMaps(ApplicationContext& context);
[[nodiscard]] bool IsSupported(Provider provider) noexcept;

/// Embeds the SDK's actual MapView/UIView via HuxerUI PlatformView.
/// Set a bounded frame or Grow. Camera/overlay properties update only when their submitted values change;
/// unrelated recompositions preserve user panning. Feed CameraIdle back to state for a controlled camera.
/// Throws for invalid options or missing consent. Unsupported platforms display an explanatory Text.
[[nodiscard]] View MapView(Provider provider, MapOptions options = {});
[[nodiscard]] View AMapView(MapOptions options = {});
[[nodiscard]] View BaiduMapView(MapOptions options = {});

} // namespace huxerui::map
