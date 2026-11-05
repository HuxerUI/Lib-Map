#include <huxerui/map/types.h>

#include <cmath>
#include <set>
#include <stdexcept>

namespace huxerui::map {
namespace {
void ValidateCoordinate(Coordinate coordinate) {
  if (!std::isfinite(coordinate.latitude) || !std::isfinite(coordinate.longitude) ||
      coordinate.latitude < -90.0 || coordinate.latitude > 90.0 ||
      coordinate.longitude < -180.0 || coordinate.longitude > 180.0) {
    throw std::invalid_argument("Map coordinates must be finite latitude [-90,90], longitude [-180,180]");
  }
}
void ValidateId(std::set<std::string>& ids, const std::string& id) {
  if (id.empty() || !ids.insert(id).second)
    throw std::invalid_argument("Map overlay IDs must be non-empty and unique within their kind");
}
} // namespace

void Validate(const MapOptions& options) {
  ValidateCoordinate(options.camera.center);
  if (!std::isfinite(options.camera.zoom) || options.camera.zoom < 3.0 || options.camera.zoom > 20.0)
    throw std::invalid_argument("Map zoom must be finite and in [3,20]");
  if (options.type != MapType::Standard && options.type != MapType::Satellite)
    throw std::invalid_argument("Unknown map type");
  std::set<std::string> marker_ids, line_ids;
  for (const auto& marker : options.markers) {
    ValidateId(marker_ids, marker.id);
    ValidateCoordinate(marker.position);
  }
  for (const auto& line : options.polylines) {
    ValidateId(line_ids, line.id);
    if (line.points.size() < 2 || !std::isfinite(line.width) || line.width <= 0.0 || line.width > 128.0)
      throw std::invalid_argument("Map polylines require at least two points and width in (0,128]");
    for (const auto point : line.points) ValidateCoordinate(point);
  }
}
} // namespace huxerui::map
