# Lib-Map

HuxerUI 地图组件拓展：将高德、百度地图的**原生 SDK 控件**嵌入 HuxerUI `PlatformView`，提供统一的 C++20 声明式接口。

- Android：高德 `TextureMapView`、百度 `TextureMapView`，通过 HuxerUI Java PlatformView 桥接。
- iOS：已实现高德 `MAMapView`、百度 `BMKMapView` 的 UIKit PlatformView 适配；尚未在 macOS / Xcode 编译或真机验证。
- Windows / macOS / Linux / Web：这两个厂商没有本库可用的对应原生后端；`IsSupported` 返回 `false`，组件显示说明文字。

不使用 WebView，不打包网页地图。API Key、SDK 二进制与用户授权由宿主应用配置。

## 功能

统一支持中心点与缩放、普通/卫星地图、实时路况、手势、比例尺、标记、折线，以及加载、点击、长按、视角停止变化、标记点击和初始化错误事件。

标记与折线通过完整列表声明；只有列表内容发生变化时才更新原生覆盖物。视角同样只在提交的 `camera` 改变时更新，切换路况等普通重组不会重置用户拖动后的视角。需要受控视角时，将 `CameraIdle` 写回状态。缩放按钮控件仅 Android 支持；iOS 可以使用 HuxerUI 按钮修改 `camera.zoom`。

本版本覆盖地图展示与交互，不包含定位、POI 检索、路线规划、离线地图，也不会申请定位权限。厂商服务鉴权失败可能由 SDK 自身界面/日志报告，`Loaded` 只表示控件加载完成，不代表在线服务鉴权成功。

## 接入

依赖具有 `PlatformView`、`RegisterPlatformView`、Java / UIKit 原生桥接能力的 HuxerUI SDK 或源码。

```cmake
huxerui_use_library(my_app
    TARGET HuxerUI::Map
    PATH "/path/to/Lib-Map")
```

也可以通过 `URL https://github.com/HuxerUI/Lib-Map.git` 与 `COMMIT <固定提交>` 接入。

```cpp
#include <huxerui/huxerui.h>
#include <huxerui/map.h>
using namespace huxerui;
using namespace huxerui::map;

View Content() {
  MapOptions options;
  options.camera = {{39.90923, 116.39748}, 14};
  options.markers = {{"tiananmen", {39.90923, 116.39748}, "天安门", "北京"}};
  // 仅在宿主已展示包含厂商条款的政策、并取得实际用户同意后传入。
  options.privacy = {true, true, true};
  return AMapView(options)
      .On<MapEvents::MarkerClicked>([](const std::string& id) { /* ... */ })
      .On<MapEvents::Clicked>([](const CoordinateEvent& event) { /* event.coordinate */ })
      .On<MapEvents::CameraIdle>([](const CameraEvent& event) { /* event.camera */ })
      .On<MapEvents::Failed>([](const MapError& error) { /* error.code, error.message */ })
      .With(Frame{.height = 400.0F});
}

const Application application{
  Content,
  {.application_hooks = {huxerui::map::Install}},
};
```

使用 `BaiduMapView(options)` 或 `MapView(Provider::Baidu, options)` 切换厂商。更换厂商时使用不同 `.Key(...)`，确保原生实例卸载并重新创建。

`Coordinate` 的字段顺序是 **latitude, longitude**。高德使用 **GCJ-02**；百度在初始化时固定为 **BD-09**。本库不自动转换坐标，不能把同一组坐标直接用于两家厂商。宿主不要在其他位置修改百度 SDK 的全局坐标类型。

`MapOptions` 在组件构造时验证：坐标必须有限且合法，缩放范围 `[3,20]`，覆盖物 ID 在同类内非空且唯一，折线至少两个点、宽度 `(0,128]`。厂商对实际缩放范围仍可能进一步限制。

`map::Install` 只注册工厂，不初始化任何地图 SDK。首次挂载前必须取得所选厂商的隐私授权；撤销授权时卸载地图。Android 会转发 attach/detach 与 Activity resume/pause/destroy；iOS 百度会转发前后台生命周期，卸载时清除 delegate、覆盖物和通知监听。进程级 SDK 管理器仍遵循厂商生命周期，撤销授权不保证终止厂商已启动的全部后台工作。

事件处理器在所属 UI 线程执行，事件中的引用仅在回调期间有效；要保存或异步转发时请复制值。接口取舍与 Scave 本人实现的其他 Lib 对照见 [接口约定](docs/interface-conventions.md)。

## Android

最低 API 23，示例 compileSdk/targetSdk 36，JDK **17**，NDK `29.0.14206865`。Gradle 模块位于 `platform/android`，兼容 HuxerUI 原生库依赖图。

固定 Maven 依赖为高德 `com.amap.api:3dmap:10.0.600`、百度 `com.baidu.lbsyun:BaiduMapSDK_Map:8.2.0`。可通过 Gradle 属性 `huxeruiMapAMapVersion` / `huxeruiMapBaiduVersion` 覆盖。这些是可解析的固定版本，不声明为厂商最新版。高德 JAR 内的 `.so` 由构建任务提取到 `jniLibs`，百度 AAR 通过 Gradle 自动合并。

