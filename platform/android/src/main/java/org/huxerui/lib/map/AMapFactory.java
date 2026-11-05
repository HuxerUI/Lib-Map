package org.huxerui.lib.map;

import android.content.Context;
import com.amap.api.maps.AMap;
import com.amap.api.maps.CameraUpdateFactory;
import com.amap.api.maps.MapsInitializer;
import com.amap.api.maps.TextureMapView;
import com.amap.api.maps.model.CameraPosition;
import com.amap.api.maps.model.LatLng;
import com.amap.api.maps.model.Marker;
import com.amap.api.maps.model.MarkerOptions;
import com.amap.api.maps.model.Polyline;
import com.amap.api.maps.model.PolylineOptions;
import org.huxerui.HuxerUIPlatformChannel;
import org.huxerui.HuxerUIPlatformView;
import org.huxerui.PlatformPayload;
import java.util.ArrayList;
import java.util.List;

public final class AMapFactory implements HuxerUIPlatformView.Factory {
    public AMapFactory() {}
    @Override public HuxerUIPlatformView create(Context context, PlatformPayload properties, HuxerUIPlatformChannel.Events events) {
        Instance instance = new Instance(context, events);
        instance.initialize(properties);
        return instance;
    }
    private static final class Instance extends MapHost {
        private TextureMapView view;
        private AMap map;
        private final List<Marker> markers = new ArrayList<>();
        private final List<Polyline> lines = new ArrayList<>();
        Instance(Context context, HuxerUIPlatformChannel.Events events) { super(context, events); }
        @Override void createMap() throws Exception {
            requireKey("com.amap.api.v2.apikey");
            MapsInitializer.updatePrivacyShow(context, true, true);
            MapsInitializer.updatePrivacyAgree(context, true);
            view = new TextureMapView(context);
            view.onCreate(null);
            map = view.getMap();
            addMap(view);
            map.setOnMapLoadedListener(() -> emit("loaded", PlatformPayload.nullValue()));
            map.setOnMapClickListener(point -> emit("clicked", coordinate(point.latitude, point.longitude)));
            map.setOnMapLongClickListener(point -> emit("longPressed", coordinate(point.latitude, point.longitude)));
            map.setOnMarkerClickListener(marker -> {
                emit("markerClicked", PlatformPayload.string((String) marker.getObject()));
                return false; // Retain the SDK's normal info-window interaction.
            });
            map.setOnCameraChangeListener(new AMap.OnCameraChangeListener() {
                @Override public void onCameraChange(CameraPosition camera) {}
                @Override public void onCameraChangeFinish(CameraPosition camera) {
                    cameraIdle(camera.target.latitude, camera.target.longitude, camera.zoom);
                }
            });
        }
        private static LatLng point(PlatformPayload value) { return new LatLng(number(value, "latitude"), number(value, "longitude")); }
        @Override void apply(PlatformPayload value, PlatformPayload previous) {
            if (changed(value, previous, "satellite")) map.setMapType(flag(value, "satellite") ? AMap.MAP_TYPE_SATELLITE : AMap.MAP_TYPE_NORMAL);
            if (changed(value, previous, "traffic")) map.setTrafficEnabled(flag(value, "traffic"));
            if (changed(value, previous, "gestures")) map.getUiSettings().setAllGesturesEnabled(flag(value, "gestures"));
            if (changed(value, previous, "zoomControls")) map.getUiSettings().setZoomControlsEnabled(flag(value, "zoomControls"));
            if (changed(value, previous, "scaleControl")) map.getUiSettings().setScaleControlsEnabled(flag(value, "scaleControl"));
            if (changed(value, previous, "camera")) {
                PlatformPayload camera = value.requireField("camera");
                map.moveCamera(CameraUpdateFactory.newLatLngZoom(point(camera.requireField("center")), (float) number(camera, "zoom")));
            }
            if (changed(value, previous, "markers")) {
                for (Marker marker : markers) marker.remove();
                markers.clear();
                for (PlatformPayload marker : value.requireField("markers").elements()) {
                    Marker nativeMarker = map.addMarker(new MarkerOptions().position(point(marker.requireField("position")))
                            .title(marker.requireField("title").requireString()).snippet(marker.requireField("subtitle").requireString()));
                    nativeMarker.setObject(marker.requireField("id").requireString());
                    markers.add(nativeMarker);
                }
            }
            if (changed(value, previous, "polylines")) {
                for (Polyline line : lines) line.remove();
                lines.clear();
                for (PlatformPayload line : value.requireField("polylines").elements()) {
                    PolylineOptions options = new PolylineOptions().width((float) number(line, "width") * context.getResources().getDisplayMetrics().density)
                            .color((int) line.requireField("color").requireInt64());
                    for (PlatformPayload point : line.requireField("points").elements()) options.add(point(point));
                    lines.add(map.addPolyline(options));
                }
            }
        }
        @Override void resumeMap() { if (view != null) view.onResume(); }
        @Override void pauseMap() { if (view != null) view.onPause(); }
        @Override void destroyMap() {
            if (map != null) {
                map.setOnMapLoadedListener(null); map.setOnMapClickListener(null); map.setOnMapLongClickListener(null);
                map.setOnMarkerClickListener(null); map.setOnCameraChangeListener(null);
            }
            markers.clear(); lines.clear();
            if (view != null) { view.onDestroy(); view = null; }
            map = null;
        }
    }
}
