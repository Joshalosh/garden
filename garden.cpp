
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "mymath.h"
#include "memory.h"
#include "shader.h"
#include "garden.h"

#include "shader.cpp"

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

void GameManagerInit(Game_Manager *manager) {
    manager->score                  = 0;
    manager->high_score             = 0;
    manager->score_multiplier       = 1;
    manager->gui                    = LoadTexture("../assets/tiles/bar.png");

    manager->animators[GodAnimator_angry].texture[0] = LoadTexture("../assets/sprites/angry.png");
    manager->animators[GodAnimator_angry].max_frames    = (f32)manager->animators[GodAnimator_angry].texture[0].width/40;
    manager->animators[GodAnimator_angry].current_frame = 0;
    manager->animators[GodAnimator_angry].looping       = false;
    manager->animators[GodAnimator_angry].frame_rec     = {0, 0, 
                                                          (f32)manager->animators[GodAnimator_angry].texture[0].width /
                                                               manager->animators[GodAnimator_angry].max_frames,
                                                          (f32)manager->animators[GodAnimator_angry].texture[0].height};

    manager->animators[GodAnimator_satisfied].texture[1] = LoadTexture("../assets/sprites/satisfied.png");
    manager->animators[GodAnimator_satisfied].max_frames    = (f32)manager->animators[GodAnimator_satisfied].texture[0].width/40;
    manager->animators[GodAnimator_satisfied].current_frame = 0;
    manager->animators[GodAnimator_satisfied].looping       = false;
    manager->animators[GodAnimator_satisfied].frame_rec     = {0, 0, 
                                                          (f32)manager->animators[GodAnimator_satisfied].texture[0].width /
                                                               manager->animators[GodAnimator_satisfied].max_frames,
                                                          (f32)manager->animators[GodAnimator_satisfied].texture[0].height};

    manager->animators[GodAnimator_happy].texture[2] = LoadTexture("../assets/sprites/happy.png");
    manager->animators[GodAnimator_happy].max_frames    = (f32)manager->animators[GodAnimator_happy].texture[0].width/40;
    manager->animators[GodAnimator_happy].current_frame = 0;
    manager->animators[GodAnimator_happy].looping       = false;
    manager->animators[GodAnimator_happy].frame_rec     = {0, 0, 
                                                          (f32)manager->animators[GodAnimator_happy].texture[0].width /
                                                               manager->animators[GodAnimator_happy].max_frames,
                                                          (f32)manager->animators[GodAnimator_happy].texture[0].height};

    manager->state                  = GameState_title; //GameState_play;

    manager->enemy_spawn_duration   = 500.0f;
    manager->spawn_timer            = manager->enemy_spawn_duration;
    manager->enemy_move_duration    = 250.0f;
    manager->enemy_move_timer       = manager->enemy_move_duration;

    manager->hype_sound_timer       = 0.0f;
    manager->hype_prev_index        = 0;

    manager->enemy_sentinel         = {};
    manager->enemy_sentinel.next    = &manager->enemy_sentinel;
    manager->enemy_sentinel.prev    = &manager->enemy_sentinel;

    // The head of the powerup linked list
    manager->powerup_sentinel       = {};
    manager->powerup_sentinel.next  = &manager->powerup_sentinel;
    manager->powerup_sentinel.prev  = &manager->powerup_sentinel;

    manager->screen_shake.intensity = 0;
    manager->screen_shake.duration  = 0;
    manager->screen_shake.decay     = 0;

    manager->fade_count             = 0;
}

void TitleScreenManagerInit(Title_Screen_Manager *manager) {
    Game_Title title;
    title.texture  = LoadTexture("../assets/titles/anunnaki.png");
    title.scale    = 2;
    title.pos.x    = (base_screen_width * 0.5) - ((title.texture.width * title.scale) * 0.5);
    title.pos.y    = base_screen_height * 0.25;
    title.bob      = 0.0f;
    manager->title = title;

    Title_Screen_Background bg;
    // TODO: Put this into a loop to load these textures with less lines
    bg.texture[0]    = LoadTexture("../assets/tiles/layer_1.png");
    bg.texture[1]    = LoadTexture("../assets/tiles/layer_2.png");
    bg.texture[2]    = LoadTexture("../assets/tiles/layer_3.png");
    bg.texture[3]    = LoadTexture("../assets/tiles/layer_4.png");
    bg.texture[4]    = LoadTexture("../assets/tiles/layer_5.png");
    bg.texture[5]    = LoadTexture("../assets/tiles/layer_6.png");
    bg.texture[6]    = LoadTexture("../assets/tiles/layer_7.png");
    bg.texture[7]    = LoadTexture("../assets/tiles/layer_8.png");
    bg.scroll_speed  = 50.0f;
    bg.pos_right_1   = {0.0f, 0.0f};
    bg.pos_left_1    = {0.0f, 0.0f};
    bg.pos_right_2   = {base_screen_width, 0.0f};
    bg.pos_left_2    = {base_screen_width, 0.0f};
    manager->bg = bg;

    Play_Text play_text;
    play_text.text      = "Spacebar Begins Ritual";
    play_text.font_size = 14;
    play_text.bob       = 0.0f;
    play_text.pos       = {(base_screen_width*0.5f) - (MeasureText(play_text.text, play_text.font_size)*0.5f), 
                           0.0f};
    manager->play_text = play_text;
}

