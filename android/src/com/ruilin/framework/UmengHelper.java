package com.ruilin.framework;

import android.location.Location;

import com.amap.api.location.AMapLocation;
import com.mapswithme.maps.MwmApplication;
import com.umeng.analytics.MobclickAgent;

import java.util.HashMap;
import java.util.Map;

/**
 * Created by Ruilin on 2017/7/28.
 */

public class UmengHelper {
    private static UmengHelper mInstance = new UmengHelper();
    private boolean mHasPostedLoc;

    private UmengHelper(){
        mHasPostedLoc = false;
    }

    public static UmengHelper get() {
        return mInstance;
    }

    public void postDownloadEvent() {
        MobclickAgent.onEvent(MwmApplication.get(), "Download");
    }

    public void postCancelDownloadEvent() {
        MobclickAgent.onEvent(MwmApplication.get(), "CancelDownload");
    }

    public void postLocation(AMapLocation location) {
        if (mHasPostedLoc)
            return;
        mHasPostedLoc = true;
        if (location != null) {
            HashMap<String, String> loc = new HashMap();
            loc.put("Country", location.getCountry());
            loc.put("City", location.getCity());
            loc.put("District", location.getDistrict());
            loc.put("AoiName", location.getAoiName());
            MobclickAgent.onEvent(MwmApplication.get(), "Location", loc);
        }
    }
}
