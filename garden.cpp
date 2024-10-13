
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "mymath.h"
#include "memory.h"

#define TILEMAP_WIDTH  16
#define TILEMAP_HEIGHT 16
#define TILE_SIZE  20
#define MB(x) x*1024ULL*1024ULL
#define ARENA_SIZE MB(500)
const int base_screen_width  = 320;
const int base_screen_height = 320; //180;

enum Tile_Type {
    TileType_none  = 0,
    TileType_wall  = 1,
    TileType_grass = 2,
    TileType_dirt  = 3,
    TileType_wall2 = 4,
};

struct Game_State {
    Vector2 player_pos;
};

int tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
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

void GameOver(Vector2 *player_pos) {
    Vector2 start_pos = {base_screen_width*0.5, base_screen_height*0.5};
    *player_pos = start_pos;
}


int main() {
    // -------------------------------------
    // Initialisation
    // -------------------------------------

    const int window_width  = 1280;
    const int window_height = 1280; //720;
    InitWindow(window_width, window_height, "Raylib basic window");

    float player_speed   = 50.0f;
    Vector2 player_pos   = {base_screen_width*0.5, base_screen_height*0.5};
    Vector2 target_pos   = player_pos;
    bool is_moving       = false;
    float half_tile_size = TILE_SIZE * 0.5;

    Vector2 rect_size  = {TILE_SIZE, TILE_SIZE};

    Rectangle player_collider;

    RenderTexture2D target = LoadRenderTexture(base_screen_width, base_screen_height); 
    SetTargetFPS(60);
    // -------------------------------------
    // Main Game Loop
    while (!WindowShouldClose()) {
        // -----------------------------------
        // Update
        // -----------------------------------

        float delta_t = GetFrameTime();

        // -Player Movement 
#if 0
        {
            Vector2 input_axis = {0, 0};
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown('D')) input_axis.x += 1.0f;
            if (IsKeyDown(KEY_LEFT)  || IsKeyDown('A')) input_axis.x -= 1.0f;
            if (IsKeyDown(KEY_UP)    || IsKeyDown('W')) input_axis.y -= 1.0f;
            if (IsKeyDown(KEY_DOWN)  || IsKeyDown('S')) input_axis.y += 1.0f;;

            VectorNorm(input_axis);

            // TODO: figure out how to get the player moving from the middle point rather than top left
            //player_pos = {player_pos.x + (float)(TILE_SIZE*0.5), player_pos.y + (float)(TILE_SIZE*0.5)};
            Vector2 potential_pos = VectorAdd(player_pos, VectorScale(input_axis, player_speed * delta_t));
            player_collider.x = potential_pos.x;
            f32 quarter_tile_size = half_tile_size*0.5;
            player_collider.y = potential_pos.y + (TILE_SIZE - quarter_tile_size);
            player_collider.width = TILE_SIZE;
            player_collider.height = quarter_tile_size;

            int tile_min_x = (int)player_collider.x / TILE_SIZE;
            int tile_min_y = (int)player_collider.y / TILE_SIZE;
            int tile_max_x = (int)(player_collider.x + player_collider.width)  / TILE_SIZE;
            int tile_max_y = (int)(player_collider.y + player_collider.height) / TILE_SIZE; 
            int tile_x     = (int)potential_pos.x / TILE_SIZE;
            int tile_y     = (int)potential_pos.y / TILE_SIZE;

            if ((tile_max_x > 0 && tile_max_x < TILEMAP_WIDTH) && (player_collider.y >= 0 && tile_max_y < TILEMAP_HEIGHT)) {
                Tile_Type tile_type1 = (Tile_Type)tilemap[tile_y][tile_x];
                Tile_Type tile_type2 = tile_type1;
                if (input_axis.x == 1.0f) {
                    tile_type1 = (Tile_Type)tilemap[tile_max_y][tile_max_x];
                    tile_type2 = (Tile_Type)tilemap[tile_min_y][tile_max_x];
                }
                if (input_axis.x == -1.0f) {
                    tile_type1 = (Tile_Type)tilemap[tile_max_y][tile_min_x];
                    tile_type2 = (Tile_Type)tilemap[tile_min_y][tile_min_x];
                }
                if (input_axis.y == -1.0f) {
                    tile_type1 = (Tile_Type)tilemap[tile_min_y][tile_max_x];
                    tile_type2 = (Tile_Type)tilemap[tile_min_y][tile_min_x];
                }
                if (input_axis.y == 1.0f) {
                    tile_type1 = (Tile_Type)tilemap[tile_max_y][tile_max_x];
                    tile_type2 = (Tile_Type)tilemap[tile_max_y][tile_min_x];
                }
                if (tile_type1 != TileType_wall && tile_type1 != TileType_wall2 && 
                    tile_type2 != TileType_wall && tile_type1 != TileType_wall2) {
                    player_pos.x = potential_pos.x;
                    player_pos.y = potential_pos.y;
                } else {
                    GameOver(&player_pos);
                }
            }
        }
#endif
        // - New player movement
        if (!is_moving) {

            Vector2 input_axis = {0, 0};
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input_axis.x += 1.0f;
            if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) input_axis.x -= 1.0f;
            if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) input_axis.y -= 1.0f;
            if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) input_axis.y += 1.0f;

            if (input_axis.x != 0 || input_axis.y != 0) {
                // NOTE: Calculate the next tile position
                s32 current_tile_x = (s32)player_pos.x / TILE_SIZE;
                s32 current_tile_y = (s32)player_pos.y / TILE_SIZE;

                s32 target_tile_x = current_tile_x + s32(input_axis.x);
                s32 target_tile_y = current_tile_y + s32(input_axis.y);


                if (target_tile_x > 0 && target_tile_x < TILEMAP_WIDTH &&
                    target_tile_y > 0 && target_tile_y < TILEMAP_HEIGHT) {
                    target_pos = {(f32)target_tile_x * TILE_SIZE, (f32)target_tile_y * TILE_SIZE};
                    is_moving = true;
                } else {
                    GameOver(&player_pos);
                }
            }
        } else {
            // MOTE: Move towards target positiong
            Vector2 direction = VectorSub(target_pos, player_pos);
            float distance    = Length(direction);
            if (distance <= player_speed * delta_t) {
                player_pos = target_pos;
                is_moving  = false;
            } else {
                direction = VectorNorm(direction);
                Vector2 movement = VectorScale(direction, player_speed * delta_t);
                player_pos = VectorAdd(player_pos, movement);
            }
        }

        // -----------------------------------
        // Draw
        // -----------------------------------

        // Draw to render texture
        BeginTextureMode(target);
        ClearBackground(BLACK);
        //DrawText("It works!", 20, 20, 20, WHITE);

        // Draw tiles
        for (s32 y = 0; y < TILEMAP_HEIGHT; y++) {
            for (s32 x = 0; x < TILEMAP_WIDTH; x++) {
                Tile_Type tile = (Tile_Type)tilemap[y][x];
                Vector2 tile_pos = {(float)x * TILE_SIZE, (float)y * TILE_SIZE};

                Color tile_col;
                switch (tile) {
                    case TileType_none:  tile_col = BLACK;    break;
                    case TileType_wall:  tile_col = PURPLE; break;
                    case TileType_wall2: tile_col = {140, 20, 140, 255}; break;
                    case TileType_grass: tile_col = {68, 68, 68, 255};    break;
                    case TileType_dirt:  tile_col = {168, 168, 168, 255};    break;
                }

                Vector2 tile_size = {TILE_SIZE, TILE_SIZE};
                DrawRectangleV(tile_pos, tile_size, tile_col);
            }
        }


        //Vector2 draw_pos = {player_pos.x - half_tile_size, player_pos.y - half_tile_size};
        DrawRectangleV(player_pos, rect_size, RED);

        //DrawRectangleLinesEx(player_collider, 0.2f, GREEN);
        EndTextureMode();

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
