//
// Created by jun on 2017/05/03.
//

#ifndef CPPFRAMEBUFFEROBJECT_UTILS_H
#define CPPFRAMEBUFFEROBJECT_UTILS_H


class UtilsMatrix {
public:
    static void setIdentityM(float matrix[16]);
    static void setPerspectiveM(float matrix[16], double fovy, int aspect, double zNear, double zFar);
    static void frustumM(float matrix[16], float left, float right, float bottom, float top, float near, float far);
    static void setLookAtM(float matrix[16], float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);
    static void translateM(float matrix[16], float x, float y, float z);
    static void multiplyMM(float retm[16], float *lhsm, float *rhsm);
    static void setRotateM(float matrix[16], float angle, float x, float y, float z);
    static void rotateM(float retm[16], float angle, float x, float y, float z);
};


#endif //CPPFRAMEBUFFEROBJECT_UTILS_H
