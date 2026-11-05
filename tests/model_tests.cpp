#include <huxerui/map/types.h>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace huxerui::map;
namespace {
int checks = 0;
void Check(bool condition) {
  ++checks;
  if (!condition) throw std::runtime_error("Map model check failed: " + std::to_string(checks));
}
void Reject(const std::function<void(MapOptions&)>& change) {
  MapOptions options;
  change(options);
  bool rejected = false;
  try { Validate(options); } catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected);
}
}
int main() {
  try {
    Validate(MapOptions{});
    Check(!PrivacyConsent{}.Granted());
    Check(!PrivacyConsent{true, false, true}.Granted());
    Check(PrivacyConsent{true, true, true}.Granted());
    Reject([](auto& o) { o.camera.zoom = std::numeric_limits<double>::quiet_NaN(); });
    Reject([](auto& o) { o.camera.zoom = 2.9; });
    Reject([](auto& o) { o.camera.zoom = 20.1; });
    Reject([](auto& o) { o.camera.center.latitude = 90.1; });
    Reject([](auto& o) { o.camera.center.longitude = -180.1; });
    Reject([](auto& o) { o.camera.center.longitude = std::numeric_limits<double>::infinity(); });
    Reject([](auto& o) { o.markers = {{"a"}, {"a"}}; });
    Reject([](auto& o) { o.markers = {{""}}; });
    Reject([](auto& o) { o.markers = {{"a", {-91, 0}}}; });
    Reject([](auto& o) { o.polylines = {{"line", {{0, 0}}}}; });
    Reject([](auto& o) { o.polylines = {{"line", {{0, 0}, {1, 1}}, 0}}; });
    Reject([](auto& o) { o.polylines = {{"line", {{0, 0}, {1, 1}}, 129}}; });
    Reject([](auto& o) { o.polylines = {{"line", {{0, 0}, {91, 0}}}}; });
    Reject([](auto& o) { o.polylines = {{"a", {{0, 0}, {1, 1}}}, {"a", {{0, 0}, {1, 1}}}}; });
    Reject([](auto& o) { o.type = static_cast<MapType>(99); });
    MapOptions valid;
    valid.camera = {{-90, 180}, 20};
    valid.markers = {{"a", {90, -180}, "Title", "Subtitle"}};
    valid.polylines = {{"a", {{0, 0}, {1, 1}}, 128}};
    Validate(valid);
    Check(valid == MapOptions(valid));
    std::cout << checks << " map model checks passed\n";
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
