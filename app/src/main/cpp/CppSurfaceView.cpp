#include <cmath>
#include <map>
#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "CppSurfaceView.h"
#include "UtilPng.h"

std::map<int, CppSurfaceView*> gpSufacesLists;

#ifdef __cplusplus
extern "C" {
#endif

void Java_com_test_cppframebufferobject_NativeFunc_create(JNIEnv *pEnv, jclass clazz, jint id) {
    gpSufacesLists[id] = new CppSurfaceView(id);
}

void Java_com_test_cppframebufferobject_NativeFunc_surfaceCreated(JNIEnv *pEnv, jclass clazz, jint id, jobject surface) {
    gpSufacesLists[id]->createThread(pEnv, surface);
}

void Java_com_test_cppframebufferobject_NativeFunc_surfaceChanged(JNIEnv *pEnv, jclass clazz, jint id, jint width, jint height) {
    gpSufacesLists[id]->isSurfaceCreated = true;
    gpSufacesLists[id]->mDspW = width;
    gpSufacesLists[id]->mDspH = height;
}

void Java_com_test_cppframebufferobject_NativeFunc_surfaceDestroyed(JNIEnv *pEnv, jclass clazz, jint id) {
    gpSufacesLists[id]->mStatus = CppSurfaceView::STATUS_FINISH;
    gpSufacesLists[id]->destroy();
}

#ifdef __cplusplus
}
#endif

/******************/
/* CppSurfaceView */
/******************/
CppSurfaceView::CppSurfaceView(int id) : mId(id), mStatus(CppSurfaceView::STATUS_NONE), mThreadId(-1) {
}

void CppSurfaceView::createThread(JNIEnv *pEnv, jobject surface) {
    mStatus = CppSurfaceView::STATUS_INITIALIZING;
    mWindow = ANativeWindow_fromSurface(pEnv, surface);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&mThreadId, &attr, CppSurfaceView::draw_thread, this);
}

#include <unistd.h>
void *CppSurfaceView::draw_thread(void *pArg) {
    if(pArg == NULL) return NULL;
    CppSurfaceView *pSurface = (CppSurfaceView*)pArg;

    pSurface->initEGL();
    pSurface->initGL();
    pSurface->mStatus = CppSurfaceView::STATUS_DRAWING;

    pSurface->predrawGL();
    for(;pSurface->mStatus==CppSurfaceView::STATUS_DRAWING;) {
        clock_t s = clock();

        /* SurfaceCreated()が動作した時は、画面サイズ変更を実行 */
        if(pSurface->isSurfaceCreated) {
            pSurface->isSurfaceCreated = false;
            glViewport(0,0,pSurface->mDspW,pSurface->mDspH);

            float projMatrix[16] = {0};
            float viewMatrix[16] = {0};
            UtilsMatrix::setPerspectiveM(projMatrix, 30.0, (double)pSurface->mDspW/pSurface->mDspH, 1.0, 100.0);
            UtilsMatrix::setLookAtM(viewMatrix, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
            UtilsMatrix::multiplyMM(pSurface->mViewProjMatrix, projMatrix, viewMatrix);
        }

        /* 通常の描画処理 */
        pSurface->drawGL();
        clock_t e = clock();
        if( (e-s) < 16667) {
            clock_t waittime = 16667 - (e-s);
            __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa tttttt. waittime=%dms", waittime);
            usleep(waittime);
        }
    }

    pSurface->finEGL();

    return NULL;
}

void CppSurfaceView::initEGL() {
    EGLint major, minor;
    mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(mEGLDisplay, &major, &minor);

    /* 設定取得 */
    const EGLint configAttributes[] = {
            EGL_LEVEL, 0,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            /* 透過設定 */
            EGL_ALPHA_SIZE, EGL_OPENGL_BIT,
            /*EGL_BUFFER_SIZE, 32 */  /* ARGB8888用 */
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(mEGLDisplay, configAttributes, &config, 1, &numConfigs);

    /* context生成 */
    const EGLint contextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    mEGLContext = eglCreateContext(mEGLDisplay, config, EGL_NO_CONTEXT, contextAttributes);

    /* ウィンドウバッファサイズとフォーマットを設定 */
    EGLint format;
    eglGetConfigAttrib(mEGLDisplay, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(mWindow, 0, 0, format);

    /* surface生成 */
    mEGLSurface = eglCreateWindowSurface(mEGLDisplay, config, mWindow, NULL);
    if(mEGLSurface == EGL_NO_SURFACE) {
        __android_log_print(ANDROID_LOG_ERROR, "CppSurfaceView", "%d aaaaa surface生成 失敗!!", mId);
        throw "surface生成 失敗!!";
    }

    /* context再生成 */
    mEGLContext = eglCreateContext(mEGLDisplay, config, EGL_NO_CONTEXT, contextAttributes);

    /* 作成したsurface/contextを関連付け */
    if(eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext) == EGL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "CppSurfaceView", "%d aaaaa surface/contextの関連付け 失敗!!", mId);
        throw "surface/contextの関連付け 失敗!!";
    }

    /* チェック */
    EGLint w,h;
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_WIDTH, &w);
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_HEIGHT,&h);
    glViewport(0,0,w,h);
}