void EndScreenInit(End_Screen *screen) {
    screen->textures[EndLayer_sky]   = LoadTexture("../assets/sprites/win_sky.png"); 
    screen->textures[EndLayer_trees] = LoadTexture("../assets/sprites/win_trees.png"); 

    screen->animator.texture[0]      = LoadTexture("../assets/sprites/win_blink.png"); 
    screen->animator.max_frames      = (f32)screen->animator.texture[0].width/base_screen_width;
    screen->animator.frame_rec       = {0.0f, 0.0f,
                                        (f32)screen->animator.texture[0].width/screen->animator.max_frames,
                                        (f32)screen->animator.texture[0].height};
    screen->animator.current_frame   = 0;
    screen->animator.looping         = false;

    screen->timer                    = 0;
    screen->blink_duration           = 3.0f;

    // Setup the shaders used on the background layers of the end 
    // screen.
    {
        f32 amplitude = 0.6f;
        f32 frequency = 12.0f;
        f32 speed     = 0.05f;
        WobbleShaderInit(&screen->shaders[EndLayer_sky], amplitude, frequency, speed);
    }
    SetShaderValue(screen->shaders[EndLayer_sky].shader, screen->shaders[EndLayer_sky].amplitude_location, 
                   &screen->shaders[EndLayer_sky].amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_sky].shader, screen->shaders[EndLayer_sky].frequency_location, 
                   &screen->shaders[EndLayer_sky].frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_sky].shader, screen->shaders[EndLayer_sky].speed_location,     
                   &screen->shaders[EndLayer_sky].speed, SHADER_UNIFORM_FLOAT);

    {
        f32 amplitude = 0.01f;
        f32 frequency = 0.7f;
        f32 speed     = 1.0f;
        WobbleShaderInit(&screen->shaders[EndLayer_trees], amplitude, frequency, speed);
    }
    SetShaderValue(screen->shaders[EndLayer_trees].shader, screen->shaders[EndLayer_trees].amplitude_location, 
                   &screen->shaders[EndLayer_trees].amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_trees].shader, screen->shaders[EndLayer_trees].frequency_location, 
                   &screen->shaders[EndLayer_trees].frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_trees].shader, screen->shaders[EndLayer_trees].speed_location,     
                   &screen->shaders[EndLayer_trees].speed, SHADER_UNIFORM_FLOAT);
}

void LoadSoundBuffer(Sound *sounds) {
    sounds[SoundEffect_powerup]         = LoadSound("../assets/sounds/powerup.wav");
    sounds[SoundEffect_powerup_end]     = LoadSound("../assets/sounds/powerup_end.wav");
    sounds[SoundEffect_powerup_collect] = LoadSound("../assets/sounds/powerup_collect.wav");
    sounds[SoundEffect_powerup_appear]  = LoadSound("../assets/sounds/powerup_appear.wav");
}

void LoadHypeSoundBuffer(Sound *sounds) {
    sounds[0]  = LoadSound("../assets/sounds/hype_1.wav");
    sounds[1]  = LoadSound("../assets/sounds/hype_2.wav");
    sounds[2]  = LoadSound("../assets/sounds/hype_3.wav");
    sounds[3]  = LoadSound("../assets/sounds/hype_4.wav");
    sounds[4]  = LoadSound("../assets/sounds/hype_5.wav");
    sounds[5]  = LoadSound("../assets/sounds/hype_6.wav");
    sounds[6]  = LoadSound("../assets/sounds/hype_7.wav");
    sounds[7]  = LoadSound("../assets/sounds/hype_8.wav");
    sounds[8]  = LoadSound("../assets/sounds/hype_9.wav");
    sounds[9]  = LoadSound("../assets/sounds/hype_10.wav");
    sounds[10] = LoadSound("../assets/sounds/hype_11.wav");
    sounds[11] = LoadSound("../assets/sounds/hype_12.wav");
}

void StopSoundBuffer(Sound *sounds) {
    for (u32 index = 0; index < SoundEffect_count; index++) {
        StopSound(sounds[index]);
    }
}

void StopHypeSoundBuffer(Sound *sounds) {
    for (u32 index = 0; index < HYPE_WORD_COUNT; index++) {
        StopSound(sounds[index]);
    }
}

void StopAllSoundBuffers(Game_Manager *manager) {
    StopSoundBuffer(manager->sounds);
    StopHypeSoundBuffer(manager->hype_sounds);
}

void PowerupInit(Powerup *powerup, Powerup *sentinel, Tile *tile) {
    powerup->tile                   = tile;
    powerup->animator.texture[0]    = LoadTexture("../assets/sprites/bowl.png");
    powerup->animator.max_frames    = (f32)powerup->animator.texture[0].width/20;
    powerup->animator.current_frame = 0;
    powerup->animator.looping       = true;
    powerup->animator.frame_rec     = {0, 0,
                                       (f32)powerup->animator.texture[0].width/powerup->animator.max_frames,
                                       (f32)powerup->animator.texture[0].height};

    powerup->next       = sentinel->next;
    powerup->prev       = sentinel;
    powerup->next->prev = powerup;
    powerup->prev->next = powerup;
}

