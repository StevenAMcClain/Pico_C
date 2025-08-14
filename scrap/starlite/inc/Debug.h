// File: Debug.h

//------------------------
#define DEBUG /**/

extern volatile uint32_t Debug_Mask;

#define DEBUG_PRINTF    BIT(0)
#define DEBUG_BUSY      BIT(1)
#define DEBUG_BLOB      BIT(2)
#define DEBUG_PARSER    BIT(3)
#define DEBUG_SCENES    BIT(4)
#define DEBUG_BLUETOOTH BIT(5)

#define DEBUG_ALL ((uint32_t)(-1))

#ifdef DEBUG
#define D(m, x) {if ((m & Debug_Mask) == m) {x}}
#else
#define D(m, x)
#endif

extern void dump_ascii(char* head, uint8_t* buffp, size_t size);

// EndFile: Debug.h
