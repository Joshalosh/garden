
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 1280
#define TILEMAP_WIDTH  16
#define TILEMAP_HEIGHT 16
#define TILE_SIZE 20
#define SPRITE_WIDTH 20
#define TILE_ATLAS_COUNT 17
#define WALL_ATLAS_COUNT 15
#define STACK_MAX_SIZE 4096
#define MB(x) x*1024ULL*1024ULL
#define ARENA_SIZE MB(500)
#define FRAME_SPEED 8
#define INPUT_MAX 5
#define HYPE_WORD_COUNT 12
#define HYPE_SFX_BASE 5
#define MAX_BURSTS 32
#define BG_LAYERS 8 
#define MAX_EVENTS 16
#define MAX_FADEABLES 32

const int base_screen_width  = 320;
const int base_screen_height = 320; //180;

enum Tile_Type {
    TileType_none        = 0,
    TileType_wall        = 1,
    TileType_floor       = 2,
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
    PlayerAnimator_down,
    PlayerAnimator_up,
    PlayerAnimator_left,
    PlayerAnimator_right,
    PlayerAnimator_celebration,
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

enum Event_Sequence {
    Sequence_win,
    Sequence_tutorial,
    Sequence_begin,
    Sequence_epilogue,

    Sequence_count,
};

enum End_Layers {
    EndLayer_sky,
    EndLayer_trees,
    EndLayer_count,
};

enum Atlas_Type {
    Atlas_tile,
    Atlas_wall,
    Atlas_count,
};

enum Song_Type {
    Song_play,
    Song_play_muted,
    Song_tutorial,
    Song_intro, 
    Song_win,
    Song_count,
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
    Texture2D texture;
    u32       max_frames;
    u32       current_frame;
    Rectangle frame_rec;
    bool      looping;
};

struct Tile {
    Tile_Type type;
    u32       flags;
    u32       seed;
    Vector2   pos;
    Animation animator;
};

struct Tilemap {
    u32           width;
    u32           height;
    u32           tile_size;
    Animation     fire_animation;
    Wobble_Shader wobble;

    u32          *original_map;
    Tile         *tiles;
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
    f32              blinking_duration;
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

struct Spacebar_Text {
    const char *text;
    u32         size;
    Vector2     pos;
    f32         bob;
};

struct Tutorial_Entities {
    Animation     enemy;
    Animation     powerup;
    Animation     fire;
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

struct Background_Layer {
    Texture2D texture;
    Vector2   pos;
    s32       dir;
    f32       scroll_speed;
    b32       should_wobble;
};

struct Title_Screen_Manager {
    Game_Title              title;
    Play_Text               play_text;
    Background_Layer layer[BG_LAYERS];
    Wobble_Shader    wobble;
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

struct Event_Manager {
    Event_Queue sequence[Sequence_count];
    Event_Queue original_sequence[Sequence_count];
};

struct Gui {
    Texture2D    bar;
    God_Animator face_type;
    Animation    animators[GodAnimator_count];
    f32          anim_timer;
    f32          anim_duration;
    f32          step;
    f32          face_scale;
    Vector2      face_pos;
};

struct Win_Screen {
    const char   *message;
    u32           font_size;
    Vector2       text_pos;
    Vector2       start_pos;
    Vector2       end_pos;
    Fade_Object   white_screen;
};

struct Game_Manager {
    u32           score;
    u32           happy_score;
    u32           satisfied_score;
    u32           score_multiplier;

    u32           frame_counter;
    b32           fire_cleared;
    Game_State    state;

    Texture2D     atlas[Atlas_count];
    Gui           gui;

    // Enemy controller
    f32           enemy_spawn_duration;
    f32           spawn_timer;
    f32           enemy_move_duration;
    f32           enemy_move_timer;

    // Linked list sentinels
    Enemy         enemy_sentinel;
    Powerup       powerup_sentinel;

    // Hype Sound
    f32           hype_sound_timer;
    u32           hype_prev_index;
    Sound         hype_sounds[HYPE_WORD_COUNT];
    const char   **hype_text;

    Sound         sounds[SoundEffect_count];
    Text_Burst    bursts[MAX_BURSTS];

    Music         song[Song_count];
    f32           play_song_volume;
    f32           play_muted_song_volume;
    b32           should_title_music_play;
    u32           last_song_bit;

    Screen_Shake  screen_shake;

    // Store a list of pointers to fadeable objects
    // to easily access them and iterate over them to 
    // fade their alpha values.
    Fade_Object   *fadeables[MAX_FADEABLES];
    u32           fade_count;

    Spacebar_Text spacebar_text;
};

struct StackU32 {
    u32 x[STACK_MAX_SIZE];
    u32 y[STACK_MAX_SIZE];
    s32 top;
};

