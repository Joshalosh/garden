
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "mymath.h"
#include "memory.h"
#include "garden.h"

void TilemapInit(Tilemap *tilemap) {
    tilemap->width       = TILEMAP_WIDTH;
    tilemap->height      = TILEMAP_HEIGHT;
    tilemap->tile_size   = TILE_SIZE;
}

void StackInit(StackU32 *stack) {
    stack->top = -1;
}

bool StackPush(StackU32 *stack, u32 x_val, u32 y_val) {
    if (stack->top >= STACK_MAX_SIZE - 1) return false;
    stack->top++;
    stack->x[stack->top] = x_val;
    stack->y[stack->top] = y_val;
    return true;
}

bool StackPop(StackU32 *stack, u32 *x_val, u32 *y_val) {
    if (stack->top < 0) return false;
    *x_val = stack->x[stack->top];
    *y_val = stack->y[stack->top];
    stack->top--;
    return true;
}

u32 TilemapIndex(u32 x, u32 y, u32 width) {
    u32 result = y * width + x;
    return result;
}

void TileSeedInit(Tile *tile) {
    // TODO: Need to consolidate tiletype_grass/dirt and tiletype_wall/wall2 
    // and any other tile types I doubled up for grid based drawing
    switch (tile->type) {
        case TileType_dirt:
        case TileType_grass: tile->seed = GetRandomValue(0, TILE_ATLAS_COUNT - 1); break;
        case TileType_wall:
        case TileType_wall2: tile->seed = GetRandomValue(0, WALL_ATLAS_COUNT - 1); break;
        default:             tile->seed = 0;                                       break;
    }
}

void TileInit(Tilemap *tilemap, Animation fire_animator) {
    for (u32 y = 0; y < tilemap->height; y++) {
        for (u32 x = 0; x < tilemap->width; x++) {
            u32 index      = TilemapIndex(x, y, tilemap->width);
            Tile *tile     = &tilemap->tiles[index];
            tile->type     = (Tile_Type)tilemap->original_map[index];
            tile->flags    = 0;
            tile->animator = fire_animator;
            TileSeedInit(tile);
        }
    }
}

void TileInit(Tilemap *tilemap) {
    for (u32 y = 0; y < tilemap->height; y++) {
        for (u32 x = 0; x < tilemap->width; x++) {
            u32 index      = TilemapIndex(x, y, tilemap->width);
            Tile *tile     = &tilemap->tiles[index];
            tile->type     = (Tile_Type)tilemap->original_map[index];
            tile->flags    = 0;
            TileSeedInit(tile);
        }
    }
}

void PlayerInit(Player *player) {
    player->pos                 = {base_screen_width*0.5, base_screen_height*0.5};
    player->target_pos          = player->pos;
    player->size                = {20, 20};
    player->col                 = WHITE;
    player->speed               = 75.0f;
    player->is_moving           = false;
    player->powered_up          = false;
    player->powerup_timer       = 0;
    player->blink_speed         = 0;
    player->time_between_blinks = 0;
    player->col_bool            = false;
    player->facing              = DirectionFacing_down;
    //player->queued_facing = DirectionFacing_none;
    for(u32 index = 0; index < INPUT_MAX; index++) {
        player->input_buffer.inputs[index] = DirectionFacing_down;
    }
    player->input_buffer.start = 0;
    player->input_buffer.end   = 0;

}

void GameManagerInit(GameManager *manager) {
    manager->score                = 0;
    manager->high_score           = 0;
    manager->state                = GameState_play;

    manager->enemy_spawn_duration = 500.0f;
    manager->spawn_timer          = manager->enemy_spawn_duration;
    manager->enemy_move_duration  = 250.0f;
    manager->enemy_move_timer     = manager->enemy_move_duration;
}

void LoadSoundBuffer(Sound *sounds) {
    sounds[SoundEffect_powerup]         = LoadSound("../assets/sounds/powerup.wav");
    sounds[SoundEffect_powerup_end]     = LoadSound("../assets/sounds/powerup_end.wav");
    sounds[SoundEffect_powerup_collect] = LoadSound("../assets/sounds/powerup_collect.wav");
    sounds[SoundEffect_powerup_appear]  = LoadSound("../assets/sounds/powerup_appear.wav");
}

void StopSoundBuffer(Sound *sounds) {
    for (u32 index = 0; index < SoundEffect_count; index++) {
        StopSound(sounds[index]);
    }
}

void EnemyInit(Enemy *enemy, u32 tile_index) {
    enemy->tile_index = tile_index;
    enemy->animator.texture[0]    = LoadTexture("../assets/sprites/enemy.png");
    enemy->animator.max_frames    = (f32)enemy->animator.texture[0].width/20;
    enemy->animator.current_frame = 0;
    enemy->animator.frame_rec     = {0, 0, 
                                     (f32)enemy->animator.texture[0].width/enemy->animator.max_frames,
                                     (f32)enemy->animator.texture[0].height};
    enemy->next = 0;
    enemy->prev = 0;
}

