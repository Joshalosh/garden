
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
    tilemap->tile_size   = TILEMAP_SIZE;
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

void PlayerInit(Player *player) {
    player->pos           = {base_screen_width*0.5, base_screen_height*0.5};
    player->target_pos    = player->pos;
    player->size          = {20, 20};
    player->col           = RED;
    player->speed         = 100.0f;
    player->is_moving     = false;
    player->powered_up    = false;
    player->powerup_timer = 0;
    player->blink_speed   = 0;
    player->blink_time    = 0;
    player->col_bool      = false;
}

void EnemyInit(Enemy *enemy) {
    enemy->pos        = {5, 5};
    enemy->target_pos = enemy->pos;
    enemy->size       = {20, 20};
    enemy->col        = YELLOW;
    enemy->speed      = 25.0f;
}

void GameOver(Player *player, Tilemap *tilemap, u32 *score, u32 *high_score, Game_State *state) {
    PlayerInit(player);
    if (*score > *high_score) {
        *high_score = *score;
    }
    *score = 0;
    *state = GameState_play;

    // Reset the tilemap back to it's original orientation
    for (u32 y = 0; y < tilemap->height; y++) {
        for (u32 x = 0; x < tilemap->width; x++) {
            u32 index = TilemapIndex(x, y, tilemap->width);
            tilemap->tiles[index].type = (Tile_Type)tilemap->original_map[index];
            tilemap->tiles[index].flags = 0;
        }
    }
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

void ModifyRandomTile(Tilemap *tilemap, Tile_Flags flag) {
    bool found_empty_tile = false;
    while (!found_empty_tile) {
        u32 random_x = GetRandomValue(1, tilemap->width - 2);
        u32 random_y = GetRandomValue(1, tilemap->height - 2);

        u32 index = TilemapIndex(random_x, random_y, tilemap->width);
        Tile *tile = &tilemap->tiles[index];

        if (!IsFlagSet(tile, TileFlag_fire) && !IsFlagSet(tile, TileFlag_powerup) && 
            !IsFlagSet(tile, TileFlag_enemy)) {
            AddFlag(tile, flag);
            found_empty_tile = true;
        }
    }
}

// TODO: Enemies aren't moving as intended, I should check this function and see 
// if eligible tiles are being calculated correctly
Tile *FindEligibleTile(Tilemap *tilemap, u32 index) {
    u32 right_tile   = index + 1;
    u32 left_tile    = index - 1;
    u32 bottom_tile  = index + tilemap->width;
    u32 top_tile     = index - tilemap->width;

    u32 adjacent_tile_indexes[ADJACENT_COUNT] = {right_tile, left_tile, bottom_tile, top_tile};
    u32 eligible_tiles[ADJACENT_COUNT] = {};
    u32 eligible_count = 0;
    Tile *tile;

    for (int adjacent_index = 0; adjacent_index < ARRAY_COUNT(adjacent_tile_indexes); adjacent_index++) {
        tile = &tilemap->tiles[adjacent_tile_indexes[adjacent_index]];

        if(tile->type == TileType_grass || tile->type == TileType_dirt) {
            if (!IsFlagSet(tile, TileFlag_fire) && !IsFlagSet(tile, TileFlag_powerup) && 
                !IsFlagSet(tile, TileFlag_enemy)) {
                eligible_tiles[eligible_count] = adjacent_tile_indexes[adjacent_index];
                eligible_count++;
            }
        }
    }

    tile = NULL;
    if (eligible_count) {
        u32 eligible_index = eligible_count - 1;
        u32 random_index = GetRandomValue(0, eligible_index);
        tile = &tilemap->tiles[eligible_tiles[random_index]];
    }

    return tile;
}

void CheckEnclosedAreas(Tilemap *tilemap, u32 current_x, u32 current_y) {
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
                        ClearFlag(tile, TileFlag_enemy);
                        enemy_slain++;
                    }
                } 
            } 

            ClearFlag(tile, TileFlag_visited);
        }
    }


    if (has_flood_fill_happened) {
        while (enemy_slain) {
            ModifyRandomTile(tilemap, TileFlag_powerup);
            enemy_slain--;
        }
        has_flood_fill_happened = false;
    }
}

