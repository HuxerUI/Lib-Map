# Map Preview

## Android

设置 `HUXERUI_HOME` 为 HuxerUI 源码或 SDK；使用 JDK 17。为示例包名 `org.huxerui.lib.map.preview` 和安装签名申请两个厂商的 Android Key。

将配置放在被 Git 忽略的 `platform/android/local.properties`：

```properties
sdk.dir=C:/Users/you/AppData/Local/Android/Sdk
amap.apiKey=YOUR_AMAP_ANDROID_KEY
baidu.apiKey=YOUR_BAIDU_ANDROID_KEY
```

也可通过 `AMAP_ANDROID_API_KEY` / `BAIDU_ANDROID_API_KEY` 环境变量传入，优先于本地配置。

在本目录运行 HuxerUI CLI：

```sh
huxerui build android
huxerui run android
```

CLI 会先生成 `.huxerui/generated/libraries.json`，再构建 Android 项目。如需直接使用 Gradle，先通过 CMake 生成库图：

```sh
cmake -S . -B build-graph -DHUXERUI_HOME=/path/to/HuxerUI -DHUXERUI_LIBRARY_GRAPH_ONLY=ON
cd platform/android
./gradlew assembleDebug
```

Windows 使用 `gradlew.bat`。默认 `huxeruiAbis=arm64-v8a`：固定的高德版本只提供 ARM 原生库。不要直接切到 x86_64 模拟器；更换 SDK 版本后也应确认两家厂商都覆盖目标架构。

## iOS

在 macOS 配置 HuxerUI SDK、地图 SDK framework 与资源。复制 `platform/ios/Config/Local.xcconfig.example` 为同目录的 `Local.xcconfig`，填入签名团队、SDK 路径及两个 iOS Key。Key 必须绑定示例 Bundle ID `org.huxerui.lib.map.preview`。

```sh
huxerui build ios
huxerui run ios
```

CLI 生成 Swift 库依赖图；也可之后打开 `platform/ios/example_map.xcodeproj`。构建脚本会把 framework 搜索目录传给 CMake，并将配置的资源目录中的 `.bundle` 拷贝到 app。

## Desktop

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build --config Release
```

运行生成的 `example_map`。桌面显示明确的平台 SDK 不可用提示，可操作工具栏检查界面状态。

## 设备验收

1. 未同意隐私政策时不创建地图；同意后加载所选厂商的 SDK 控件。
2. 验证中心点、缩放、卫星图、路况、标记和折线；确认厂商 Logo 保持可见。
3. 拖动地图后切换路况/覆盖物，视角应保留；点击、长按、标记事件显示在状态栏。
4. 卸载、重新挂载，切换厂商，应用进入后台再回前台，确认资源和回调生命周期正常。
5. 撤销同意后卸载地图；缺失 Key 显示配置错误。错误 Key 的鉴权结果同时查看厂商 SDK 日志。
