#ifndef _MD_PHIL_H_
#define _MD_PHIL_H_

#define MD_UNTOUCHED   (0xF303F)

#define MD_UP_MASK     (1)        //  0x00001
#define MD_DOWN_MASK   (1 << 1)   //  0x00002
#define MD_LEFT_MASK   (1 << 2)   //  0x00004
#define MD_RIGHT_MASK  (1 << 3)   //  0x00008
#define MD_B_MASK      (1 << 4)   //  0x00010
#define MD_C_MASK      (1 << 5)   //  0x00020
#define MD_A_MASK      (1 << 12)  //  0x01000
#define MD_START_MASK  (1 << 13)  //  0x02000
#define MD_Z_MASK      (1 << 16)  //  0x10000
#define MD_Y_MASK      (1 << 17)  //  0x20000
#define MD_X_MASK      (1 << 18)  //  0x40000
#define MD_MODE_MASK   (1 << 19)  //  0x80000

#endif // _MD_PHIL_H_

