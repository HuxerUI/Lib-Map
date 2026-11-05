#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace huxerui::map {

enum class Provider { AMap, Baidu };
enum class MapType { Standard, Satellite };

/// Coordinates are provider-native: GCJ-02 for AMap, BD-09 for Baidu.
/// Switching providers does not convert coordinates.
struct Coordinate {
  double latitude = 39.909;
  double longitude = 116.397;
  bool operator==(const Coordinate&) const = default;
};

struct CameraPosition {
  Coordinate center;
  double zoom = 12.0; ///< Shared supported input range: [3, 20]. SDKs may clamp further.
  bool operator==(const CameraPosition&) const = default;
};

struct Marker {
  std::string id;
  Coordinate position;
  std::string title;
  std::string subtitle;
  bool operator==(const Marker&) const = default;
};

struct Polyline {
  std::string id;
  std::vector<Coordinate> points;
  double width = 5.0; ///< Logical pixels / iOS points; converted to Android pixels.
  std::uint32_t color_argb = 0xFF2563EB;
  bool operator==(const Polyline&) const = default;
};

/// Report actual host-app consent before mounting a map. No SDK is initialized by InstallMaps.
struct PrivacyConsent {
  bool policy_shown = false;
  bool policy_contains_provider = false;
  bool agreed = false;
  [[nodiscard]] bool Granted() const noexcept {
    return policy_shown && policy_contains_provider && agreed;
  }
  bool operator==(const PrivacyConsent&) const = default;
};

struct MapOptions {
  CameraPosition camera;
  std::vector<Marker> markers;
  std::vector<Polyline> polylines;
  MapType type = MapType::Standard;
  bool traffic_enabled = false;
  bool gestures_enabled = true;
  bool zoom_controls = true;
  bool scale_control = true;
  PrivacyConsent privacy;
  bool operator==(const MapOptions&) const = default;
};

/// Rejects non-finite/out-of-range coordinates, invalid camera/line values, and duplicate or empty IDs.
/// Throws std::invalid_argument. IDs are unique within each overlay kind.
void Validate(const MapOptions& options);

} // namespace huxerui::map
