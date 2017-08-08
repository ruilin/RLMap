package com.ruilin.framework.util;

import android.widget.Toast;

import com.mapswithme.maps.MwmApplication;

/**
 * Created by Ruilin on 2017/7/28.
 */

public class ToastUtil {

    public static void show(String text) {
        Toast.makeText(MwmApplication.get(), text, Toast.LENGTH_SHORT).show();
    }
}
