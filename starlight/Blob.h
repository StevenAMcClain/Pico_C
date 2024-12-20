// File: Blob.h

#define BLOB_VERSION "0.2a"

typedef uint32_t TRIG_ID;
typedef int32_t PROG_ID;
typedef uint32_t SCENE_ID;

typedef uint32_t PROG;
typedef uint32_t SCENE;


typedef struct Blob
{
    uint8_t* Blob_Base;		// Pointer to base of Blob.
    uint32_t Blob_Size;		// Number of bytes in Blob.

    PROG_ID* Trigger_Base;  // Start of the trigger table.
    PROG*    Program_Base;	// Blob Programs start here.

    SCENE_ID* Scene_Index;	// Scene index start here.
    SCENE*    Scene_Array;	// Scene array start here.

	uint32_t Num_Leds;		// Number of leds defined.

    uint32_t Num_Trig;		// Number of trigger records.
    uint32_t Num_Prog;		// Number of PROG records.
    uint32_t Num_Scenes;	// Number of scenes defined.
    uint32_t Scene_Size;	// Sizeof the scene array.

} BLOB;

extern BLOB Blob;
extern const int Tick_Speed;                        // Clock for blob player.

extern void Blob_Init(void);                        // Prepare BLOB for use.  Call once at startup.
extern void Blob_NumLeds(int n);                    // Set number of LEDS.   Call when Num_Leds changes.
extern void Blob_Load(uint8_t* /* blob_base */);    // Call to load a new blob_base.
extern void Blob_Unload(void);                      // Release blob_base memory.

extern void Blob_Trigger(TRIG_ID n);                // Immediately Start playing a BLOB program.  (Cancel any running or queue'd)
extern void Blob_Queue_Next(TRIG_ID n);             // Start playing a BLOB program (after current completes).
extern void Blob_Stop(void);                        // Stop current program and clear queue.

// Endfile: Blob.h
