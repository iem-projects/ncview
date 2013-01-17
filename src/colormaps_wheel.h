/* Periodic over 360-degree hues color palette.  Maximum saturation and brightness.
    Useful for angles of orientation, tidal phase, and similar data. */

static int cmap_wheel[] = { 255,  0,255, 249,  0,255, 242,  0,255, 237,  0,255,
   230,  0,255, 224,  0,255, 219,  0,255, 212,  0,255, 206,  0,255, 201,  0,255, 195,  0,255,
   188,  0,255, 183,  0,255, 177,  0,255, 170,  0,255, 165,  0,255, 159,  0,255, 153,  0,255,
   147,  0,255, 140,  0,255, 135,  0,255, 128,  0,255, 122,  0,255, 117,  0,255, 110,  0,255,
   104,  0,255,  99,  0,255,  93,  0,255,  86,  0,255,  81,  0,255,  75,  0,255,  68,  0,255,
    63,  0,255,  57,  0,255,  51,  0,255,  45,  0,255,  38,  0,255,  33,  0,255,  26,  0,255,
    20,  0,255,  15,  0,255,   8,  0,255,   2,  0,255,   0,  2,255,   0,  8,255,   0, 15,255,
     0, 20,255,   0, 26,255,   0, 33,255,   0, 38,255,   0, 44,255,   0, 51,255,   0, 56,255,
     0, 62,255,   0, 68,255,   0, 75,255,   0, 80,255,   0, 86,255,   0, 93,255,   0, 98,255,
     0,104,255,   0,110,255,   0,116,255,   0,122,255,   0,128,255,   0,135,255,   0,141,255,
     0,147,255,   0,153,255,   0,159,255,   0,165,255,   0,170,255,   0,177,255,   0,183,255,
     0,189,255,   0,195,255,   0,201,255,   0,207,255,   0,212,255,   0,219,255,   0,225,255,
     0,230,255,   0,237,255,   0,243,255,   0,249,255,   0,255,255,   0,255,248,   0,255,242,
     0,255,237,   0,255,230,   0,255,224,   0,255,219,   0,255,212,   0,255,206,   0,255,201,
     0,255,195,   0,255,188,   0,255,183,   0,255,177,   0,255,170,   0,255,165,   0,255,158,
     0,255,153,   0,255,146,   0,255,140,   0,255,135,   0,255,128,   0,255,122,   0,255,117,
     0,255,110,   0,255,104,   0,255, 99,   0,255, 93,   0,255, 86,   0,255, 81,   0,255, 75,
     0,255, 68,   0,255, 63,   0,255, 56,   0,255, 51,   0,255, 45,   0,255, 38,   0,255, 33,
     0,255, 26,   0,255, 20,   0,255, 15,   0,255,  8,   0,255,  2,   3,255,  0,   9,255,  0,
    15,255,  0,  21,255,  0,  27,255,  0,  33,255,  0,  39,255,  0,  45,255,  0,  51,255,  0,
    57,255,  0,  63,255,  0,  69,255,  0,  75,255,  0,  81,255,  0,  87,255,  0,  93,255,  0,
    99,255,  0, 105,255,  0, 111,255,  0, 117,255,  0, 123,255,  0, 129,255,  0, 135,255,  0,
   141,255,  0, 147,255,  0, 153,255,  0, 159,255,  0, 165,255,  0, 171,255,  0, 177,255,  0,
   183,255,  0, 189,255,  0, 195,255,  0, 201,255,  0, 207,255,  0, 212,255,  0, 219,255,  0,
   225,255,  0, 231,255,  0, 237,255,  0, 243,255,  0, 249,255,  0, 255,254,  0, 255,248,  0,
   255,242,  0, 255,236,  0, 255,230,  0, 255,224,  0, 255,218,  0, 255,212,  0, 255,206,  0,
   255,200,  0, 255,194,  0, 255,188,  0, 255,182,  0, 255,176,  0, 255,170,  0, 255,164,  0,
   255,158,  0, 255,152,  0, 255,146,  0, 255,140,  0, 255,134,  0, 255,128,  0, 255,122,  0,
   255,116,  0, 255,110,  0, 255,104,  0, 255, 98,  0, 255, 92,  0, 255, 86,  0, 255, 80,  0,
   255, 74,  0, 255, 68,  0, 255, 62,  0, 255, 56,  0, 255, 50,  0, 255, 44,  0, 255, 38,  0,
   255, 32,  0, 255, 26,  0, 255, 20,  0, 255, 14,  0, 255,  8,  0, 255,  2,  0, 255,  0,  2,
   255,  0,  9, 255,  0, 15, 255,  0, 20, 255,  0, 26, 255,  0, 33, 255,  0, 38, 255,  0, 45,
   255,  0, 51, 255,  0, 56, 255,  0, 63, 255,  0, 68, 255,  0, 75, 255,  0, 81, 255,  0, 86,
   255,  0, 92, 255,  0, 99, 255,  0,104, 255,  0,111, 255,  0,117, 255,  0,122, 255,  0,128,
   255,  0,135, 255,  0,140, 255,  0,147, 255,  0,153, 255,  0,158, 255,  0,165, 255,  0,170,
   255,  0,177, 255,  0,183, 255,  0,188, 255,  0,194, 255,  0,201, 255,  0,206, 255,  0,213,
   255,  0,219, 255,  0,224, 255,  0,230, 255,  0,237, 255,  0,242, 255,  0,249, 255,  0,255};