void CppSurfaceView::initGL() {
    mProgram = createProgram(VSHADER_SOURCE, FSHADER_SOURCE);
}

GLuint CppSurfaceView::createProgram(const char *vertexshader, const char *fragmentshader) {
    GLuint vhandle = loadShader(GL_VERTEX_SHADER, vertexshader);
    if(vhandle == GL_FALSE) return GL_FALSE;

    GLuint fhandle = loadShader(GL_FRAGMENT_SHADER, fragmentshader);
    if(fhandle == GL_FALSE) return GL_FALSE;

    GLuint programhandle = glCreateProgram();
    if(programhandle == GL_FALSE) {
        checkGlError("glCreateProgram");
        return GL_FALSE;
    }

    glAttachShader(programhandle, vhandle);
    checkGlError("glAttachShader(VERTEX_SHADER)");
    glAttachShader(programhandle, fhandle);
    checkGlError("glAttachShader(FRAGMENT_SHADER)");

    glLinkProgram(programhandle);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(programhandle, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE) {
        GLint bufLen = 0;
        glGetProgramiv(programhandle, GL_INFO_LOG_LENGTH, &bufLen);
        if(bufLen) {
            char *logstr = (char*)malloc(bufLen);
            glGetProgramInfoLog(mProgram, bufLen, NULL, logstr);
            __android_log_print(ANDROID_LOG_ERROR, "CppSurfaceView", "%d aaaaa glLinkProgram() Fail!!\n %s", mId, logstr);
            free(logstr);
        }
        glDeleteProgram(programhandle);
        programhandle = GL_FALSE;
    }

    return programhandle;
}

GLuint CppSurfaceView::loadShader(int shadertype, const char *sourcestring) {
    GLuint shaderhandle = glCreateShader(shadertype);
    if(!shaderhandle) return GL_FALSE;

    glShaderSource(shaderhandle, 1, &sourcestring, NULL);
    glCompileShader(shaderhandle);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shaderhandle, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shaderhandle, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen) {
            char *logbuf = (char*)malloc(infoLen);
            if(logbuf) {
                glGetShaderInfoLog(shaderhandle, infoLen, NULL, logbuf);
                __android_log_print(ANDROID_LOG_ERROR, "CppSurfaceView", "%d aaaaa shader failuer!!\n%s", mId, logbuf);
                free(logbuf);
            }
        }
        glDeleteShader(shaderhandle);
        shaderhandle = GL_FALSE;
    }

    return shaderhandle;
}

void CppSurfaceView::checkGlError(const char *argstr) {
    for(GLuint error = glGetError(); error; error = glGetError())
        __android_log_print(ANDROID_LOG_ERROR, "CppSurfaceView", "%d aaaaa after %s errcode=%d", mId, argstr, error);
}