Enemy *FindEnemyInList(Enemy *sentinel, u32 index)
{
    Enemy *enemy_to_find = sentinel->next;
    bool enemy_found = false;
    while (enemy_to_find != sentinel) {
        if (enemy_to_find->tile_index == index) {
            enemy_found = true;
            break;
        } else {
            enemy_to_find = enemy_to_find->next;
        }
    }

    if (!enemy_found) {
        enemy_to_find = NULL;
    }

    return enemy_to_find;
}

void DeleteEnemyInList(Enemy *sentinel, u32 index)
{
    Enemy *enemy_to_delete = sentinel->next;
    bool enemy_found = false;
    while (enemy_to_delete != sentinel) {
        if (enemy_to_delete->tile_index == index) {
            enemy_to_delete->prev->next = enemy_to_delete->next;
            enemy_to_delete->next->prev = enemy_to_delete->prev;
            enemy_found = true;
            break;
        } else {
            enemy_to_delete = enemy_to_delete->next;
        }
    }
    ASSERT(enemy_found);
}

// TODO: Instead of passing the sentinel which is a global, I could maybe pack it into
// the GameManager and then figure out the memory storage for it?
// TODO: Need to set make sure the audio completely stops here perhaps put all the sounds into a sound 
// buffer and loop through and stop all sounds... or close the audio device and re-init... but then I 
// will have to load in all the sounds again.
void GameOver(Player *player, Tilemap *tilemap,  Enemy *sentinel, GameManager *manager) {
    PlayerInit(player);
    if (manager->score > manager->high_score) {
        manager->high_score = manager->score;
    }
    manager->score         = 0;
    manager->state         = GameState_play;

    StopSoundBuffer(manager->sounds);

    sentinel->next = sentinel;
    sentinel->prev = sentinel;

    // Reset the tilemap back to it's original orientation
    TileInit(tilemap);
}

inline void AddFlag(Tile *tile, u32 flag) {
    tile->flags |= flag;
}

inline void ClearFlag(Tile *tile, u32 flag) {
    tile->flags &= ~flag;
}

b32 IsFlagSet(Tile *tile, u32 flag) {
    b32 result = tile->flags & flag;
    return result;
}

void FloodFillFromPlayerPosition(Tilemap *tilemap, u32 start_x, u32 start_y) {
    StackU32 nodes;
    StackInit(&nodes);

    StackPush(&nodes, start_x, start_y);

    u32 x, y;

    // Flood fill from players position
    while(StackPop(&nodes, &x, &y)) {
        if (x >= (u32)tilemap->width || y >= tilemap->height) continue;

        u32 index = TilemapIndex(x, y, tilemap->width);
        Tile *tile = &tilemap->tiles[index];

        if (IsFlagSet(tile, TileFlag_visited))                           continue;
        if (tile->type != TileType_grass && tile->type != TileType_dirt) continue;
        if (IsFlagSet(tile, TileFlag_fire))                              continue;
        //if (x == start_x && y == start_y) continue;

        AddFlag(tile, TileFlag_visited);

        // Add adjacent tiles 
        StackPush(&nodes, x+1, y);
        StackPush(&nodes, x-1, y);
        StackPush(&nodes, x, y+1);
        StackPush(&nodes, x, y-1);
    }
} 

u32 GetRandomEmptyTileIndex(Tilemap *tilemap) {
    bool found_empty_tile = false;
    u32 index             = 0;
    u32 attempts          = 10;

    while (!found_empty_tile && attempts != 0) {
        u32 random_x = GetRandomValue(1, tilemap->width - 2);
        u32 random_y = GetRandomValue(1, tilemap->height - 2);

        index = TilemapIndex(random_x, random_y, tilemap->width);
        Tile *tile = &tilemap->tiles[index];

        if (!IsFlagSet(tile, TileFlag_fire) && !IsFlagSet(tile, TileFlag_powerup) && 
            !IsFlagSet(tile, TileFlag_enemy)) {
            found_empty_tile = true;
        } else {
            attempts--;
            index = 0;
        }
    }

    return index;
}

