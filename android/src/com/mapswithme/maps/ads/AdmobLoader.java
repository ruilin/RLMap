package com.mapswithme.maps.ads;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.google.android.gms.ads.MobileAds;

/**
 * Created by Ruilin on 2017/7/26.
 */

public class AdmobLoader extends CachingNativeAdLoader {

    public AdmobLoader(@Nullable OnAdCacheModifiedListener cacheListener,
                       @Nullable AdTracker tracker) {
        super(tracker, cacheListener);
    }

    @Override
    void loadAdFromProvider(@NonNull Context context, @NonNull String bannerId) {

    }

    @NonNull
    @Override
    String getProvider() {
        return null;
    }
}