void CppSurfaceView::predrawGL() {
    /* GLパラメータハンドラ初期化 */
    ma_Position = glGetAttribLocation(mProgram, "a_Position");
    ma_TexCoord = glGetAttribLocation(mProgram, "a_TexCoord");
    mu_MvpMatrix= glGetUniformLocation(mProgram, "u_MvpMatrix");
    if(ma_Position == -1 || ma_TexCoord == -1 || mu_MvpMatrix == -1)
        throw "a_Position, a_TexCoord, u_MvpMatrix変数の格納場所の取得に失敗";

    /* 立方体生成 */
    initVertexBuffersForCube(mCube);
    /* 平面生成 */
    initVertexBuffersForPlane(mPlane);

    /* テクスチャ生成 */
    mTexture = initTextures(mProgram, "u_Sampler0", 0, "/data/local/sky_cloud.png");     // 立方体に貼るテクスチャを設定する

    // フレームバッファオブジェクト(FBO)を初期化する
    mFBO = initFramebufferObject();
    if(mFBO <= 0)
        throw "フレームバッファオブジェクトの初期化に失敗";

    // デプステストを有効にする
    glEnable(GL_DEPTH_TEST);
//  glEnable(GL_CULL_FACE);

    float projMatrix[16] = {0};
    float viewMatrix[16] = {0};
    UtilsMatrix::setPerspectiveM(projMatrix, 30.0, OFFSCREEN_WIDTH/OFFSCREEN_HEIGHT, 1.0, 100.0);
    UtilsMatrix::setLookAtM(viewMatrix, 0.0f, 2.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    UtilsMatrix::multiplyMM(mViewProjMatrixFBO, projMatrix, viewMatrix);

    mLast = clock() / (CLOCKS_PER_SEC/1000);

    glUseProgram(mProgram);

}

void CppSurfaceView::drawGL() {
    mCurrentAngle = animate(mCurrentAngle);  // 回転角度を更新する

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);      // 描画先をFBOにする
    glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT); // FBO用のビューポートを設定

    glClearColor(0.2f, 0.2f, 0.4f, 1.0f); // クリアカラーを指定する(ちょっと色を変えておく)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // FBOをクリアする

    drawTexturedCube(mCube, mCurrentAngle, mTexture, mViewProjMatrixFBO);   // 立方体を描画

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 描画先をカラーバッファに
    glViewport(0, 0, mDspW, mDspH);           // ビューポートの設定を画面のサイズに戻す

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // カラーバッファをクリアする

    drawTexturedPlane(mPlane, mCurrentAngle, mFBOTexture, mViewProjMatrix);  // 平面を描画
    eglSwapBuffers(mEGLDisplay, mEGLSurface);
}

void CppSurfaceView::finEGL() {
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa%d CppSurfaceView::finEGL()", mId);
    ANativeWindow_release(mWindow);
    mWindow = NULL;
}

void CppSurfaceView::destroy() {
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa%d CppSurfaceView::destroy()", mId);
    pthread_join(mThreadId, NULL);
}

CppSurfaceView::~CppSurfaceView() {
}

void CppSurfaceView::initVertexBuffersForCube(Model &model) {
    // 立方体を生成する
    //    v6----- v5
    //   /|      /|
    //  v1------v0|
    //  | |     | |
    //  | |v7---|-|v4
    //  |/      |/
    //  v2------v3

    // 頂点座標
    float vertices[] = {
         1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f,-1.0f, 1.0f,   1.0f,-1.0f, 1.0f,    // v0-v1-v2-v3 前
         1.0f, 1.0f, 1.0f,   1.0f,-1.0f, 1.0f,   1.0f,-1.0f,-1.0f,   1.0f, 1.0f,-1.0f,    // v0-v3-v4-v5 右
         1.0f, 1.0f, 1.0f,   1.0f, 1.0f,-1.0f,  -1.0f, 1.0f,-1.0f,  -1.0f, 1.0f, 1.0f,    // v0-v5-v6-v1 上
        -1.0f, 1.0f, 1.0f,  -1.0f, 1.0f,-1.0f,  -1.0f,-1.0f,-1.0f,  -1.0f,-1.0f, 1.0f,    // v1-v6-v7-v2 左
        -1.0f,-1.0f,-1.0f,   1.0f,-1.0f,-1.0f,   1.0f,-1.0f, 1.0f,  -1.0f,-1.0f, 1.0f,    // v7-v4-v3-v2 下
         1.0f,-1.0f,-1.0f,  -1.0f,-1.0f,-1.0f,  -1.0f, 1.0f,-1.0f,   1.0f, 1.0f,-1.0f     // v4-v7-v6-v5 奥
    };

    // テクスチャ座標
    float texCoords[] = {
        1.0f, 0.0f,   0.0f, 0.0f,   0.0f, 1.0f,   1.0f, 1.0f,    // v0-v1-v2-v3 前
        0.0f, 0.0f,   0.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f,    // v0-v3-v4-v5 右
        1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f,   0.0f, 1.0f,    // v0-v5-v6-v1 上
        1.0f, 0.0f,   0.0f, 0.0f,   0.0f, 1.0f,   1.0f, 1.0f,    // v1-v6-v7-v2 左
        0.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f,    // v7-v4-v3-v2 下
        0.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f     // v4-v7-v6-v5 奥
    };

    // インデックス
    char indices[] = {
         0, 1, 2,   0, 2, 3,    // 前
         4, 5, 6,   4, 6, 7,    // 右
         8, 9,10,   8,10,11,    // 上
        12,13,14,  12,14,15,    // 左
        16,17,18,  16,18,19,    // 下
        20,21,22,  20,22,23     // 奥
    };

    // 頂点情報をバッファオブジェクトに書き込む
    initArrayBufferForLaterUse(vertices, sizeof(vertices)/sizeof(float), 3, GL_FLOAT, model.vertexBufobj);
    initArrayBufferForLaterUse(texCoords,sizeof(texCoords)/sizeof(float),2, GL_FLOAT, model.texCoordBufobj);
    initElementArrayBufferForLaterUse(indices, sizeof(indices), GL_UNSIGNED_BYTE, model.indexBufobj);
    model.numIndices = sizeof(indices);
    return;
}

