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
import com.ruilin.framework.UmengHelper;
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
                if (aMapLocation == null
                        || (Double.compare(aMapLocation.getLatitude(), 0) == 0
                            && Double.compare(aMapLocation.getLongitude(), 0) == 0)) {
                    return;
                }
                Location location = new Location("AMap");
                location.setAccuracy(aMapLocation.getAccuracy());
                location.setSpeed(aMapLocation.getSpeed());
                location.setAltitude(aMapLocation.getAltitude());
                location.setTime(aMapLocation.getTime());
                location.setBearing(aMapLocation.getBearing());
                location.setExtras(aMapLocation.getExtras());

                UmengHelper.get().postLocation(aMapLocation);

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
                LocationHelper.INSTANCE.notifyLocationUpdated();
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
        //Gps优先
        mLocationOption.setGpsFirst(true);

        //给定位客户端对象设置定位参数
        mLocationClient.setLocationOption(mLocationOption);
    }

    private void checkErrorCode(final int errCode) {
        LogUtil.e("Location ErrorCode == " + errCode);
        switch (errCode) {
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            default:
                break;
        }
    }

    @Override
    protected void start(int interval) {
        if (mLocationClient.isStarted() && mLocationOption.getInterval() == interval) {
            setActive(true);
            return;
        }
        mLocationOption.setInterval(interval);
        mLocationClient.setLocationOption(mLocationOption);
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

/**
 0 定位成功。
 1 一些重要参数为空，如context；
 2 定位失败，由于仅扫描到单个wifi，且没有基站信息。
 3 获取到的请求参数为空，可能获取过程中出现异常。
 4 请求服务器过程中的异常，多为网络情况差，链路不通导致
 5 请求被恶意劫持，定位结果解析失败。
 6 定位服务返回定位失败。
 7 KEY鉴权失败。
 8 Android exception常规错误
 9 定位初始化时出现异常。
 10 定位客户端启动失败。
 11 定位时的基站信息错误。
 12 缺少定位权限。
 13 定位失败，由于未获得WIFI列表和基站信息，且GPS当前不可用。
 14 GPS 定位失败，由于设备当前 GPS 状态差。
 15 定位结果被模拟导致定位失败
 16 当前POI检索条件、行政区划检索条件下，无可用地理围栏
 18 定位失败，由于手机WIFI功能被关闭同时设置为飞行模式
 19 定位失败，由于手机没插sim卡且WIFI功能被关闭
 */
