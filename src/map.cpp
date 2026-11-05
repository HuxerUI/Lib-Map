#include "map_internal.h"
#include <stdexcept>
#include <utility>

namespace huxerui::map {
namespace {
PlatformPayload EncodeCoordinate(Coordinate value) {
  return PlatformPayload::Object{{"latitude", value.latitude}, {"longitude", value.longitude}};
}
Coordinate DecodeCoordinate(const PlatformPayload& payload) {
  const auto& value = payload.AsObject();
  return {value.at("latitude").AsDouble(), value.at("longitude").AsDouble()};
}
} // namespace

PlatformPayload detail::MapProperties::Encode(const MapProperties& properties) {
  const auto& value = properties.options;
  Validate(value);
  PlatformPayload::List markers, polylines;
  for (const auto& marker : value.markers) {
    markers.emplace_back(PlatformPayload::Object{
        {"id", marker.id}, {"position", EncodeCoordinate(marker.position)},
        {"title", marker.title}, {"subtitle", marker.subtitle}});
  }
  for (const auto& line : value.polylines) {
    PlatformPayload::List points;
    for (const auto point : line.points) points.push_back(EncodeCoordinate(point));
    polylines.emplace_back(PlatformPayload::Object{
        {"id", line.id}, {"points", std::move(points)}, {"width", line.width},
        {"color", static_cast<std::int64_t>(line.color_argb)}});
  }
  return PlatformPayload::Object{
      {"camera", PlatformPayload::Object{{"center", EncodeCoordinate(value.camera.center)}, {"zoom", value.camera.zoom}}},
      {"markers", std::move(markers)}, {"polylines", std::move(polylines)},
      {"satellite", value.type == MapType::Satellite}, {"traffic", value.traffic_enabled},
      {"gestures", value.gestures_enabled}, {"zoomControls", value.zoom_controls}, {"scaleControl", value.scale_control},
      {"privacy", value.privacy.Granted()}};
}

CoordinateEvent CoordinateEvent::Decode(const PlatformPayload& payload) { return {DecodeCoordinate(payload)}; }
CameraEvent CameraEvent::Decode(const PlatformPayload& payload) {
  const auto& value = payload.AsObject();
  return {{DecodeCoordinate(value.at("center")), value.at("zoom").AsDouble()}};
}
MapError MapError::Decode(const PlatformPayload& payload) {
  const auto& value = payload.AsObject();
  return {std::string(value.at("code").AsString()), std::string(value.at("message").AsString())};
}

void InstallMaps(ApplicationContext& context) { detail::InstallPlatformMaps(context); }
View MapView(Provider provider, MapOptions options) {
  if (provider != Provider::AMap && provider != Provider::Baidu) throw std::invalid_argument("Unknown map provider");
  Validate(options);
  if (!IsSupported(provider)) return Text("Native AMap / Baidu Map SDKs require an enabled Android or iOS backend.");
  if (!options.privacy.Granted()) throw std::invalid_argument("Obtain provider privacy consent before mounting MapView");
  return PlatformView(provider == Provider::AMap ? detail::amap_type : detail::baidu_type,
                      detail::MapProperties{std::move(options)});
}
View AMapView(MapOptions options) { return MapView(Provider::AMap, std::move(options)); }
View BaiduMapView(MapOptions options) { return MapView(Provider::Baidu, std::move(options)); }
} // namespace huxerui::map
