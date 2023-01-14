#include <stdint.h>

typedef uint64_t ulonglong;
typedef int64_t longlong;
typedef uint32_t uint;
typedef uint32_t undefined4;
typedef uint8_t undefined;
typedef uint32_t uint;
typedef uint64_t undefined8;

undefined8 PerlinNoiseI(ulonglong bytes_x, ulonglong bytes_y) {
  uint iy;
  uint ix;
  longlong lVar2;
  longlong lVar3;
  ulonglong X;
  uint Y;
  undefined4 in_XMM0_Dc;
  undefined4 in_XMM0_Dd;
  undefined auVar5 [16];
  undefined auVar6 [16];
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  undefined4 in_XMM1_Dc;
  undefined4 in_XMM1_Dd;
  float fVar11;
  float fVar12;
  float y;
  float fVar13;
  float x;
  float fVar14;
  
  // Bytes interpreted as float
  y = (float)((uint)bytes_y & 0x7fffffff);
  x = (float)((uint)bytes_x & 0x7fffffff);
  
  // Casted to uint
  iy = (uint)y;
  
  // Wrap
  Y = iy & 0xff;
    
  ix = (uint)x;
  X = (ulonglong)(ix & 0xff);
  
  // Relative space
  y -= (float)iy;
  
  lVar2 = (longlong)(int)(*(int *)(&p1 + X * 4) + Y);
  lVar3 = (longlong)(int)(*(int *)(&p2 + X * 4) + Y);
  x -= (float)ix;
  iy = *(uint *)(&p1 + (longlong)*(int *)(&p2 + lVar3 * 4) * 4);
  
  // wth is this?
  fVar8 = 1.0;
  if (x <= 1.0) {
    fVar8 = x;
  }
  
  // Compiler-decompiler spaghetti
  ix = iy & 0xf;
  fVar12 = x - 1.0;
  fVar7 = 1.0;
  if (y <= 1.0) {
    fVar7 = y;
  }
  fVar14 = ((fVar8 * 6.0 - 15.0) * fVar8 + 10.0) * fVar8 * fVar8 * fVar8;
  fVar8 = y - 1.0;
  if (ix < 8) {
    fVar11 = fVar12;
    if (ix < 4) {
      auVar5 = CONCAT412(in_XMM1_Dd,
                         CONCAT48(in_XMM1_Dc,bytes_y & 0xffffffff00000000 | (ulonglong)(uint)fVar8))
               & (undefined[16]) 0xffffffffffffffff;
    }
    else {
LAB_18075f1a4:
      auVar5 = ZEXT416(0);
    }
  }
  else {
    fVar11 = fVar8;
    if ((ix - 0xc & 0xfffffffd) != 0) goto LAB_18075f1a4;
    auVar5 = CONCAT412(in_XMM0_Dd,
                       CONCAT48(in_XMM0_Dc,bytes_x & 0xffffffff00000000 | (ulonglong)(uint)fVar12))
             & (undefined[16]) 0xffffffffffffffff;
  }
  if ((iy & 1) != 0) {
      // negation
    fVar11 = (float)((uint)fVar11 ^ 0x80000000);
  }
  if ((iy & 2) != 0) {
      // negation
    auVar6 = auVar5 ^ (undefined  [16])0x80000000;
    auVar5 = CONCAT412(SUB164(auVar6 >> 0x60,0),
                       CONCAT48(SUB164(auVar6 >> 0x40,0),
                                CONCAT44(SUB164(auVar5 >> 0x20,0),SUB164(auVar6,0))));
  }
  iy = *(uint *)(&p1 + (longlong)*(int *)(&p2 + lVar2 * 4) * 4);
  ix = iy & 0xf;
  if (ix < 8) {
    fVar9 = fVar8;
    fVar8 = x;
    if (3 < ix) {
LAB_18075f1ed:
      fVar9 = 0.0;
    }
  }
  else {
    fVar9 = x;
    if ((ix - 0xc & 0xfffffffd) != 0) goto LAB_18075f1ed;
  }
  if (iy & 1) {
    fVar8 = (float)((uint)fVar8 ^ 0x80000000);
  }
  if (iy & 2) {
    fVar9 = (float)((uint)fVar9 ^ 0x80000000);
  }
  iy = *(uint *)(&p1 + (longlong)*(int *)(&p1 + lVar3 * 4) * 4);
  ix = iy & 0xf;
  if (ix < 8) {
    fVar10 = y;
    if (3 < ix) {
LAB_18075f23e:
      fVar10 = 0.0;
    }
  }
  else {
    fVar10 = fVar12;
    fVar12 = y;
    if ((ix - 0xc & 0xfffffffd) != 0) goto LAB_18075f23e;
  }
  if ((iy & 1) != 0) {
    fVar12 = (float)((uint)fVar12 ^ 0x80000000);
  }
  if ((iy & 2) != 0) {
    fVar10 = (float)((uint)fVar10 ^ 0x80000000);
  }
  iy = *(uint *)(&p1 + (longlong)*(int *)(&p1 + lVar2 * 4) * 4);
  ix = iy & 0xf;
  if (ix < 8) {
    fVar13 = y;
    y = x;
    if (ix < 4) goto LAB_18075f287;
  }
  else {
    fVar13 = x;
    x = y;
    if ((ix - 0xc & 0xfffffffd) == 0) goto LAB_18075f287;
  }
  fVar13 = 0.0;
  x = y;
LAB_18075f287:
  if ((iy & 1) != 0) {
    x = (float)((uint)x ^ 0x80000000);
  }
  if ((iy & 2) != 0) {
    fVar13 = (float)((uint)fVar13 ^ 0x80000000);
  }
  x = ((fVar10 + fVar12) - (fVar13 + x)) * fVar14 + fVar13 + x;
  return SUB168(CONCAT124(SUB1612(auVar5 >> 0x20,0),
                          ((((SUB164(auVar5,0) + fVar11) - (fVar9 + fVar8)) * fVar14 + fVar9 + fVar8
                           ) - x) * ((fVar7 * 6.0 - 15.0) * fVar7 + 10.0) * fVar7 * fVar7 * fVar7 +
                          x),0);
}
