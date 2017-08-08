package com.mapswithme.maps.location;

import android.content.Context;
import android.location.Location;
import android.support.annotation.NonNull;
import android.util.Log;

import com.amap.api.location.AMapLocation;
import com.amap.api.location.AMapLocationClient;
import com.amap.api.location.AMapLocationClientOption;
import com.amap.api.location.AMapLocationListener;
import com.amap.api.location.CoordinateConverter;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.concurrency.UiThread;
import com.ruilin.framework.util.LogUtil;
import com.ruilin.framework.util.ToastUtil;

/**
 * 高德定位模块
 * Created by Ruilin on 2017/8/6.
 */

public class AMapLocationProvider extends BaseLocationProvider {

    //声明AMapLocationClient类对象
    public AMapLocationClient mLocationClient = null;
    //声明AMapLocationClientOption对象
    public AMapLocationClientOption mLocationOption = null;

    public AMapLocationProvider(@NonNull LocationFixChecker locationFixChecker) {
        super(locationFixChecker);
        final Context context = MwmApplication.get();
        //初始化定位
        mLocationClient = new AMapLocationClient(context);
        //设置定位回调监听
        mLocationClient.setLocationListener(new AMapLocationListener() {
            @Override
            public void onLocationChanged(AMapLocation aMapLocation) {
                //定位回调监听器
                if (aMapLocation.getErrorCode() != 0) {
                    checkErrorCode(aMapLocation.getErrorCode());
                    return;
                }
                if (Double.compare(aMapLocation.getLatitude(), 0) == 0 && Double.compare(aMapLocation.getLongitude(), 0) == 0) {
                    return;
                }
                Location location = new Location("AMap");
                location.setAccuracy(aMapLocation.getAccuracy());
                location.setSpeed(aMapLocation.getSpeed());
                location.setAltitude(aMapLocation.getAltitude());
                location.setTime(aMapLocation.getTime());
                location.setBearing(aMapLocation.getBearing());
                location.setExtras(aMapLocation.getExtras());

                if (CoordinateConverter.isAMapDataAvailable(aMapLocation.getLatitude(), aMapLocation.getLongitude())) {
                    // 中国大陆、港澳台地区，需要转换坐标系
                    PositionUtil.Gps point = PositionUtil.gcj_To_Gps84(aMapLocation.getLatitude(), aMapLocation.getLongitude());
                    location.setLatitude(point.getWgLat());
                    location.setLongitude(point.getWgLon());
                } else {
                    location.setLatitude(aMapLocation.getLatitude());
                    location.setLongitude(aMapLocation.getLongitude());
                }

                LocationHelper.INSTANCE.resetMagneticField(location);
                LocationHelper.INSTANCE.onLocationUpdated(location);
            }
        });


        //初始化AMapLocationClientOption对象
        mLocationOption = new AMapLocationClientOption();
        //设置定位模式为AMapLocationMode.Hight_Accuracy，高精度模式。
        mLocationOption.setLocationMode(AMapLocationClientOption.AMapLocationMode.Hight_Accuracy);
        //设置定位间隔,单位毫秒,默认为2000ms，最低1000ms。
        mLocationOption.setInterval(1000);
        //单位是毫秒，默认30000毫秒，建议超时时间不要低于8000毫秒。
        mLocationOption.setHttpTimeOut(20000);
        //开启缓存机制
        mLocationOption.setLocationCacheEnable(true);

        //给定位客户端对象设置定位参数
        mLocationClient.setLocationOption(mLocationOption);
    }

    private void checkErrorCode(final int errCode) {
        LogUtil.e("Location ErrorCode == " + errCode);
        if (BuildConfig.DEBUG) {
            UiThread.run(new Runnable() {
                @Override
                public void run() {
                    ToastUtil.show("Location ErrorCode == " + errCode);
                }
            });
        }
        switch (errCode) {
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                break;
            default:
                break;
        }
    }

    @Override
    protected void start() {
        if (mLocationClient.isStarted()) {
            setActive(true);
            return;
        }
        //启动定位
        mLocationClient.startLocation();
        setActive(true);
    }

    @Override
    protected void stop() {
        mLocationClient.stopLocation();
        setActive(false);
    }
}
