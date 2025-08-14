// File: Scene.h

typedef uint32_t SCENE_ID;
typedef uint32_t SCENE;

#define FLAGS_END      0xFF
#define FLAGS_END_ALL  0xFE
#define FLAGS_END_LAST 0xFD
#define FLAGS_SKIP     0xFC


// Render scene given pointer.
//
extern void Render_Scene(uint32_t* start_ptr);


// Render scene given scene id.
//
extern void Set_Scene(SCENE_ID id);

// Endfile: Scene.h
