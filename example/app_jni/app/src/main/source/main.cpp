#include <jni.h>
#include <testscene.h>

#include <sys\types.h>
#include <android\asset_manager.h>
#include <android\asset_manager_jni.h>

static TestScene renderer;

#define PREIFX_CAT(arg1, arg2, arg3, arg4) arg1 ##_## arg2 ##_## arg3 ##_## arg4
#define jni_declaration(metod_name, ret_type, ...)  \
	extern "C" JNIEXPORT ret_type JNICALL PREIFX_CAT(Java, com_softmob_gles3jni, GLES3JNILib, metod_name)(__VA_ARGS__)

jni_declaration(init, void, JNIEnv* env, jobject obj)
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

jni_declaration(resize, void, JNIEnv* env, jobject obj, jint width, jint height)
{
    renderer.resize(0, 0, width, height);
}

jni_declaration(render, void, JNIEnv* env, jobject obj)
{
    renderer.draw();
}

jni_declaration(nativeSetAssetManager, void, JNIEnv* env, jobject obj, jobject assetManager)
{
    AAssetManager* pAssetMgr = AAssetManager_fromJava(env, assetManager);
    AAssetFile::getInstance().setManager(pAssetMgr);
}