void CppSurfaceView::initArrayBufferForLaterUse(float *datas, int datalen, int vertexnum, int datatype, BufferObject &buffobj) {
    // バッファオブジェクトを作成する
    GLuint bufferid;
    glGenBuffers(1, &bufferid);
    // バッファオブジェクトにデータを書き込む
    glBindBuffer(GL_ARRAY_BUFFER, bufferid);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * datalen, datas, GL_STATIC_DRAW);

    // 後でattribute変数に割り当てるのに必要な情報を設定しておく
    buffobj.bufferid= bufferid;
    buffobj.numofelements   = vertexnum;
    buffobj.type  = datatype;
    return;
}

void CppSurfaceView::initElementArrayBufferForLaterUse(char *datas, int datalen, int datatype, BufferObject &buffobj) {
    // バッファオブジェクトを作成する
    GLuint bufferid;
    glGenBuffers(1, &bufferid);
    // バッファオブジェクトにデータを書き込む
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferid);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, datalen, datas, GL_STATIC_DRAW);

    // 後でattribute変数に割り当てるのに必要な情報を設定しておく
    buffobj.bufferid= bufferid;
    buffobj.numofelements   = 0;
    buffobj.type  = datatype;

    return;
}

void CppSurfaceView::initVertexBuffersForPlane(Model &model) {
    // 平面を生成する
    //  v1------v0
    //  |        |
    //  |        |
    //  |        |
    //  v2------v3

    // 頂点座標
    float vertices[] = {
        1.0f, 1.0f, 0.0f,  -1.0f, 1.0f, 0.0f,  -1.0f,-1.0f, 0.0f,   1.0f,-1.0f, 0.0f    // v0-v1-v2-v3
    };

    // テクスチャ座標
    float texCoords[] = {1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f,   1.0f, 0.0f};

    // インデックス
    char indices[] = {0, 1, 2,   0, 2, 3};

    // 頂点情報をバッファオブジェクトに書き込む
    initArrayBufferForLaterUse(vertices, sizeof(vertices)/sizeof(float), 3, GL_FLOAT, model.vertexBufobj);
    initArrayBufferForLaterUse(texCoords,sizeof(texCoords)/sizeof(float),2, GL_FLOAT, model.texCoordBufobj);
    initElementArrayBufferForLaterUse(indices, sizeof(indices), GL_UNSIGNED_BYTE, model.indexBufobj);
    model.numIndices = sizeof(indices);

    return;
}

int CppSurfaceView::initTextures(GLuint programid, char *samplername, int unitno, char *filepath) {
    GLuint textureid;
    glGenTextures(1, &textureid);   // テクスチャオブジェクトを作成する
    glBindTexture(GL_TEXTURE_2D, textureid);

    CppPng png(filepath);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa Image: alpha:%d width:%dpx height%dpx", png.has_alpha(), png.get_width(), png.get_height());
    GLint format = png.has_alpha() ? GL_RGBA : GL_RGB;

    // テクスチャオブジェクトに画像データを書きこむ
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, png.get_width(), png.get_height(), 0, format, GL_UNSIGNED_BYTE, png.get_data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // u_Samplerの格納場所を取得する
    mu_Sampler = glGetUniformLocation(programid, samplername);
    if(mu_Sampler == -1)
        throw "u_Samplerの格納場所の取得に失敗";

    return textureid;
}

