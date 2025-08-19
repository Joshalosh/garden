
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "mymath.h"
#include "memory.h"
#include "shader.h"
#include "garden.h"

#include "shader.cpp"

void AnimatorInit(Animation *animator, const char *path, u32 sprite_width, b32 looping) {
    animator->texture       = LoadTexture(path);
    animator->max_frames    = (f32)animator->texture.width / sprite_width;
    animator->frame_rec     = {0.0f, 0.0f,
                               (f32)animator->texture.width / animator->max_frames,
                               (f32)animator->texture.height};
    animator->current_frame = 0;
    animator->looping       = looping;
}

void TilemapInit(Tilemap *tilemap) {
    tilemap->width       = TILEMAP_WIDTH;
    tilemap->height      = TILEMAP_HEIGHT;
    tilemap->tile_size   = TILE_SIZE;
    b32 animation_looping = true;
    AnimatorInit(&tilemap->fire_animation, "../assets/sprites/fire.png", SPRITE_WIDTH, animation_looping); 

    f32 amplitude = 0.015, frequency = 15.0f, speed = 32.0f;
    WobbleShaderInit(&tilemap->wobble, amplitude, frequency, speed); 
    SetShaderValue(tilemap->wobble.shader, tilemap->wobble.amplitude_location, &tilemap->wobble.amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(tilemap->wobble.shader, tilemap->wobble.frequency_location, &tilemap->wobble.frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(tilemap->wobble.shader, tilemap->wobble.speed_location,     &tilemap->wobble.speed,     SHADER_UNIFORM_FLOAT);
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
    switch (tile->type) {
        case TileType_floor: tile->seed = GetRandomValue(0, TILE_ATLAS_COUNT - 1); break;
        case TileType_wall:  tile->seed = GetRandomValue(0, WALL_ATLAS_COUNT - 1); break;
        default:             tile->seed = 0;                                       break;
    }
}

void TileInit(Tilemap *tilemap) {
    for (u32 y = 0; y < tilemap->height; y++) {
        for (u32 x = 0; x < tilemap->width; x++) {
            u32 index      = TilemapIndex(x, y, tilemap->width);
            Tile *tile     = &tilemap->tiles[index];
            tile->type     = (Tile_Type)tilemap->original_map[index];
            tile->flags    = 0;
            tile->animator = tilemap->fire_animation;
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
    player->blinking_duration   = 0;
    player->col_bool            = false;
    player->facing              = DirectionFacing_down;
    for(u32 index = 0; index < INPUT_MAX; index++) {
        player->input_buffer.inputs[index] = DirectionFacing_down;
    }
    player->input_buffer.start = 0;
    player->input_buffer.end   = 0;
}

void PlayerAnimatorInit(Player *player) {
    b32 looping = true;
    AnimatorInit(&player->animators[PlayerAnimator_down], "../assets/sprites/hat_down.png", 
                 SPRITE_WIDTH, looping); 
    AnimatorInit(&player->animators[PlayerAnimator_up], "../assets/sprites/hat_up.png", 
                 SPRITE_WIDTH, looping); 
    // The left animation uses the same texture as the right animation 
    // and then it just gets flipped along the x axis.
    AnimatorInit(&player->animators[PlayerAnimator_left], "../assets/sprites/hat_right.png", 
                 SPRITE_WIDTH, looping); 
    AnimatorInit(&player->animators[PlayerAnimator_right], "../assets/sprites/hat_right.png", 
                 SPRITE_WIDTH, looping); 
    AnimatorInit(&player->animators[PlayerAnimator_celebration], "../assets/sprites/celebration.png", 
                 SPRITE_WIDTH, looping); 
    AnimatorInit(&player->animators[PlayerAnimator_water], "../assets/sprites/water_down.png", 
                 SPRITE_WIDTH, looping); 
}


void TitleScreenManagerInit(Title_Screen_Manager *manager) {
    // TODO: Clean this up so that everything initialises into 
    // their direct destination and not into a temporary variable 
    // that is then coppied across.
    manager->title.texture    = LoadTexture("../assets/titles/anunnaki.png");
    manager->title.scale      = 2;
    manager->title.pos.x      = (base_screen_width * 0.5) - 
                                 ((manager->title.texture.width * manager->title.scale) * 0.5);
    manager->title.pos.y      = base_screen_height * 0.25;
    manager->title.bob        = 0.0f;

    manager->layer[0].texture = LoadTexture("../assets/tiles/layer_1.png");
    manager->layer[1].texture = LoadTexture("../assets/tiles/layer_2.png");
    manager->layer[2].texture = LoadTexture("../assets/tiles/layer_3.png");
    manager->layer[3].texture = LoadTexture("../assets/tiles/layer_4.png");
    manager->layer[4].texture = LoadTexture("../assets/tiles/layer_5.png");
    manager->layer[5].texture = LoadTexture("../assets/tiles/layer_6.png");
    manager->layer[6].texture = LoadTexture("../assets/tiles/layer_7.png");
    manager->layer[7].texture = LoadTexture("../assets/tiles/layer_8.png");

    f32 pos_y = 0.0f;
    for (u32 index = 0; index < BG_LAYERS; index++) {
        Background_Layer *layer = &manager->layer[index];
        layer->scroll_speed     = 50.0f;
        // Cool little bit hack here as all odd numbers have their least significant 
        // bit set to 1 and odd numbers conversely have it set to 0.
        layer->dir              = (index & 1) ? 1 : -1;
        layer->should_wobble    = (index & 1) ? true : false;
        layer->pos              = {0.0f, pos_y};
        pos_y                  += (f32)layer->texture.height;
    }

    f32 amplitude = 0.06f, frequency = 1.25f, speed = 2.0f;
    WobbleShaderInit(&manager->wobble, amplitude, frequency, speed); 
    SetShaderValue(manager->wobble.shader, manager->wobble.amplitude_location, &manager->wobble.amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(manager->wobble.shader, manager->wobble.frequency_location, &manager->wobble.frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(manager->wobble.shader, manager->wobble.speed_location,     &manager->wobble.speed,     SHADER_UNIFORM_FLOAT);

    manager->play_text.text      = "Spacebar Begins Ritual";
    manager->play_text.font_size = 14;
    manager->play_text.bob       = 0.0f;
    manager->play_text.pos       = {(base_screen_width*0.5f) - 
                                    (MeasureText(manager->play_text.text, manager->play_text.font_size)*0.5f), 
                                    0.0f};
}

void EndScreenInit(End_Screen *screen) {
    screen->textures[EndLayer_sky]   = LoadTexture("../assets/sprites/win_sky.png"); 
    screen->textures[EndLayer_trees] = LoadTexture("../assets/sprites/win_trees.png"); 

    u32 texture_width     = base_screen_width;
    b32 animation_looping = false;
    AnimatorInit(&screen->animator, "../assets/sprites/win_blink.png", texture_width, animation_looping);

    screen->timer          = 0;
    screen->blink_duration = 3.0f;

    // Setup the shaders used on the background layers of the end 
    // screen.
    {
    f32 amplitude = 0.6f; f32 frequency = 12.0f; f32 speed = 0.05f;
    WobbleShaderInit(&screen->shaders[EndLayer_sky], amplitude, frequency, speed);
    }
    SetShaderValue(screen->shaders[EndLayer_sky].shader, screen->shaders[EndLayer_sky].amplitude_location, 
                   &screen->shaders[EndLayer_sky].amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_sky].shader, screen->shaders[EndLayer_sky].frequency_location, 
                   &screen->shaders[EndLayer_sky].frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_sky].shader, screen->shaders[EndLayer_sky].speed_location,     
                   &screen->shaders[EndLayer_sky].speed, SHADER_UNIFORM_FLOAT);

    {
    f32 amplitude = 0.01f; f32 frequency = 0.7f; f32 speed = 1.0f;
    WobbleShaderInit(&screen->shaders[EndLayer_trees], amplitude, frequency, speed);
    }
    SetShaderValue(screen->shaders[EndLayer_trees].shader, screen->shaders[EndLayer_trees].amplitude_location, 
                   &screen->shaders[EndLayer_trees].amplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_trees].shader, screen->shaders[EndLayer_trees].frequency_location, 
                   &screen->shaders[EndLayer_trees].frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(screen->shaders[EndLayer_trees].shader, screen->shaders[EndLayer_trees].speed_location,     
                   &screen->shaders[EndLayer_trees].speed, SHADER_UNIFORM_FLOAT);
}

void LoadSoundBuffer(Sound *sound) {
    sound[SoundEffect_powerup]         = LoadSound("../assets/sounds/powerup.wav");
    sound[SoundEffect_powerup_end]     = LoadSound("../assets/sounds/powerup_end.wav");
    sound[SoundEffect_powerup_collect] = LoadSound("../assets/sounds/powerup_collect.wav");
    sound[SoundEffect_powerup_appear]  = LoadSound("../assets/sounds/powerup_appear.wav");
    sound[SoundEffect_spacebar]        = LoadSound("../assets/sounds/start.wav");
}

void LoadHypeSoundBuffer(Sound *sound) {
    sound[0]  = LoadSound("../assets/sounds/hype_1.wav");
    sound[1]  = LoadSound("../assets/sounds/hype_2.wav");
    sound[2]  = LoadSound("../assets/sounds/hype_3.wav");
    sound[3]  = LoadSound("../assets/sounds/hype_4.wav");
    sound[4]  = LoadSound("../assets/sounds/hype_5.wav");
    sound[5]  = LoadSound("../assets/sounds/hype_6.wav");
    sound[6]  = LoadSound("../assets/sounds/hype_7.wav");
    sound[7]  = LoadSound("../assets/sounds/hype_8.wav");
    sound[8]  = LoadSound("../assets/sounds/hype_9.wav");
    sound[9]  = LoadSound("../assets/sounds/hype_10.wav");
    sound[10] = LoadSound("../assets/sounds/hype_11.wav");
    sound[11] = LoadSound("../assets/sounds/hype_12.wav");
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

void UnloadAllSoundBuffers(Game_Manager *manager) {
    u32 max_count = HYPE_WORD_COUNT >= SoundEffect_count ? HYPE_WORD_COUNT : SoundEffect_count;
    for (u32 index = 0; index < max_count; index++) {
        if (index < SoundEffect_count) UnloadSound(manager->sounds[index]);
        if (index < HYPE_WORD_COUNT)   UnloadSound(manager->hype_sounds[index]);
    }
}

void SpacebarTextInit(Spacebar_Text *text) {
    text->text = "Press Spacebar";
    text->size = 7;
    text->pos  = {(base_screen_width*0.5f) - (MeasureText(text->text, text->size)*0.5f),
                  base_screen_height*0.70};
    text->bob  = 0.0f;
};

void GameManagerInit(Game_Manager *manager) {
    manager->score                  = 0;
    manager->happy_score            = 2500;
    manager->satisfied_score        = 500;
    manager->score_multiplier       = 1;
    manager->frame_counter          = 0;
    manager->fire_cleared           = false;

    manager->atlas[Atlas_tile]      = LoadTexture("../assets/tiles/tile_row.png");
    manager->atlas[Atlas_wall]      = LoadTexture("../assets/tiles/wall_tiles.png");
    manager->gui.bar                = LoadTexture("../assets/tiles/bar.png");
    manager->gui.anim_timer         = 0;
    manager->gui.anim_duration      = 4.0f;
    manager->gui.step               = 0.0f;

    u32 gui_face_size               = SPRITE_WIDTH * 2;
    b32 animation_looping           = false;
    AnimatorInit(&manager->gui.animators[GodAnimator_angry],     "../assets/sprites/angry.png", 
                 gui_face_size, animation_looping);
    AnimatorInit(&manager->gui.animators[GodAnimator_satisfied], "../assets/sprites/meh.png",
                 gui_face_size, animation_looping);
    AnimatorInit(&manager->gui.animators[GodAnimator_happy],     "../assets/sprites/happy.png", 
                 gui_face_size, animation_looping);

    manager->state                   = GameState_title;

    manager->enemy_spawn_duration    = 510.0f; // TODO: This should be in seconds.
    manager->enemy_move_duration     = 250.0f; // TODO: This should be in seconds.
    manager->spawn_timer             = manager->enemy_spawn_duration;
    manager->enemy_move_timer        = manager->enemy_move_duration;

    manager->hype_sound_timer        = 0.0f;
    manager->hype_prev_index         = 0;

    manager->song[Song_play]         = LoadMusicStream("../assets/sounds/music.wav");
    manager->song[Song_play_muted]   = LoadMusicStream("../assets/sounds/music_muted.wav");
    manager->song[Song_tutorial]     = LoadMusicStream("../assets/sounds/tutorial_track.wav");
    manager->song[Song_intro]        = LoadMusicStream("../assets/sounds/intro_music.wav");
    manager->song[Song_win]          = LoadMusicStream("../assets/sounds/win_track.wav");
    manager->play_song_volume        = 1.0f;
    manager->play_muted_song_volume  = 0.0f;
    manager->should_title_music_play = false;
    manager->last_song_bit           = 0xFFFFFFFF;
    SetMusicVolume(manager->song[Song_play], manager->play_song_volume);
    SetMusicVolume(manager->song[Song_play_muted], manager->play_muted_song_volume);
    // Set all the songs in the song buffer to loop
    for (u32 index = 0; index < Song_count; index++) {
        ASSERT(IsMusicReady(manager->song[index]));
        manager->song[index].looping = true;
    }

    manager->enemy_sentinel          = {};
    manager->enemy_sentinel.next     = &manager->enemy_sentinel;
    manager->enemy_sentinel.prev     = &manager->enemy_sentinel;

    // The head of the powerup linked list
    manager->powerup_sentinel        = {};
    manager->powerup_sentinel.next   = &manager->powerup_sentinel;
    manager->powerup_sentinel.prev   = &manager->powerup_sentinel;

    manager->screen_shake.intensity  = 0;
    manager->screen_shake.duration   = 0;
    manager->screen_shake.decay      = 0;

    manager->fade_count              = 0;

    LoadSoundBuffer(manager->sounds);
    LoadHypeSoundBuffer(manager->hype_sounds);

#if 0
    manager->hype_text = {"WOW",      "YEAH",   "AMAZING",      "SANCTIFY", 
                          "HOLY COW", "DIVINE", "UNBELIEVABLE", "WOAH",
                          "AWESOME",  "COSMIC", "RITUALISTIC",  "LEGENDARY"};
#endif
    manager->spacebar_text;
    SpacebarTextInit(&manager->spacebar_text);
}

void PowerupInit(Powerup *powerup, Powerup *sentinel, Tile *tile) {
    powerup->tile       = tile;
    powerup->next       = sentinel->next;
    powerup->prev       = sentinel;
    powerup->next->prev = powerup;
    powerup->prev->next = powerup;
    AnimatorInit(&powerup->animator, "../assets/sprites/bowl.png", SPRITE_WIDTH, true);
}

void EnemyInit(Enemy *enemy, Enemy *sentinel, u32 tile_index) {
    enemy->tile_index = tile_index;
    enemy->next       = sentinel->next;
    enemy->prev       = sentinel;
    enemy->next->prev = enemy;
    enemy->prev->next = enemy;
    AnimatorInit(&enemy->animators[EnemyAnimator_idle], "../assets/sprites/demon.png", SPRITE_WIDTH, true);
    AnimatorInit(&enemy->animators[EnemyAnimator_destroy], "../assets/sprites/disappear.png", SPRITE_WIDTH, false);
}

void TutorialAnimationInit(Tutorial_Entities *entities) {
    AnimatorInit(&entities->enemy,   "../assets/sprites/demon.png", SPRITE_WIDTH, true);
    AnimatorInit(&entities->powerup, "../assets/sprites/bowl.png",  SPRITE_WIDTH, true);
    AnimatorInit(&entities->fire,    "../assets/sprites/fire.png",  SPRITE_WIDTH, true);
}

void ResetEvents(Event_Manager *manager) {
    manager->sequence[Sequence_win]      = manager->original_sequence[Sequence_win];
    manager->sequence[Sequence_tutorial] = manager->original_sequence[Sequence_tutorial];
    manager->sequence[Sequence_begin]    = manager->original_sequence[Sequence_begin];
};

void SetupEventSequences(Event_Manager *manager) {
    // All of this initialisation is for the original base sequences 
    // so that the actual sequence copies that will ultimately have their  
    // values changed can reset back to these defaults at the end. 
    // The final ResetEvent() call at the end of this sets the actual 
    // events to their base value at the end of this initialisation.
    
    // Setup the winning event sequence.
    manager->original_sequence[Sequence_win].events[1].type           = EventType_fade_in;
    manager->original_sequence[Sequence_win].events[1].duration       = 1.0f;
    manager->original_sequence[Sequence_win].events[1].fadeable       = {};
    manager->original_sequence[Sequence_win].events[2]                = {EventType_wait, 2.0f, 0, 0, 0};
    manager->original_sequence[Sequence_win].events[2].fadeable.alpha = 1.0f;
    manager->original_sequence[Sequence_win].events[3].type           = EventType_fade_in;
    manager->original_sequence[Sequence_win].events[3].duration       = 2.0f;
    manager->original_sequence[Sequence_win].events[3].fadeable       = {};
    manager->original_sequence[Sequence_win].count                    = 4;
    manager->original_sequence[Sequence_win].active                   = false;

    // The tutorial screen sequence.
    manager->original_sequence[Sequence_tutorial].events[0].type     = EventType_fade_in;
    manager->original_sequence[Sequence_tutorial].events[0].duration = 2.0f;
    manager->original_sequence[Sequence_tutorial].events[0].fadeable = {};
    manager->original_sequence[Sequence_tutorial].events[1].type     = EventType_fade_in;
    manager->original_sequence[Sequence_tutorial].events[1].duration = 2.5f;
    manager->original_sequence[Sequence_tutorial].events[1].fadeable = {};
    manager->original_sequence[Sequence_tutorial].events[2].type     = EventType_fade_in;
    manager->original_sequence[Sequence_tutorial].events[2].duration = 2.5f;
    manager->original_sequence[Sequence_tutorial].events[2].fadeable = {};
    manager->original_sequence[Sequence_tutorial].events[3]          = {EventType_wait, 2.5f, 0, 0, 0};
    manager->original_sequence[Sequence_tutorial].events[4].type     = EventType_fade_in;
    manager->original_sequence[Sequence_tutorial].events[4].duration = 2.0f;
    manager->original_sequence[Sequence_tutorial].events[4].fadeable = {};
    manager->original_sequence[Sequence_tutorial].count              = 5;
    manager->original_sequence[Sequence_tutorial].active             = false;

    // The sequence when the spacebar is pressed at the title screen 
    // to begin the game.
    manager->original_sequence[Sequence_begin].events[0] = {EventType_wait, 1.0f};
    manager->original_sequence[Sequence_begin].events[1] = {EventType_state_change, 0, GameState_tutorial};
    manager->original_sequence[Sequence_begin].count     = 2;
    manager->original_sequence[Sequence_begin].active    = false;

    // I need to call this function here to make sure that the 
    // actual event sequences that get used are set to the base 
    // values that have just been set up above.
    ResetEvents(manager);
}

Enemy *FindEnemyAtTile(Enemy *sentinel, u32 index)
{
    Enemy *result = NULL;
    for (Enemy *enemy_to_find = sentinel->next; 
         enemy_to_find != sentinel; 
         enemy_to_find = enemy_to_find->next) {
        if (enemy_to_find->tile_index == index) {
            result = enemy_to_find;
            break;
        }
    }
    return result;
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
        shake->duration  -= delta_t;
        shake->intensity -= shake->decay * delta_t;
        if (shake->duration < 0.0f) {
            shake->duration  = 0.0f;
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
    f32 damping     = 0.85f;
    float stiffness = 100.0f;

    title->bob_velocity -= stiffness * title->bob * delta_t;
    title->bob_velocity *= damping;
    title->bob          += title->bob_velocity * delta_t;
}

void StopAllTextBursts(Game_Manager *manager) {
    for(int index = 0; index < MAX_BURSTS; index++) {
        if (manager->bursts[index].active) {
            manager->bursts[index].active = false;
        }
    }
}

void GameOver(Player *player, Tilemap *tilemap,  Game_Manager *manager) {
    PlayerInit(player);
    manager->state            = GameState_play;
    manager->score            = 0;
    manager->score_multiplier = 0;

    StopSoundBuffer(manager->sounds);

    // Delete all enemies and powerups from the enemy/powerup linked lists.
    manager->enemy_sentinel.next = &manager->enemy_sentinel;
    manager->enemy_sentinel.prev = &manager->enemy_sentinel;
    manager->powerup_sentinel.next = &manager->powerup_sentinel;
    manager->powerup_sentinel.prev = &manager->powerup_sentinel;
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

void CheckEnclosedAreasFromPlayerPosition(Tilemap *tilemap, u32 start_x, u32 start_y) {
    StackU32 nodes;
    StackInit(&nodes);

    StackPush(&nodes, start_x, start_y);

    u32 x, y;

    // Flood fill from players position
    while(StackPop(&nodes, &x, &y)) {
        if (x >= (u32)tilemap->width || y >= tilemap->height) continue;

        u32 index = TilemapIndex(x, y, tilemap->width);
        Tile *tile = &tilemap->tiles[index];

        if (IsFlagSet(tile, TileFlag_visited)) continue;
        if (tile->type != TileType_floor)      continue;
        if (IsFlagSet(tile, TileFlag_fire))    continue;
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

    // TODO: Right now it just randomly picks from all tiles 
    // it doesn't take into account whether the tile has fire or is 
    // eligible in any way. It randomly chooses and then checks to see 
    // if the tile is empty. It will try this 10 times before giving up. 
    // However if the board is majoritively willed with fire there's a solid 
    // chance that no tile will ever be picked. Perhaps a better way to do this 
    // is do a pass over all the tiles, and store the eligible tile indexes 
    // into an array and then randomly choose an index from there. That way I 
    // can ensure that if a tile can be chosen it always will be.
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

    const u32 adjacent_count = 4;
    u32 adjacent_tile_indexes[adjacent_count] = {right_tile, left_tile, bottom_tile, top_tile};
    u32 eligible_tiles[adjacent_count] = {};
    u32 eligible_count = 0;
    u32 result = 0;

    for (int adjacent_index = 0; adjacent_index < ARRAY_COUNT(adjacent_tile_indexes); adjacent_index++) {
        Tile *tile = &tilemap->tiles[adjacent_tile_indexes[adjacent_index]];

        if(tile->type == TileType_floor) {
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

void FillEnclosedAreas(Memory_Arena *arena, Tilemap *tilemap, Player *player, Game_Manager *manager, 
                       u32 current_x, u32 current_y) {
    // Mark all reachable areas from the player with a visited flag on the tile. 
    // All areas not marked are enclosed areas.
    CheckEnclosedAreasFromPlayerPosition(tilemap, current_x, current_y);
    bool has_flood_fill_happened = false;
    u32 enemy_slain              = 0;
    // Any floor tiles not marked as visited are enclosed
    for (u32 y = 0; y < (u32)tilemap->height; y++) {
        for (u32 x = 0; x < (u32)tilemap->width; x++) {
            u32 index   = TilemapIndex(x, y, tilemap->width);
            Tile *tile  = &tilemap->tiles[index];

            if ((tile->type == TileType_floor) && !IsFlagSet(tile, TileFlag_fire)) {
                if (!IsFlagSet(tile, TileFlag_visited)) {
                    AddFlag(tile, TileFlag_fire);
                    has_flood_fill_happened = true;
                    
                    if (IsFlagSet(tile, TileFlag_powerup)) {
                        ClearFlag(tile, TileFlag_powerup);
                        DeletePowerupInList(&manager->powerup_sentinel, tile);
                    }
                    if (IsFlagSet(tile, TileFlag_enemy)) {
                        DeleteEnemyInList(&manager->enemy_sentinel, index);
                        ClearFlag(tile, TileFlag_enemy);
                        enemy_slain++;
                        f32 speed_increase = 10.0f;
                        player->speed += speed_increase;
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
            }
            enemy_slain--;
        }
        has_flood_fill_happened = false;
    }
}

Rectangle SetAtlasFrameRec(Tile_Type type, u32 seed) {
    Rectangle frame_rec = {0, 0, TILE_SIZE, TILE_SIZE};
    switch (type) {
        case TileType_wall:
        case TileType_floor: {
            frame_rec.x = seed * TILE_SIZE;
            frame_rec.y = 0;
        } break;
        default: {
        } break;
    }
    return frame_rec;
}

void Animate(Animation *animator, u32 frame_counter) {
    if (frame_counter % (60/FRAME_SPEED) == 0) {
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
                           ((f32)animator->texture.width /
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

void StorePlayerDirectionsInBuffer(Player *player) {
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

Text_Burst CreateTextBurst(const char *text) {
    Text_Burst burst =  {};
    burst.text       =  text;
    burst.pos        =  {(f32)GetRandomValue(0+TILE_SIZE, base_screen_width-TILE_SIZE), 
                         (f32)GetRandomValue(0+TILE_SIZE, base_screen_height-TILE_SIZE)};
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

void DrawTextTripleEffect (const char *text, Vector2 pos, u32 size, f32 alpha = 1.0f) {

    DrawText(text, (u32)pos.x+2.0f, (u32)pos.y+2.0f, size, Fade(BLACK,  alpha));
    DrawText(text, (u32)pos.x+1.0f, (u32)pos.y+1.0f, size, Fade(MAROON, alpha));
    DrawText(text, (u32)pos.x,      (u32)pos.y,      size, Fade(GOLD,   alpha));
}

void DrawTextDoubleEffect (const char *text, Vector2 pos, u32 size, f32 alpha = 1.0f) {
    DrawText(text, (u32)pos.x+2.0f, (u32)pos.y+2.0f, size, Fade(BLACK, alpha));
    DrawText(text, (u32)pos.x,      (u32)pos.y,      size, Fade(WHITE,   alpha));
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

void DrawTextBurst(Text_Burst *burst) {
    f32 font_base_size = 32;
    f32 font_size      = font_base_size * burst->scale;
    Color col          = Fade(WHITE, burst->alpha);
    DrawTextDoubleEffect(burst->text, burst->pos, font_size, burst->alpha);
}

void UpdateAndDrawAllTextBursts(Game_Manager *manager, f32 delta_t) {
    for(int index = 0; index < MAX_BURSTS; index++) {
        UpdateTextBurst(&manager->bursts[index], delta_t);
        if (manager->bursts[index].active) {
            DrawTextBurst(&manager->bursts[index]);
        }
    }
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
        Fade_Object *fadeable = manager->fadeables[index];
        if (fadeable->fade_type) {
            fadeable->timer += delta_t;
            if (fadeable->timer > fadeable->duration) {
                fadeable->timer = fadeable->duration;
            }
            ASSERT(fadeable->duration > 0);
            f32 step = fadeable->timer / fadeable->duration;
            if (fadeable->fade_type == FadeType_in) {
                fadeable->alpha = step;
                if (step >= 1.0f) {
                    fadeable->fade_type = FadeType_none;
                }
            } else if (fadeable->fade_type == FadeType_out) {
                fadeable->alpha = 1.0f - step;
                if (step >= 1.0f) {
                    fadeable->fade_type = FadeType_none;
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
                    queue->active = false;
                } break;

                default: queue->index++; break;
            }
        }
    }
}

void StartEventSequence(Event_Queue *queue) {
    queue->index  = 0;
    queue->timer  = 0.0f;
    queue->active = true;
}
 
void UpdateGodFaceAnimation(Game_Manager *manager, f32 delta_t) {
    manager->gui.anim_timer += delta_t;
    if (manager->gui.anim_timer > manager->gui.anim_duration) {
        manager->gui.anim_timer = 0;
        manager->gui.anim_duration = GetRandomValue(1.0f, 4.0f);
        manager->gui.animators[manager->gui.face_type].current_frame = 0;
    }
}

void SetGodFaceType(Game_Manager *manager) {
    manager->gui.face_type = manager->score > manager->happy_score     ? GodAnimator_happy     :
                             manager->score > manager->satisfied_score ? GodAnimator_satisfied : 
                                                                         GodAnimator_angry;
}

void DrawGodFace(Game_Manager *manager, f32 delta_t) {
    SetGodFaceType(manager);
    manager->gui.face_pos = {(f32)(manager->gui.bar.width*0.5) - 
                        (f32)(manager->gui.animators[0].frame_rec.width*0.5), 0};

    UpdateGodFaceAnimation(manager, delta_t);
    Animate(&manager->gui.animators[manager->gui.face_type], manager->frame_counter);
    DrawTextureRec(manager->gui.animators[manager->gui.face_type].texture, 
                   manager->gui.animators[manager->gui.face_type].frame_rec, manager->gui.face_pos, WHITE);
}

void DrawGame(Tilemap *map, Game_Manager *manager, Player *player, f32 delta_t)
{
    DrawTextureV(manager->gui.bar, {0, 0}, WHITE);
    DrawGodFace(manager, delta_t);
    
    // Draw tiles in background
    for (u32 y = 0; y < map->height; y++) {
        for (u32 x = 0; x < map->width; x++) {
            u32 index  = TilemapIndex(x, y, map->width);
            Tile *tile = &map->tiles[index];
            tile->pos  = {(float)x * map->tile_size, (float)y * map->tile_size};

            Rectangle atlas_frame_rec = SetAtlasFrameRec(tile->type, tile->seed);
            if (tile->type == TileType_floor) {
                DrawTextureRec(manager->atlas[Atlas_tile], atlas_frame_rec, tile->pos, WHITE);
            } else if (tile->type == TileType_wall) {  
                if (player->powered_up) {
                    BeginShaderMode(map->wobble.shader);
                    DrawTextureRec(manager->atlas[Atlas_wall], atlas_frame_rec, tile->pos, WHITE);
                    EndShaderMode();
                } else {
                    DrawTextureRec(manager->atlas[Atlas_wall], atlas_frame_rec, tile->pos, WHITE);
                }
            } 
            if (IsFlagSet(tile, TileFlag_fire)) {
                Color tile_col = player->powered_up ? PURPLE : WHITE;
                manager->fire_cleared = false;
                Animate(&tile->animator, manager->frame_counter);
                BeginShaderMode(map->wobble.shader);
                DrawTextureRec(tile->animator.texture, 
                               tile->animator.frame_rec, tile->pos, tile_col);
                EndShaderMode();
            }
            if (IsFlagSet(tile, TileFlag_powerup)) {
                Powerup *found_powerup = FindPowerupInList(&manager->powerup_sentinel, tile);
                if (found_powerup) {
                    Animate(&found_powerup->animator, manager->frame_counter);
                    DrawTextureRec(found_powerup->animator.texture, 
                                   found_powerup->animator.frame_rec, tile->pos, WHITE);
                }
            }
            if (IsFlagSet(tile, TileFlag_enemy)) {
                Enemy *found_enemy = FindEnemyAtTile(&manager->enemy_sentinel, index);
                if (found_enemy) {
                    // If the game has been won then change the enemy animation to thier 
                    // destroyed one.
                    Animation *enemy_animation = (manager->state == GameState_win) ? 
                                                 &found_enemy->animators[EnemyAnimator_destroy] : 
                                                 &found_enemy->animators[EnemyAnimator_idle];
                    Vector2 draw_pos = {tile->pos.x, tile->pos.y - 20.f};
                    Animate(enemy_animation, manager->frame_counter);
                    DrawTextureRec(enemy_animation->texture,
                                   enemy_animation->frame_rec, 
                                   draw_pos, WHITE);
                }
            }
            if (IsFlagSet(tile, TileFlag_moved)) {
                ClearFlag(tile, TileFlag_moved);
            }
        }
    }
}

void UpdateSpacebarBob(Spacebar_Text *text, f32 delta_t) {
    text->pos.y += 0.1f*sinf(8.0f*text->bob);
    text->bob   += delta_t;
}

u32 SongBit(u32 SongId) {
    return 1u << SongId;
}

void PlayAllMusicForGameCorrectly(Game_Manager *manager) {
    u32 wanted_song_bit = 0;
    switch (manager->state) {
        case GameState_play: {
            wanted_song_bit = SongBit(Song_play) | SongBit(Song_play_muted);
        } break;
        case GameState_tutorial: {
            wanted_song_bit = SongBit(Song_tutorial);
        } break;
        case GameState_title: {
            wanted_song_bit = manager->should_title_music_play ? SongBit(Song_intro) : 0u;
        } break;
        case GameState_epilogue:
        case GameState_win_text:
        case GameState_win: {
            wanted_song_bit = SongBit(Song_win);
        } break;
    }

    if (wanted_song_bit != manager->last_song_bit) {
        for (u32 index = 0; index < Song_count; index++) {
            b32 should_play = (wanted_song_bit & SongBit(index)) != 0;
            b32 is_playing = IsMusicStreamPlaying(manager->song[index]);

            if (should_play && !is_playing) {
                PlayMusicStream(manager->song[index]);
            } else if (!should_play && is_playing) {
                StopMusicStream(manager->song[index]);
            }
        }
        manager->last_song_bit = wanted_song_bit;
    }

    for (u32 index = 0; index < Song_count; index++) {
        if (IsMusicStreamPlaying(manager->song[index])) {
            UpdateMusicStream(manager->song[index]);
        }
    }
}

void SetTimeValueForWobbleShader(Wobble_Shader *shader, f32 time) {
    SetShaderValue(shader->shader, shader->time_location, &time, SHADER_UNIFORM_FLOAT);
}

void AnimateAndDrawPlayer(Player *player, u32 frame_counter){
            Rectangle src = player->animators[player->facing].frame_rec;
            if (player->facing == DirectionFacing_left) {
                src.x     +=  src.width;
                src.width  = -src.width;
            }
            
            f32 frame_width     = (f32)player->animators[player->facing].frame_rec.width;
            f32 frame_height    = (f32)player->animators[player->facing].texture.height;
            Rectangle dest_rect = {player->pos.x, player->pos.y, 
                                   frame_width, frame_height}; 
            Vector2 texture_offset = {0.0f, 20.0f};
            Animate(&player->animators[player->facing], frame_counter);
            DrawTexturePro(player->animators[player->facing].texture, src,
                           dest_rect, texture_offset, 0.0f, player->col);
}

void UpdateWinScreen(Game_Manager *manager, Win_Screen *screen, f32 delta_t) {
    SetGodFaceType(manager);
    switch (manager->gui.face_type) {
        case GodAnimator_happy:     screen->message = "The Gods are Pleased!";            break;
        case GodAnimator_satisfied: screen->message = "The Gods are Satisfied... Barely"; break;
        case GodAnimator_angry:     screen->message = "The Gods are Unsatisfied...";      break;
    }

    u32 scaling_duration = 3.0f;
    manager->gui.step = Clamp01(manager->gui.step + delta_t / scaling_duration);
    
    screen->font_size = 14;
    f32 frame_w   = (f32)manager->gui.animators[manager->gui.face_type].frame_rec.width;
    f32 frame_h   = (f32)manager->gui.animators[manager->gui.face_type].texture.height;

    f32 start_scale = 1.0f;
    f32 end_scale   = 2.0f;
    f32 current_scale       = Lerp(start_scale, end_scale, manager->gui.step);
    manager->gui.face_scale = current_scale;

    screen->text_pos = {(base_screen_width * 0.5f) - (MeasureText(screen->message, screen->font_size) * 0.5f),
                        base_screen_height * 0.5f};

    screen->start_pos     = {(f32)(manager->gui.bar.width * 0.5f) - (frame_w * current_scale * 0.5f), 0.0f};
    screen->end_pos       = {screen->start_pos.x, screen->text_pos.y - (frame_h * current_scale)};
    manager->gui.face_pos = LerpV2(screen->start_pos, screen->end_pos, manager->gui.step);

    UpdateGodFaceAnimation(manager, delta_t);
    Animate(&manager->gui.animators[manager->gui.face_type], manager->frame_counter);
}

void DrawWinScreenGodFace(Game_Manager *manager) {
    Animation *animation = &manager->gui.animators[manager->gui.face_type];

    Rectangle src_rec = animation->frame_rec;
    Rectangle dest_rec = {manager->gui.face_pos.x, manager->gui.face_pos.y, 
                          animation->frame_rec.width * manager->gui.face_scale,
                          animation->texture.height  * manager->gui.face_scale};
    DrawTexturePro(animation->texture, src_rec, dest_rec, {0,0}, 0.0f, WHITE);
}

f32 WrapMod(f32 pos_x, f32 period) {
    f32 result = fmodf(pos_x, period);
    result     = (result < 0.0f) ? result + period : result;
    return result;
}

void UpdateBackgroundLayer(Background_Layer *layer, f32 delta_t) {
    layer->pos.x += layer->dir * layer->scroll_speed * delta_t;
    f32 width = (f32)layer->texture.width;

    if (layer->dir < 0) {
        layer->pos.x = -WrapMod(-layer->pos.x, width);
    } else {
        layer->pos.x = WrapMod(layer->pos.x, width);
    }
}

void DrawBackgroundLayer(Background_Layer *layer) {
    f32 width = (f32)layer->texture.width;
    f32 x0 = (layer->dir < 0) ? layer->pos.x : layer->pos.x - width;
    f32 x1 = x0 + width;

    DrawTextureV(layer->texture, {x0, layer->pos.y}, WHITE);
    DrawTextureV(layer->texture, {x1, layer->pos.y}, WHITE);
}

void UpdateTitleScreenBackground(Title_Screen_Manager *bg, f32 delta_t) {
    for (u32 index = 0; index < BG_LAYERS; index++) {
        UpdateBackgroundLayer(&bg->layer[index], delta_t);
    }
}

void DrawTitleScreenBackground(Title_Screen_Manager *bg, float current_time) {
    // Draw the non-wobbling layers first to minimise the shader mode switching.
    for (u32 index = 0; index < BG_LAYERS; index++) {
        if (!bg->layer[index].should_wobble) {
            DrawBackgroundLayer(&bg->layer[index]);
        }
    }

    // Then draw the wobble shader layers in a single block
    SetShaderValue(bg->wobble.shader, bg->wobble.time_location, &current_time, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(bg->wobble.shader);
    for (u32 index = 0; index < BG_LAYERS; index++) {
        if (bg->layer[index].should_wobble) {
            DrawBackgroundLayer(&bg->layer[index]);
        }
    }
    EndShaderMode();
}

void UpdateAndDrawFrame(Memory_Arena *arena, Tilemap *map, Game_Manager *manager, Player *player, 
                        End_Screen *end_screen, Event_Manager *event_manager, 
                        Tutorial_Entities *tutorial_entities, Win_Screen *win_screen, 
                        Title_Screen_Manager *title_screen_manager, RenderTexture2D *target) {
    // -----------------------------------
    // Update
    // -----------------------------------
    
    f32 delta_t      = GetFrameTime();
    f32 current_time = GetTime();

    UpdateScreenShake(&manager->screen_shake, delta_t);
    UpdateAlphaFade(manager, delta_t);

    // NOTE: reset the counter back to zero after everything to not mess up 
    // the individual animations
    if (manager->frame_counter >= 60/FRAME_SPEED) {
        manager->frame_counter = 0;
    }
    manager->frame_counter++;

    // Draw to render texture
    BeginTextureMode(*target);
    ClearBackground(BLACK);
        
    if (manager->state == GameState_play) {
        manager->fire_cleared = true;

        SetTimeValueForWobbleShader(&map->wobble, current_time);
        DrawGame(map, manager, player, delta_t);

        // TODO: My unchecked/unsubstantiated theory of why the player movement 
        // feel off and why buttons pressed in quick successsion aren't logged 
        // is because of the is_moving flag. I think maybe while the player is moving 
        // it ignores directions inputs. But as you can see the directions are stored 
        // inside the buffer before is is_moving flag is checked. So i'm not 100% sure, 
        // because I don't see how that could happen. But it definitely feels like inputs  
        // get ignored while the player is moving. But I guess only if the buttons are pressed 
        // in quick succession. Also strangely I feel like when the player picks up speed 
        // and starts to move a lot faster things feel a bit more tighter. This I guess 
        // is also why I think it's related to is_moving because the window where that 
        // bool switches is so much shorter and the late game when the character is moving 
        // really fast.
        StorePlayerDirectionsInBuffer(player);
        if (!player->is_moving) {
            Direction_Facing dir = InputBufferPop(player);
            if (dir == DirectionFacing_none) dir = player->facing;

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
                u32 current_tile_x = (u32)player->pos.x / map->tile_size;
                u32 current_tile_y = (u32)player->pos.y / map->tile_size;
                u32 target_tile_x  = current_tile_x + (u32)input_axis.x;
                u32 target_tile_y  = current_tile_y + (u32)input_axis.y;
                u32 target_tile_index = TilemapIndex(target_tile_x, target_tile_y, map->width);
                Tile *target_tile     = &map->tiles[target_tile_index];

                if (target_tile_x > 0 && target_tile_x < map->width-1 &&
                    target_tile_y > 0 && target_tile_y < map->height-1) {

                    if (target_tile->type != TileType_wall  && 
                        target_tile->type != TileType_none) {
                        
                        // Start moving
                        player->facing     = dir;
                        player->target_pos = {(float)target_tile_x * map->tile_size, (float)target_tile_y * map->tile_size};
                        player->is_moving  = true;

                        u32 current_tile_index = TilemapIndex(current_tile_x, current_tile_y, map->width);
                        Tile *current_tile     = &map->tiles[current_tile_index];
                        if (!player->powered_up) {
                            AddFlag(current_tile, TileFlag_fire);
                        }
                    } else {
                        GameOver(player, map, manager);
                    }
                } else {
                    // Out of bounds
                    GameOver(player, map, manager);
                }

                if (IsFlagSet(target_tile, TileFlag_powerup)) {
                    f32 powerup_duration        = 10.0f;
                    player->powerup_timer       = current_time + powerup_duration;
                    player->powered_up          = true;
                    player->blink_speed         = 5.0f;
                    player->blinking_duration   = player->blink_speed;
                    ClearFlag(target_tile, TileFlag_powerup);
                    if (IsSoundPlaying(manager->sounds[SoundEffect_powerup_end])) {
                        StopSound(manager->sounds[SoundEffect_powerup_end]);
                    }
                    PlaySound(manager->sounds[SoundEffect_powerup_collect]);
                    manager->hype_sound_timer   = 0;
                }

                if (player->powered_up) {
                    //manager.hype_sound_timer += delta_t;
                    if (IsFlagSet(target_tile, TileFlag_fire)) {
                        ClearFlag(target_tile, TileFlag_fire);
                        manager->score += (10 * manager->score_multiplier);
                        BeginScreenShake(&manager->screen_shake, 1.5f, 0.6f, 10.0f); 
                        // Create the text bursts
                        for (int index = 0; index < MAX_BURSTS; index++) {
                            if (!manager->bursts[index].active) {
                                u32 random_index      = GetRandomValue(0, HYPE_WORD_COUNT - 1);
                                const char *word      = manager->hype_text[random_index];
                                // TODO: Settle on what kind of positioning I want to have the create 
                                // text burst appear at.
                                manager->bursts[index] = CreateTextBurst(word);
                                break;
                            }
                        }
                        // Play the hype sounds
                        if (manager->hype_sound_timer <= current_time) {
                            u32 index = GetRandomValue(0, HYPE_WORD_COUNT - 1);
                            while (index == manager->hype_prev_index) {
                                index = GetRandomValue(0, HYPE_WORD_COUNT -1);
                            }
                            ASSERT(index < HYPE_WORD_COUNT);
                            Sound hype_sound = manager->hype_sounds[index];
                            f32 sound_boost  = 3.0f;
                            SetSoundVolume(hype_sound, sound_boost);
                            PlaySound(hype_sound);

                            manager->hype_prev_index  = index;
                            f32 sound_duration        = current_time + 0.90f;
                            manager->hype_sound_timer = sound_duration;
                        }
                    }
                }

                if (IsFlagSet(target_tile, TileFlag_fire) || IsFlagSet(target_tile, TileFlag_enemy)) {
                    GameOver(player, map, manager);
                }
            }
        } else {
            // Move towards target position
            Vector2 direction = VectorSub(player->target_pos, player->pos);
            float distance    = Length(direction);
            if (distance <= player->speed * delta_t) {
                player->pos       = player->target_pos;
                player->is_moving = false;

                u32 current_tile_x = (u32)player->pos.x / map->tile_size;
                u32 current_tile_y = (u32)player->pos.y / map->tile_size;

                FillEnclosedAreas(arena, map, player, manager, current_tile_x, current_tile_y);
            } else {
                direction        = VectorNorm(direction);
                Vector2 movement = VectorScale(direction, player->speed * delta_t);
                player->pos       = VectorAdd(player->pos, movement);
            }
        }

        if (player->powered_up) {
            f32 end_duration_signal  = 3.0f;
            // TODO: I've set up a seperate frame counter here for the water that I can double 
            // or halve to speed the animation up or slow it down. Perhaps a better implementation 
            // in the future would be to have the animate function take a speed multiplier that can 
            // effect the global frame counter for the individual animation. This would be better 
            // than each animation that wants a speed change to have it's own counter. Right now 
            // only this animation in the game that wants to change it's speed so 
            // this current implemenation is fine for now.
            u32 water_frame_counter  = manager->frame_counter;

            // TODO: I'm moving the volume value by a set amount which isn't very frame independant. 
            // I should actually increment and decrement by some rate * delta_t.
            manager->play_song_volume       -= 0.02f;
            manager->play_muted_song_volume += 0.02f;

            Sound powerup_effect     = manager->sounds[SoundEffect_powerup];
            Sound powerup_end_effect = manager->sounds[SoundEffect_powerup_end];

            if(!IsSoundPlaying(powerup_effect) && !IsSoundPlaying(powerup_end_effect))
            {
                PlaySound(powerup_effect);
                SetSoundVolume(powerup_effect, 1.5f);
            }

            if (player->powerup_timer < current_time) { // Powerup is over.
                player->powered_up = false;
                player->col_bool   = false;
                manager->score_multiplier = 1;
                StopSound(powerup_effect);
                StopSound(powerup_end_effect);
            } else {
                if ((player->powerup_timer - end_duration_signal) < current_time) { // Powerup over soon warning.
                    player->blink_speed   = 2.0f; 
                    water_frame_counter *= 2;

                    if (!IsSoundPlaying(powerup_end_effect)) {
                        StopSound(powerup_effect);
                        PlaySound(powerup_end_effect);
                        SetSoundVolume(powerup_end_effect, 2.0f);
                    }
                }

                if (player->blinking_duration > 0) {
                player->blinking_duration -= 1.0f;
                } else {
                    player->blinking_duration = player->blink_speed;
                    player->col_bool          = !player->col_bool;
                }
            }

            Animate(&player->animators[PlayerAnimator_water], water_frame_counter);
            DrawTextureRec(player->animators[PlayerAnimator_water].texture, 
                           player->animators[PlayerAnimator_water].frame_rec, player->target_pos, WHITE);
            if (player->col_bool) {
                player->col = BLUE;
            } else {
                player->col = WHITE;
            }
        } else {
            manager->play_song_volume       += 0.02f;
            manager->play_muted_song_volume -= 0.02f;
        }

        // TODO: I need to set up a better way to crossfade these tracks. Right not this is the 
        // only track I fade so it's probably fine 
        manager->play_song_volume       = CLAMP(manager->play_song_volume,  0.0f, 1.0f);
        manager->play_muted_song_volume = CLAMP(manager->play_muted_song_volume, 0.0f, 1.0f);
        SetMusicVolume(manager->song[Song_play],       manager->play_song_volume);
        SetMusicVolume(manager->song[Song_play_muted], manager->play_muted_song_volume);

        // Enemy spawning
        if (manager->spawn_timer > 0) {
            // TODO: This right now is frame dependant. I should decrement by delta_t 
            // and set the enemy_spawn_duration to reflect the real world seconds I 
            // want to wait.
            manager->spawn_timer -= 1.0f;
        } else {
            manager->spawn_timer = manager->enemy_spawn_duration;

            s32 player_tile_x     = (u32)player->pos.x / map->tile_size; 
            s32 player_tile_y     = (u32)player->pos.y / map->tile_size; 
            u32 player_tile_index = TilemapIndex(player_tile_x, player_tile_y, map->width);

            u32 tile_index = GetRandomEmptyTileIndex(map, player_tile_index);
            if (tile_index) {
                Tile *tile = &map->tiles[tile_index];
                AddFlag(tile, TileFlag_enemy);

                // Add enemy
                Enemy *new_enemy = (Enemy *)ArenaAlloc(arena, sizeof(Enemy));
                EnemyInit(new_enemy, &manager->enemy_sentinel, tile_index);
            }
        }

        // Enemy movement
        if (manager->enemy_move_timer > 0) {
            // TODO: Again this timer is frame dependant. Needs to decrement by delta_t 
            manager->enemy_move_timer -= 1.0f;
        } else {
            for (u32 y = 0; y < map->height; y++) {
                for (u32 x = 0; x < map->width; x++) {
                    u32 tile_index = TilemapIndex(x, y, map->width); 
                    Tile *tile     = &map->tiles[tile_index];

                    // The moved flag is necessary so that enemies don't end up moving multiple times.
                    if (IsFlagSet(tile, TileFlag_enemy) && !IsFlagSet(tile, TileFlag_moved)) {
                        u32 eligible_tile_index = FindEligibleTileIndexForEnemyMove(map, tile_index); 
                        if (eligible_tile_index) {
                            Tile *eligible_tile = &map->tiles[eligible_tile_index];
                            Enemy *found_enemy = FindEnemyAtTile(&manager->enemy_sentinel, tile_index);
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
            // TODO: Need to make move duration happen in seconds and decrement the timer 
            // by delta_t;
            manager->enemy_move_timer = manager->enemy_move_duration;
        }

        AnimateAndDrawPlayer(player, manager->frame_counter);
        UpdateAndDrawAllTextBursts(manager, delta_t);

        // TODO: Take this out of the game before shipping.
        if (IsKeyPressed(KEY_P)) {
            manager->state = GameState_win;
        }

    } else if (manager->state == GameState_win) {
        SetTimeValueForWobbleShader(&map->wobble, current_time);
        DrawGame(map, manager, player, delta_t);

        UpdateAndDrawAllTextBursts(manager, delta_t);
        StopSoundBuffer(manager->sounds);
        BeginScreenShake(&manager->screen_shake, 4.0f, 5.0f, 10.0f);
        // Hard coding the facing direction here so constantly play 
        // the win celebration animation.
        player->facing = DirectionFacing_celebration;
        AnimateAndDrawPlayer(player, manager->frame_counter);

        if (win_screen->white_screen.alpha == 0.0f) {
            AlphaFadeIn(manager, &win_screen->white_screen, 5.0f);
        } else if (win_screen->white_screen.alpha == 1.0f) {
            manager->state = GameState_win_text;
        }
        DrawScreenFadeCol(&win_screen->white_screen, base_screen_width, base_screen_height, WHITE);
        DrawGodFace(manager, delta_t);

    } else if (manager->state == GameState_win_text) {
        Event_Queue *win_sequence = &event_manager->sequence[Sequence_win];
        UpdateEventQueue(win_sequence, manager, delta_t);

        DrawScreenFadeCol(&win_screen->white_screen, base_screen_width, base_screen_height, WHITE);

        // TODO: This feels a bit like spaghetti code as lots of things are dependant on each other 
        // and it makes the flow hard to reason about at a glance.
        UpdateWinScreen(manager, win_screen, delta_t);
        DrawWinScreenGodFace(manager);

        if (!win_sequence->active) StartEventSequence(win_sequence);
        Event event = win_sequence->events[1];
        DrawTextTripleEffect(win_screen->message, win_screen->text_pos, win_screen->font_size, event.fadeable.alpha); 

        UpdateSpacebarBob(&manager->spacebar_text, delta_t);
        DrawTextTripleEffect(manager->spacebar_text.text, manager->spacebar_text.pos, manager->spacebar_text.size, 
                             win_sequence->events[3].fadeable.alpha); 

        if (IsKeyPressed(KEY_SPACE)) {
            win_sequence->active = false;
            ResetEvents(event_manager);
            manager->gui.step = 0.0f;
            if (manager->gui.face_type != GodAnimator_happy) {
                win_screen->white_screen.alpha = 0.0f;
                manager->state = GameState_title;
            }
            else {
                manager->state = GameState_epilogue;
            }
        }

    } else if (manager->state == GameState_epilogue) {
        if (win_screen->white_screen.alpha == 1.0f)
        {
            AlphaFadeOut(manager, &win_screen->white_screen, 5.0f);
        }
        end_screen->timer += delta_t;
        BeginShaderMode(end_screen->shaders[EndLayer_sky].shader);
        SetTimeValueForWobbleShader(&end_screen->shaders[EndLayer_sky], current_time);
        DrawTextureV(end_screen->textures[EndLayer_sky], {0, 0}, WHITE);
        EndShaderMode();
        SetTimeValueForWobbleShader(&end_screen->shaders[EndLayer_trees], current_time);
        BeginShaderMode(end_screen->shaders[EndLayer_trees].shader);
        DrawTextureV(end_screen->textures[EndLayer_trees], {-30.0f, 0}, WHITE);
        EndShaderMode();
        Animate(&end_screen->animator, manager->frame_counter);
        DrawTextureRec(end_screen->animator.texture, 
                       end_screen->animator.frame_rec, {0, 0}, WHITE);
        // TODO: I could probably make this random duration animation code 
        // a function because the god face uses the exact same code. I don't 
        // think anyone else uses this at the moment so it's perhaps unecessary
        // right now though. Something to keep an eye on.
        if (end_screen->timer > end_screen->blink_duration) {
            end_screen->animator.current_frame = 0;
            end_screen->timer = 0;
            end_screen->blink_duration = GetRandomValue(0.5f, 3.0f);
        }

        DrawScreenFadeCol(&win_screen->white_screen, base_screen_width, base_screen_height, WHITE);
        if (IsKeyPressed(KEY_SPACE)) {
            ResetEvents(event_manager);
            win_screen->white_screen.alpha = 0.0f;
            manager->state = GameState_title;
        }

    } else if (manager->state == GameState_tutorial) {
        Event_Queue *tutorial    = &event_manager->sequence[Sequence_tutorial];
        UpdateEventQueue(tutorial, manager, delta_t);
        DrawRectangle(0, 0, base_screen_width, base_screen_height, BLACK);
        if (!tutorial->active) StartEventSequence(tutorial);

        const char *sacred_fire  = "Clear all of the fire to complete the ritual";
        const char *powerup      = "Collect powerups to clear the fire";
        const char *demon        = "Sacrifice demons by trapping them in fire";
        u32 font_size            = 7;
        f32 icon_padding         = 10.0f;
        f32 text_pos_x           = (base_screen_width*0.5f) - ((MeasureText(sacred_fire, font_size) - 
                                   (SPRITE_WIDTH + icon_padding))*0.5f);
        Vector2 powerup_text_pos = {text_pos_x, (base_screen_height*0.5f) - font_size};
        Vector2 demon_text_pos   = {text_pos_x, powerup_text_pos.y - font_size*4};
        Vector2 fire_text_pos    = {text_pos_x, powerup_text_pos.y + font_size*4};
        DrawTextTripleEffect(demon,       demon_text_pos,   font_size, tutorial->events[0].fadeable.alpha);
        DrawTextTripleEffect(powerup,     powerup_text_pos, font_size, tutorial->events[1].fadeable.alpha);
        DrawTextTripleEffect(sacred_fire, fire_text_pos,    font_size, tutorial->events[2].fadeable.alpha);

        UpdateSpacebarBob(&manager->spacebar_text, delta_t);
        DrawTextTripleEffect(manager->spacebar_text.text, manager->spacebar_text.pos, font_size, 
                             tutorial->events[4].fadeable.alpha);
        Vector2 demon_pos   = {(f32)demon_text_pos.x - tutorial_entities->enemy.frame_rec.width - icon_padding, 
                               (f32)demon_text_pos.y - (tutorial_entities->enemy.frame_rec.height*0.5f)-font_size};
        Vector2 powerup_pos = {(f32)powerup_text_pos.x - tutorial_entities->powerup.frame_rec.width - icon_padding, 
                               (f32)powerup_text_pos.y - font_size};
        Vector2 fire_pos    = {(f32)fire_text_pos.x - tutorial_entities->fire.frame_rec.width - icon_padding, 
                               (f32)fire_text_pos.y - font_size};
        Animate(&tutorial_entities->enemy, manager->frame_counter);
        DrawTextureRec(tutorial_entities->enemy.texture, tutorial_entities->enemy.frame_rec, 
                       demon_pos, Fade(WHITE, tutorial->events[0].fadeable.alpha)); 
        Animate(&tutorial_entities->powerup, manager->frame_counter);
        DrawTextureRec(tutorial_entities->powerup.texture, tutorial_entities->powerup.frame_rec, 
                       powerup_pos, Fade(WHITE, tutorial->events[1].fadeable.alpha)); 
        SetTimeValueForWobbleShader(&map->wobble, current_time);
        BeginShaderMode(map->wobble.shader);
        Animate(&tutorial_entities->fire, manager->frame_counter);
        DrawTextureRec(tutorial_entities->fire.texture, tutorial_entities->fire.frame_rec, 
                       fire_pos, Fade(WHITE, tutorial->events[2].fadeable.alpha)); 
        EndShaderMode();

        if (IsKeyPressed(KEY_SPACE)) {
            tutorial->active = false;
            GameOver(player, map, manager);
        }

    } else if (manager->state == GameState_title) {
        Event_Queue *title_press = &event_manager->sequence[Sequence_begin];
        UpdateEventQueue(title_press, manager, delta_t);
        Game_Title *title = &title_screen_manager->title;
        UpdateTitleBob(title, delta_t);
        manager->should_title_music_play = !title_press->active;

        if (!IsMusicStreamPlaying(manager->song[Song_intro]) && !title_press->active)  
        {
            TriggerTitleBob(title, 5.0f);
        } else {
            UpdateMusicStream(manager->song[Song_intro]);
        }

        if (IsKeyPressed(KEY_P)) TriggerTitleBob(title, 150.0f);

        UpdateTitleScreenBackground(title_screen_manager, delta_t);
        DrawTitleScreenBackground(title_screen_manager, current_time);

        Vector2 draw_pos = {title->pos.x, title->pos.y += title->bob};
        if (title->pos.y > base_screen_height) {
            title->pos.y = 0.0f - title->texture.height;
        }
        DrawTextureEx(title->texture, {draw_pos.x-4.0f, draw_pos.y+4.0f}, 0.0f, title->scale, BLACK);
        DrawTextureEx(title->texture, draw_pos, 0.0f, title->scale, WHITE);

        Play_Text *play_text = &title_screen_manager->play_text;
        play_text->pos.y     = draw_pos.y + 80.0f;
        play_text->pos.y    += 1.0f*sinf(8.0f*play_text->bob);
        play_text->bob      += delta_t;
        DrawTextTripleEffect(play_text->text, play_text->pos, play_text->font_size);
        
        if (IsKeyPressed(KEY_SPACE)) {
            if (!title_press->active) {
                StartEventSequence(title_press);
            }
            if (!IsSoundPlaying(manager->sounds[SoundEffect_spacebar])) {
                PlaySound(manager->sounds[SoundEffect_spacebar]);
            }
        }

        if (title_press->active) {
            f32 pulse = (sinf(current_time * 48.0f) * 0.5f + 0.5f);
            u32 alpha = (u32)(pulse * 255);
            Color flash_col = {255, 255, 255, (u8)alpha};
            DrawText(play_text->text, play_text->pos.x, play_text->pos.y, play_text->font_size, flash_col);
        }
    }

    // TODO: Play the music here after the game logic has occured to make 
    // sure that all the tracks are playing correctly on thier exact frames 
    // they are supposed to and not a frame behind.
    PlayAllMusicForGameCorrectly(manager);
    EndTextureMode();

    // -----------------------------------
    // Draw
    // -----------------------------------

    // NOTE: Draw the render texture to the screen, scaling it with window size
    BeginDrawing();
    ClearBackground(DARKGRAY);

    f32 scale_x = (f32)WINDOW_WIDTH  / base_screen_width;
    f32 scale_y = (f32)WINDOW_HEIGHT / base_screen_height;
    Vector2 shake_offset = GetScreenShakeOffset(&manager->screen_shake);

    Rectangle dest_rect = {((WINDOW_WIDTH  - (base_screen_width  * scale_x)) * 0.5f) + shake_offset.x,
                           ((WINDOW_HEIGHT - (base_screen_height * scale_y)) * 0.5f) + shake_offset.y,
                           (base_screen_width * scale_x), (base_screen_height * scale_y)};
    Rectangle rect   = {0.0f, 0.0f, (f32)target->texture.width, -(f32)target->texture.height}; 
    Vector2 zero_vec = {0, 0};
    DrawTexturePro(target->texture, rect, dest_rect, zero_vec, 0.0f, WHITE);
    if (manager->state == GameState_play || manager->state == GameState_win || 
        manager->state == GameState_win_text) {
        u32 font_size     = 38;
        f32 text_pos_x    = 192 + shake_offset.x;
        f32 text_pos_y    = 25  + shake_offset.y;
        u32 shadow_offset = 2;

        if (manager->state != GameState_win_text) {
            DrawTextDoubleEffect(TextFormat("%d", manager->score), {text_pos_x, text_pos_y}, font_size);
        } else {
            DrawTextTripleEffect(TextFormat("%d", manager->score), {text_pos_x, text_pos_y}, font_size);
        }
        
        if (manager->state == GameState_play || manager->state == GameState_win) {
            const char *combo = manager->score_multiplier > 1 ? 
                                TextFormat("%d Combo", manager->score_multiplier) :
                                ("0 Combo");
            DrawText(combo, WINDOW_WIDTH - ((text_pos_x + shadow_offset) + MeasureText(combo, font_size)), 
                     (text_pos_y + shadow_offset), font_size, BLACK);
            DrawText(combo, WINDOW_WIDTH - (text_pos_x + MeasureText(combo, font_size)), 
                     text_pos_y, font_size, WHITE);
        }
    };

    if (manager->fire_cleared && player->powered_up) {
        if (manager->state == GameState_play) {
            manager->state = GameState_win;
        }
    }

    EndDrawing();
    // -----------------------------------
}

int main() {
    // -------------------------------------
    // Initialisation
    // -------------------------------------

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Anunnaki");
    InitAudioDevice();

    // TODO: I don't really know how I feel about this living here. At least if it's 
    // here I can initialise it how I want it straight away. If I put it into the 
    // game manager struct then it's more annoying to initialise this array. I'd 
    // probably have to loop over the array... so I guess for now it can live here 
    // until I can come up with a clearly better solution.
    const char *hype_text[HYPE_WORD_COUNT] = {"WOW",      "YEAH",   "AMAZING",      "SANCTIFY", 
                                              "HOLY COW", "DIVINE", "UNBELIEVABLE", "WOAH",
                                              "AWESOME",  "COSMIC", "RITUALISTIC",  "LEGENDARY"};

    Tilemap map;
    TilemapInit(&map);
    u32 tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        { 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, },
        { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },

    };
    Tile tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH];

    map.original_map = &tilemap[0][0];
    map.tiles        = &tiles[0][0];
    TileInit(&map);

    Player player;
    PlayerInit(&player);
    // I'm seperating initialising the player animators from 
    // the rest of the initialisation because I re-init the player
    // on a game over to set the player back to default values. I 
    // do not need to re-init the player animators at game over though.
    PlayerAnimatorInit(&player);

    Game_Manager manager;
    GameManagerInit(&manager);
    manager.hype_text = hype_text;

    Title_Screen_Manager title_screen_manager;
    TitleScreenManagerInit(&title_screen_manager);

    End_Screen end_screen;
    EndScreenInit(&end_screen);

    Event_Manager event_manager;
    SetupEventSequences(&event_manager);

    Tutorial_Entities tutorial_entities;
    TutorialAnimationInit(&tutorial_entities);

    Win_Screen win_screen = {};

    Memory_Arena arena;
    size_t arena_size = 1024*1024;
    ArenaInit(&arena, arena_size); 

    RenderTexture2D target   = LoadRenderTexture(base_screen_width, base_screen_height); 
    SetTargetFPS(60);
    // -------------------------------------
    // Main Game Loop
    while (!WindowShouldClose()) {
        UpdateAndDrawFrame(&arena, &map, &manager, &player, &end_screen, &event_manager, 
                           &tutorial_entities, &win_screen, &title_screen_manager, &target);
    }
    // -------------------------------------
    // De-Initialisation
    // -------------------------------------
    // TODO: Need to make sure I unload the music and probably the textures.
    UnloadAllSoundBuffers(&manager);
    CloseAudioDevice();
    CloseWindow();
    // -------------------------------------
    return 0;
}
