
#define TILEMAP_WIDTH  16
#define TILEMAP_HEIGHT 16
#define MB(x) x*1024ULL*1024ULL
#define ARENA_SIZE MB(500)
const int base_screen_width  = 320;
const int base_screen_height = 320; //180;

enum Tile_Type {
    TileType_none        = 0,
    TileType_wall        = 1,
    TileType_grass       = 2,
    TileType_dirt        = 3,
    TileType_wall2       = 4,
    TileType_fire        = 5,
    TileType_temp_grass  = 6,
    TileType_temp_dirt   = 7,
};

enum Tile_Flags {
    TileFlag_fire    = 1 << 0,
    TileFlag_visited = 1 << 1,
};

struct Tile {
    Tile_Type  type;
    Tile_Flags flags;
};

struct Tilemap {
    u32 width;
    u32 height;
    u32 tile_size;

    Tile tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH];
};

struct Player {
    Vector2   pos;
    Vector2   target_pos;
    Vector2   size;

    float     speed;
    bool      is_moving;
    // TODO: Do we really need path still? Get rid of it if not
    Vector2   path[TILEMAP_WIDTH*TILEMAP_HEIGHT];
    u32       path_len;
};

#define STACK_MAX_SIZE 4096
struct StackU32 {
    u32 x[STACK_MAX_SIZE];
    u32 y[STACK_MAX_SIZE];
    s32 top;
};

