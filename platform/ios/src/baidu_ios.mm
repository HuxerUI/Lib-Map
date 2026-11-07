#include "map_ios_internal.h"
#import <BaiduMapAPI_Base/BMKBaseComponent.h>
#import <BaiduMapAPI_Map/BMKMapComponent.h>
#include <map>

using namespace huxerui::map;
using namespace huxerui::map::detail;

@interface HUIBaiduMarker : BMKPointAnnotation
@property(nonatomic, copy) NSString* markerId;
@end
@implementation HUIBaiduMarker
@end

@interface HUIBaiduDelegate : NSObject <BMKMapViewDelegate> {
@public
  std::shared_ptr<EventSink> sink;
  std::map<const void*, Polyline> lines;
}
@end
@implementation HUIBaiduDelegate
- (void)mapViewDidFinishLoading:(BMKMapView*)map { HUXERUI_MAP_UNUSED(map); sink->Emit("loaded"); }
- (void)mapView:(BMKMapView*)map onClickedMapBlank:(CLLocationCoordinate2D)coordinate {
  HUXERUI_MAP_UNUSED(map); sink->CoordinateEvent("clicked", coordinate);
}
- (void)mapview:(BMKMapView*)map onLongClick:(CLLocationCoordinate2D)coordinate {
  HUXERUI_MAP_UNUSED(map); sink->CoordinateEvent("longPressed", coordinate);
}
- (void)mapView:(BMKMapView*)map regionDidChangeAnimated:(BOOL)animated {
  HUXERUI_MAP_UNUSED(animated); sink->CameraIdle(map.centerCoordinate, map.zoomLevel);
}
- (BMKAnnotationView*)mapView:(BMKMapView*)map viewForAnnotation:(id<BMKAnnotation>)annotation {
  if (![annotation isKindOfClass:HUIBaiduMarker.class]) return nil;
  BMKPinAnnotationView* view = (BMKPinAnnotationView*)[map dequeueReusableAnnotationViewWithIdentifier:@"hui-marker"];
  if (!view) view = [[BMKPinAnnotationView alloc] initWithAnnotation:annotation reuseIdentifier:@"hui-marker"];
  view.annotation = annotation; view.canShowCallout = YES;
  return view;
}
- (void)mapView:(BMKMapView*)map didSelectAnnotationView:(BMKAnnotationView*)view {
  HUXERUI_MAP_UNUSED(map);
  if ([view.annotation isKindOfClass:HUIBaiduMarker.class])
    sink->Emit("markerClicked", ((HUIBaiduMarker*)view.annotation).markerId.UTF8String ?: "");
}
- (BMKOverlayView*)mapView:(BMKMapView*)map viewForOverlay:(id<BMKOverlay>)overlay {
  HUXERUI_MAP_UNUSED(map);
  const auto found = lines.find((__bridge const void*)overlay);
  if (found == lines.end() || ![overlay isKindOfClass:BMKPolyline.class]) return nil;
  BMKPolylineView* view = [[BMKPolylineView alloc] initWithPolyline:(BMKPolyline*)overlay];
  view.lineWidth = found->second.width; view.strokeColor = Color(found->second.color_argb);
  return view;
}
@end

