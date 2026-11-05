// swift-tools-version: 5.9
import PackageDescription

// HuxerUI's library graph requires an iOS package product for every native library.
// The actual C++/Objective-C++ SDK adapters are built and linked by CMake.
let package = Package(
    name: "Map",
    platforms: [.iOS(.v15)],
    products: [.library(name: "Map", targets: ["Map"])],
    targets: [.target(name: "Map")]
)
