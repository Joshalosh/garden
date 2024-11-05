
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
    player->pos = {base_screen_width*0.5, base_screen_height*0.5};
    player->target_pos = player->pos;
    player->size       = {20, 20};
    player->speed      = 50.0f;
    player->is_moving  = false;

    // Set the starting square to be the first element of the path
    player->path[0]    = {player->pos.x/TILEMAP_SIZE, player->pos.y/TILEMAP_SIZE};
    player->path_len   = 1;
}

void GameOver(Player *player, Tilemap *tilemap) {
    PlayerInit(player);

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
        if (x < 0 || x >= (u32)tilemap->width || y < 0 || y >= tilemap->height) continue;

        u32 index = TilemapIndex(x, y, tilemap->width);
        Tile *tile = &tilemap->tiles[index];

        if (IsFlagSet(tile, TileFlag_visited))                           continue;
        if (IsFlagSet(tile, TileFlag_fire))                              continue;
        if (tile->type != TileType_grass && tile->type != TileType_dirt) continue;

        AddFlag(tile, TileFlag_visited);

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
            u32 index = TilemapIndex(x, y, tilemap->width);
            Tile *tile  = &tilemap->tiles[index];

            if (tile->type == TileType_grass || tile->type == TileType_dirt) {
                if (!IsFlagSet(tile, TileFlag_visited)) {
                    AddFlag(tile, TileFlag_fire);
                } 
            } 

            ClearFlag(tile, TileFlag_visited);
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
    //map.tiles = (u32 *)tilemap;
    u32 original_map[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    Tile tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH];

    for (u32 y = 0; y < TILEMAP_HEIGHT; y++) {
        for (u32 x = 0; x < TILEMAP_WIDTH; x++) {
            original_map[y][x] = tilemap[y][x];
            tiles[y][x].type   = (Tile_Type)tilemap[y][x];
            tiles[y][x].flags  = 0;
        }
    }

    map.original_map = (u32 *)&original_map;
    map.tiles        = (Tile *)&tiles;

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
                    u32 index = TilemapIndex(x, y, map.width);
                    Tile tile = map.tiles[index];
                    Vector2 tile_pos = {(float)x * map.tile_size, (float)y * map.tile_size};

                    Color tile_col;
                    switch (tile.type) {
                        case TileType_none:  tile_col = BLACK;                break;
                        case TileType_wall:  tile_col = PURPLE;               break;
                        case TileType_wall2: tile_col = {140, 20, 140, 255};  break;
                        case TileType_grass: tile_col = {68, 68, 68, 255};    break;
                        case TileType_dirt:  tile_col = {168, 168, 168, 255}; break;
                        case TileType_fire:  tile_col = {168, 0, 0, 255};     break;
                        case TileType_temp_grass: tile_col = {68, 68, 68, 255}; break;
                        case TileType_temp_dirt:  tile_col = {168, 168, 168, 255}; break;
                    }

                    if (IsFlagSet(&tile, TileFlag_fire)) {
                        tile_col = {168, 0, 0, 255};
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

                    u32 target_tile_index = TilemapIndex(target_tile_x, target_tile_y, map.width);
                    Tile *target_tile = &map.tiles[target_tile_index];

                    if (target_tile->type != TileType_wall  && 
                        target_tile->type != TileType_wall2 && 
                        target_tile->type != TileType_fire) {
                        // Start moving
                        player.target_pos = {(float)target_tile_x * map.tile_size, (float)target_tile_y * map.tile_size};
                        player.is_moving  = true;

                        u32 current_tile_index = TilemapIndex(current_tile_x, current_tile_y, map.width);
                        Tile *current_tile = &map.tiles[current_tile_index];
                        AddFlag(current_tile, TileFlag_fire);
                    } 
                    else {
                        GameOver(&player, &map);
                    }
                } else {
                    // Out of bounds
                    GameOver(&player, &map);
                }

                if (is_in_path) {
                    GameOver(&player, &map);
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
