# Validation — 2026-11-05

Verified against HuxerUI `cadb1696cfa7b87d9d2d316b3ecba07b85a46962`.

| Check | Result |
| --- | --- |
| Standalone model build and CTest, MSVC / Release | Passed |
| Full HuxerUI component build, MSVC / Release | Passed |
| Model and native payload/event CTest | 2/2 passed |
| Library and preview project-plan output | Passed |
| Preview library dependency graph | Passed |
| Android Java adapters / debug AAR | Passed |
| Android arm64 C++/JNI build and complete debug APK | Passed |
| APK contents: both provider SDKs, app, HuxerUI and resources | Verified |
| Packaged C++ runtime | Build ID matches NDK 29 runtime |
| Desktop preview: layout, provider switch, toolbar interaction | Checked in running app |
| iOS SDK API signatures | Reviewed against AMap 11.2.000 and BaiduMapKit 7.2.0 headers |
| iOS compilation / signing / rendering | Not run: Windows host |
| Live map authentication / rendering / mobile lifecycle | Not run: no provider keys or connected device |

Android build used JDK 17, Gradle 8.13, AGP 8.13.2, API 36, NDK 29.0.14206865, AMap 10.0.600 and Baidu 8.2.0. The vendor common JAR emits D8 stack-map warnings; the complete APK still builds successfully. No real API key was used or committed.

Use the device acceptance steps in `examples/preview/README.md` after configuring app-specific keys. A successful build does not establish SDK authentication or mobile rendering correctness.
