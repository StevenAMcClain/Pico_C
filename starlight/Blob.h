// File: blob.h

#ifndef BLOB_H
#define BLOB_H

#include "bvar.h"

#define MAX_BLOB_SIZE (64 * 1024)    // max size for a blob (check implications for FLASH storage).

#define BLOB_VERSION "0.2d"

#define BLOB_COOKIE 0x424F4C42
#define BLOB_COOKIE_SIZE 4
#define BLOB_PRE_HEADER_SIZE 12      // Number of bytes to read before reading header.

typedef int32_t PROG_ID;
typedef uint32_t SCENE_ID;
typedef uint32_t TRIG_ID;

typedef uint32_t PROG;
typedef uint32_t SCENE;


typedef struct BLOB_Raw
{
    uint32_t Cookie;           // Must be 'BLOB'
    uint32_t Version;          // Version number for blob.
    uint32_t Size;             // Number of bytes in body.
    uint32_t Checksum;         // Checksum for body only.

	uint32_t blob_name;	        // Stringdx for blob name.

	uint32_t phystr_start;	    // Physical string definitions.
	uint32_t phystr_size;	    // Size of the phystring table.

	uint32_t strindx_start;	    // Start of the string table.
	uint32_t strindx_size;	    // Size of the string table.

	uint32_t vartab_start;	    // Start of the variable table.
	uint32_t vartab_size;	    // Size of the variable table.

	uint32_t symtab_start;	    // Start of the program symbol table
	uint32_t symtab_size;	    // Size of the program sysmbol table.

	uint32_t vstr_index;	    // Virual LED string array index starts here (relative to start).
	uint32_t vstr_count;	    // Number of Virual LED string arrays defined.

	uint32_t vstr_array;	    // Virual LED string record start here.
	uint32_t vstr_size;   	    // Size of the Virual LED string array.

    uint32_t scen_index;	    // Scene index starts here (relative to start).
    uint32_t scen_count;        // Number of scenes defined.

    uint32_t scen_array;        // Scene array starts here (relative to start).
    uint32_t scen_size;		    // Size of scene array.

    uint32_t trig_start;	    // Trigger array starts here (relative to start).
    uint32_t trig_size;		    // Number of triggers defined.

    uint32_t prog_start;	    // Program array starts here (relative to start).
    uint32_t prog_size;		    // Size of program array.

} BLOB_RAW;


typedef struct Blob
{
    uint8_t *name;          // Points to blob name string.
    BLOB_RAW* Blob_BASE;    // Points to raw blob.

    uint8_t* Blob_Bin;		// Pointer to start of blob binary data.

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

    SCENE*   Scene_Array;	// Scene array start here.
    uint32_t Scene_Size;	// Sizeof the scene array.

    PROG_ID* Trigger_Base;  // Start of the trigger table.
    uint32_t Num_Trig;		// Number of trigger records.

    PROG*    Program_Base;	// Blob Programs start here.
    uint32_t Num_Prog;		// Number of PROG records.

    BLOB_VAR* VarTab_Base;	// Variable table starts here.
    uint32_t Num_VarRecs;	// Number of variable records.

} BLOB;

extern volatile bool Blob_Is_Loaded;        // Set to true when blob is loaded and ready.

extern BLOB Blob;       // System blob.

extern uint32_t Version();
extern char* version_to_str(char* buff, uint32_t val);

extern void Blob_Init(void);                    // Prepare BLOB for use.  Call once at startup.

extern uint8_t* Blob_Base_Get_New(void);        // Prepare new base for load return pointer to BLOB base.
extern void Blob_Base_Switch(void);             // Switch to new base
extern BLOB_RAW* Blob_Base_Current(void);       // Returns pointer to current BLOB base.

extern PROG* Prog_Ptr(PROG_ID id);
extern PROG_ID Prog_Id(PROG *cmdp);
extern PROG_ID Get_Trigger_Prog(TRIG_ID id);

extern uint32_t Checksum(uint8_t* buff, size_t size);
extern bool Blob_Verify_Checksum(uint8_t* base);
extern bool Blob_Verify_Checksum_Loaded(void);

extern bool Unpack_Blob_Header(uint8_t* /* blob_base */);   // Call to load a new blob_base into BLOB.
extern void Blob_Unload(void);                              // Release blob_base memory.

#endif // BLOB_H

// Endfile: blob.h
