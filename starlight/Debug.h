// File: Debug.h

//------------------------
#define DEBUG /**/

extern uint32_t Debug_Mask;

#define DEBUG_BUSY      BIT(0)
#define DEBUG_BLOB      BIT(1)
#define DEBUG_PARSER    BIT(2)
#define DEBUG_SCENES    BIT(3)
#define DEBUG_BLUETOOTH BIT(4)

#define DEBUG_ALL ((uint32_t)(-1))

#ifdef DEBUG
#define D(m, x) {if ((m & Debug_Mask) == m) {x}}
#else
#define D(m, x)
#endif


// EndFile: Debug.h
