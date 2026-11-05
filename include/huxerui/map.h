#pragma once

#include <huxerui/app.h>
#include <huxerui/map/types.h>
#include <huxerui/platform_registry.h>
#include <huxerui/view.h>
#include <string_view>

namespace huxerui::map {

/// A provider-native coordinate reported by a map gesture.
/// This is a value snapshot; retaining it does not retain the native map.
struct CoordinateEvent {
  Coordinate coordinate;
  bool operator==(const CoordinateEvent&) const = default;
  [[nodiscard]] static CoordinateEvent Decode(const PlatformPayload& payload);
};

/// Observed camera position after a native camera movement finishes.
/// This describes the SDK's actual position, which may be clamped from the requested MapOptions::camera.
struct CameraEvent {
  CameraPosition camera;
  bool operator==(const CameraEvent&) const = default;
  [[nodiscard]] static CameraEvent Decode(const PlatformPayload& payload);
};

/// An operational failure emitted by a mounted native map.
struct MapError {
  /// Adapter diagnostic identifier. Vendor authentication failures are not normalized by this value.
  std::string code;
  /// Human-readable diagnostic text; wording and language are not stable machine-readable categories.
  std::string message;
  bool operator==(const MapError&) const = default;
  [[nodiscard]] static MapError Decode(const PlatformPayload& payload);
};

/// Typed events bound through View::On<Event>(). Handlers run on the owning UI thread.
/// References are borrowed for the handler call; copy a value before retaining it or forwarding it asynchronously.
struct MapEvents {
  /// Reports SDK map loading. This does not establish online-service authentication or successful presentation.
  struct Loaded : Event<void()> { static constexpr std::string_view Name = "loaded"; };
  /// Reports a tap on the map in the selected provider's coordinate system.
  struct Clicked : Event<void(const CoordinateEvent&)> { static constexpr std::string_view Name = "clicked"; };
  /// Reports a native long press, with the same coordinate contract as Clicked.
  struct LongPressed : Event<void(const CoordinateEvent&)> { static constexpr std::string_view Name = "longPressed"; };
  /// Reports the actual camera after user or application movement. It is an observation, not a request.
  /// Writing it back into application State is optional; equal submitted properties do not reset user panning.
  struct CameraIdle : Event<void(const CameraEvent&)> { static constexpr std::string_view Name = "cameraIdle"; };
  /// Reports Marker::id, independent of marker order, title, or the SDK's native object identity.
  struct MarkerClicked : Event<void(const std::string&)> { static constexpr std::string_view Name = "markerClicked"; };
  /// Reports initialization or update failure. The native adapter stops accepting updates; remount to retry.
  /// Invalid C++ arguments throw instead of emitting Failed. Some vendor service errors remain in SDK logs.
  struct Failed : Event<void(const MapError&)> { static constexpr std::string_view Name = "error"; };
  /// Compatibility spelling for the original event key; both names identify the same event.
  using Error = Failed;
};

/// Registers the native factories once through AppOptions::application_hooks, before mounting maps.
/// No provider SDK is initialized until its view is mounted with actual host-app consent.
/// @param context Borrowed application installation context; it is not retained.
void Install(ApplicationContext& context);
/// Compatibility spelling for Install. Register one spelling, not both.
void InstallMaps(ApplicationContext& context);
/// Reports whether this build includes a native backend for the provider.
/// Does not inspect API keys, service authorization, connectivity, or device graphics capabilities.
[[nodiscard]] bool IsSupported(Provider provider) noexcept;

/// Embeds the SDK's actual MapView/UIView via HuxerUI PlatformView.
/// Set a bounded frame or Grow. Camera/overlay properties update only when their submitted values change;
/// unrelated recompositions preserve user panning. Feed CameraIdle back to state for a controlled camera.
/// Changing provider requires a distinct View::Key so the native instance is replaced.
/// Unmounting releases the view's listeners and suppresses queued events; provider-global managers may remain alive.
/// @param provider SDK selection; coordinates are never converted when this changes.
/// @param options Owned declarative configuration. An equal configuration preserves the native instance's state.
/// @return A PlatformView leaf, or explanatory Text when this build has no backend for the provider.
/// @throws std::invalid_argument For invalid options/provider, or missing consent on a supported platform.
/// @throws std::logic_error When composing a supported native map without its registered factory.
[[nodiscard]] View MapView(Provider provider, MapOptions options = {});
/// Equivalent to MapView(Provider::AMap, options); expects GCJ-02 coordinates.
[[nodiscard]] View AMapView(MapOptions options = {});
/// Equivalent to MapView(Provider::Baidu, options); expects BD-09 coordinates.
[[nodiscard]] View BaiduMapView(MapOptions options = {});

} // namespace huxerui::map
