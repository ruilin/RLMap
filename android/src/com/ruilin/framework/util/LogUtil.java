package com.ruilin.framework.util;

import android.util.Log;

import com.mapswithme.maps.BuildConfig;

/**
 * Created by Ruilin on 2017/8/8.
 */

public class LogUtil {
    private static String TAG = "RLMap";

    public static void e(String msg) {
        if (BuildConfig.DEBUG)
            Log.e(TAG, msg);
    }

    public static void i(String msg) {
        if (BuildConfig.DEBUG)
            Log.i(TAG, msg);
    }
}
