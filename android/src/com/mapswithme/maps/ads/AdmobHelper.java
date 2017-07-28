package com.mapswithme.maps.ads;

import android.content.Context;
import android.util.Log;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.InterstitialAd;
import com.google.android.gms.ads.MobileAds;
import com.mapswithme.maps.SplashActivity;

/**
 * Created by Ruilin on 2017/7/26.
 */

public class AdmobHelper {

    static AdmobHelper mInstance = new AdmobHelper();

    private InterstitialAd mInterstitialAd;


    private AdmobHelper() {}

    public static AdmobHelper get() {
        return mInstance;
    }

    public void init(Context context) {
        MobileAds.initialize(context, "ca-app-pub-7146181069155443~6332394081");
        initIntersTitialAd(context, SplashActivity.isFirstStart());
    }

    public void loadBannerAd(final AdView view) {
        AdRequest adRequest = new AdRequest.Builder().build();
        view.setAdListener(new AdListener() {
            @Override
            public void onAdLoaded() {
                // Code to be executed when an ad finishes loading.
                Log.i("Ads", "xxx Banner onAdLoaded; isShow==" + view.isShown());
            }

            @Override
            public void onAdFailedToLoad(int errorCode) {
                // Code to be executed when an ad request fails.
                Log.i("Ads", "xxx Banner onAdFailedToLoad: " + errorCode);
            }

            @Override
            public void onAdOpened() {
                // Code to be executed when an ad opens an overlay that
                // covers the screen.
                Log.i("Ads", "xxx Banner onAdOpened");
            }

            @Override
            public void onAdLeftApplication() {
                // Code to be executed when the user has left the app.
                Log.i("Ads", "xxx Banner onAdLeftApplication");
            }

            @Override
            public void onAdClosed() {
                // Code to be executed when when the user is about to return
                // to the app after tapping on an ad.
                Log.i("Ads", "xxx Banner onAdClosed");
            }
        });
        view.loadAd(adRequest);
    }


    public void loadWelcomeAd(AdView view) {
        AdRequest adRequest = new AdRequest.Builder().build();
        view.setAdListener(new AdListener() {
            @Override
            public void onAdLoaded() {
                // Code to be executed when an ad finishes loading.
                Log.i("Ads", "xxx WelcomeAd onAdLoaded");
            }

            @Override
            public void onAdFailedToLoad(int errorCode) {
                // Code to be executed when an ad request fails.
                Log.i("Ads", "xxx WelcomeAd onAdFailedToLoad: " + errorCode);
            }

            @Override
            public void onAdOpened() {
                // Code to be executed when an ad opens an overlay that
                // covers the screen.
                Log.i("Ads", "xxx WelcomeAd onAdOpened");
            }

            @Override
            public void onAdLeftApplication() {
                // Code to be executed when the user has left the app.
                Log.i("Ads", "xxx WelcomeAd onAdLeftApplication");
            }

            @Override
            public void onAdClosed() {
                // Code to be executed when when the user is about to return
                // to the app after tapping on an ad.
                Log.i("Ads", "xxx WelcomeAd onAdClosed");
            }
        });
        view.loadAd(adRequest);
    }

    public void initIntersTitialAd(Context context, boolean isFirstOpen) {
        mInterstitialAd = new InterstitialAd(context);
        mInterstitialAd.setAdUnitId("ca-app-pub-7146181069155443/3049579029");
        if (!isFirstOpen) {
            mInterstitialAd.loadAd(new AdRequest.Builder().build());
        }
    }

    public void showIntersTitialAd() {
        if (mInterstitialAd.isLoaded()) {
            mInterstitialAd.show();
        }
    }
}