int CppSurfaceView::initFramebufferObject() {
    // フレームバッファオブジェクト(FBOを作成する
    GLuint framebufferid;
    glGenFramebuffers(1, &framebufferid);

    // テクスチャオブジェクトを作成し、サイズ、パラメータを設定する
    GLuint textureid;
    glGenTextures(1, &textureid); // テクスチャオブジェクトを作成
    glBindTexture(GL_TEXTURE_2D, textureid); // ターゲットにバインド
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    mFBOTexture = textureid; // テクスチャオブジェクトを格納しておく

    // レンダーバッファオブジェクトを作成し、サイズ、パラメータを設定する
    GLuint depthBufferid;
    glGenRenderbuffers(1, &depthBufferid); // レンダーバッファオブジェクトを作成
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferid); // ターゲットにバインド
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

    // FBOにテクスチャとレンダーバッファオブジェクトをアタッチする
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferid);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureid, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferid);

    // FBOが正しく設定できたかチェックする
    int e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (e != GL_FRAMEBUFFER_COMPLETE)
        throw "フレームバッファオブジェクトが不完全";

    return framebufferid;
}

static const float ANGLE_STEP = 30.0f;   // 回転角の増分(度)
float CppSurfaceView::animate(float angle) {
    long now = clock() / (CLOCKS_PER_SEC/1000);   // 前回呼び出されてからの経過時間を計算
    long elapsed = now - mLast;
    mLast = now;
    // 回転角度を更新する(経過時間により調整)
    float newAngle = angle + (ANGLE_STEP * elapsed) / 1000.0f * 20/* 20倍速 */;
    return fmod( newAngle, 360 );
}

void CppSurfaceView::drawTexturedCube(Model &cube, float angle, int texture, float datas[16]) {
    // モデル変換行列を計算する
    UtilsMatrix::setRotateM(mModelMatrix, 20.0f, 1.0f, 0.0f, 0.0f);
    UtilsMatrix::rotateM(mModelMatrix, angle, 0.0f, 1.0f, 0.0f);

    // モデルビュー投影行列を計算し、u_MvpMatrixに設定する
    UtilsMatrix::multiplyMM(mMvpMatrix, datas, mModelMatrix);
    glUniformMatrix4fv(mu_MvpMatrix, 1, false, mMvpMatrix);

    drawTexturedObject(cube, texture);
}

void CppSurfaceView::drawTexturedObject(Model &model, int textureid) {
    // バッファオブジェクトをattribute変数に割り当て、有効にする
    BufferObject &vb = model.vertexBufobj;
    initAttributeVariable(ma_Position, vb.bufferid, vb.numofelements, vb.type);    // 頂点座標
    BufferObject &tcb = model.texCoordBufobj;
    initAttributeVariable(ma_TexCoord, tcb.bufferid, tcb.numofelements, tcb.type); // テクスチャ座標

    // テクスチャオブジェクトをバインドする
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureid);
    glUniform1i(mu_Sampler, GL_TEXTURE0-GL_TEXTURE0);

    // 描画する
    BufferObject &ib = model.indexBufobj;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.bufferid);
    glDrawElements(GL_TRIANGLES, model.numIndices, ib.type, 0);
    return;
}

void CppSurfaceView::initAttributeVariable(int position, int bufid, int numofelements, int buffer_type) {
    glBindBuffer(GL_ARRAY_BUFFER, bufid);
    glVertexAttribPointer(position, numofelements, buffer_type, false, 0, NULL);
    glEnableVertexAttribArray(position);
}

void CppSurfaceView::drawTexturedPlane(Model &plane, float angle, int textureid, float vpmatrix[16]) {
    // モデル変換行列を計算する
    UtilsMatrix::setIdentityM(mModelMatrix);
    UtilsMatrix::translateM(mModelMatrix, 0.0f, 0.0f, 1.0f);
    UtilsMatrix::rotateM(mModelMatrix, 20.0f, 1.0f, 0.0f, 0.0f);
    UtilsMatrix::rotateM(mModelMatrix, angle, 0.0f, 1.0f, 0.0f);

    // モデルビュー投影行列を計算し、u_MvpMatrixに設定する
    UtilsMatrix::multiplyMM(mMvpMatrix, vpmatrix, mModelMatrix);
    glUniformMatrix4fv(mu_MvpMatrix, 1, false, mMvpMatrix);

    drawTexturedObject(plane, textureid);
}
