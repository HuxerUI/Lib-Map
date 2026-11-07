#include "map_ios_internal.h"
#import <AMapFoundationKit/AMapFoundationKit.h>
#import <MAMapKit/MAMapKit.h>
#include <map>

using namespace huxerui::map;
using namespace huxerui::map::detail;

@interface HUIAMapMarker : MAPointAnnotation
@property(nonatomic, copy) NSString* markerId;
@end
@implementation HUIAMapMarker
@end

@interface HUIAMapDelegate : NSObject <MAMapViewDelegate> {
@public
  std::shared_ptr<EventSink> sink;
  std::map<const void*, Polyline> lines;
}
@end
@implementation HUIAMapDelegate
- (void)mapViewDidFinishLoadingMap:(MAMapView*)map { HUXERUI_MAP_UNUSED(map); sink->Emit("loaded"); }
- (void)mapView:(MAMapView*)map didSingleTappedAtCoordinate:(CLLocationCoordinate2D)coordinate {
  HUXERUI_MAP_UNUSED(map); sink->CoordinateEvent("clicked", coordinate);
}
- (void)mapView:(MAMapView*)map didLongPressedAtCoordinate:(CLLocationCoordinate2D)coordinate {
  HUXERUI_MAP_UNUSED(map); sink->CoordinateEvent("longPressed", coordinate);
}
- (void)mapView:(MAMapView*)map regionDidChangeAnimated:(BOOL)animated {
  HUXERUI_MAP_UNUSED(animated); sink->CameraIdle(map.centerCoordinate, map.zoomLevel);
}
- (MAAnnotationView*)mapView:(MAMapView*)map viewForAnnotation:(id<MAAnnotation>)annotation {
  if (![annotation isKindOfClass:HUIAMapMarker.class]) return nil;
  MAPinAnnotationView* view = (MAPinAnnotationView*)[map dequeueReusableAnnotationViewWithIdentifier:@"hui-marker"];
  if (!view) view = [[MAPinAnnotationView alloc] initWithAnnotation:annotation reuseIdentifier:@"hui-marker"];
  view.annotation = annotation;
  view.canShowCallout = YES;
  return view;
}
- (void)mapView:(MAMapView*)map didSelectAnnotationView:(MAAnnotationView*)view {
  HUXERUI_MAP_UNUSED(map);
  if ([view.annotation isKindOfClass:HUIAMapMarker.class])
    sink->Emit("markerClicked", ((HUIAMapMarker*)view.annotation).markerId.UTF8String ?: "");
}
- (MAOverlayRenderer*)mapView:(MAMapView*)map rendererForOverlay:(id<MAOverlay>)overlay {
  HUXERUI_MAP_UNUSED(map);
  const auto found = lines.find((__bridge const void*)overlay);
  if (found == lines.end() || ![overlay isKindOfClass:MAPolyline.class]) return nil;
  MAPolylineRenderer* renderer = [[MAPolylineRenderer alloc] initWithPolyline:(MAPolyline*)overlay];
  renderer.lineWidth = found->second.width;
  renderer.strokeColor = Color(found->second.color_argb);
  return renderer;
}
- (void)mapViewDidFailLoadingMap:(MAMapView*)map withError:(NSError*)error {
  HUXERUI_MAP_UNUSED(map);
  sink->Emit("failed", huxerui::PlatformPayload::Object{
      {"code", "sdk-load-failed"}, {"message", error.localizedDescription.UTF8String ?: "AMap load failed"}});
}
@end

namespace huxerui::map::detail {
namespace {
class AMapInstance final : public PlatformMap {
public:
  AMapInstance(const MapOptions& options, std::shared_ptr<EventSink> sink) : sink_(std::move(sink)) {
    NSString* key = [NSBundle.mainBundle objectForInfoDictionaryKey:@"HuxerUIAMapAPIKey"];
    if (![key isKindOfClass:NSString.class] || key.length == 0 || [key hasPrefix:@"YOUR_"])
      throw std::invalid_argument("Set HuxerUIAMapAPIKey in the app Info.plist");
    [MAMapView updatePrivacyShow:AMapPrivacyShowStatusDidShow privacyInfo:AMapPrivacyInfoStatusDidContain];
    [MAMapView updatePrivacyAgree:AMapPrivacyAgreeStatusAgreed];
    [AMapServices sharedServices].apiKey = key;
    map_ = [[MAMapView alloc] initWithFrame:CGRectZero];
    delegate_ = [HUIAMapDelegate new];
    delegate_->sink = sink_;
    map_.delegate = delegate_;
    Update(options);
  }
  ~AMapInstance() override { Dispose(); }
  UIView* View() override { return map_; }
  void Update(const MapOptions& options) override {
    if (!options.privacy.Granted()) { Dispose(); return; }
    const auto& old = previous_;
    if (!old || old->type != options.type) map_.mapType = options.type == MapType::Satellite ? MAMapTypeSatellite : MAMapTypeStandard;
    if (!old || old->traffic_enabled != options.traffic_enabled) map_.showTraffic = options.traffic_enabled;
    if (!old || old->scale_control != options.scale_control) map_.showsScale = options.scale_control;
    if (!old || old->gestures_enabled != options.gestures_enabled) {
      map_.scrollEnabled = options.gestures_enabled; map_.zoomEnabled = options.gestures_enabled;
      map_.rotateEnabled = options.gestures_enabled; map_.rotateCameraEnabled = options.gestures_enabled;
    }
    if (!old || old->camera != options.camera) {
      [map_ setCenterCoordinate:Coordinate2D(options.camera.center) animated:NO];
      [map_ setZoomLevel:options.camera.zoom animated:NO];
    }
    if (!old || old->markers != options.markers) {
      [map_ removeAnnotations:markers_];
      markers_ = [NSMutableArray new];
      for (const auto& marker : options.markers) {
        HUIAMapMarker* item = [HUIAMapMarker new];
        item.coordinate = Coordinate2D(marker.position); item.title = String(marker.title);
        item.subtitle = String(marker.subtitle); item.markerId = String(marker.id);
        [markers_ addObject:item];
      }
      [map_ addAnnotations:markers_];
    }
    if (!old || old->polylines != options.polylines) {
      [map_ removeOverlays:overlays_];
      delegate_->lines.clear(); overlays_ = [NSMutableArray new];
      for (const auto& line : options.polylines) {
        std::vector<CLLocationCoordinate2D> points;
        for (auto point : line.points) points.push_back(Coordinate2D(point));
        MAPolyline* item = [MAPolyline polylineWithCoordinates:points.data() count:points.size()];
        delegate_->lines[(__bridge const void*)item] = line;
        [overlays_ addObject:item];
      }
      [map_ addOverlays:overlays_];
    }
    previous_ = options;
  }
  void Dispose() override {
    if (!sink_->active) return;
    sink_->active = false;
    map_.delegate = nil;
    [map_ removeAnnotations:markers_]; [map_ removeOverlays:overlays_];
    [map_ removeFromSuperview];
    delegate_ = nil; map_ = nil; markers_ = nil; overlays_ = nil;
  }
private:
  std::shared_ptr<EventSink> sink_;
  MAMapView* map_;
  HUIAMapDelegate* delegate_;
  NSMutableArray* markers_;
  NSMutableArray* overlays_;
  std::optional<MapOptions> previous_;
};
} // namespace
std::shared_ptr<PlatformMap> CreateAMap(const MapOptions& options, std::shared_ptr<EventSink> sink) {
  return std::make_shared<AMapInstance>(options, std::move(sink));
}
} // namespace huxerui::map::detail
