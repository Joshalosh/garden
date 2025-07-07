
#define TILEMAP_WIDTH  16
#define TILEMAP_HEIGHT 16
#define TILE_SIZE 20
#define SPRITE_SIZE 20
#define TILE_ATLAS_COUNT 17
#define WALL_ATLAS_COUNT 15
#define ADJACENT_COUNT 4
#define STACK_MAX_SIZE 4096
#define MB(x) x*1024ULL*1024ULL
#define ARENA_SIZE MB(500)
#define FRAME_SPEED 16
#define INPUT_MAX 5
#define MAX_PLAYER_ANIMATORS 2
#define HYPE_WORD_COUNT 12
#define MAX_BURSTS 32

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
    GameState_title,
};

enum Direction_Facing {
    DirectionFacing_down  = 0,
    DirectionFacing_up    = 1,
    DirectionFacing_left  = 2,
    DirectionFacing_right = 3,
    DirectionFacing_none  = 4,
};

enum Player_Animator {
    PlayerAnimator_body  = 0,
    PlayerAnimator_water = 1,
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
    Vector2    pos;
    Animation  animator;
};

struct Tilemap {
    u32   width;
    u32   height;
    u32   tile_size;

    u32  *original_map;
    Tile *tiles;
};

struct Input_Buffer {
    Direction_Facing inputs[INPUT_MAX];
    u32              start;
    u32              end;

};

struct Player {
    Vector2          pos;
    Vector2          target_pos;
    Vector2          size;
    Color            col;

    f32              speed;
    f64              powerup_timer;
    f32              time_between_blinks;
    f32              blink_speed;
    bool             is_moving;
    bool             powered_up;
    bool             col_bool;

    Animation        animators[MAX_PLAYER_ANIMATORS];
    Direction_Facing facing;
    Input_Buffer     input_buffer;
};

struct Enemy {
    u32        tile_index;
    Animation  animator;
    Enemy     *next;
    Enemy     *prev;
};

struct Powerup {
    Tile      *tile;
    Animation  animator;
    Powerup   *next;
    Powerup   *prev;
};

enum Sound_Index {
    SoundEffect_powerup,
    SoundEffect_powerup_end,
    SoundEffect_powerup_collect,
    SoundEffect_powerup_appear,

    SoundEffect_count,
};

struct Text_Burst {
    const char *text;
    Vector2     pos;
    f32         alpha;
    f32         scale;
    f32         max_scale;
    f32         lifetime;
    f32         age;
    Vector2     drift;
    bool        active;
};

struct Game_Title {
    Texture2D texture;
    u32       scale;
    Vector2   pos;
    f32       bob;
};

struct Title_Screen_Background {
    Texture2D texture;
    f32       scroll_speed;
    Vector2   initial_pos;
    Vector2   secondary_pos;
};

struct Title_Screen_Manager {
    Game_Title title;
    Title_Screen_Background bg;
};

struct Game_Manager {
    u32         score;
    u32         high_score;
    Game_State  state;
    f32         bg_scroll_time;


    // Enemy controller
    f32         enemy_spawn_duration;
    f32         spawn_timer;
    f32         enemy_move_duration;
    f32         enemy_move_timer;

    // Linked list sentinels
    Enemy       enemy_sentinel;
    Powerup     powerup_sentinel;

    // Hype Sound
    f32         hype_sound_timer;
    u32         hype_prev_index;
    Sound       hype_sounds[HYPE_WORD_COUNT];

    Sound       sounds[SoundEffect_count];
    Text_Burst  bursts[MAX_BURSTS];
};

struct StackU32 {
    u32 x[STACK_MAX_SIZE];
    u32 y[STACK_MAX_SIZE];
    s32 top;
};

