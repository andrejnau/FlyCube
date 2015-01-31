package com.softmob.gles3jni;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import android.content.res.AssetManager;

class GLES3JNIView extends GLSurfaceView {

    public GLES3JNIView(Context context) {
        super(context);
        setEGLContextClientVersion(3);
        setRenderer(new Renderer());
        GLES3JNILib.nativeSetAssetManager(context.getApplicationContext().getAssets());
    }

    private static class Renderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            GLES3JNILib.render();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            GLES3JNILib.resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            GLES3JNILib.init();
        }
    }
}
