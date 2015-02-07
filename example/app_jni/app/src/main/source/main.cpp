#include <jni.h>
#include <platform.h>
#include <testscene.h>

static TestScene renderer;

#define PREIFX_CAT(arg1, arg2, arg3, arg4) arg1 ##_## arg2 ##_## arg3 ##_## arg4
#define jni_declaration(method_name, ret_type, ...)  \
	extern "C" JNIEXPORT ret_type JNICALL PREIFX_CAT(Java, com_softmob_gles3jni, GLES3JNILib, method_name)(__VA_ARGS__)

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
            MLOG(mlog::error, "Initialize error");
        }
    }
    else
        MLOG(mlog::error, "Unsupported OpenGL ES version");
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
    DBG("nativeSetAssetManager\n");
    AAssetManager* pAssetMgr = AAssetManager_fromJava(env, assetManager);
    AAssetFile::getInstance().setManager(pAssetMgr);
}