package com.ruilin.framework.navigation;

import android.content.Context;
import android.speech.SpeechRecognizer;

import com.mapswithme.maps.MwmApplication;
import com.unisound.client.SpeechConstants;
import com.unisound.client.SpeechSynthesizer;
import com.unisound.client.SpeechSynthesizerListener;

import java.io.File;

/**
 * Created by Ruilin on 2017/8/30.
 */

public class TTSPlayer implements SpeechSynthesizerListener {
    private static TTSPlayer mInstance;
    private SpeechSynthesizer mTTSPlayer;
    private static final String APP_KEY = "lxfp2whzllsrkwr6mclhdgbw5ekakf5xi5cpa5it";
    private static final String SECRET = "33413d35e244398d845f709485b64585";
    private final String mFrontendModel= "/sdcard/unisound/tts/frontend_model";
    private final String mBackendModel = "/sdcard/unisound/tts/backend_lzl";

    public static TTSPlayer get() {
        if (mInstance == null) {
            mInstance = new TTSPlayer(MwmApplication.get());
        }
        return mInstance;
    }

    private TTSPlayer(Context context) {
        mTTSPlayer = new SpeechSynthesizer(context, APP_KEY, SECRET);
        mTTSPlayer.setOption(SpeechConstants.TTS_SERVICE_MODE, SpeechConstants.TTS_SERVICE_MODE_LOCAL);
        mTTSPlayer.setTTSListener(this);
        mTTSPlayer.init(APP_KEY);
    }

    /**
     * 初始化本地离线TTS
     */
    private void initTts(Context context) {

        // 初始化语音合成对象
        mTTSPlayer = new SpeechSynthesizer(context, APP_KEY, SECRET);
        // 设置本地合成
        mTTSPlayer.setOption(SpeechConstants.TTS_SERVICE_MODE, SpeechConstants.TTS_SERVICE_MODE_LOCAL);
        File _FrontendModelFile = new File(mFrontendModel);
        if (!_FrontendModelFile.exists()) {
//            toastMessage("文件：" + mFrontendModel + "不存在，请将assets下相关文件拷贝到SD卡指定目录！");
        }
        File _BackendModelFile = new File(mBackendModel);
        if (!_BackendModelFile.exists()) {
//            toastMessage("文件：" + mBackendModel + "不存在，请将assets下相关文件拷贝到SD卡指定目录！");
        }
        // 设置前端模型
        mTTSPlayer.setOption(SpeechConstants.TTS_KEY_FRONTEND_MODEL_PATH, mFrontendModel);
        // 设置后端模型
        mTTSPlayer.setOption(SpeechConstants.TTS_KEY_BACKEND_MODEL_PATH, mBackendModel);
        // 设置回调监听
        mTTSPlayer.setTTSListener(new SpeechSynthesizerListener() {

            @Override
            public void onEvent(int type) {
                switch (type) {
                    case SpeechConstants.TTS_EVENT_INIT:
                        // 初始化成功回调
                        break;
                    case SpeechConstants.TTS_EVENT_SYNTHESIZER_START:
                        // 开始合成回调
                        break;
                    case SpeechConstants.TTS_EVENT_SYNTHESIZER_END:
                        // 合成结束回调
                        break;
                    case SpeechConstants.TTS_EVENT_BUFFER_BEGIN:
                        // 开始缓存回调
                        break;
                    case SpeechConstants.TTS_EVENT_BUFFER_READY:
                        // 缓存完毕回调
                        break;
                    case SpeechConstants.TTS_EVENT_PLAYING_START:
                        // 开始播放回调
                        break;
                    case SpeechConstants.TTS_EVENT_PLAYING_END:
                        // 播放完成回调
                        break;
                    case SpeechConstants.TTS_EVENT_PAUSE:
                        // 暂停回调
                        break;
                    case SpeechConstants.TTS_EVENT_RESUME:
                        // 恢复回调
                        break;
                    case SpeechConstants.TTS_EVENT_STOP:
                        // 停止回调
                        break;
                    case SpeechConstants.TTS_EVENT_RELEASE:
                        // 释放资源回调
                        break;
                    default:
                        break;
                }

            }

            @Override
            public void onError(int type, String errorMSG) {
                // 语音合成错误回调
            }
        });
        // 初始化合成引擎
        mTTSPlayer.init("");
    }


    public boolean play(String text) {
        mTTSPlayer.playText(text);
        return true;
    }

    @Override
    public void onEvent(int type) {
        switch (type) {
            case SpeechConstants.TTS_EVENT_INIT: // 初始化成功回调
                break;
            case SpeechConstants.TTS_EVENT_SYNTHESIZER_START: // 开始合成回调
                break;
            case SpeechConstants.TTS_EVENT_SYNTHESIZER_END: // 合成结束回调
                break;
            case SpeechConstants.TTS_EVENT_BUFFER_BEGIN: // 开始缓存回调
                break;
            case SpeechConstants.TTS_EVENT_BUFFER_READY: // 缓存完毕回调
                break;
            case SpeechConstants.TTS_EVENT_PLAYING_START: // 开始播放回调
                break;
            case SpeechConstants.TTS_EVENT_PLAYING_END: // 播放完成回调
                break;
            case SpeechConstants.TTS_EVENT_PAUSE: // 暂停回调
                break;
            case SpeechConstants.TTS_EVENT_RESUME: // 恢复回调
                break;
            case SpeechConstants.TTS_EVENT_STOP: // 停止回调
                break;
            case SpeechConstants.TTS_EVENT_RELEASE: // 释放资源回调
                break;
            default: break;
        }
    }

    @Override
    public void onError(int i, String s) {

    }
}