u32 GetRandomEmptyTileIndex(Tilemap *tilemap, u32 tile_index) {
    bool found_empty_tile = false;
    u32 index             = 0;
    u32 attempts          = 10;

    u32 right_tile  = tile_index + 1;
    u32 left_tile   = tile_index - 1;
    u32 bottom_tile = tile_index + tilemap->width;
    u32 top_tile    = tile_index - tilemap->height;

    while (!found_empty_tile && attempts != 0) {
        u32 random_x = GetRandomValue(1, tilemap->width - 2);
        u32 random_y = GetRandomValue(1, tilemap->height - 2);

        index = TilemapIndex(random_x, random_y, tilemap->width);
        Tile *tile = &tilemap->tiles[index];

        if (!IsFlagSet(tile, TileFlag_fire) && !IsFlagSet(tile, TileFlag_powerup) && 
            !IsFlagSet(tile, TileFlag_enemy)) {
            if (index != right_tile  && index != left_tile &&
                index != bottom_tile && index != top_tile) {
                found_empty_tile = true;
            } else {
                attempts--;
                index = 0;
            }
        }
    }

    return index;
}

u32 FindEligibleTileIndexForEnemyMove(Tilemap *tilemap, u32 index) {
    u32 right_tile   = index + 1;
    u32 left_tile    = index - 1;
    u32 bottom_tile  = index + tilemap->width;
    u32 top_tile     = index - tilemap->width;

    u32 adjacent_tile_indexes[ADJACENT_COUNT] = {right_tile, left_tile, bottom_tile, top_tile};
    u32 eligible_tiles[ADJACENT_COUNT] = {};
    u32 eligible_count = 0;
    u32 result = 0;

    for (int adjacent_index = 0; adjacent_index < ARRAY_COUNT(adjacent_tile_indexes); adjacent_index++) {
        Tile *tile = &tilemap->tiles[adjacent_tile_indexes[adjacent_index]];

        if(tile->type == TileType_grass || tile->type == TileType_dirt) {
            if (!IsFlagSet(tile, TileFlag_fire) && !IsFlagSet(tile, TileFlag_powerup) && 
                !IsFlagSet(tile, TileFlag_enemy)) {
                eligible_tiles[eligible_count] = adjacent_tile_indexes[adjacent_index];
                eligible_count++;
            }
        }
    }

    if (eligible_count) {
        u32 eligible_index = eligible_count - 1;
        u32 random_index = GetRandomValue(0, eligible_index);
        result = eligible_tiles[random_index];
    }

    return result;
}

void CheckEnclosedAreas(Tilemap *tilemap, Player *player, Enemy *sentinel, Sound *sounds,
                        u32 current_x, u32 current_y) {
    // Flood fill from borders to mark reachable areas
    FloodFillFromPlayerPosition(tilemap, current_x, current_y);

    bool has_flood_fill_happened = false;
    u32 enemy_slain              = 0;
    // Any grass or dirt tiles not marked are enclosed
    for (u32 y = 0; y < (u32)tilemap->height; y++) {
        for (u32 x = 0; x < (u32)tilemap->width; x++) {
            u32 index = TilemapIndex(x, y, tilemap->width);
            Tile *tile  = &tilemap->tiles[index];

            if ((tile->type == TileType_grass || tile->type == TileType_dirt) && !IsFlagSet(tile, TileFlag_fire)) {
                if (!IsFlagSet(tile, TileFlag_visited)) {
                    AddFlag(tile, TileFlag_fire);
                    //tile->type = TileType_fire;
                    has_flood_fill_happened = true;
                    
                    if (IsFlagSet(tile, TileFlag_powerup)) {
                        ClearFlag(tile, TileFlag_powerup);
                    }
                    if (IsFlagSet(tile, TileFlag_enemy)) {
                        DeleteEnemyInList(sentinel, index);
                        ClearFlag(tile, TileFlag_enemy);
                        enemy_slain++;
                        player->speed += 10.0f;
                    }
                } 
            } 

            ClearFlag(tile, TileFlag_visited);
        }
    }

    if (has_flood_fill_happened) {
        if (enemy_slain) {
            PlaySound(sounds[SoundEffect_powerup_appear]);
        }
        while (enemy_slain) {
            u32 tile_index = GetRandomEmptyTileIndex(tilemap);
            if (tile_index) {
                Tile *tile = &tilemap->tiles[tile_index];
                AddFlag(tile, TileFlag_powerup);
                enemy_slain--;
            }
        }
        has_flood_fill_happened = false;
    }
}

Rectangle SetAtlasFrameRec(Tile_Type type, u32 seed) {
    Rectangle frame_rec = {0, 0, TILE_SIZE, TILE_SIZE};

    switch (type) {
        case TileType_wall:
        case TileType_wall2:
        case TileType_grass:
        case TileType_dirt: {
            frame_rec.x = seed * TILE_SIZE;
            frame_rec.y = 0;
        } break;
        default: {
        } break;
    }

    return frame_rec;
}

void Animate(Animation *animator, u32 frame_counter, u32 facing = 0) {
    if (frame_counter >= 60/FRAME_SPEED) {
        animator->current_frame++;
    }
        
    if (animator->current_frame > animator->max_frames) {
        animator->current_frame = 0;
    }

    animator->frame_rec.x = (f32)animator->current_frame *
                           ((f32)animator->texture[facing].width /
                           animator->max_frames);
}

