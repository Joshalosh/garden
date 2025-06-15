
#define TILEMAP_WIDTH  16
#define TILEMAP_HEIGHT 16
#define TILE_SIZE 20
#define ATLAS_COUNT 17
#define ADJACENT_COUNT 4
#define MB(x) x*1024ULL*1024ULL
#define ARENA_SIZE MB(500)
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define FRAME_SPEED 16
#define INPUT_MAX 5
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

enum Direction_Facing {
    DirectionFacing_down,
    DirectionFacing_up,
    DirectionFacing_left,
    DirectionFacing_right,
    DirectionFacing_none,
};

struct Animation {
    Texture2D texture[10];
    u32 max_frames;
    u32 current_frame;
    Rectangle frame_rec;
};

struct Tile {
    Tile_Type  type;
    u32        flags;
    u32        seed;
    Vector2    tile_pos;
    Animation  animator;
};

struct Tilemap {
    u32  width;
    u32  height;
    u32  tile_size;

    u32  *original_map;
    Tile *tiles;
};

struct Input_Buffer {
    Direction_Facing inputs[INPUT_MAX];
    u32 start;
    u32 end;

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

    Animation        animator;
    Direction_Facing facing;
    Input_Buffer     input_buffer;
};

struct Enemy {
    u32 tile_index;
    Animation animator;
    Enemy *next;
    Enemy *prev;
};

struct GameManager {
    u32 score;
    u32 high_score;
    Game_State state;

    // Enemy controller
    f32 enemy_spawn_duration;
    f32 spawn_timer;
    f32 enemy_move_duration;
    f32 enemy_move_timer;
};

#define STACK_MAX_SIZE 4096
struct StackU32 {
    u32 x[STACK_MAX_SIZE];
    u32 y[STACK_MAX_SIZE];
    s32 top;
};

