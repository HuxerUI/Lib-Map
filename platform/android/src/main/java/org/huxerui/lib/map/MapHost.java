package org.huxerui.lib.map;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;
import org.huxerui.HuxerUIPlatformChannel;
import org.huxerui.HuxerUIPlatformView;
import org.huxerui.PlatformPayload;
import java.util.HashMap;
import java.util.Map;

/** Shared lifetime of one actual SDK TextureMapView, owned by HuxerUI's PlatformView host. */
abstract class MapHost implements HuxerUIPlatformView, Application.ActivityLifecycleCallbacks {
    final Context context;
    final FrameLayout root;
    private final Activity activity;
    private final HuxerUIPlatformChannel.Events events;
    private PlatformPayload previous;
    private boolean disposed, failed, resumed, registered;

    MapHost(Context context, HuxerUIPlatformChannel.Events events) {
        this.context = context;
        this.events = events;
        activity = activityOf(context);
        root = new FrameLayout(context);
        root.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
            @Override public void onViewAttachedToWindow(View view) { resume(); }
            @Override public void onViewDetachedFromWindow(View view) { pause(); }
        });
    }

    final void initialize(PlatformPayload properties) {
        try {
            requireConsent(properties);
            if (activity == null) throw new IllegalStateException("MapView requires an Activity-backed Context");
            createMap();
            update(properties);
            if (failed) return;
            activity.getApplication().registerActivityLifecycleCallbacks(this);
            registered = true;
        } catch (Exception | LinkageError error) { fail("initialization-failed", error); }
    }

    abstract void createMap() throws Exception;
    abstract void apply(PlatformPayload value, PlatformPayload previous);
    abstract void resumeMap();
    abstract void pauseMap();
    abstract void destroyMap();

    final void addMap(View view) {
        root.addView(view, new FrameLayout.LayoutParams(-1, -1));
    }

    final void requireKey(String metadata) throws PackageManager.NameNotFoundException {
        Bundle values = context.getPackageManager().getApplicationInfo(
                context.getPackageName(), PackageManager.GET_META_DATA).metaData;
        String key = values == null ? null : values.getString(metadata);
        if (key == null || key.trim().isEmpty() || key.startsWith("YOUR_"))
            throw new IllegalStateException("Configure AndroidManifest meta-data: " + metadata);
    }

    @Override public final View getView() { return root; }

    @Override public final void update(PlatformPayload value) {
        if (disposed || failed) return;
        try {
            requireConsent(value);
            apply(value, previous);
            previous = value;
        } catch (Exception | LinkageError error) { fail("update-failed", error); }
    }

    static boolean changed(PlatformPayload value, PlatformPayload previous, String field) {
        return previous == null || !value.requireField(field).equals(previous.requireField(field));
    }
    static double number(PlatformPayload value, String field) { return value.requireField(field).requireDouble(); }
    static boolean flag(PlatformPayload value, String field) { return value.requireField(field).requireBoolean(); }
    static PlatformPayload object(Object... values) {
        Map<String, PlatformPayload> fields = new HashMap<>();
        for (int i = 0; i < values.length; i += 2) fields.put((String) values[i], (PlatformPayload) values[i + 1]);
        return PlatformPayload.object(fields);
    }
    static PlatformPayload coordinate(double latitude, double longitude) {
        return object("latitude", PlatformPayload.doubleValue(latitude), "longitude", PlatformPayload.doubleValue(longitude));
    }
    final void emit(String name, PlatformPayload value) {
        // SDK initialization may call back before HuxerUI has attached event bindings.
        root.post(() -> { if (!disposed && !failed) events.emit(name, value); });
    }
    final void cameraIdle(double latitude, double longitude, double zoom) {
        emit("cameraIdle", object("center", coordinate(latitude, longitude), "zoom", PlatformPayload.doubleValue(zoom)));
    }
    private static void requireConsent(PlatformPayload value) {
        if (!flag(value, "privacy")) throw new IllegalStateException("Obtain provider privacy consent before mounting maps");
    }
    private void fail(String code, Throwable error) {
        if (failed || disposed) return;
        failed = true;
        pause();
        try { destroyMap(); } catch (Exception | LinkageError ignored) { /* Release partial initialization. */ }
        root.removeAllViews();
        TextView message = new TextView(context);
        String text = error.getMessage() == null ? error.getClass().getSimpleName() : error.getMessage();
        message.setText("HuxerUI Map: " + text);
        root.addView(message);
        root.post(() -> { if (!disposed) events.emit("error", object(
                "code", PlatformPayload.string(code), "message", PlatformPayload.string(text))); });
    }
    private void resume() {
        if (!disposed && !failed && !resumed) { resumed = true; resumeMap(); }
    }
    private void pause() {
        if (resumed) { resumed = false; pauseMap(); }
    }
    @Override public final void dispose() {
        if (disposed) return;
        disposed = true;
        if (registered) activity.getApplication().unregisterActivityLifecycleCallbacks(this);
        pause();
        destroyMap();
        root.removeAllViews();
        previous = null;
    }
    private static Activity activityOf(Context context) {
        while (context instanceof ContextWrapper) {
            if (context instanceof Activity) return (Activity) context;
            Context next = ((ContextWrapper) context).getBaseContext();
            if (next == context) break;
            context = next;
        }
        return null;
    }
    @Override public final void onActivityResumed(Activity value) { if (value == activity && root.isAttachedToWindow()) resume(); }
    @Override public final void onActivityPaused(Activity value) { if (value == activity) pause(); }
    @Override public final void onActivityDestroyed(Activity value) { if (value == activity) dispose(); }
    @Override public final void onActivityCreated(Activity value, Bundle state) {}
    @Override public final void onActivityStarted(Activity value) {}
    @Override public final void onActivityStopped(Activity value) {}
    @Override public final void onActivitySaveInstanceState(Activity value, Bundle state) {}
}
