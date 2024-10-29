
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "mymath.h"
#include "memory.h"
#include "garden.h"

void TilemapInit(Tilemap *tilemap) {
    tilemap->width     = TILEMAP_WIDTH;
    tilemap->height    = TILEMAP_HEIGHT;
    tilemap->tile_size = 20;
    tilemap->tiles     = 0;
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

void PlayerInit(Player *player) {
    player->pos = {base_screen_width*0.5, base_screen_height*0.5};
    player->target_pos = player->pos;
    player->size       = {20, 20};
    player->speed      = 50.0f;
    player->is_moving  = false;
    player->path_len   = 0;
}

void GameOver(Player *player) {
    player->is_moving = false;
    Vector2 start_pos = {base_screen_width*0.5, base_screen_height*0.5};
    player->pos = start_pos;
    player->target_pos = start_pos;
    player->path_len = 0;
}

void FloodFill(Tilemap *tilemap, u32 x, u32 y, u32 replacement_tile) {
    if (TileType_grass == replacement_tile || TileType_dirt == replacement_tile) return;

    StackU32 nodes;
    StackInit(&nodes);
    StackPush(&nodes, x, y);

    while (StackPop(&nodes, &x, &y)) {
        u32 tilemap_index = y * tilemap->width + x;
        if (x < 0 || x >= TILEMAP_WIDTH || y < 0 || y >=TILEMAP_HEIGHT) continue;
        if (tilemap->tiles[tilemap_index] != TileType_grass && 
            tilemap->tiles[tilemap_index] != TileType_dirt) continue;

        tilemap->tiles[tilemap_index] = replacement_tile;

        StackPush(&nodes, x+1, y);
        StackPush(&nodes, x-1, y);
        StackPush(&nodes, x, y+1);
        StackPush(&nodes, x, y-1);
    }
}

u32 TilemapIndex(Tilemap tilemap, u32 x, u32 y) {
    u32 result = y * tilemap.width + x;
    return result;
}

#if 0
void FloodFillFromBorders(Tilemap *tilemap) {
    StackU32 nodes;
    StackInit(&nodes);
    u32 x, y;

    // Add border tiles to the stack
    for (x = 1; x < (s32)tilemap->width; x++) {
        // Top border
        y = 1;
        u32 index = TilemapIndex(*tilemap, x, y);
        if (tilemap->tiles[index] == TileType_grass || tilemap->tiles[index] == TileType_dirt) {
            StackPush(&nodes, x, y);
        }
        // Bottom border
        y = tilemap->height - 1;
        index = TilemapIndex(*tilemap, x, y);
        if (tilemap->tiles[index] == TileType_grass || tilemap->tiles[index] == TileType_dirt) {
            StackPush(&nodes, x, y);
        }
    }
    for (y = 1; y < tilemap->height; y++)
    {
        // Left border
        x = 1;
        u32 index = TilemapIndex(*tilemap, x, y);
        if (tilemap->tiles[index] == TileType_grass || tilemap->tiles[index] == TileType_dirt) {
            StackPush(&nodes, x, y);
        }
        // Right border
        x = tilemap->width - 1;
        index = TilemapIndex(*tilemap, x, y);
        if (tilemap->tiles[index] == TileType_grass || tilemap->tiles[index] == TileType_dirt) {
            StackPush(&nodes, x, y);
        }
    }

    // Flood fill from borders
    while (StackPop(&nodes, &x, &y)) {
        if (x < 0 || x >= (s32)tilemap->width || y < 0 || y >= (s32)tilemap->height) continue;

        u32 index = TilemapIndex(*tilemap, x, y);
        u32 tile  = tilemap->tiles[index];

        if (tile == TileType_grass) {
            tilemap->tiles[index] = TileType_temp_grass;
        } else if (tile == TileType_dirt) {
            tilemap->tiles[index] = TileType_temp_dirt;
        } else continue;

        // Add adjacent tiles
        StackPush(&nodes, x+1, y);
        if (x > 0) StackPush(&nodes, x-1, y);
        StackPush(&nodes, x, y+1);
        if (y > 0) StackPush(&nodes, x, y-1);
    }
}
#endif

void FloodFillFromPlayerPosition(Tilemap *tilemap, u32 start_x, u32 start_y) {
    StackU32 nodes;
    StackInit(&nodes);

    StackPush(&nodes, start_x, start_y);

    u32 x, y;

    // Flood fill from players position
    while(StackPop(&nodes, &x, &y)) {
        if (x < 0 || x >= (u32)tilemap->width || y < 0 || y >= tilemap->height) continue;

        u32 index = TilemapIndex(*tilemap, (u32)x, (u32)y);
        u32 tile  = tilemap->tiles[index];

        if (tile == TileType_grass) {
            tilemap->tiles[index] = TileType_temp_grass;
        } else if (tile == TileType_dirt) {
            tilemap->tiles[index] = TileType_temp_dirt;
        } else continue;

        // Add adjacent tiles 
        StackPush(&nodes, x+1, y);
        StackPush(&nodes, x-1, y);
        StackPush(&nodes, x, y+1);
        StackPush(&nodes, x, y-1);
    }
}

void CheckEnclosedAreas(Tilemap *tilemap, u32 current_x, u32 current_y) {
    // Flood fill from borders to mark reachable areas
    FloodFillFromPlayerPosition(tilemap, current_x, current_y);

    // Any grass or dirt tiles not marked are enclosed
    for (u32 y = 0; y < (s32)tilemap->height; y++) {
        for (u32 x = 0; x < (s32)tilemap->width; x++) {
            u32 index = TilemapIndex(*tilemap, x, y);
            u32 tile  = tilemap->tiles[index];

            if (tile == TileType_grass || tile == TileType_dirt) {
                // Tile is enclosed so fill it
                tilemap->tiles[index] = TileType_fire;
            } else if (tile == TileType_temp_grass) {
                // Restore originla tile type
                tilemap->tiles[index] = TileType_grass;
            } else if (tile == TileType_temp_dirt) {
                tilemap->tiles[index] = TileType_dirt;
            }
        }
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
    map.tiles = (u32 *)tilemap;

    u32 original_tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    for (u32 y = 0; y < TILEMAP_HEIGHT; y++) {
        for (u32 x = 0; x < TILEMAP_WIDTH; x++) {
            original_tilemap[y][x] = tilemap[y][x];
        }
    }

    Player player;
    PlayerInit(&player);
    Vector2 input_axis = {0, 0};

    RenderTexture2D target = LoadRenderTexture(base_screen_width, base_screen_height); 
    SetTargetFPS(60);
    // -------------------------------------
    // Main Game Loop
    while (!WindowShouldClose()) {
        // -----------------------------------
        // Update
        // -----------------------------------

        float delta_t = GetFrameTime();

        // Draw to render texture
        BeginTextureMode(target);
        ClearBackground(BLACK);

        // Draw tiles in background
        {
            for (u32 y = 0; y < map.height; y++) {
                for (u32 x = 0; x < map.width; x++) {
                    Tile_Type tile = (Tile_Type)map.tiles[y*map.width+x];
                    Vector2 tile_pos = {(float)x * map.tile_size, (float)y * map.tile_size};

                    Color tile_col;
                    switch (tile) {
                        case TileType_none:  tile_col = BLACK;                break;
                        case TileType_wall:  tile_col = PURPLE;               break;
                        case TileType_wall2: tile_col = {140, 20, 140, 255};  break;
                        case TileType_grass: tile_col = {68, 68, 68, 255};    break;
                        case TileType_dirt:  tile_col = {168, 168, 168, 255}; break;
                        case TileType_fire:  tile_col = {168, 0, 0, 255};     break;
                        case TileType_temp_grass: tile_col = {68, 68, 68, 255}; break;
                        case TileType_temp_dirt:  tile_col = {168, 168, 168, 255}; break;
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

                //player.collider = {(f32)target_tile_x*map.tile_size, (f32)target_tile_y*map.tile_size, (f32)map.tile_size, (f32)map.tile_size};

                bool is_in_path = false;
                for (u32 i = 0; i < player.path_len; i++) {
                    if (player.path[i].x == (f32)target_tile_x && player.path[i].y == (f32)target_tile_y) {
                        is_in_path = true;
                        break;
                    }
                }

                // TODO: need to continue the refactor from here
                if (target_tile_x > 0 && target_tile_x < map.width-1 &&
                    target_tile_y > 0 && target_tile_y < map.height-1) {

                    u32 target_tile_index = target_tile_y * map.width + target_tile_x;
                    u32 target_tile = map.tiles[target_tile_index];

                    if (target_tile != TileType_wall  && 
                        target_tile != TileType_wall2 && 
                        target_tile != TileType_fire) {
                        // Start moving
                        player.target_pos = {(float)target_tile_x * map.tile_size, (float)target_tile_y * map.tile_size};
                        player.is_moving  = true;

                        u32 current_tile_index = current_tile_y * map.width + current_tile_x;
                        map.tiles[current_tile_index] = TileType_fire;
                    } 
                    else {
                        GameOver(&player);
                        // NOTE: Reset the tiles to original state
                        for (u32 y = 0; y < map.height; y++) {
                            for (u32 x = 0; x < map.width; x++) {
                                map.tiles[y*map.width+x] = original_tilemap[y][x];
                            }
                        }
                    }
                } else {
                    // Out of bounds
                    GameOver(&player);
                    // NOTE: Reset the tiles to original state
                    for (u32 y = 0; y < map.height; y++) {
                        for (u32 x = 0; x < map.width; x++) {
                            map.tiles[y*map.width+x] = original_tilemap[y][x];
                        }
                    }
                    // TODO: Set a starting tile for the target tile or the player will keep
                    // moving after restart
                }

                if (is_in_path) {
                    GameOver(&player);
                    // NOTE: Reset the tiles to original state
                    for (u32 y = 0; y < map.height; y++) {
                        for (u32 x = 0; x < map.width; x++) {
                            map.tiles[y*map.width+x] = original_tilemap[y][x];
                        }
                    }
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

                player.path[player.path_len] = {(float)current_tile_x, (float)current_tile_y};
                player.path_len++;

                // Check for enclosed areas
                CheckEnclosedAreas(&map, current_tile_x, current_tile_y);
            } else {
                direction = VectorNorm(direction);
                Vector2 movement = VectorScale(direction, player.speed * delta_t);
                player.pos = VectorAdd(player.pos, movement);
            }
        }
        DrawRectangleV(player.pos, player.size, RED);

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
