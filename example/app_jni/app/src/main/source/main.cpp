#include <jni.h>
#include <testscene.h>

#include <sys\types.h>
#include <android\asset_manager.h>
#include <android\asset_manager_jni.h>

static TestScene renderer;

extern "C"
{
    JNIEXPORT void JNICALL Java_com_softmob_gles3jni_GLES3JNILib_init(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_softmob_gles3jni_GLES3JNILib_resize(JNIEnv* env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_softmob_gles3jni_GLES3JNILib_render(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_softmob_gles3jni_GLES3JNILib_nativeSetAssetManager(JNIEnv* env, jobject obj, jobject assetManager);
};

void Java_com_softmob_gles3jni_GLES3JNILib_init(JNIEnv* env, jobject obj)
{
    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);

    const char* versionStr = (const char*)glGetString(GL_VERSION);
    if (strstr(versionStr, "OpenGL ES 3."))
    {
        if (!renderer.init())
        {
            ALOGE("Initialize error");
        }
    }
    else
        ALOGE("Unsupported OpenGL ES version");
}

void Java_com_softmob_gles3jni_GLES3JNILib_resize(JNIEnv* env, jobject obj, jint width, jint height)
{
    renderer.resize(0, 0, width, height);
}

void Java_com_softmob_gles3jni_GLES3JNILib_render(JNIEnv* env, jobject obj)
{
    renderer.draw();
}

void Java_com_softmob_gles3jni_GLES3JNILib_nativeSetAssetManager(JNIEnv* env, jobject obj, jobject assetManager)
{
    AAssetManager* pAssetMgr = AAssetManager_fromJava(env, assetManager);
    AAssetFile::getInstance().setManager(pAssetMgr);
}