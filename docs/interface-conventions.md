# Scave 的 Lib 接口约定对照

本文以 GitHub 用户 **FinalScave（Scave）本人编写的提交**为样本，而不是仅按组织归属或 PR 合并者判断作者。以下是源码中观察到的约定，不是 HuxerUI 发布的强制接口标准。

## 参考样本

| 库 | 本人实现提交 | 重点 |
| --- | --- | --- |
| Lib-WebView | [0353c9d4 的公开头文件](https://github.com/HuxerUI/Lib-WebView/blob/0353c9d4d4a5/include/huxerui/webview.h) | PlatformView、属性、状态快照、类型化事件、Controller |
| Lib-Camera | [cada0539 的公开头文件](https://github.com/HuxerUI/Lib-Camera/blob/cada05390987/include/huxerui/camera.h) | UseCamera、会话所有权、Options / Status 分离、Task / Result |
| Lib-Lottie | [7dad8008 的公开头文件](https://github.com/HuxerUI/Lib-Lottie/blob/7dad80083cdd/include/huxerui/lottie.h) | application_hooks 安装、声明式控制、重组和异步结果生命周期 |
| Lib-Charts | [0a265aad 的公开头文件](https://github.com/HuxerUI/Lib-Charts/blob/0a265aadde44/include/huxerui/charts.h) | 不可变数据、稳定身份、受控状态、专用链式配置 |
| Lib-Live2D | [801ef05a 的公开头文件](https://github.com/HuxerUI/Lib-Live2D/blob/801ef05aa3cf/include/huxerui/live2d.h) | Asset / Controller / View 分工、按需安装、异步加载与错误类型 |

这些提交的 GitHub author 均为 FinalScave。WebView 的合并提交未被当成他本人实现接口的证据；Camera 本地工作目录也未被直接当成作者版本。旧提交中出现的 `RootContext` / `root_hooks` 属于当时 SDK 接口，Lib-Map 继续使用当前项目已经验证的 `ApplicationContext` / `application_hooks`。

## 可以借鉴的共同做法

1. **先确定组件模型，再确定形式。** WebView 接收 Properties 并可绑定 Controller，Camera 使用 UseCamera 管理共享会话，Lottie 直接接收声明式播放控制，Charts 使用 View 派生类提供专用链式配置。不能把其中一种形式套到全部 Lib。

2. **声明输入与观察结果分开。** CameraOptions 不等于 CameraStatus；WebViewProperties 不等于 WebViewNavigationState。平台事件先报告观察结果，宿主决定是否写回 State。输入值相等时，应保留平台组件当前状态，避免重组导致重新加载或重置交互。

3. **复用 HuxerUI 的公共 UI 接口。** 组件交给调用方使用 `.On<Events::...>()`、`.With(...)`、`.Key(...)`。领域专用 builder 只有在配置复杂度需要时才引入；布局和普通事件不另造一套 API。

4. **命名保持领域清晰，而非一律改成一个命名空间。** WebView、Charts 在 `huxerui` 下使用完整领域类型名；Camera、Lottie、Live2D 使用领域子命名空间。函数/类型为 PascalCase，字段为 snake_case；配置值通常有默认值和相等比较。Lib-Map 的 `huxerui::map` 与 `MapOptions` 可以保留。

5. **Controller、UseXxx、Install 都按实际所有权需要提供。** 命令与声明输入分开；需要保留命令身份时，Controller 通过 UseXxx 或 UseState 保留。Camera 的会话由 UseCamera 的组合生命周期拥有。Lottie 使用 `lottie::Install` 注册共享后端；Live2D 则明确不需要根级安装。不要只为名称对称而添加没有用途的 Controller 或 Hook。

6. **事件携带有含义的数据，说明借用期限和线程。** WebView、Lottie、Charts 的结构化事件多使用 `const T&`，并要求调用方在保留或异步转发前复制。Live2D 也有合法的按值事件，因此引用传参不是强制规则。跨语言 PlatformView 事件需要稳定 Name 与 Encode / Decode 契约；纯 C++ 事件不必额外添加通道字符串。

7. **错误契约覆盖失败后的行为。** 非法调用参数可以抛异常，实际加载/渲染失败由领域事件或 Result 报告。说明错误后是否还能更新、如何重试、取消是否只停止等待、资源何时释放。诊断 message 不作为稳定错误分类；是否提供领域枚举，取决于接口是否确实承诺跨平台恢复语义。

8. **注释描述可依赖的行为。** 写清单位、范围、默认值、所有权、重组时的身份判定、事件发生时点、卸载与过期异步结果、平台差异，以及成功信号不保证的后续结果。SDK 函数名和实现细节不足以构成公共接口文档。

## Lib-Map 的应用

- 保留 `MapView(provider, MapOptions)` 与两个厂商快捷入口；现有操作可由状态表达，暂不增加没有命令需求的 MapController / UseMap。
- 安装入口为 `huxerui::map::Install`。
- 失败事件为 `MapEvents::Failed`，跨平台桥接通道事件名统一为 `failed`。
- 结构化事件和标记 ID 使用 `const T&`，文档明确事件回调引用只在同步调用期间有效。
- 明确 `MapOptions::camera` 是输入请求，`CameraEvent` 是实际视角快照；稳定 overlay ID 与数组下标、显示标题、厂商对象身份无关。
- 明确平台能力查询不代表鉴权成功，Loaded 不代表在线地图已成功显示，失败需重新挂载重试，平台级管理器不随每个 View 一起销毁。

以上调整统一调用习惯和行为契约，不改变地图提供商、坐标系、平台挂载方式或布局策略。