int main() {
    // -------------------------------------
    // Initialisation
    // -------------------------------------

    const int window_width  = 1280;
    const int window_height = 1280; //720;
    InitWindow(window_width, window_height, "Raylib basic window");

    Tilemap map;
    TilemapInit(&map);
    u32 tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, },
        {1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        {4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        {1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        {4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        {1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        {4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        {1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        {4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        {1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        {4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        {1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        {4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        {1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, },
        {4, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, },
        {1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, },
    };
    //map.tiles = (u32 *)tilemap;
    u32 original_map[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    Tile tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH];

    // NOTE: TILE INIT
    for (u32 y = 0; y < TILEMAP_HEIGHT; y++) {
        for (u32 x = 0; x < TILEMAP_WIDTH; x++) {
            original_map[y][x] = tilemap[y][x];
            tiles[y][x].type   = (Tile_Type)tilemap[y][x];
            tiles[y][x].flags  = 0;
        }
    }

    map.original_map = (u32 *)&original_map;
    map.tiles        = (Tile *)&tiles;

    Player player = {};
    PlayerInit(&player);
    Vector2 input_axis = {0, 0};

    f32 enemy_spawn_duration = 500.0f;
    f32 spawn_timer          = enemy_spawn_duration;
    f32 enemy_move_duration  = 250.0f;
    f32 enemy_move_timer     = enemy_move_duration;

    u32 score                = 0;
    u32 high_score           = 0;

    b32 game_win             = false;
    Game_State state         = GameState_play;

    b32 fire_cleared;

    RenderTexture2D target = LoadRenderTexture(base_screen_width, base_screen_height); 
    SetTargetFPS(60);
    // -------------------------------------
    // Main Game Loop
    while (!WindowShouldClose()) {
        // -----------------------------------
        // Update
        // -----------------------------------
        
        // TODO: Need to figure out a better way to stop everything when a win has occured
        float delta_t = GetFrameTime();
        if (state == GameState_play) {

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

            if      (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input_axis = {1.0f, 0};
            else if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) input_axis = {-1.0f, 0};
            else if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) input_axis = {0, -1.0f};
            else if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) input_axis = {0, 1.0f};

            // - New player movement
            if (!player.is_moving) {

                if (input_axis.x != 0 || input_axis.y != 0) {
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
                            player.target_pos = {(float)target_tile_x * map.tile_size, (float)target_tile_y * map.tile_size};
                            player.is_moving  = true;

                            u32 current_tile_index = TilemapIndex(current_tile_x, current_tile_y, map.width);
                            Tile *current_tile = &map.tiles[current_tile_index];
                            if (!player.powered_up) {
                                AddFlag(current_tile, TileFlag_fire);
                            }
                            //current_tile->type = TileType_fire;
                        } 
                        else {
                            GameOver(&player, &map, &score, &high_score, &state);
                        }
                    } else {
                        // Out of bounds
                        GameOver(&player, &map, &score, &high_score, &state);
                    }

                    if (IsFlagSet(target_tile, TileFlag_powerup)) {
                        float powerup_duration = 10.0f;
                        player.powerup_timer = GetTime() + powerup_duration;
                        player.powered_up  = true;
                        player.blink_speed = 5.0f;
                        player.blink_time = player.blink_speed;
                        ClearFlag(target_tile, TileFlag_powerup);
                    }

                    if (player.powered_up) {
                        if (IsFlagSet(target_tile, TileFlag_fire)) {
                            ClearFlag(target_tile, TileFlag_fire);
                            score += 10;
                        }
                    }

                    if (IsFlagSet(target_tile, TileFlag_fire) || IsFlagSet(target_tile, TileFlag_enemy)) {
                        GameOver(&player, &map, &score, &high_score, &state);
                    }
                }
            } else {
                // MOTE: Move towards target position
                Vector2 direction = VectorSub(player.target_pos, player.pos);
                float distance    = Length(direction);
                if (distance <= player.speed * delta_t) {
                    player.pos = player.target_pos;
                    player.is_moving  = false;

                    u32 current_tile_x = (u32)player.pos.x / map.tile_size;
                    u32 current_tile_y = (u32)player.pos.y / map.tile_size;

                    // Check for enclosed areas
                    //if (player.powerup_timer < GetTime()) {
                        CheckEnclosedAreas(&map, current_tile_x, current_tile_y);
                    //}
                } else {
                    direction = VectorNorm(direction);
                    Vector2 movement = VectorScale(direction, player.speed * delta_t);
                    player.pos = VectorAdd(player.pos, movement);
                }
            }

            // NOTE: Powerup blinking
            if (player.powered_up) {
                f32 end_duration_signal = 3.0f;
                if (player.powerup_timer < GetTime()) {
                    player.powered_up = false;
                    player.col_bool   = false;
                } else {
                    if (player.powerup_timer - end_duration_signal < GetTime()) {
                        player.blink_speed = 2.0f; 
                    }

                    if (player.blink_time > 0) {
                    player.blink_time -= 1.0f;
                    } else {
                        player.blink_time =  player.blink_speed;
                        player.col_bool   = !player.col_bool;
                    }
                }

                if (player.col_bool) {
                    player.col = WHITE;
                } else {
                    player.col = RED;
                }
            }

            if (spawn_timer > 0) {
                spawn_timer -= 1.0f;
            } else {
                ModifyRandomTile(&map, TileFlag_enemy);
                spawn_timer = enemy_spawn_duration;
            }

            // NOTE: Enemy spawning
            if (enemy_move_timer > 0) {
                enemy_move_timer -= 1.0f;
            } else {
                for (u32 y = 0; y < map.height; y++) {
                    for (u32 x = 0; x < map.width; x++) {
                        u32 tile_index = TilemapIndex(x, y, map.width); 
                        Tile *tile = &map.tiles[tile_index];

                        if (IsFlagSet(tile, TileFlag_enemy) && !IsFlagSet(tile, TileFlag_moved)) {
                            Tile *eligible_tile = FindEligibleTile(&map, tile_index); 

                            if (eligible_tile) {
                                ClearFlag(tile, TileFlag_enemy);
                                AddFlag(eligible_tile, TileFlag_enemy);
                                AddFlag(eligible_tile, TileFlag_moved);
                            }
                        }
                    }
                }
                enemy_move_timer = enemy_move_duration;
            }

            DrawRectangleV(player.pos, player.size, player.col);
        } else if (state == GameState_win) {
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
            
            if (IsKeyPressed(KEY_SPACE)) {
                GameOver(&player, &map, &score, &high_score, &state);
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
        DrawText(TextFormat("Score: %d", score), 25, 25, 38, WHITE);
        DrawText(TextFormat("High Score: %d", high_score), window_width - 300, 25, 38, WHITE);

        if (fire_cleared && player.powered_up) {
            DrawText("WIN", window_width*0.5, window_height*0.5, 69, WHITE);
            state = GameState_win;
        }

        EndDrawing();
        // -----------------------------------
    }
    // -------------------------------------
    // De-Initialisation
    // -------------------------------------
    CloseWindow();
    // -------------------------------------
    return 0;
}
