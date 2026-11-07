package org.huxerui.lib.map;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import com.baidu.mapapi.CoordType;
import com.baidu.mapapi.SDKInitializer;
import com.baidu.mapapi.map.BaiduMap;
import com.baidu.mapapi.map.BitmapDescriptor;
import com.baidu.mapapi.map.BitmapDescriptorFactory;
import com.baidu.mapapi.map.MapStatus;
import com.baidu.mapapi.map.MapStatusUpdateFactory;
import com.baidu.mapapi.map.Marker;
import com.baidu.mapapi.map.MarkerOptions;
import com.baidu.mapapi.map.Overlay;
import com.baidu.mapapi.map.PolylineOptions;
import com.baidu.mapapi.map.TextureMapView;
import com.baidu.mapapi.model.LatLng;
import org.huxerui.HuxerUIPlatformChannel;
import org.huxerui.HuxerUIPlatformView;
import org.huxerui.PlatformPayload;
import java.util.ArrayList;
import java.util.List;

public final class BaiduMapFactory implements HuxerUIPlatformView.Factory {
    public BaiduMapFactory() {}
    @Override public HuxerUIPlatformView create(Context context, PlatformPayload properties, HuxerUIPlatformChannel.Events events) {
        Instance instance = new Instance(context, events);
        instance.initialize(properties);
        return instance;
    }
    private static final class Instance extends MapHost {
        private TextureMapView view;
        private BaiduMap map;
        private BitmapDescriptor markerIcon;
        private final List<Overlay> markers = new ArrayList<>(), lines = new ArrayList<>();
        Instance(Context context, HuxerUIPlatformChannel.Events events) { super(context, events); }
        @Override void createMap() throws Exception {
            requireKey("com.baidu.lbsapi.API_KEY");
            Context application = context.getApplicationContext();
            SDKInitializer.setAgreePrivacy(application, true);
            SDKInitializer.initialize(application);
            SDKInitializer.setCoordType(CoordType.BD09LL);
            view = new TextureMapView(context);
            map = view.getMap();
            addMap(view);
            markerIcon = createMarkerIcon();
            map.setOnMapLoadedCallback(() -> emit("loaded", PlatformPayload.nullValue()));
            map.setOnMapClickListener(new BaiduMap.OnMapClickListener() {
                @Override public void onMapClick(LatLng point) { emit("clicked", coordinate(point.latitude, point.longitude)); }
                @Override public void onMapPoiClick(com.baidu.mapapi.map.MapPoi poi) {}
            });
            map.setOnMapLongClickListener(point -> emit("longPressed", coordinate(point.latitude, point.longitude)));
            map.setOnMarkerClickListener(marker -> {
                emit("markerClicked", PlatformPayload.string(marker.getExtraInfo().getString("huxerui.id")));
                return false;
            });
            map.setOnMapStatusChangeListener(new BaiduMap.OnMapStatusChangeListener() {
                @Override public void onMapStatusChangeStart(MapStatus status) {}
                @Override public void onMapStatusChangeStart(MapStatus status, int reason) {}
                @Override public void onMapStatusChange(MapStatus status) {}
                @Override public void onMapStatusChangeFinish(MapStatus status) {
                    cameraIdle(status.target.latitude, status.target.longitude, status.zoom);
                }
            });
        }
        private BitmapDescriptor createMarkerIcon() {
            int size = Math.max(24, Math.round(32 * context.getResources().getDisplayMetrics().density));
            Bitmap bitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setColor(0xFF2563EB); canvas.drawCircle(size / 2f, size / 2f, size * .45f, paint);
            paint.setColor(Color.WHITE); canvas.drawCircle(size / 2f, size / 2f, size * .17f, paint);
            BitmapDescriptor descriptor = BitmapDescriptorFactory.fromBitmap(bitmap);
            bitmap.recycle();
            return descriptor;
        }
        private static LatLng point(PlatformPayload value) { return new LatLng(number(value, "latitude"), number(value, "longitude")); }
        @Override void apply(PlatformPayload value, PlatformPayload previous) {
            if (changed(value, previous, "satellite")) map.setMapType(flag(value, "satellite") ? BaiduMap.MAP_TYPE_SATELLITE : BaiduMap.MAP_TYPE_NORMAL);
            if (changed(value, previous, "traffic")) map.setTrafficEnabled(flag(value, "traffic"));
            if (changed(value, previous, "gestures")) map.getUiSettings().setAllGesturesEnabled(flag(value, "gestures"));
            if (changed(value, previous, "zoomControls")) view.showZoomControls(flag(value, "zoomControls"));
            if (changed(value, previous, "scaleControl")) view.showScaleControl(flag(value, "scaleControl"));
            if (changed(value, previous, "camera")) {
                PlatformPayload camera = value.requireField("camera");
                map.setMapStatus(MapStatusUpdateFactory.newLatLngZoom(point(camera.requireField("center")), (float) number(camera, "zoom")));
            }
            if (changed(value, previous, "markers")) {
                for (Overlay marker : markers) marker.remove();
                markers.clear();
                for (PlatformPayload marker : value.requireField("markers").elements()) {
                    Bundle metadata = new Bundle(); metadata.putString("huxerui.id", marker.requireField("id").requireString());
                    metadata.putString("huxerui.subtitle", marker.requireField("subtitle").requireString());
                    Marker mapMarker = (Marker) map.addOverlay(new MarkerOptions().position(point(marker.requireField("position")))
                            .icon(markerIcon).title(marker.requireField("title").requireString()).extraInfo(metadata));
                    markers.add(mapMarker);
                }
            }
            if (changed(value, previous, "polylines")) {
                for (Overlay line : lines) line.remove();
                lines.clear();
                for (PlatformPayload line : value.requireField("polylines").elements()) {
                    List<LatLng> points = new ArrayList<>();
                    for (PlatformPayload point : line.requireField("points").elements()) points.add(point(point));
                    lines.add(map.addOverlay(new PolylineOptions().points(points)
                            .width(Math.max(1, Math.round((float) number(line, "width") * context.getResources().getDisplayMetrics().density)))
                            .color((int) line.requireField("color").requireInt64())));
                }
            }
        }
        @Override void resumeMap() { if (view != null) view.onResume(); }
        @Override void pauseMap() { if (view != null) view.onPause(); }
        @Override void destroyMap() {
            if (map != null) {
                map.setOnMapLoadedCallback(null); map.setOnMapClickListener(null); map.setOnMapLongClickListener(null);
                map.setOnMarkerClickListener(null); map.setOnMapStatusChangeListener(null);
                map.clear();
            }
            markers.clear(); lines.clear();
            if (view != null) { view.onDestroy(); view = null; }
            if (markerIcon != null) { markerIcon.recycle(); markerIcon = null; }
            map = null;
        }
    }
}
