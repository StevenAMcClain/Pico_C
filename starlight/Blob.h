// File: Blob.h

#define MAX_BLOB_SIZE (64 * 1024)    // max size for a blob (check implications for FLASH storage).

#define BLOB_VERSION "0.2d"

typedef uint32_t TRIG_ID;
typedef int32_t PROG_ID;
typedef uint32_t SCENE_ID;

typedef uint32_t PROG;
typedef uint32_t SCENE;


typedef struct BLOB_Pre_Header
{
    uint32_t Cookie;                // Must be 'BLOB'
    uint32_t Version;               // Version number for blob.
    uint32_t Size;                  // Number of bytes in body.
    uint32_t Checksum;              // Checksum for body only.

} BLOB_PRE_HEADER;


typedef struct Blob
{
    uint8_t *name;

    uint8_t* Blob_Base;		// Pointer to base of Blob.
    uint32_t Blob_Size;		// Number of bytes in Blob.

    uint8_t* StrindX;       // Point to base of string table.
    uint32_t StrindX_Size;

    void* SymTab;           // Pointer to start of symbol table.
    uint32_t SymTab_Size;

    void* VStr_Index;       // Pointer to base of virtual string index.
    uint32_t VStr_Index_Size;

    void* VStr_Array;       // Pointer to base of virtual string array.
    uint32_t VStr_Array_Size;

    SCENE_ID* Scene_Index;	// Scene index start here.
    uint32_t Num_Scenes;	// Number of scenes defined.

    SCENE*    Scene_Array;	// Scene array start here.
    uint32_t Scene_Size;	// Sizeof the scene array.

    PROG_ID* Trigger_Base;  // Start of the trigger table.
    uint32_t Num_Trig;		// Number of trigger records.

    PROG*    Program_Base;	// Blob Programs start here.
    uint32_t Num_Prog;		// Number of PROG records.

} BLOB;

extern BLOB Blob;
extern const int Tick_Speed;                        // Clock for blob player.
extern bool Blob_Is_Loaded;

extern void Blob_Init(void);                        // Prepare BLOB for use.  Call once at startup.

extern uint8_t* Get_New_Blob_Base(void);            // Switch to other base and return pointer to BLOB base.
extern uint8_t* Get_Blob_Base(void);                // Returns pointer to current BLOB base.

extern void Blob_NumLeds(int n);                    // Set number of LEDS.   Call when Num_Leds changes.
extern bool Unpack_Blob_Header(uint8_t* /* blob_base */);    // Call to load a new blob_base.
extern void Blob_Unload(void);                      // Release blob_base memory.

extern void Blob_Trigger(TRIG_ID n);                // Immediately Start playing a BLOB program.  (Cancel any running or queue'd)
extern void Blob_Queue_Next(TRIG_ID n);             // Start playing a BLOB program (after current completes).
extern void Blob_Stop(void);                        // Stop current program and clear queue.

extern uint32_t Version();
extern char* version_to_str(char* buff, uint32_t val);

// extern uint32_t Blobhead();
// extern uint32_t Version();

// extern uint32_t str_to_uint32(char* str);
// extern char* uint32_to_str(char* buff, uint32_t val);


// Endfile: Blob.h
