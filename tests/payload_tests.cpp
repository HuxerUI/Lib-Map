#include "map_internal.h"
#include <iostream>
#include <stdexcept>

using namespace huxerui;
using namespace huxerui::map;
void Check(bool condition) { if (!condition) throw std::runtime_error("Map payload check failed"); }
int main() {
  try {
    MapOptions options;
    options.privacy = {true, true, true};
    options.markers = {{"landmark", {39.9, 116.4}, "Beijing", "Map marker"}};
    options.polylines = {{"route", {{39.9, 116.4}, {39.91, 116.41}}, 4, 0xFF2563EB}};
    const auto encoded = huxerui::map::detail::MapProperties::Encode({options});
    const auto restored = PlatformPayload::Decode(encoded.Encode());
    Check(restored == encoded);
    const auto& fields = restored.AsObject();
    Check(fields.at("privacy").AsBoolean());
    Check(fields.at("markers").AsList()[0].AsObject().at("id").AsString() == "landmark");
    Check(fields.at("polylines").AsList()[0].AsObject().at("color").AsInteger() == 0xFF2563EBLL);
    Check(CameraEvent::Decode(fields.at("camera")).camera == options.camera);
    Check(CoordinateEvent::Decode(fields.at("markers").AsList()[0].AsObject().at("position")).coordinate == options.markers[0].position);
    const auto error = MapError::Decode(PlatformPayload::Object{{"code", "test"}, {"message", "message"}});
    Check(error.code == "test" && error.message == "message");
    auto changed = options;
    changed.traffic_enabled = true;
    const auto next = huxerui::map::detail::MapProperties::Encode({changed});
    Check(next.AsObject().at("camera") == fields.at("camera"));
    Check(next.AsObject().at("markers") == fields.at("markers"));
    Check(next != encoded);
    std::cout << "Map payload and event checks passed\n";
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