// TODO: Maybe make these functions take the input buffer as the argument 
// instead of the player
inline bool InputBufferEmpty(Player *player) {
    bool result = player->input_buffer.start == player->input_buffer.end;
    return result;
}

inline bool InputBufferFull(Player *player) {
    bool result = ((player->input_buffer.end + 1) % INPUT_MAX) == player->input_buffer.start;
    return result;
}

void InputBufferPush(Player *player, Direction_Facing dir) {
    if (!InputBufferFull(player)) {

        player->input_buffer.inputs[player->input_buffer.end] = dir;
        player->input_buffer.end = (player->input_buffer.end + 1) % INPUT_MAX;
    }
}

Direction_Facing InputBufferPop(Player *player) {
    Direction_Facing result = DirectionFacing_none;
    if(!InputBufferEmpty(player)) {
        result = player->input_buffer.inputs[player->input_buffer.start];
        player->input_buffer.start = (player->input_buffer.start + 1) % INPUT_MAX;
    }
    return result;
}

Direction_Facing KeyToDirection(s32 key) {
    Direction_Facing result;
    switch(key) {
        case KEY_UP:    case KEY_W: result = DirectionFacing_up;    break;
        case KEY_DOWN:  case KEY_S: result = DirectionFacing_down;  break;
        case KEY_LEFT:  case KEY_A: result = DirectionFacing_left;  break;
        case KEY_RIGHT: case KEY_D: result = DirectionFacing_right; break;
        default:                    result = DirectionFacing_none;  break;
    }
    return result;
}

void GatherInput(Player *player) {
    s32 key;
    while ((key = GetKeyPressed()) != 0) {
        Direction_Facing dir = KeyToDirection(key);
        if (dir == DirectionFacing_none) continue;

        if ((player->facing == DirectionFacing_up    && dir == DirectionFacing_down) ||
            (player->facing == DirectionFacing_down  && dir == DirectionFacing_up)   ||
            (player->facing == DirectionFacing_left  && dir == DirectionFacing_right)||
            (player->facing == DirectionFacing_right && dir == DirectionFacing_left)) {
            continue;
        }

        InputBufferPush(player, dir);
    }
}


