package com.ruilin.framework;

import com.mapswithme.maps.MwmApplication;
import com.umeng.analytics.MobclickAgent;

/**
 * Created by Ruilin on 2017/7/28.
 */

public class UmengHelper {

    private static UmengHelper mInstance = new UmengHelper();

    private UmengHelper(){}

    public static UmengHelper get() {
        return mInstance;
    }

    public void postDownloadEvent() {
        MobclickAgent.onEvent(MwmApplication.get(), "Download");
    }

    public void postCancelDownloadEvent() {
        MobclickAgent.onEvent(MwmApplication.get(), "CancelDownload");
    }
}
