
#define TILEMAP_WIDTH  16
#define TILEMAP_HEIGHT 16
#define TILE_SIZE 20
#define SPRITE_WIDTH 20
#define TILE_ATLAS_COUNT 17
#define WALL_ATLAS_COUNT 15
#define ADJACENT_COUNT 4
#define STACK_MAX_SIZE 4096
#define MB(x) x*1024ULL*1024ULL
#define ARENA_SIZE MB(500)
#define FRAME_SPEED 8
#define INPUT_MAX 5
#define HYPE_WORD_COUNT 12
#define MAX_BURSTS 32
#define BG_LAYERS 8 
#define MAX_EVENTS 16
#define MAX_FADEABLES 32

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
    GameState_win_text,
    GameState_epilogue,
    GameState_tutorial,
};

enum Direction_Facing {
    DirectionFacing_down        = 0,
    DirectionFacing_up          = 1,
    DirectionFacing_left        = 2,
    DirectionFacing_right       = 3,
    DirectionFacing_celebration = 4,
    DirectionFacing_none        = 5,
};

enum Player_Animator {
    PlayerAnimator_body,
    PlayerAnimator_water,
    PlayerAnimator_count,
};

enum Enemy_Animator {
    EnemyAnimator_idle,
    EnemyAnimator_destroy,
    EnemyAnimator_count,
};

enum God_Animator{
    GodAnimator_angry,
    GodAnimator_satisfied,
    GodAnimator_happy,
    GodAnimator_count,
};

enum Fade_Type {
    FadeType_none,
    FadeType_in,
    FadeType_out,
};

enum Event_Type {
    EventType_none,
    EventType_wait,
    EventType_fade_out,
    EventType_fade_in,
    EventType_state_change,
};

enum End_Layers {
    EndLayer_sky,
    EndLayer_trees,
    EndLayer_count,
};


struct Play_Text {
    const char *text;
    u32         font_size;
    f32         bob;
    Vector2     pos;
};

struct Fade_Object {
    f32       alpha;
    f32       duration;
    f32       timer;
    Fade_Type fade_type;
};

struct Animation {
    Texture2D texture[10];
    u32       max_frames;
    u32       current_frame;
    Rectangle frame_rec;
    bool      looping;
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

    Animation        animators[PlayerAnimator_count];
    Direction_Facing facing;
    Input_Buffer     input_buffer;
};

struct Enemy {
    u32        tile_index;
    Animation  animators[EnemyAnimator_count];
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
    SoundEffect_spacebar,

    SoundEffect_count,
};

struct Tutorial_Entities {
    Animation   enemy;
    Animation   powerup;
    Animation   fire;
    const char *text;
    u32         font_size;
    Vector2     text_pos;
    f32         bob;
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
    f32       bob_velocity;
};

struct Title_Screen_Background {
    Texture2D texture[BG_LAYERS];
    f32       scroll_speed;
    Vector2   pos_left_1;
    Vector2   pos_left_2;
    Vector2   pos_right_1;
    Vector2   pos_right_2;
};

struct Title_Screen_Manager {
    Game_Title              title;
    Play_Text               play_text;
    Title_Screen_Background bg;
};

struct End_Screen {
    Texture2D     textures[EndLayer_count];
    Wobble_Shader shaders[EndLayer_count];
    Animation     animator;
    f32           timer;
    f32           blink_duration;
};

struct Screen_Shake {
    f32 intensity;
    f32 duration;
    f32 decay;
};

struct Event {
    Event_Type  type;
    f32         duration;
    u32         new_state;
    Fade_Object fadeable;
};

struct Event_Queue {
    Event events[MAX_EVENTS];
    u32   count;
    u32   index;
    f32   timer;
    bool  active;
};

struct Gui {
    Texture2D    bar;
    Animation    animators[GodAnimator_count];
    f32          anim_timer;
    f32          anim_duration;
};

struct Game_Manager {
    u32          score;
    u32          high_score;
    u32          score_multiplier;
    Game_State   state;

    Gui          gui;

    // Enemy controller
    f32          enemy_spawn_duration;
    f32          spawn_timer;
    f32          enemy_move_duration;
    f32          enemy_move_timer;

    // Linked list sentinels
    Enemy        enemy_sentinel;
    Powerup      powerup_sentinel;

    // Hype Sound
    f32          hype_sound_timer;
    u32          hype_prev_index;
    Sound        hype_sounds[HYPE_WORD_COUNT];

    Sound        sounds[SoundEffect_count];
    Text_Burst   bursts[MAX_BURSTS];

    Screen_Shake screen_shake;

    // For fadeable objects
    Fade_Object  *fadeables[MAX_FADEABLES];
    u32          fade_count;

};

struct StackU32 {
    u32 x[STACK_MAX_SIZE];
    u32 y[STACK_MAX_SIZE];
    s32 top;
};

