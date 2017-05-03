//
// Created by jun on 2017/05/03.
//

#include <string.h>
#include "UtilsMatrix.h"

void UtilsMatrix::setIdentityM(float matrix[16]) {
    float tmp[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    memcpy(matrix, tmp, sizeof(tmp));
}

#include <math.h>
#include <cmath>
void UtilsMatrix::setPerspectiveM(float matrix[16], double fovy, int aspect, double zNear, double zFar) {
    UtilsMatrix::setIdentityM(matrix);
    double ymax = zNear * std::tan(fovy * M_PI / 360.0);
    double ymin = -ymax;
    double xmin = ymin * aspect;
    double xmax = ymax * aspect;
    UtilsMatrix::frustumM(matrix, (float)xmin, (float)xmax, (float)ymin, (float)ymax, (float)zNear, (float)zFar);
}

void UtilsMatrix::frustumM(float matrix[16], float left, float right, float bottom, float top, float near, float far) {
    float r_width  = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth  = 1.0f / (near - far);
    float x = 2.0f * (near * r_width);
    float y = 2.0f * (near * r_height);
    float A = (right + left) * r_width;
    float B = (top + bottom) * r_height;
    float C = (far + near) * r_depth;
    float D = 2.0f * (far * near * r_depth);
    matrix[0] = x;
    matrix[5] = y;
    matrix[8] = A;
    matrix[9] = B;
    matrix[10]= C;
    matrix[14]= D;
    matrix[11]= -1.0f;
    matrix[1] = 0.0f;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;
    matrix[4] = 0.0f;
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;
    matrix[12]= 0.0f;
    matrix[13]= 0.0f;
    matrix[15]= 0.0f;
}

void UtilsMatrix::setLookAtM(float matrix[16], float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize f
    float rlf = 1.0f / (float)sqrt(fx * fx + fy * fy + fz * fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // compute s = f x up (x means "cross product")
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    // and normalize s
    float rls = 1.0f / (float)sqrt(sx * sx + sy * sy + sz * sz);
    sx *= rls;
    sy *= rls;
    sz *= rls;

    // compute u = s x f
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    matrix[0] = sx;
    matrix[1] = ux;
    matrix[2] = -fx;
    matrix[3] = 0.0f;

    matrix[4] = sy;
    matrix[5] = uy;
    matrix[6] = -fy;
    matrix[7] = 0.0f;

    matrix[8] = sz;
    matrix[9] = uz;
    matrix[10] = -fz;
    matrix[11] = 0.0f;

    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;

    translateM(matrix, -eyeX, -eyeY, -eyeZ);
}

void UtilsMatrix::translateM(float matrix[16], float x, float y, float z) {
    for(int mi=0 ; mi<4 ; mi++)
        matrix[12 + mi] += matrix[mi] * x + matrix[4 + mi] * y + matrix[8 + mi] * z;
}

#define I(_i, _j) ((_j)+ 4*(_i))
void UtilsMatrix::multiplyMM(float retm[16], float *lhsm, float *rhsm) {
    for (int i=0 ; i<4 ; i++) {
        register const float rhs_i0 = rhsm[ I(i,0) ];
        register float ri0 = lhsm[ I(0,0) ] * rhs_i0;
        register float ri1 = lhsm[ I(0,1) ] * rhs_i0;
        register float ri2 = lhsm[ I(0,2) ] * rhs_i0;
        register float ri3 = lhsm[ I(0,3) ] * rhs_i0;
        for (int j=1 ; j<4 ; j++) {
            register const float rhs_ij = rhsm[ I(i,j) ];
            ri0 += lhsm[ I(j,0) ] * rhs_ij;
            ri1 += lhsm[ I(j,1) ] * rhs_ij;
            ri2 += lhsm[ I(j,2) ] * rhs_ij;
            ri3 += lhsm[ I(j,3) ] * rhs_ij;
        }
        retm[ I(i,0) ] = ri0;
        retm[ I(i,1) ] = ri1;
        retm[ I(i,2) ] = ri2;
        retm[ I(i,3) ] = ri3;
    }
}

void UtilsMatrix::setRotateM(float matrix[16], float angle, float x, float y, float z) {
    matrix[3] = 0;
    matrix[7] = 0;
    matrix[11]= 0;
    matrix[12]= 0;
    matrix[13]= 0;
    matrix[14]= 0;
    matrix[15]= 1;
    angle *= (float) (M_PI / 180.0f);
    float s = (float) sin(angle);
    float c = (float) cos(angle);
    if (1.0f == x && 0.0f == y && 0.0f == z) {
        matrix[5] = c;   matrix[10]= c;
        matrix[6] = s;   matrix[9] = -s;
        matrix[1] = 0;   matrix[2] = 0;
        matrix[4] = 0;   matrix[8] = 0;
        matrix[0] = 1;
    } else if (0.0f == x && 1.0f == y && 0.0f == z) {
        matrix[0] = c;   matrix[10]= c;
        matrix[8] = s;   matrix[2] = -s;
        matrix[1] = 0;   matrix[4] = 0;
        matrix[6] = 0;   matrix[9] = 0;
        matrix[5] = 1;
    } else if (0.0f == x && 0.0f == y && 1.0f == z) {
        matrix[0] = c;   matrix[5] = c;
        matrix[1] = s;   matrix[4] = -s;
        matrix[2] = 0;   matrix[6] = 0;
        matrix[8] = 0;   matrix[9] = 0;
        matrix[10]= 1;
    } else {
        float len = (float)sqrt(x * x + y * y + z * z);
        if (1.0f != len) {
            float recipLen = 1.0f / len;
            x *= recipLen;
            y *= recipLen;
            z *= recipLen;
        }
        float nc = 1.0f - c;
        float xy = x * y;
        float yz = y * z;
        float zx = z * x;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        matrix[ 0] = x*x*nc +  c;
        matrix[ 4] =  xy*nc - zs;
        matrix[ 8] =  zx*nc + ys;
        matrix[ 1] =  xy*nc + zs;
        matrix[ 5] = y*y*nc +  c;
        matrix[ 9] =  yz*nc - xs;
        matrix[ 2] =  zx*nc - ys;
        matrix[ 6] =  yz*nc + xs;
        matrix[10] = z*z*nc +  c;
    }
}

void UtilsMatrix::rotateM(float retm[16], float angle, float x, float y, float z) {
    float tmp1[16] = {0};
    setRotateM(tmp1, angle, x, y, z);
    float tmp2[16] = {0};
    multiplyMM(tmp2, retm, tmp1);
    memcpy(retm, tmp2, sizeof(tmp2));
    return;
}
