// File: Debug.h

//------------------------
#define DEBUG /**/

extern uint32_t Debug_Mask;

#define DEBUG_BUSY 1
#define DEBUG_BLOB 2
#define DEBUG_PARSER 4
#define DEBUG_SCENES 8


#define DEBUG_ALL ((uint32_t)(-1))

#ifdef DEBUG
#define D(m, x) {if ((m & Debug_Mask) == m) {x}}
#else
#define D(m, x)
#endif


// EndFile: Debug.h