应用 Manifest 需配置：

```xml
<meta-data android:name="com.amap.api.v2.apikey" android:value="你的高德 Android Key" />
<meta-data android:name="com.baidu.lbsapi.API_KEY" android:value="你的百度 Android AK" />
```

Key 需与应用包名、实际签名 SHA-1 对应。库 Manifest 合并网络权限，不申请设备定位或存储权限。宿主应保留库的 consumer ProGuard 规则。

百度 base AAR 也带有 `libc++_shared.so`；HuxerUI 应用应使用自身 NDK 的运行库，并在应用 Gradle 中消除重复打包（demo 已配置）：

```groovy
android {
    packaging { jniLibs { pickFirsts += ["**/libc++_shared.so"] } }
}
```

直接分发本地 AAR 时，Maven 传递依赖不会自动嵌入 AAR；还需同时声明上述两个厂商的 Maven 依赖，或使用 HuxerUI 的 Gradle 项目集成。

## iOS

最低 iOS 15。需在 macOS / Xcode 环境取得并配置厂商 SDK 的匹配架构 `.framework` 切片：

- 高德：`MAMapKit.framework`、`AMapFoundationKit.framework`。
- 百度：`BaiduMapAPI_Base.framework`、`BaiduMapAPI_Map.framework`。

通过 CMake `HUXERUI_MAP_IOS_FRAMEWORK_DIRS` 传入包含这些 framework 的目录（分号分隔），厂商额外的静态依赖（如百度 `thirdlibs`）通过 `HUXERUI_MAP_IOS_EXTRA_LIBRARIES` 传入绝对路径列表。如果厂商分发 XCFramework，选择对应设备/模拟器切片，不要把两种架构目录混用。默认启用两个厂商；只使用一家时可关闭 `HUXERUI_MAP_ENABLE_AMAP` 或 `HUXERUI_MAP_ENABLE_BAIDU`。所需 framework 缺失时配置会明确报错。

在应用 Info.plist 中配置 `HuxerUIAMapAPIKey` / `HuxerUIBaiduMapAPIKey`，并将厂商 SDK 所需的资源 bundle（如 `AMap.bundle`、`mapapi.bundle`）放入应用资源。动态 framework 还需按厂商要求 Embed & Sign。本库不重新分发 SDK，不替代厂商的依赖、资源、隐私清单与签名要求。

`platform/ios/Package.swift` 是 HuxerUI 库图的 Swift 包入口；地图适配器本身由 CMake 编译 Objective-C++。

## Demo

`examples/preview` 遵循其他 HuxerUI lib 的应用布局，包含 `HuxerUIProject.cmake`、原生平台入口和统一 C++ 界面。支持厂商切换、缩放、回到天安门、卫星图、路况、手势、标记与折线、卸载/重新挂载和事件状态。切换厂商会重新请求该厂商的隐私确认。

详细步骤见 [demo README](examples/preview/README.md)。桌面 demo 用于检查布局与交互，不模拟地图成功加载。

## 构建与测试

只验证数据模型，无需 HuxerUI / 地图 SDK：

```sh
cmake -S . -B build-core -DHUXERUI_MAP_BUILD_VIEW=OFF -DHUXERUI_MAP_BUILD_TESTS=ON
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
```

完整组件、事件载荷测试与桌面 demo：

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_MAP_BUILD_TESTS=ON -DHUXERUI_MAP_BUILD_EXAMPLES=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

模型测试检查坐标/缩放/覆盖物边界，载荷测试检查 C++ 到原生桥接的数据与事件契约。移动端真实地图的显示、鉴权和手势仍需使用自己的 Key 与设备验收；当前 Windows 开发环境不能执行 iOS 构建。

## 官方参考

- [HuxerUI](https://github.com/HuxerUI/HuxerUI) / [Lib-Camera](https://github.com/HuxerUI/Lib-Camera) / [Lib-WebView](https://github.com/HuxerUI/Lib-WebView)
- [高德 Android MapsInitializer / 隐私初始化接口](https://a.amap.com/lbs/static/unzip/Android_Map_Doc/3D/com/amap/api/maps/MapsInitializer.html)
- [高德 iOS MAMapView](https://a.amap.com/lbs/static/unzip/iOS_Map_Doc/AMap_iOS_API_Doc_3D/interface_m_a_map_view.html)
- [百度 Android Gradle 接入](https://lbsyun.baidu.com/docs/android?title=androidsdk%2Fguide%2Fcreate-project%2Fandroidstudio)
- [百度 iOS 显示地图与坐标类型](https://lbsyun.baidu.com/docs/ios?title=iossdk%2Fguide%2Fcreate-map%2Fshowmap)
- [百度 iOS 初始化与隐私接口](https://lbs.baidu.com/docs/ios?title=iossdk%2Fguide%2Fcreate-project%2Fattention)

MIT License；地图 SDK 及在线服务分别适用各厂商条款。
