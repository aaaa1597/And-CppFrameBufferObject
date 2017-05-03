#ifndef TESTNATIVESURFACE_H
#define TESTNATIVESURFACE_H

#include <jni.h>
#include <android/native_window.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_test_cppframebufferobject_NativeFunc_create(JNIEnv *env, jclass type, jint id);
JNIEXPORT void JNICALL Java_com_test_cppframebufferobject_NativeFunc_surfaceCreated(JNIEnv *env, jclass type, jint id, jobject surface);
JNIEXPORT void JNICALL Java_com_test_cppframebufferobject_NativeFunc_surfaceChanged(JNIEnv *env, jclass type, jint id, jint width, jint height);
JNIEXPORT void JNICALL Java_com_test_cppframebufferobject_NativeFunc_surfaceDestroyed(JNIEnv *env, jclass type, jint id);

#ifdef __cplusplus
}
#endif

// バッファと関連する変数を管理するクラス
class BufferObject {
public:
    int bufferid; // バッファオブジェクト
    int numofelements;    // 成分数
    int type;   // データ型
};

// モデルを構成するバッファオブジェクトを管理するクラス
class Model {
public:
    BufferObject vertexBufobj;    // 頂点座標用のバッファオブジェクト
    BufferObject texCoordBufobj;  // テクスチャ座標用のバッファオブジェクト
    BufferObject indexBufobj;     // インデックス用のバッファオブジェクト
    int numIndices;
};

class UtilsMatrix {
public:
    static void setIdentityM(float matrix[16]);
    static void setPerspectiveM(float matrix[16], double fovy, int aspect, double zNear, double zFar);
    static void frustumM(float matrix[16], float left, float right, float bottom, float top, float near, float far);
    static void setLookAtM(float matrix[16], float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);
    static void translateM(float matrix[16], float x, float y, float z);
    static void multiplyMM(float retm[16], float lhsm[16], float rhsm[16]);
    static void setRotateM(float matrix[16], float angle, float x, float y, float z);
    static void rotateM(float retm[16], float angle, float x, float y, float z);
};

class CppSurfaceView {
private:
    const char *VSHADER_SOURCE =
        "attribute vec4 a_Position;\n"
        "attribute vec2 a_TexCoord;\n"
        "uniform mat4 u_MvpMatrix;\n"
        "varying vec2 v_TexCoord;\n"
        "void main() {\n"
        "  gl_Position = u_MvpMatrix * a_Position;\n"
        "  v_TexCoord = a_TexCoord;\n"
        "}\n";

    const char *FSHADER_SOURCE =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D u_Sampler0;\n"
        "varying vec2 v_TexCoord;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(u_Sampler0, v_TexCoord);\n"
        "}\n";

private:
    // オフスクリーン描画のサイズ
    static const int OFFSCREEN_WIDTH = 256;
    static const int OFFSCREEN_HEIGHT = 256;

    // メンバー変数
    int mFBO;     // フレームバッファオブジェクト(FBO)
    Model mCube;  // 立方体の頂点情報
    Model mPlane; // 平面の頂点情報
    int ma_Position;
    int ma_TexCoord;
    int mu_MvpMatrix;
    int mu_Sampler;
    int mTexture;    // 立方体に貼るテクスチャ
    int mFBOTexture; // 平面に貼るテクスチャ

    // 座標変換行列
    float mViewProjMatrix[16]={0};   // ビュー投影行列
    float mViewProjMatrixFBO[16]={0};// ビュー投影行列(FBO用)
    float mModelMatrix[16]={0};
    float mMvpMatrix[16]={0};

    float mCurrentAngle = 0.0f; // 立方体の現在の回転角
    long mLast; // 最後に呼び出された時刻

private:
    void initVertexBuffersForCube(Model &model);
    void initVertexBuffersForPlane(Model &model);
    void initArrayBufferForLaterUse(float *datas, int datalen, int vertexnum, int datatype, BufferObject &buffobj);
    void initElementArrayBufferForLaterUse(char *datas, int datalen, int datatype, BufferObject &buffobj);
    int initTextures(GLuint program, char *samplername, int unitno, char *filepath);
    int initFramebufferObject();
    float animate(float angle);
    void drawTexturedCube(Model &cube, float angle, int texture, float datas[16]);
    void drawTexturedObject(Model &model, int textureid);
    void initAttributeVariable(int position, int bufid, int numofelements, int buffer_type);
    void drawTexturedPlane(Model &model, float angle, int textureid, float vpmatrix[16]);

private:
    GLuint createProgram(const char *vertexshader, const char *fragmentshader);
    GLuint loadShader(int i, const char *vertexshader);
    void checkGlError(const char *argstr);

public:
    static const int STATUS_NONE   = 0;
    static const int STATUS_INITIALIZING = 1;
    static const int STATUS_DRAWING= 2;
    static const int STATUS_FINISH = 3;
    int mId = -1;
    int mStatus = STATUS_NONE;
    pthread_t mThreadId = -1;
    ANativeWindow *mWindow = NULL;
    EGLDisplay mEGLDisplay = NULL;
    EGLContext mEGLContext = NULL;
    EGLSurface mEGLSurface = NULL;
    GLuint mProgram = -1;
    bool isSurfaceCreated = false;
    int mDspW = 0;
    int mDspH = 0;

public:
    CppSurfaceView(int id);
    virtual ~CppSurfaceView();
    static void *draw_thread(void *pArg);
    void createThread(JNIEnv *pEnv, jobject surface);
    void initEGL();
    void finEGL();
    void initGL();
    void predrawGL();
    void drawGL();
    void destroy();
};

#endif //TESTNATIVESURFACE_H