namespace huxerui::map::detail {
namespace {
// BMKMapManager is process-wide; retain it while any map can be mounted.
BMKMapManager* manager;
class BaiduInstance final : public PlatformMap {
public:
  BaiduInstance(const MapOptions& options, std::shared_ptr<EventSink> sink) : sink_(std::move(sink)) {
    NSString* key = [NSBundle.mainBundle objectForInfoDictionaryKey:@"HuxerUIBaiduMapAPIKey"];
    if (![key isKindOfClass:NSString.class] || key.length == 0 || [key hasPrefix:@"YOUR_"])
      throw std::invalid_argument("Set HuxerUIBaiduMapAPIKey in the app Info.plist");
    if (!manager) {
      [BMKMapManager setAgreePrivacy:YES];
      [BMKMapManager setCoordinateTypeUsedInBaiduMapSDK:BMK_COORDTYPE_BD09LL];
      BMKMapManager* candidate = [BMKMapManager new];
      if (![candidate start:key generalDelegate:nil]) throw std::runtime_error("BMKMapManager start failed");
      manager = candidate;
    }
    map_ = [[BMKMapView alloc] initWithFrame:CGRectZero];
    delegate_ = [HUIBaiduDelegate new]; delegate_->sink = sink_; map_.delegate = delegate_;
    [map_ viewWillAppear];
    __weak BMKMapView* weakMap = map_;
    background_ = [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidEnterBackgroundNotification
        object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification*) { [weakMap viewWillDisappear]; }];
    foreground_ = [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationWillEnterForegroundNotification
        object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification*) { [weakMap viewWillAppear]; }];
    Update(options);
  }
  ~BaiduInstance() override { Dispose(); }
  UIView* View() override { return map_; }
  void Update(const MapOptions& options) override {
    if (!options.privacy.Granted()) { Dispose(); return; }
    const auto& old = previous_;
    if (!old || old->type != options.type) map_.mapType = options.type == MapType::Satellite ? BMKMapTypeSatellite : BMKMapTypeStandard;
    if (!old || old->traffic_enabled != options.traffic_enabled) map_.trafficEnabled = options.traffic_enabled;
    if (!old || old->scale_control != options.scale_control) map_.showMapScaleBar = options.scale_control;
    if (!old || old->gestures_enabled != options.gestures_enabled) {
      map_.scrollEnabled = options.gestures_enabled; map_.zoomEnabled = options.gestures_enabled;
      map_.zoomEnabledWithTap = options.gestures_enabled;
      map_.rotateEnabled = options.gestures_enabled; map_.overlookEnabled = options.gestures_enabled;
    }
    if (!old || old->camera != options.camera) {
      [map_ setCenterCoordinate:Coordinate2D(options.camera.center) animated:NO];
      map_.zoomLevel = options.camera.zoom;
    }
    if (!old || old->markers != options.markers) {
      [map_ removeAnnotations:markers_]; markers_ = [NSMutableArray new];
      for (const auto& marker : options.markers) {
        HUIBaiduMarker* item = [HUIBaiduMarker new];
        item.coordinate = Coordinate2D(marker.position); item.title = String(marker.title);
        item.subtitle = String(marker.subtitle); item.markerId = String(marker.id);
        [markers_ addObject:item];
      }
      [map_ addAnnotations:markers_];
    }
    if (!old || old->polylines != options.polylines) {
      [map_ removeOverlays:overlays_]; delegate_->lines.clear(); overlays_ = [NSMutableArray new];
      for (const auto& line : options.polylines) {
        std::vector<CLLocationCoordinate2D> points;
        for (auto point : line.points) points.push_back(Coordinate2D(point));
        BMKPolyline* item = [BMKPolyline polylineWithCoordinates:points.data() count:points.size()];
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
    if (background_) [NSNotificationCenter.defaultCenter removeObserver:background_];
    if (foreground_) [NSNotificationCenter.defaultCenter removeObserver:foreground_];
    background_ = nil; foreground_ = nil;
    [map_ viewWillDisappear]; map_.delegate = nil;
    [map_ removeAnnotations:markers_]; [map_ removeOverlays:overlays_]; [map_ removeFromSuperview];
    map_ = nil; delegate_ = nil; markers_ = nil; overlays_ = nil;
  }
private:
  std::shared_ptr<EventSink> sink_;
  BMKMapView* map_;
  HUIBaiduDelegate* delegate_;
  NSMutableArray* markers_;
  NSMutableArray* overlays_;
  id background_, foreground_;
  std::optional<MapOptions> previous_;
};
} // namespace
std::shared_ptr<PlatformMap> CreateBaiduMap(const MapOptions& options, std::shared_ptr<EventSink> sink) {
  return std::make_shared<BaiduInstance>(options, std::move(sink));
}
} // namespace huxerui::map::detail