void EnemyInit(Enemy *enemy, Enemy *sentinel, u32 tile_index) {
    enemy->tile_index             = tile_index;
    enemy->animators[EnemyAnimator_idle].texture[0]    = LoadTexture("../assets/sprites/demon.png");
    enemy->animators[EnemyAnimator_idle].max_frames    = (f32)enemy->animators[EnemyAnimator_idle].texture[0].width/20;
    enemy->animators[EnemyAnimator_idle].current_frame = 0;
    enemy->animators[EnemyAnimator_idle].looping       = true;
    enemy->animators[EnemyAnimator_idle].frame_rec     = {0, 0, 
                                                          (f32)enemy->animators[EnemyAnimator_idle].texture[0].width /
                                                               enemy->animators[EnemyAnimator_idle].max_frames,
                                                          (f32)enemy->animators[EnemyAnimator_idle].texture[0].height};

    enemy->animators[EnemyAnimator_destroy].texture[0]    = LoadTexture("../assets/sprites/disappear.png");
    enemy->animators[EnemyAnimator_destroy].max_frames    = (f32)enemy->animators[EnemyAnimator_destroy].texture[0].width/20;
    enemy->animators[EnemyAnimator_destroy].current_frame = 0;
    enemy->animators[EnemyAnimator_destroy].looping       = false;
    enemy->animators[EnemyAnimator_destroy].frame_rec     = {0, 0, 
                                                          (f32)enemy->animators[EnemyAnimator_destroy].texture[0].width /
                                                               enemy->animators[EnemyAnimator_destroy].max_frames,
                                                          (f32)enemy->animators[EnemyAnimator_destroy].texture[0].height};

    enemy->next       = sentinel->next;
    enemy->prev       = sentinel;
    enemy->next->prev = enemy;
    enemy->prev->next = enemy;
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

Powerup *FindPowerupInList(Powerup *sentinel, Tile *tile)
{
    Powerup *powerup_to_find = sentinel->next;
    bool powerup_found = false;
    while (powerup_to_find != sentinel) {
        if (powerup_to_find->tile == tile) {
            powerup_found = true;
            break;
        } else {
            powerup_to_find = powerup_to_find->next;
        }
    }

    if (!powerup_found) {
        powerup_to_find = NULL;
    }
    return powerup_to_find;
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

void DeletePowerupInList(Powerup *sentinel, Tile *tile)
{
    Powerup *powerup_to_delete = sentinel->next;
    bool powerup_found         = false;
    while (powerup_to_delete != sentinel) {
        if (powerup_to_delete->tile == tile) {
            powerup_to_delete->prev->next = powerup_to_delete->next;
            powerup_to_delete->next->prev = powerup_to_delete->prev;
            powerup_found = true;
            break;
        } else { 
            powerup_to_delete = powerup_to_delete->next;
        }
    }
    ASSERT(powerup_found);
}

void UpdateScreenShake(Screen_Shake *shake, f32 delta_t)
{
    if (shake->duration > 0.0f)
    {
        shake->duration -= delta_t;
        shake->intensity -= shake->decay * delta_t;
        if (shake->duration < 0.0f) {
            shake->duration = 0.0f;
            shake->intensity = 0.0f;
        }
        if (shake->intensity < 0.0f) shake->intensity = 0.0f;
    }
}

Vector2 GetScreenShakeOffset(Screen_Shake *shake)
{
    Vector2 result = {0.0f, 0.0f};
    if (shake->duration > 0.0f)
    {
        f32 offset_x = (GetRandomValue(-100, 100) / 100.0f) * shake->intensity;
        f32 offset_y = (GetRandomValue(-100, 100) / 100.0f) * shake->intensity;
        result = {offset_x, offset_y};
    }
    return result;
}

void BeginScreenShake(Screen_Shake *shake, f32 intensity, f32 duration, f32 decay) {
    shake->intensity = intensity;
    shake->duration  = duration;
    shake->decay     = decay;
}

void TriggerTitleBob(Game_Title *title, f32 impulse) {
    title->bob_velocity += impulse;
}

void UpdateTitleBob(Game_Title *title, f32 delta_t) {
    f32 damping = 0.85f;
    float stiffness = 100.0f;

    title->bob_velocity -= stiffness * title->bob          * delta_t;
    title->bob_velocity *= damping;

    title->bob += title->bob_velocity * delta_t;
}

// TODO: Need to set make sure the audio completely stops here perhaps put all the sounds into a sound 
// buffer and loop through and stop all sounds... or close the audio device and re-init... but then I 
// will have to load in all the sounds again.
// TODO: Need to make sure the event sequencer goes back to the first index
void GameOver(Player *player, Tilemap *tilemap,  Game_Manager *manager) {
    PlayerInit(player);
    if (manager->score > manager->high_score) {
        manager->high_score = manager->score;
    }
    manager->score         = 0;
    manager->state         = GameState_play;

    StopSoundBuffer(manager->sounds);

    manager->enemy_sentinel.next = &manager->enemy_sentinel;
    manager->enemy_sentinel.prev = &manager->enemy_sentinel;
    manager->fade_count = 0;

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
        u32 random_y = GetRandomValue(2, tilemap->height - 2);

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
        u32 random_y = GetRandomValue(2, tilemap->height - 2);

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

void CheckEnclosedAreas(Memory_Arena *arena, Tilemap *tilemap, Player *player, Game_Manager *manager, 
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
                        DeletePowerupInList(&manager->powerup_sentinel, tile);
                    }
                    if (IsFlagSet(tile, TileFlag_enemy)) {
                        DeleteEnemyInList(&manager->enemy_sentinel, index);
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
            PlaySound(manager->sounds[SoundEffect_powerup_appear]);
            BeginScreenShake(&manager->screen_shake, 2.0f*enemy_slain, 0.6f, 10.0f);
            manager->score_multiplier = enemy_slain;
        }
        while (enemy_slain) {
            u32 tile_index = GetRandomEmptyTileIndex(tilemap);
            if (tile_index) {
                Tile *tile = &tilemap->tiles[tile_index];
                AddFlag(tile, TileFlag_powerup);
                Powerup *new_powerup = (Powerup *)ArenaAlloc(arena, sizeof(Powerup));
                PowerupInit(new_powerup, &manager->powerup_sentinel, tile);
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

// TODO: need to clean up this facing argument call
void Animate(Animation *animator, u32 frame_counter, u32 facing = 0) {
    if (frame_counter >= 60/FRAME_SPEED) {
        animator->current_frame++;
    }
        
    if (animator->current_frame >= animator->max_frames) {
        if (animator->looping) {
            animator->current_frame = 0;
        } else { 
            animator->current_frame = animator->max_frames - 1;
        }
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

Text_Burst CreateTextBurst(const char *text, Vector2 pos) {
    Text_Burst burst =  {};
    burst.text       =  text;
    burst.pos        =  pos;
    burst.alpha      =  0.0f;
    burst.scale      =  0.25f;
    burst.max_scale  =  0.75f + (float)(rand() % 100) / 100.0f;
    burst.drift.x    = -1.25f + (float)(rand() % 2);
    burst.drift.y    = -1.25f + (float)(rand() % 2);
    burst.lifetime   =  1.0f;
    burst.age        =  0.0f;
    burst.active     =  true;
    return burst;
}

void UpdateTextBurst(Text_Burst *burst, float dt) {
    if (burst->active) {
        burst->age += dt;
        float t     = burst->age / burst->lifetime; 

        if      (t < 0.2f) burst->alpha = t / 0.2f;                 // fade in (0-0.2s)
        else if (t > 0.8f) burst->alpha = 1.0f - (t - 0.8f) / 0.2f; // fade out (last 0.2s)
        else               burst->alpha = 1.0f;

        f32 eased_t  = t*t;
        burst->scale = Lerp(0.25f, eased_t, burst->max_scale);

        burst->pos.x += burst->drift.x *t;
        burst->pos.y += burst->drift.y *t;

        if (burst->age >= burst->lifetime) burst->active = false;
    }
}

void DrawTextBurst(Text_Burst *burst, Font font) {
    f32 font_size = (font.baseSize) * burst->scale;
    Color col     = Fade(WHITE, burst->alpha);

    DrawText(burst->text, (u32)burst->pos.x, (u32)burst->pos.y, font_size, col);
    //DrawTextEx(font, burst->text, burst->pos, font_size, 2, col);
}

void AlphaFadeIn(Game_Manager *manager, Fade_Object *object, f32 duration) {
    object->alpha     = 0.0f;
    object->duration  = duration;
    object->timer     = 0.0f;
    object->fade_type = FadeType_in;
    manager->fadeables[manager->fade_count] = object;
    manager->fade_count++;
}

void AlphaFadeOut(Game_Manager *manager, Fade_Object *object, f32 duration) {
    object->alpha     = 1.0f;
    object->duration  = duration;
    object->timer     = 0.0f;
    object->fade_type = FadeType_out;
    manager->fadeables[manager->fade_count] = object;
    manager->fade_count++;
}

void UpdateAlphaFade(Game_Manager *manager, f32 delta_t) {
    for (u32 index = 0; index < manager->fade_count; index++) {
        if (manager->fadeables[index]->fade_type) {
            manager->fadeables[index]->timer += delta_t;
            if (manager->fadeables[index]->timer >= manager->fadeables[index]->duration) {
                manager->fadeables[index]->timer = manager->fadeables[index]->duration;
            }

            f32 step = manager->fadeables[index]->timer / manager->fadeables[index]->duration;

            if (manager->fadeables[index]->fade_type == FadeType_in) {
                manager->fadeables[index]->alpha = step;
                if (step >= 1.0f) {
                    manager->fadeables[index]->fade_type = FadeType_none;
                }
            } else if (manager->fadeables[index]->fade_type == FadeType_out) {
                manager->fadeables[index]->alpha = 1.0f - step;
                if (step >= 1.0f) {
                    manager->fadeables[index]->fade_type = FadeType_none;
                }
            }
        }
    }
}

void DrawScreenFadeCol(Fade_Object *object, u32 screen_width, u32 screen_height, Color col) {
    if (object->alpha > 0.0f)
    {
        DrawRectangle(0, 0, screen_width, screen_height, Fade(col, object->alpha));
    }
}

void UpdateEventQueue(Event_Queue *queue, Game_Manager *manager, f32 delta_t) {
    if (queue->active) {
        if (queue->index < queue->count) {
            Event *event = &queue->events[queue->index];

            switch (event->type) {
                case EventType_wait: {
                    queue->timer += delta_t;
                    if (queue->timer >= event->duration) {
                        queue->timer = 0.0f;
                        queue->index++;
                    }
                } break;
                case EventType_fade_out: {
                    if (event->fadeable.fade_type == FadeType_none && queue->timer == 0.0f) {
                        AlphaFadeOut(manager, &event->fadeable, event->duration);
                    }
                    queue->timer += delta_t;
                    if (event->fadeable.alpha == 0.0f) {
                        queue->timer = 0.0f;
                        queue->index++;
                    }
                } break;
                case EventType_fade_in: {
                    if (event->fadeable.fade_type == FadeType_none && queue->timer == 0.0f) {
                        AlphaFadeIn(manager, &event->fadeable, event->duration);
                    }
                    queue->timer += delta_t;
                    if (event->fadeable.alpha == 1.0f) {
                        queue->timer = 0.0f;
                        queue->index++;
                    }
                } break;
                case EventType_state_change: {
                    manager->state = (Game_State)event->new_state;
                    queue->timer = 0.0f;
                    queue->index++;
                } break;

                default: queue->index++; break;
            }
        }
    }
}

void StartEventSequence(Event_Queue *queue)
{
    queue->index  = 0;
    queue->timer  = 0.0f;
    queue->active = true;
}

int main() {
    // -------------------------------------
    // Initialisation
    // -------------------------------------

    const int window_width  = 1280;
    const int window_height = 1280; //720;
    const int extra_height  = 100; //720;
    InitWindow(window_width, window_height, "Garden");

    InitAudioDevice();

    Font font = LoadFont("../assets/fonts/Ammaine-Standard.ttf");
    const char *hype_text[HYPE_WORD_COUNT] = {"WOW",      "YEAH",   "AMAZING",      "SANCTIFY", 
                                              "HOLY COW", "DIVINE", "UNBELIEVABLE", "WOAH",
                                              "AWESOME",  "COSMIC", "RITUALISTIC",  "LEGENDARY"};

    Tilemap map;
    TilemapInit(&map);
#if 0
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
#else 
    u32 tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        { 4, 1, 0, 0, 0, 0, 4, 0, 0, 1, 0, 0, 0, 0, 4, 1, },
        { 1, 4, 1, 4, 1, 4, 1, 0, 0, 4, 1, 4, 1, 4, 1, 4, },
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
#endif
    Tile tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH];

    Animation fire_animator;
    fire_animator.texture[0]     = LoadTexture("../assets/sprites/fire.png");
    fire_animator.max_frames     = (f32)fire_animator.texture[0].width/SPRITE_SIZE;
    fire_animator.frame_rec      = {0.0f, 0.0f,
                                    (f32)fire_animator.texture[0].width/fire_animator.max_frames,
                                    (f32)fire_animator.texture[0].height};
    fire_animator.current_frame  = 0;
    fire_animator.looping        = true;

    map.original_map = (u32 *)&tilemap;
    map.tiles        = (Tile *)&tiles;

    TileInit(&map, fire_animator);

    Player player = {};
    PlayerInit(&player);

    // Initialise the basic player body animator
    player.animators[PlayerAnimator_body].texture[DirectionFacing_down]  = 
        LoadTexture("../assets/sprites/hat_down.png");
    player.animators[PlayerAnimator_body].texture[DirectionFacing_up]    = 
        LoadTexture("../assets/sprites/hat_up.png");
    player.animators[PlayerAnimator_body].texture[DirectionFacing_left]  = 
        LoadTexture("../assets/sprites/hat_right.png");
    player.animators[PlayerAnimator_body].texture[DirectionFacing_right] = 
        LoadTexture("../assets/sprites/hat_right.png");
    player.animators[PlayerAnimator_body].texture[DirectionFacing_celebration] = 
        LoadTexture("../assets/sprites/celebration.png");
    player.animators[PlayerAnimator_body].frame_rec = 
        {0.0f, 0.0f, 
        (f32)player.animators[PlayerAnimator_body].texture[DirectionFacing_down].width/4,
        (f32)player.animators[PlayerAnimator_body].texture[DirectionFacing_down].height}; 
    player.animators[PlayerAnimator_body].current_frame = 0;
    player.animators[PlayerAnimator_body].looping       = true;

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
    player.animators[PlayerAnimator_water].looping       = true;

    Vector2 input_axis       = {0, 0};

    Game_Manager manager;
    GameManagerInit(&manager);
    LoadSoundBuffer(manager.sounds);
    LoadHypeSoundBuffer(manager.hype_sounds);

    // Music Init
    // TODO: Should the music be coupled inside of it's own struct to keep thier volume values together?
    Music song_main  = LoadMusicStream("../assets/sounds/music.wav");
    Music song_muted = LoadMusicStream("../assets/sounds/music_muted.wav");

    Music song_intro = LoadMusicStream("../assets/sounds/intro_music.wav");

    f32 song_volume  = 1.0f;
    f32 muted_volume = 0.0f;

    SetMusicVolume(song_main,  song_volume);
    SetMusicVolume(song_muted, muted_volume);
    song_main.looping  = true;
    song_muted.looping = true;
    song_intro.looping = true;

    
    b32 fire_cleared; // NOTE: Perhaps this should live in the Game_Manager struct???

    Texture2D tile_atlas      = LoadTexture("../assets/tiles/tile_row.png");
    Texture2D wall_atlas      = LoadTexture("../assets/tiles/wall_tiles.png");
    Texture2D powerup_texture = LoadTexture("../assets/sprites/powerup.png");

    // Title screen initialisation
    Title_Screen_Manager title_screen_manager;
    TitleScreenManagerInit(&title_screen_manager);


    Wobble_Shader bg_wobble;
    // Putting these variables inside of a scope to make them temporary 
    // purely for the init shader function
    // TODO: Need to figure out a better way to get this variables into the setup of 
    // these shaders without being super jank.
    {
        f32 amplitude = 0.06f;
        f32 frequency = 1.25f;
        f32 speed     = 2.0f;
        WobbleShaderInit(&bg_wobble, amplitude, frequency, speed); 
    }

    SetShaderValue(bg_wobble.shader, bg_wobble.amplitude_location, &bg_wobble.amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bg_wobble.shader, bg_wobble.frequency_location, &bg_wobble.frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bg_wobble.shader, bg_wobble.speed_location,     &bg_wobble.speed,     SHADER_UNIFORM_FLOAT);

    Wobble_Shader fire_wobble;
    {
        f32 amplitude = 0.015;
        f32 frequency = 15.0f;
        f32 speed     = 32.0f;
        WobbleShaderInit(&fire_wobble, amplitude, frequency, speed); 
    }
    
    SetShaderValue(fire_wobble.shader, fire_wobble.amplitude_location, &fire_wobble.amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fire_wobble.shader, fire_wobble.frequency_location, &fire_wobble.frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fire_wobble.shader, fire_wobble.speed_location,     &fire_wobble.speed,     SHADER_UNIFORM_FLOAT);

    End_Screen end_screen;
    EndScreenInit(&end_screen);

    // TODO: I need to initialise an event in the queue, and then in the future 
    // make this a list of queues to be able to hold multiple event queue sequences
    Event_Queue win_text_sequence;
    win_text_sequence.events[0] = {EventType_wait, 1.5f};
    win_text_sequence.events[1].type = EventType_fade_in;
    win_text_sequence.events[1].duration = 1.0f;
    win_text_sequence.events[1].fadeable = {};
    win_text_sequence.events[2] = {EventType_wait, 1.0f, 0, 0, 0};
    win_text_sequence.events[2].fadeable.alpha = 1.0f;
    win_text_sequence.events[3].type = EventType_fade_out;
    win_text_sequence.events[3].duration = 2.0f;
    win_text_sequence.events[3].fadeable.alpha = 1.0f;
    win_text_sequence.events[4] = {EventType_state_change, 0, GameState_epilogue};
    win_text_sequence.count = 5;
    win_text_sequence.active = false;

    Fade_Object white_screen = {};

    // TODO: Maybe this should go into the game manager?
    u32 frame_counter        = 0; 

    Memory_Arena arena;
    size_t arena_size = 1024*1024;
    ArenaInit(&arena, arena_size); 

    RenderTexture2D target   = LoadRenderTexture(base_screen_width, base_screen_height); 
    SetTargetFPS(60);
    // -------------------------------------
    // Main Game Loop
    while (!WindowShouldClose()) {
        // -----------------------------------
        // Update
        // -----------------------------------
        
        // TODO: Need to figure out a better way to stop everything when a win has occured
        f32 delta_t      = GetFrameTime();
        f32 current_time = GetTime();

        UpdateScreenShake(&manager.screen_shake, delta_t);
        // TODO: figure out a way to make this work for multiple fade objects
        // perahps I should create an array of objects to fade and this will go through the list 
        // fading them all.
        UpdateAlphaFade(&manager, delta_t);
        UpdateEventQueue(&win_text_sequence, &manager, delta_t);

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
        UpdateMusicStream(song_intro);

        // Set Shader variables
        SetShaderValue(bg_wobble.shader,   bg_wobble.time_location,   &current_time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(fire_wobble.shader, fire_wobble.time_location, &current_time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(end_screen.shaders[EndLayer_sky].shader,   end_screen.shaders[EndLayer_sky].time_location,  &current_time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(end_screen.shaders[EndLayer_trees].shader, end_screen.shaders[EndLayer_trees].time_location, &current_time, SHADER_UNIFORM_FLOAT);

        // Draw to render texture
        BeginTextureMode(target);
        ClearBackground(BLACK);
            
        if (manager.state == GameState_play) {

            if (IsMusicStreamPlaying(song_intro))  StopMusicStream(song_intro);
            if (!IsMusicStreamPlaying(song_main))  PlayMusicStream(song_main);
            if (!IsMusicStreamPlaying(song_muted)) PlayMusicStream(song_muted);

            fire_cleared = true;
            // Draw tiles in background
            {
                DrawTextureV(manager.gui, {0, 0}, WHITE);
                Vector2 centre_pos = {(f32)(manager.gui.width*0.5) - 20, 0};
                Animate(&manager.animators[GodAnimator_angry], frame_counter);
                DrawTextureRec(manager.animators[GodAnimator_angry].texture[0], 
                               manager.animators[GodAnimator_angry].frame_rec, centre_pos, WHITE);
                for (u32 y = 0; y < map.height; y++) {
                    for (u32 x = 0; x < map.width; x++) {
                        u32 index  = TilemapIndex(x, y, map.width);
                        Tile *tile = &map.tiles[index];
                        tile->pos  = {(float)x * map.tile_size, (float)y * map.tile_size};

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
                            DrawTextureRec(tile_atlas, atlas_frame_rec, tile->pos, WHITE);
                        } else if (tile->type == TileType_wall || tile->type == TileType_wall2) {  
                            if (player.powered_up) {
                                BeginShaderMode(fire_wobble.shader);
                                DrawTextureRec(wall_atlas, atlas_frame_rec, tile->pos, WHITE);
                                EndShaderMode();
                            } else {
                                DrawTextureRec(wall_atlas, atlas_frame_rec, tile->pos, WHITE);
                            }
                        } else {
                            //DrawRectangleV(tile->pos, tile_size, tile_col);
                            continue;
                        }
                        if (IsFlagSet(tile, TileFlag_fire)) {
                            tile_col = {168, 0, 0, 255};
                            fire_cleared = false;
                            //DrawRectangleV(tile->pos, tile_size, tile_col);
                            Animate(&tile->animator, frame_counter);
                            BeginShaderMode(fire_wobble.shader);
                            DrawTextureRec(tile->animator.texture[0], 
                                           tile->animator.frame_rec, tile->pos, WHITE);
                            EndShaderMode();
                        }
                        if (IsFlagSet(tile, TileFlag_powerup)) {
                            Powerup *found_powerup = FindPowerupInList(&manager.powerup_sentinel, tile);
                            if (found_powerup) {
                                Animate(&found_powerup->animator, frame_counter);
                                DrawTextureRec(found_powerup->animator.texture[0], 
                                               found_powerup->animator.frame_rec, tile->pos, WHITE);
                            }
                            //DrawTextureV(powerup_texture, tile->pos, WHITE);
                        }
                        if (IsFlagSet(tile, TileFlag_enemy)) {
                            Enemy *found_enemy = FindEnemyInList(&manager.enemy_sentinel, index);
                            if (found_enemy) {
                                Animate(&found_enemy->animators[EnemyAnimator_idle], frame_counter);
                                Vector2 draw_pos = {tile->pos.x, tile->pos.y - 20.f};
                                DrawTextureRec(found_enemy->animators[EnemyAnimator_idle].texture[0],
                                               found_enemy->animators[EnemyAnimator_idle].frame_rec, 
                                               draw_pos, WHITE);
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
                    u32 current_tile_x = (u32)player.pos.x / map.tile_size;
                    u32 current_tile_y = (u32)player.pos.y / map.tile_size;

                    u32 target_tile_x = current_tile_x + (u32)input_axis.x;
                    u32 target_tile_y = current_tile_y + (u32)input_axis.y;

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
                        } else {
                            GameOver(&player, &map, &manager);
                        }
                    } else {
                        // Out of bounds
                        GameOver(&player, &map, &manager);
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
                        manager.hype_sound_timer   = 0;

                    }

                    if (player.powered_up) {
                        //manager.hype_sound_timer += delta_t;
                        if (IsFlagSet(target_tile, TileFlag_fire)) {
                            ClearFlag(target_tile, TileFlag_fire);
                            manager.score += (10 * manager.score_multiplier);
                            BeginScreenShake(&manager.screen_shake, 1.5f, 0.6f, 10.0f); 
                            // Create the text bursts
                            for (int index = 0; index < MAX_BURSTS; index++) {
                                if (!manager.bursts[index].active) {
                                    u32 random_index      = GetRandomValue(0, HYPE_WORD_COUNT - 1);
                                    const char *word      = hype_text[random_index];
                                    manager.bursts[index] = CreateTextBurst(word, target_tile->pos);
                                    break;
                                }
                            }
                            // Play the hype sounds
                            if (manager.hype_sound_timer <= GetTime()) {
                                u32 index = GetRandomValue(0, HYPE_WORD_COUNT - 1);
                                while (index == manager.hype_prev_index) {
                                    index = GetRandomValue(0, HYPE_WORD_COUNT -1);
                                }
                                Sound hype_sound = manager.hype_sounds[index];
                                f32 sound_boost  = 3.0f;
                                SetSoundVolume(hype_sound, sound_boost);
                                PlaySound(hype_sound);

                                manager.hype_prev_index  = index;
                                f32 sound_duration = GetTime() + 0.90f;
                                manager.hype_sound_timer = sound_duration;
                            }
                        }
                    }

                    if (IsFlagSet(target_tile, TileFlag_fire) || IsFlagSet(target_tile, TileFlag_enemy)) {
                        GameOver(&player, &map, &manager);
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

                    CheckEnclosedAreas(&arena, &map, &player, &manager, current_tile_x, current_tile_y);
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
                    manager.score_multiplier = 1;
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

            song_volume  = CLAMP(song_volume,  0.0f, 1.0f);
            muted_volume = CLAMP(muted_volume, 0.0f, 1.0f);

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

                    // Add enemy
                    Enemy *new_enemy = (Enemy *)ArenaAlloc(&arena, sizeof(Enemy));
                    EnemyInit(new_enemy, &manager.enemy_sentinel, tile_index);
                }
            }

            // Enemy movement
            if (manager.enemy_move_timer > 0) {
                manager.enemy_move_timer -= 1.0f;
            } else {
                for (u32 y = 0; y < map.height; y++) {
                    for (u32 x = 0; x < map.width; x++) {
                        u32 tile_index = TilemapIndex(x, y, map.width); 
                        Tile *tile     = &map.tiles[tile_index];

                        if (IsFlagSet(tile, TileFlag_enemy) && !IsFlagSet(tile, TileFlag_moved)) {
                            u32 eligible_tile_index = FindEligibleTileIndexForEnemyMove(&map, tile_index); 
                            Tile *eligible_tile = &map.tiles[eligible_tile_index];

                            if (eligible_tile_index) {
                                Enemy *found_enemy = FindEnemyInList(&manager.enemy_sentinel, tile_index);
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

            Rectangle src = player.animators[PlayerAnimator_body].frame_rec;

            if (player.facing == DirectionFacing_left) {
                src.x     += src.width;
                src.width  = -src.width;
            }
            
            f32 frame_width  = (f32)player.animators[PlayerAnimator_body].frame_rec.width;
            f32 frame_height = (f32)player.animators[PlayerAnimator_body].texture[player.facing].height;

            Rectangle dest_rect = {player.pos.x, player.pos.y, 
                                   frame_width, frame_height}; 

            Vector2 texture_offset = {0.0f, 20.0f};
            DrawTexturePro(player.animators[PlayerAnimator_body].texture[player.facing], src,
                           dest_rect, texture_offset, 0.0f, player.col);
            for(int index = 0; index < MAX_BURSTS; index++) {
                UpdateTextBurst(&manager.bursts[index], delta_t);
                if (manager.bursts[index].active) {
                    DrawTextBurst(&manager.bursts[index], font);
                }
            }

            if (IsKeyPressed(KEY_P)) {
                manager.state = GameState_win;
            }

        } else if (manager.state == GameState_win) {
            // Draw tiles in background.
            {
                for (u32 y = 0; y < map.height; y++) {
                    for (u32 x = 0; x < map.width; x++) {
                        u32 index  = TilemapIndex(x, y, map.width);
                        Tile *tile = &map.tiles[index];
                        tile->pos  = {(float)x * map.tile_size, (float)y * map.tile_size};

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
                            DrawTextureRec(tile_atlas, atlas_frame_rec, tile->pos, WHITE);
                        } else if (tile->type == TileType_wall || tile->type == TileType_wall2) {  
                            if (player.powered_up) {
                                BeginShaderMode(fire_wobble.shader);
                                DrawTextureRec(wall_atlas, atlas_frame_rec, tile->pos, WHITE);
                                EndShaderMode();
                            } else {
                                DrawTextureRec(wall_atlas, atlas_frame_rec, tile->pos, WHITE);
                            }
                        } else {
                            DrawRectangleV(tile->pos, tile_size, tile_col);
                        }
                        if (IsFlagSet(tile, TileFlag_fire)) {
                            tile_col = {168, 0, 0, 255};
                            fire_cleared = false;
                            //DrawRectangleV(tile->pos, tile_size, tile_col);
                            Animate(&tile->animator, frame_counter);
                            BeginShaderMode(fire_wobble.shader);
                            DrawTextureRec(tile->animator.texture[0], 
                                           tile->animator.frame_rec, tile->pos, WHITE);
                            EndShaderMode();
                        }
                        if (IsFlagSet(tile, TileFlag_powerup)) {
                            Powerup *found_powerup = FindPowerupInList(&manager.powerup_sentinel, tile);
                            if (found_powerup) {
                                Animate(&found_powerup->animator, frame_counter);
                                DrawTextureRec(found_powerup->animator.texture[0], 
                                               found_powerup->animator.frame_rec, tile->pos, WHITE);
                            }
                            //DrawTextureV(powerup_texture, tile->pos, WHITE);
                        }
                        // TODO: I need to draw enemies in a smarter way
                        if (IsFlagSet(tile, TileFlag_enemy)) {
                            Enemy *found_enemy = FindEnemyInList(&manager.enemy_sentinel, index);
                            if (found_enemy) {
                                Animate(&found_enemy->animators[EnemyAnimator_destroy], frame_counter);
                                Vector2 draw_pos = {tile->pos.x, tile->pos.y - 20.f};
                                DrawTextureRec(found_enemy->animators[EnemyAnimator_destroy].texture[0],
                                               found_enemy->animators[EnemyAnimator_destroy].frame_rec, draw_pos, WHITE);
                            }
                        }
                        if (IsFlagSet(tile, TileFlag_moved)) {
                            ClearFlag(tile, TileFlag_moved);
                        }
                    }
                }
            }

            input_axis = {0, 0};

            StopSoundBuffer(manager.sounds);
        
            Rectangle src = player.animators[PlayerAnimator_body].frame_rec;

            player.facing = DirectionFacing_celebration;
            BeginScreenShake(&manager.screen_shake, 4.0f, 5.0f, 10.0f);

            f32 frame_width  = (f32)player.animators[PlayerAnimator_body].frame_rec.width;
            f32 frame_height = (f32)player.animators[PlayerAnimator_body].texture[player.facing].height;

            Rectangle dest_rect = {player.pos.x, player.pos.y, 
                                   frame_width, frame_height}; 

            Vector2 texture_offset = {0.0f, 20.0f};
            DrawTexturePro(player.animators[PlayerAnimator_body].texture[player.facing], src,
                           dest_rect, texture_offset, 0.0f, player.col);
            if (white_screen.alpha == 0.0f) {
                AlphaFadeIn(&manager, &white_screen, 5.0f);
            } else if (white_screen.alpha == 1.0f) {
                manager.state = GameState_win_text;
            }
            DrawScreenFadeCol(&white_screen, base_screen_width, base_screen_height, WHITE);
            if (IsKeyPressed(KEY_SPACE)) {
                GameOver(&player, &map, &manager);
            }
        } else if (manager.state == GameState_win_text) {

            DrawScreenFadeCol(&white_screen, base_screen_width, base_screen_height, WHITE);
            if (!win_text_sequence.active) StartEventSequence(&win_text_sequence);

            Event event = win_text_sequence.events[win_text_sequence.index];
            const char *message = "The Gods Are Pleased!";
            u32 font_size = 14;
            Vector2 text_pos = {(base_screen_width*0.5f) - (MeasureText(message, font_size)*0.5f), 
                                base_screen_height*0.5f};
            DrawText(message, (u32)text_pos.x+2.0f, (u32)text_pos.y+2.0f, font_size, 
                     Fade(BLACK, event.fadeable.alpha));
            DrawText(message, (u32)text_pos.x+1.0f, (u32)text_pos.y+1.0f, font_size, 
                     Fade(MAROON, event.fadeable.alpha));
            DrawText(message, (u32)text_pos.x, (u32)text_pos.y, font_size, 
                     Fade(GOLD, event.fadeable.alpha));

        } else if (manager.state == GameState_epilogue) {
            if (white_screen.alpha == 1.0f)
            {
                AlphaFadeOut(&manager, &white_screen, 5.0f);
            }
            end_screen.timer += delta_t;
            BeginShaderMode(end_screen.shaders[EndLayer_sky].shader);
            DrawTextureV(end_screen.textures[EndLayer_sky], {0, 0}, WHITE);
            EndShaderMode();
            BeginShaderMode(end_screen.shaders[EndLayer_trees].shader);
            DrawTextureV(end_screen.textures[EndLayer_trees], {-30.0f, 0}, WHITE);
            EndShaderMode();
            Animate(&end_screen.animator, frame_counter);
            DrawTextureRec(end_screen.animator.texture[0], 
                           end_screen.animator.frame_rec, {0, 0}, WHITE);
            if (end_screen.timer > end_screen.blink_duration) {
                end_screen.animator.current_frame = 0;
                end_screen.timer = 0;
            }

            DrawScreenFadeCol(&white_screen, base_screen_width, base_screen_height, WHITE);
            if (IsKeyPressed(KEY_SPACE)) {
                GameOver(&player, &map, &manager);
            }
        } else if (manager.state == GameState_title) {

            Game_Title *title = &title_screen_manager.title;
            UpdateTitleBob(title, delta_t);

            if (!IsMusicStreamPlaying(song_intro))  
            {
                PlayMusicStream(song_intro);
                TriggerTitleBob(title, 5.0f);
            }

            if (IsKeyPressed(KEY_P)) TriggerTitleBob(title, 150.0f);

            // The background is split up into layers and moves in two different
            // directions. Each background layer needs a secondary layer that is 
            // drawn right next to the first layer for seamless bg scrolling. This is 
            // why there are two positions for each background direction. It is to 
            // accomodate for the position of both background textures next to each 
            // other.
            //
            Title_Screen_Background *bg = &title_screen_manager.bg;

            bg->pos_left_1.x  -= bg->scroll_speed * delta_t;
            bg->pos_left_2.x  -= bg->scroll_speed * delta_t;
            bg->pos_right_1.x += bg->scroll_speed * delta_t;
            bg->pos_right_2.x += bg->scroll_speed * delta_t;

            if (bg->pos_left_1.x <= -base_screen_width) {
                bg->pos_left_1.x = bg->pos_left_2.x + base_screen_width;
            }
            if (bg->pos_left_2.x <= -base_screen_width) {
                bg->pos_left_2.x = bg->pos_left_1.x + base_screen_width;
            }
            if (bg->pos_right_1.x >= base_screen_width) {
                bg->pos_right_1.x = bg->pos_right_2.x - base_screen_width;
            }
            if (bg->pos_right_2.x >= base_screen_width) {
                bg->pos_right_2.x = bg->pos_right_1.x - base_screen_width;
            }

            for (int index = 0; index < BG_LAYERS; index++ ) {
                if (index % 2) {
                    Vector2 draw_pos_right_1 = {bg->pos_right_1.x, 
                                                bg->pos_right_1.y + bg->texture[index].height*index};
                    Vector2 draw_pos_right_2 = {bg->pos_right_2.x, 
                                                bg->pos_right_2.y + bg->texture[index].height*index};
                    // I prefer the look when only half the layers have the wobble shader attached 
                    // that's why the right moving ones wobble and the left moving ones don't.
                    BeginShaderMode(bg_wobble.shader);
                    DrawTextureV(bg->texture[index], draw_pos_right_1, WHITE);
                    DrawTextureV(bg->texture[index], draw_pos_right_2, WHITE);
                    EndShaderMode();
                } else {
                    Vector2 draw_pos_left_1 = {bg->pos_left_1.x, 
                                               bg->pos_left_1.y + bg->texture[index].height*index};
                    Vector2 draw_pos_left_2 = {bg->pos_left_2.x, 
                                               bg->pos_left_2.y + bg->texture[index].height*index};
                    DrawTextureV(bg->texture[index], draw_pos_left_1, WHITE);
                    DrawTextureV(bg->texture[index], draw_pos_left_2, WHITE);
                }
            }

#if 0
            title->pos.y += 0.1f*sinf(8.0f*title->bob);
            title->bob   += delta_t;
#endif
            Vector2 draw_pos = {title->pos.x, title->pos.y += title->bob};
            if (title->pos.y > base_screen_height) {
                title->pos.y = 0.0f - title->texture.height;
            }
            DrawTextureEx(title->texture, {draw_pos.x-4.0f, draw_pos.y+4.0f}, 0.0f, title->scale, BLACK);
            DrawTextureEx(title->texture, draw_pos, 0.0f, title->scale, WHITE);

            Play_Text *play_text = &title_screen_manager.play_text;
            play_text->pos.y     = draw_pos.y + 80.0f;
            play_text->pos.y    += 1.0f*sinf(8.0f*play_text->bob);
            play_text->bob      += delta_t;

            DrawText(play_text->text, (u32)play_text->pos.x+2.0f, (u32)play_text->pos.y+2.0f, 
                     play_text->font_size, BLACK);
            DrawText(play_text->text, (u32)play_text->pos.x+1.0f, (u32)play_text->pos.y+1.0f, 
                     play_text->font_size, MAROON);
            DrawText(play_text->text, (u32)play_text->pos.x, (u32)play_text->pos.y, 
                     play_text->font_size, GOLD);

            if (IsKeyPressed(KEY_SPACE)) {
                GameOver(&player, &map, &manager);
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
        float scale_y = (float)(window_height ) / base_screen_height;
        Vector2 shake_offset = GetScreenShakeOffset(&manager.screen_shake);

#if 0
        Rectangle dest_rect = {
            ((window_width - (base_screen_width * scale_x)) * 0.5f) + shake_offset.x,
            ((window_height - (base_screen_height * scale_y)) * 0.5f) + extra_height + shake_offset.y,
            base_screen_width * scale_x,
            base_screen_height * scale_y,
        };

#else
            Rectangle dest_rect = {
                ((window_width - (base_screen_width * scale_x)) * 0.5f) + shake_offset.x,
                ((window_height - (base_screen_height * scale_y)) * 0.5f) + shake_offset.y,
                base_screen_width * scale_x,
                base_screen_height * scale_y,
            };
#endif

        Rectangle rect = {0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height}; 
        Vector2 zero_vec = {0, 0};
        DrawTexturePro(target.texture, rect,
                       dest_rect, zero_vec, 0.0f, WHITE);
        if (manager.state == GameState_play || manager.state == GameState_win) {
            DrawText(TextFormat("%d", manager.score), 194 + shake_offset.x, 27 + shake_offset.y, 38, BLACK);
            DrawText(TextFormat("%d", manager.score), 192 + shake_offset.x, 25 + shake_offset.y, 38, WHITE);
            DrawText(TextFormat("High Score: %d", manager.high_score), window_width - 350, 25, 38, WHITE);
        };

        if (fire_cleared && player.powered_up) {
            //DrawText("WIN", window_width*0.5, window_height*0.5, 69, WHITE);
            if (manager.state != GameState_win && manager.state != GameState_epilogue && 
                manager.state != GameState_win_text) {
                manager.state = GameState_win;
            }
        }

        if (manager.state == GameState_play) {
        }

        EndDrawing();
        // -----------------------------------
    }
    // -------------------------------------
    // De-Initialisation
    // -------------------------------------
    // TODO: Unload the sounds in the Game_Manager
    CloseAudioDevice();
    CloseWindow();
    // -------------------------------------
    return 0;
}