int main() {
    // -------------------------------------
    // Initialisation
    // -------------------------------------

    const int window_width  = 1280;
    const int window_height = 1280; //720;
    InitWindow(window_width, window_height, "Raylib basic window");

    InitAudioDevice();

    Tilemap map;
    TilemapInit(&map);
    u32 tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        { 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, },
        { 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        { 4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        { 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        { 4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        { 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        { 4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        { 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        { 4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        { 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        { 4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        { 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        { 4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        { 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        { 4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        { 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, },
    };
    //map.tiles = (u32 *)tilemap;
    //u32 original_map[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    Tile tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH];

    Animation fire_animator;
    fire_animator.texture[0]     = LoadTexture("../assets/sprites/fire.png");
    fire_animator.max_frames     = (f32)fire_animator.texture[0].width/SPRITE_SIZE;
    fire_animator.frame_rec      = {0.0f, 0.0f,
                                    (f32)fire_animator.texture[0].width/fire_animator.max_frames,
                                    (f32)fire_animator.texture[0].height};
    fire_animator.current_frame  = 0;

    map.original_map = (u32 *)&tilemap;
    map.tiles        = (Tile *)&tiles;

    TileInit(&map, fire_animator);

    Player player = {};
    PlayerInit(&player);

    // Initialise the basic player body animator
    player.animators[PlayerAnimator_body].texture[DirectionFacing_down]  = 
        LoadTexture("../assets/sprites/thing.png");
    player.animators[PlayerAnimator_body].texture[DirectionFacing_up]    = 
        LoadTexture("../assets/sprites/thing_back.png");
    player.animators[PlayerAnimator_body].texture[DirectionFacing_left]  = 
        LoadTexture("../assets/sprites/thing_side.png");
    player.animators[PlayerAnimator_body].texture[DirectionFacing_right] = 
        LoadTexture("../assets/sprites/thing_side.png");
    player.animators[PlayerAnimator_body].frame_rec = 
        {0.0f, 0.0f, 
        (f32)player.animators[PlayerAnimator_body].texture[DirectionFacing_down].width/6,
        (f32)player.animators[PlayerAnimator_body].texture[DirectionFacing_down].height}; 
    player.animators[PlayerAnimator_body].current_frame = 0;

    // Initialise the powerup animator
    player.animators[PlayerAnimator_water].texture[0] = LoadTexture("../assets/sprites/water_down.png");
    player.animators[PlayerAnimator_water].max_frames = 
        (f32)player.animators[PlayerAnimator_water].texture[0].width/SPRITE_SIZE;
    player.animators[PlayerAnimator_water].frame_rec  = 
        {0.0f, 0.0f, 
        (f32)player.animators[PlayerAnimator_water].texture[0].width /
             player.animators[PlayerAnimator_water].max_frames,
        (f32)player.animators[PlayerAnimator_water].texture[0].height};
    player.animators[PlayerAnimator_water].current_frame = 0;

    Vector2 input_axis       = {0, 0};

    GameManager manager;
    GameManagerInit(&manager);
    LoadSoundBuffer(manager.sounds);

    // Music Init
    Music song_main  = LoadMusicStream("../assets/sounds/music.wav");
    Music song_muted = LoadMusicStream("../assets/sounds/music_muted.wav");

    f32 song_volume  = 1.0f;
    f32 muted_volume = 0.0f;

    SetMusicVolume(song_main,  song_volume);
    SetMusicVolume(song_muted, muted_volume);
    song_main.looping  = true;
    song_muted.looping = true;

    
    b32 fire_cleared; // NOTE: Perhaps this should live in the GameManager struct???

    Texture2D tile_atlas          = LoadTexture("../assets/tiles/tile_row.png");
    Texture2D wall_atlas          = LoadTexture("../assets/tiles/wall_tiles.png");
    Texture2D powerup_texture     = LoadTexture("../assets/sprites/powerup.png");
    u32 frame_counter             = 0;

    Memory_Arena arena;
    size_t arena_size = 1024*1024;
    ArenaInit(&arena, arena_size); 

    // NOTE: This is for the enemy linked list
    // TODO: Perhaps the sentinel should also be in the arena to be closer 
    // in memory to the other enemies?
    Enemy enemy_sentinel = {};
    enemy_sentinel.next = &enemy_sentinel;
    enemy_sentinel.prev = &enemy_sentinel;

    RenderTexture2D target    = LoadRenderTexture(base_screen_width, base_screen_height); 
    SetTargetFPS(60);
    // -------------------------------------
    // Main Game Loop
    while (!WindowShouldClose()) {
        // -----------------------------------
        // Update
        // -----------------------------------
        
        // TODO: Need to figure out a better way to stop everything when a win has occured
        float delta_t = GetFrameTime();

        // NOTE: reset the counter back to zero after everything to not mess up 
        // the individual animations
        if (frame_counter >= 60/FRAME_SPEED) {
            frame_counter = 0;
        }

        frame_counter++;

        // The max frames for the body need to be set up here during the main loop 
        // because they are dependant on whichever direction the player is facing
        player.animators[PlayerAnimator_body].max_frames = 
            (f32)player.animators[PlayerAnimator_body].texture[player.facing].width/20;
        Animate(&player.animators[PlayerAnimator_body], frame_counter, player.facing);

        UpdateMusicStream(song_main);
        UpdateMusicStream(song_muted);

        if (!IsMusicStreamPlaying(song_main))  PlayMusicStream(song_main);
        if (!IsMusicStreamPlaying(song_muted)) PlayMusicStream(song_muted);

        if (manager.state == GameState_play) {

            // Draw to render texture
            BeginTextureMode(target);
            ClearBackground(BLACK);
            
            fire_cleared = true;
            // Draw tiles in background
            {
                for (u32 y = 0; y < map.height; y++) {
                    for (u32 x = 0; x < map.width; x++) {
                        u32 index = TilemapIndex(x, y, map.width);
                        Tile *tile = &map.tiles[index];
                        tile->tile_pos = {(float)x * map.tile_size, (float)y * map.tile_size};

                        Color tile_col;
                        switch (tile->type) {
                            case TileType_none:       tile_col = BLACK;                break;
                            case TileType_wall:       tile_col = PURPLE;               break;
                            case TileType_wall2:      tile_col = {140, 20, 140, 255};  break;
                            case TileType_grass:      tile_col = {68, 68, 68, 255};    break;
                            case TileType_dirt:       tile_col = {168, 168, 168, 255}; break;
                            case TileType_fire:       tile_col = {168, 0, 0, 255};     break;
                            case TileType_temp_grass: tile_col = {68, 68, 68, 255};    break;
                            case TileType_temp_dirt:  tile_col = {168, 168, 168, 255}; break;
                        }

                        Vector2 tile_size = {(f32)map.tile_size, (f32)map.tile_size};

                        // TODO: I need to make getting the source rec and the random seed for 
                        // the atlas seperate functions
                        
                        Rectangle atlas_frame_rec = SetAtlasFrameRec(tile->type, tile->seed);

                        if (tile->type == TileType_grass || tile->type == TileType_dirt) {
                            DrawTextureRec(tile_atlas, atlas_frame_rec, tile->tile_pos, WHITE);
                        } else if (tile->type == TileType_wall || tile->type == TileType_wall2) {  
                            DrawTextureRec(wall_atlas, atlas_frame_rec, tile->tile_pos, WHITE);
                        } else {
                            DrawRectangleV(tile->tile_pos, tile_size, tile_col);
                        }
                        if (IsFlagSet(tile, TileFlag_fire)) {
                            tile_col = {168, 0, 0, 255};
                            fire_cleared = false;
                            //DrawRectangleV(tile->tile_pos, tile_size, tile_col);
                            Animate(&tile->animator, frame_counter);
                            DrawTextureRec(tile->animator.texture[0], 
                                           tile->animator.frame_rec, tile->tile_pos, WHITE);
                        }
                        if (IsFlagSet(tile, TileFlag_powerup)) {
                            DrawTextureV(powerup_texture, tile->tile_pos, WHITE);
                        }
                        if (IsFlagSet(tile, TileFlag_enemy)) {
                            Enemy *found_enemy = FindEnemyInList(&enemy_sentinel, index);
                            if (found_enemy) {
                                Animate(&found_enemy->animator, frame_counter);
                                DrawTextureRec(found_enemy->animator.texture[0],
                                               found_enemy->animator.frame_rec, tile->tile_pos, WHITE);
                            }
                        }
                        if (IsFlagSet(tile, TileFlag_moved)) {
                            ClearFlag(tile, TileFlag_moved);
                        }
                    }
                }
            }

            GatherInput(&player);

            // - New player movement
            if (!player.is_moving) {
                Direction_Facing dir = InputBufferPop(&player);
                if (dir == DirectionFacing_none) dir = player.facing;

                Vector2 input_axis = {0, 0};
                switch (dir) {
                    case DirectionFacing_up:    input_axis = { 0,-1}; break;
                    case DirectionFacing_down:  input_axis = { 0, 1}; break;
                    case DirectionFacing_left:  input_axis = {-1, 0}; break;
                    case DirectionFacing_right: input_axis = { 1, 0}; break;
                    default: break;
                }
                

                if (input_axis.x || input_axis.y) {
                    // NOTE: Calculate the next tile position
                    s32 current_tile_x = (u32)player.pos.x / map.tile_size;
                    s32 current_tile_y = (u32)player.pos.y / map.tile_size;

                    s32 target_tile_x = current_tile_x + u32(input_axis.x);
                    s32 target_tile_y = current_tile_y + u32(input_axis.y);

                    u32 target_tile_index = TilemapIndex(target_tile_x, target_tile_y, map.width);
                    Tile *target_tile = &map.tiles[target_tile_index];

                    // TODO: need to continue the refactor from here
                    if (target_tile_x > 0 && target_tile_x < map.width-1 &&
                        target_tile_y > 0 && target_tile_y < map.height-1) {

                        if (target_tile->type != TileType_wall  && 
                            target_tile->type != TileType_wall2 && 
                            target_tile->type != TileType_fire) {
                            
                            // Start moving
                            player.facing     = dir;
                            player.target_pos = {(float)target_tile_x * map.tile_size, (float)target_tile_y * map.tile_size};
                            player.is_moving  = true;

                            u32 current_tile_index = TilemapIndex(current_tile_x, current_tile_y, map.width);
                            Tile *current_tile     = &map.tiles[current_tile_index];
                            if (!player.powered_up) {
                                AddFlag(current_tile, TileFlag_fire);
                            }
                        } 
                        else {
                            GameOver(&player, &map, &enemy_sentinel, &manager);
                        }
                    } else {
                        // Out of bounds
                        GameOver(&player, &map, &enemy_sentinel, &manager);
                    }

                    if (IsFlagSet(target_tile, TileFlag_powerup)) {
                        float powerup_duration     = 10.0f;
                        player.powerup_timer       = GetTime() + powerup_duration;
                        player.powered_up          = true;
                        player.blink_speed         = 5.0f;
                        player.time_between_blinks = player.blink_speed;
                        ClearFlag(target_tile, TileFlag_powerup);
                        if (IsSoundPlaying(manager.sounds[SoundEffect_powerup_end])) {
                            StopSound(manager.sounds[SoundEffect_powerup_end]);
                        }
                        PlaySound(manager.sounds[SoundEffect_powerup_collect]);
                    }

                    if (player.powered_up) {
                        if (IsFlagSet(target_tile, TileFlag_fire)) {
                            ClearFlag(target_tile, TileFlag_fire);
                            manager.score += 10;
                        }
                    }

                    if (IsFlagSet(target_tile, TileFlag_fire) || IsFlagSet(target_tile, TileFlag_enemy)) {
                        GameOver(&player, &map, &enemy_sentinel, &manager);
                    }
                }
            } else {
                // Move towards target position
                Vector2 direction = VectorSub(player.target_pos, player.pos);
                float distance    = Length(direction);
                if (distance <= player.speed * delta_t) {
                    player.pos       = player.target_pos;
                    player.is_moving = false;

                    u32 current_tile_x = (u32)player.pos.x / map.tile_size;
                    u32 current_tile_y = (u32)player.pos.y / map.tile_size;

                    CheckEnclosedAreas(&map, &player, &enemy_sentinel, manager.sounds, 
                                       current_tile_x, current_tile_y);
                } else {
                    direction        = VectorNorm(direction);
                    Vector2 movement = VectorScale(direction, player.speed * delta_t);
                    player.pos       = VectorAdd(player.pos, movement);
                }
            }

            // Powerup blinking
            if (player.powered_up) {
                f32 end_duration_signal  = 3.0f;
                u32 water_frame_counter  = frame_counter;

                song_volume  -= 0.02f;
                muted_volume += 0.02f;

                Sound powerup_effect     = manager.sounds[SoundEffect_powerup];
                Sound powerup_end_effect = manager.sounds[SoundEffect_powerup_end];

                // TODO: Need to either figure out a way to make this repeat while the player 
                // is powered up or I need to make the sound effect last a really long time
                if(!IsSoundPlaying(powerup_effect) && !IsSoundPlaying(powerup_end_effect))
                {
                    PlaySound(powerup_effect);
                }

                if (player.powerup_timer < GetTime()) {
                    player.powered_up = false;
                    player.col_bool   = false;
                    StopSound(powerup_effect);
                    StopSound(powerup_end_effect);
                } else {
                    if (player.powerup_timer - end_duration_signal < GetTime()) {
                        player.blink_speed = 2.0f; 
                        water_frame_counter *= 2;

                        if (!IsSoundPlaying(powerup_end_effect)) {
                            StopSound(powerup_effect);
                            PlaySound(powerup_end_effect);
                        }
                    }

                    if (player.time_between_blinks > 0) {
                    player.time_between_blinks -= 1.0f;
                    } else {
                        player.time_between_blinks = player.blink_speed;
                        player.col_bool            = !player.col_bool;
                    }

                }

                Animate(&player.animators[PlayerAnimator_water], water_frame_counter);
                DrawTextureRec(player.animators[PlayerAnimator_water].texture[0], 
                               player.animators[PlayerAnimator_water].frame_rec, player.target_pos, WHITE);

                if (player.col_bool) {
                    player.col = BLUE;
                } else {
                    player.col = WHITE;
                }
            } else {
                song_volume  += 0.02f;
                muted_volume -= 0.02f;
            }

            if (song_volume < 0.0f)  song_volume  = 0.0f;
            if (song_volume > 1.0f)  song_volume  = 1.0f;
            if (muted_volume < 0.0f) muted_volume = 0.0f;
            if (muted_volume > 1.0f) muted_volume = 1.0f;

            SetMusicVolume(song_main,  song_volume);
            SetMusicVolume(song_muted, muted_volume);

            // Enemy spawning
            if (manager.spawn_timer > 0) {
                manager.spawn_timer -= 1.0f;
            } else {
                manager.spawn_timer = manager.enemy_spawn_duration;

                s32 player_tile_x     = (u32)player.pos.x / map.tile_size; 
                s32 player_tile_y     = (u32)player.pos.y / map.tile_size; 
                u32 player_tile_index = TilemapIndex(player_tile_x, player_tile_y, map.width);

                u32 tile_index = GetRandomEmptyTileIndex(&map, player_tile_index);
                if (tile_index) {
                    Tile *tile = &map.tiles[tile_index];
                    AddFlag(tile, TileFlag_enemy);

                    Enemy *new_enemy = (Enemy *)ArenaAlloc(&arena, sizeof(Enemy));
                    EnemyInit(new_enemy, tile_index);
                    // NOTE: Add enemy to enemy_list;
                    new_enemy->next       = enemy_sentinel.next;
                    new_enemy->prev       = &enemy_sentinel;
                    new_enemy->next->prev = new_enemy;
                    new_enemy->prev->next = new_enemy;
                }
            }

            // Enemy movement
            if (manager.enemy_move_timer > 0) {
                manager.enemy_move_timer -= 1.0f;
            } else {
                for (u32 y = 0; y < map.height; y++) {
                    for (u32 x = 0; x < map.width; x++) {
                        u32 tile_index = TilemapIndex(x, y, map.width); 
                        Tile *tile = &map.tiles[tile_index];

                        if (IsFlagSet(tile, TileFlag_enemy) && !IsFlagSet(tile, TileFlag_moved)) {
                            u32 eligible_tile_index = FindEligibleTileIndexForEnemyMove(&map, tile_index); 
                            Tile *eligible_tile = &map.tiles[eligible_tile_index];

                            if (eligible_tile_index) {
                                Enemy *found_enemy = FindEnemyInList(&enemy_sentinel, tile_index);
                                if (found_enemy) {
                                    found_enemy->tile_index = eligible_tile_index;
                                }
                                ClearFlag(tile, TileFlag_enemy);
                                AddFlag(eligible_tile, TileFlag_enemy);
                                AddFlag(eligible_tile, TileFlag_moved);
                            }
                        }
                    }
                }
                manager.enemy_move_timer = manager.enemy_move_duration;
            }

#if 1
            Rectangle src = player.animators[PlayerAnimator_body].frame_rec;

            if (player.facing == DirectionFacing_left) {
                src.x     += src.width;
                src.width  = -src.width;
            }
            
            f32 frame_width  = (f32)player.animators[PlayerAnimator_body].frame_rec.width;
            f32 frame_height = (f32)player.animators[PlayerAnimator_body].texture[player.facing].height*2;

            Rectangle dest_rect = {player.pos.x, player.pos.y, 
                                   frame_width, frame_height}; 

            Vector2 texture_offset = {0.0f, 20.0f};
            DrawTexturePro(player.animators[PlayerAnimator_body].texture[player.facing], src,
                           dest_rect, texture_offset, 0.0f, player.col);

#else
            DrawRectangleV(player.pos, player.size, player.col);
#endif


        } else if (manager.state == GameState_win) {
            // Draw tiles in background
            {
                for (u32 y = 0; y < map.height; y++) {
                    for (u32 x = 0; x < map.width; x++) {
                        u32 index = TilemapIndex(x, y, map.width);
                        Tile *tile = &map.tiles[index];
                        Vector2 tile_pos = {(float)x * map.tile_size, (float)y * map.tile_size};

                        Color tile_col;
                        switch (tile->type) {
                            case TileType_none:       tile_col = BLACK;                break;
                            case TileType_wall:       tile_col = PURPLE;               break;
                            case TileType_wall2:      tile_col = {140, 20, 140, 255};  break;
                            case TileType_grass:      tile_col = {68, 68, 68, 255};    break;
                            case TileType_dirt:       tile_col = {168, 168, 168, 255}; break;
                            case TileType_fire:       tile_col = {168, 0, 0, 255};     break;
                            case TileType_temp_grass: tile_col = {68, 68, 68, 255};    break;
                            case TileType_temp_dirt:  tile_col = {168, 168, 168, 255}; break;
                        }

                        if (IsFlagSet(tile, TileFlag_fire)) {
                            tile_col = {168, 0, 0, 255};
                            fire_cleared = false;
                        }
                        if (IsFlagSet(tile, TileFlag_powerup)) {
                            tile_col = BLUE;
                        }
                        if (IsFlagSet(tile, TileFlag_enemy)) {
                            tile_col = YELLOW;
                        }
                        if (IsFlagSet(tile, TileFlag_moved)) {
                            ClearFlag(tile, TileFlag_moved);
                        }

                        Vector2 tile_size = {(f32)map.tile_size, (f32)map.tile_size};
                        DrawRectangleV(tile_pos, tile_size, tile_col);
                    }
                }
            }

            input_axis = {0, 0};

            StopSoundBuffer(manager.sounds);
        
            if (IsKeyPressed(KEY_SPACE)) {
                GameOver(&player, &map, &enemy_sentinel, &manager);
            }
        }

        EndTextureMode();

        // -----------------------------------
        // Draw
        // -----------------------------------

        // NOTE: Draw the render texture to the screen, scaling it with window size
        BeginDrawing();
        ClearBackground(DARKGRAY);

        float scale_x = (float)window_width  / base_screen_width;
        float scale_y = (float)window_height / base_screen_height;

        Rectangle dest_rect = {
            (window_width - (base_screen_width * scale_x)) * 0.5f,
            (window_height - (base_screen_height * scale_y)) * 0.5f,
            base_screen_width * scale_x,
            base_screen_height * scale_y,
        };

        Rectangle rect = {0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height}; 
        Vector2 zero_vec = {0, 0};
        DrawTexturePro(target.texture, rect,
                       dest_rect, zero_vec, 0.0f, WHITE);
        DrawText(TextFormat("Score: %d", manager.score), 25, 25, 38, WHITE);
        DrawText(TextFormat("High Score: %d", manager.high_score), window_width - 350, 25, 38, WHITE);

        if (fire_cleared && player.powered_up) {
            DrawText("WIN", window_width*0.5, window_height*0.5, 69, WHITE);
            manager.state = GameState_win;
        }

        EndDrawing();
        // -----------------------------------
    }
    // -------------------------------------
    // De-Initialisation
    // -------------------------------------
    // TODO: Unload the sounds in the GameManager
    //UnloadSound(powerup_sound);
    //UnloadSound(powerup_end_sound);
    CloseAudioDevice();
    CloseWindow();
    // -------------------------------------
    return 0;
}
