package com.ruilin.framework;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;

import com.mapswithme.maps.location.LocationHelper;

/**
 * Created by Ruilin on 2017/8/11.
 */

public class NetworkChangeReceiver extends BroadcastReceiver {
    private static boolean sIsNetworkEnable = true;

    public static void init(boolean isNetworkEnable) {
        sIsNetworkEnable = isNetworkEnable;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        /** 监听网络变化，改变定位 */
        ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        //NetworkInfo mobileInfo = manager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
        //NetworkInfo wifiInfo = manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        boolean active = manager.getActiveNetworkInfo() != null;
        if (sIsNetworkEnable != active) {
            //如果无网络连接activeInfo为null
            sIsNetworkEnable = active;
            LocationHelper.INSTANCE.restart();
        }
    }
}
