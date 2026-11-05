#include <huxerui/huxerui.h>
#include <huxerui/map.h>
#include <algorithm>
#include <string>

namespace map_preview {
using namespace huxerui;
using namespace huxerui::map;

Coordinate Landmark(Provider provider) {
  return provider == Provider::AMap ? Coordinate{39.90923, 116.39748} : Coordinate{39.91542, 116.40387};
}

[[huxerui::composable]]
View AppContent() {
  auto provider = UseState(Provider::AMap);
  auto camera = UseState(CameraPosition{Landmark(Provider::AMap), 14});
  auto consent = UseState(false);
  auto visible = UseState(true);
  auto satellite = UseState(false);
  auto traffic = UseState(false);
  auto markers = UseState(true);
  auto gestures = UseState(true);
  auto status = UseState(std::string("点击地图查看坐标；点击标记查看 ID。"));
  const auto theme = UseTheme();
  const auto current = provider.Get();

  View content = Text("请先确认已阅读所选地图厂商的隐私政策。");
  if (!IsSupported(current)) {
    content = Column{
      Text("此平台没有启用所选厂商的原生地图 SDK。", TextRole::Title),
      Text("请在 Android 或 iOS 运行本 demo。桌面端可查看界面和操作状态。"),
    }.With(Padding(24.0F), Spacing(12.0F));
  } else if (consent.Get() && visible.Get()) {
    MapOptions options;
    options.camera = camera.Get();
    options.type = satellite.Get() ? MapType::Satellite : MapType::Standard;
    options.traffic_enabled = traffic.Get();
    options.gestures_enabled = gestures.Get();
    options.privacy = {true, true, true};
    const auto center = Landmark(current);
    if (markers.Get()) {
      options.markers = {{"landmark", center, "天安门", "Provider-native coordinate"},
                        {"north", {center.latitude + .008, center.longitude}, "北侧", "Native marker"}};
      options.polylines = {{"walk", {center, {center.latitude + .004, center.longitude + .004},
                                      {center.latitude + .008, center.longitude}}, 5, 0xFF2563EB}};
    }
    content = MapView(current, std::move(options))
        .On<MapEvents::Loaded>([status] { status = "原生地图已加载。"; })
        .On<MapEvents::Clicked>([status](CoordinateEvent event) {
          status = "点击：" + std::to_string(event.coordinate.latitude) + ", " + std::to_string(event.coordinate.longitude);
        })
        .On<MapEvents::LongPressed>([status](CoordinateEvent event) {
          status = "长按：" + std::to_string(event.coordinate.latitude) + ", " + std::to_string(event.coordinate.longitude);
        })
        .On<MapEvents::CameraIdle>([camera](CameraEvent event) {
          // Preserve the latest native camera when toolbar actions change zoom.
          auto next = event.camera;
          next.zoom = std::clamp(next.zoom, 3.0, 20.0);
          if (camera.Get() != next) camera = next;
        })
        .On<MapEvents::MarkerClicked>([status](std::string id) { status = "标记：" + id; })
        .On<MapEvents::Error>([status](MapError error) { status = error.code + ": " + error.message; })
        .Key(current == Provider::AMap ? "amap-native" : "baidu-native");
  } else if (!visible.Get()) {
    content = Text("地图已卸载，SDK 控件和事件监听已释放。");
  }

  return Column{
    Text("Native Map Playground", TextRole::Title),
    Text(current == Provider::AMap ? "高德 · GCJ-02" : "百度 · BD-09"),
    Flow{
      Button("高德地图").OnClick([=] {
        provider = Provider::AMap; camera = CameraPosition{Landmark(Provider::AMap), 14}; consent = false;
      }).Key("provider-amap"),
      Button("百度地图").OnClick([=] {
        provider = Provider::Baidu; camera = CameraPosition{Landmark(Provider::Baidu), 14}; consent = false;
      }).Key("provider-baidu"),
      Button("放大").OnClick([camera] { auto next = camera.Get(); next.zoom = std::min(20.0, next.zoom + 1); camera = next; }),
      Button("缩小").OnClick([camera] { auto next = camera.Get(); next.zoom = std::max(3.0, next.zoom - 1); camera = next; }),
      Button("回到天安门").OnClick([=] { camera = CameraPosition{Landmark(current), 14}; }),
      Button(visible.Get() ? "卸载地图" : "挂载地图").OnClick([visible] { visible = !visible.Get(); }),
    }.With(Spacing(8.0F)),
    Flow{
      Checkbox("卫星地图", satellite.Get()).OnChanged([satellite](bool value) { satellite = value; }),
      Checkbox("实时路况", traffic.Get()).OnChanged([traffic](bool value) { traffic = value; }),
      Checkbox("标记与折线", markers.Get()).OnChanged([markers](bool value) { markers = value; }),
      Checkbox("允许手势", gestures.Get()).OnChanged([gestures](bool value) { gestures = value; }),
    }.With(Spacing(12.0F)),
    Text(current == Provider::AMap ? "高德隐私政策：https://lbs.amap.com/pages/privacy/"
                                  : "百度隐私政策：https://lbsyun.baidu.com/index.php?title=openprivacy"),
    Checkbox("我已阅读并同意所选地图厂商的隐私政策；同意后加载在线原生地图。", consent.Get())
        .OnChanged([consent](bool value) { consent = value; }).Key("privacy-consent"),
    std::move(content).With(Grow{}, Background{theme.colors.surface_container_low}).Key("map-container"),
    Text(status.Get()).With(Foreground{theme.colors.on_surface_variant}).Key("status"),
  }.With(Padding(16.0F), Spacing(12.0F), CrossAlign(CrossAxisAlignment::Stretch), Background{theme.colors.background});
}
View App() { return MaterialTheme{AppContent()}; }
} // namespace map_preview

const huxerui::Application application{
    map_preview::App,
    {.window = {.title = "HuxerUI Native Maps", .initial_size = {1080.0F, 800.0F}},
     .show_debug_overlay = false, .application_hooks = {huxerui::map::InstallMaps}},
};
