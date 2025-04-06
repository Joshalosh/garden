
#define TILEMAP_WIDTH  16
#define TILEMAP_HEIGHT 16
#define TILEMAP_SIZE 20
#define ATLAS_COUNT 17
#define ADJACENT_COUNT 4
#define MB(x) x*1024ULL*1024ULL
#define ARENA_SIZE MB(500)
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
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
    TileFlag_fire      = 1 << 0,
    TileFlag_visited   = 1 << 1,
    TileFlag_powerup   = 1 << 2,
    TileFlag_enemy     = 1 << 3,
    TileFlag_moved     = 1 << 4,
};

enum Game_State {
    GameState_play,
    GameState_lose,
    GameState_win,
};

struct GameManager {
    u32 score;
    u32 high_score;
    Game_State state;
};

struct Tile {
    Tile_Type  type;
    u32        flags;
    u32        seed;
};

struct Tilemap {
    u32  width;
    u32  height;
    u32  tile_size;

    u32  *original_map;
    Tile *tiles;
};

struct Player {
    Vector2   pos;
    Vector2   target_pos;
    Vector2   size;
    Color     col;

    f32       speed;
    f64       powerup_timer;
    f32       blink_time;
    f32       blink_speed;
    bool      is_moving;
    bool      powered_up;
    bool      col_bool;
};

struct Enemy {
    Vector2 pos;
    Vector2 target_pos;
    Vector2 size;
    Color col;

    f32 speed;
};

#define STACK_MAX_SIZE 4096
struct StackU32 {
    u32 x[STACK_MAX_SIZE];
    u32 y[STACK_MAX_SIZE];
    s32 top;
};

