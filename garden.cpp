
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
    TileType_fire  = 5,
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

struct Player {
    Vector2   pos;
    Vector2   target_pos;
    Vector2   size;
    Rectangle collider;

    float     speed;
    bool      is_moving;
};

void PlayerInit(Player *player) {
    player->pos = {base_screen_width*0.5, base_screen_height*0.5};
    player->target_pos = player->pos;
    player->size       = {TILE_SIZE, TILE_SIZE};
    player->collider   = {player->pos.x, player->pos.y, player->size.x, player->size.y};
    player->speed      = 50.0f;
    player->is_moving  = false;
}

void GameOver(Player *player) {
    Vector2 start_pos = {base_screen_width*0.5, base_screen_height*0.5};
    player->pos = start_pos;
}


int main() {
    // -------------------------------------
    // Initialisation
    // -------------------------------------

    const int window_width  = 1280;
    const int window_height = 1280; //720;
    InitWindow(window_width, window_height, "Raylib basic window");

    float half_tile_size = TILE_SIZE * 0.5;
    Vector2 rect_size  = {TILE_SIZE, TILE_SIZE};

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
            for (s32 y = 0; y < TILEMAP_HEIGHT; y++) {
                for (s32 x = 0; x < TILEMAP_WIDTH; x++) {
                    Tile_Type tile = (Tile_Type)tilemap[y][x];
                    Vector2 tile_pos = {(float)x * TILE_SIZE, (float)y * TILE_SIZE};

                    Color tile_col;
                    switch (tile) {
                        case TileType_none:  tile_col = BLACK;                break;
                        case TileType_wall:  tile_col = PURPLE;               break;
                        case TileType_wall2: tile_col = {140, 20, 140, 255};  break;
                        case TileType_grass: tile_col = {68, 68, 68, 255};    break;
                        case TileType_dirt:  tile_col = {168, 168, 168, 255}; break;
                        case TileType_fire:  tile_col = {168, 0, 0, 255};     break;
                    }

                    Vector2 tile_size = {TILE_SIZE, TILE_SIZE};
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
                s32 current_tile_x = (s32)player.pos.x / TILE_SIZE;
                s32 current_tile_y = (s32)player.pos.y / TILE_SIZE;

                s32 target_tile_x = current_tile_x + s32(input_axis.x);
                s32 target_tile_y = current_tile_y + s32(input_axis.y);

                player.collider = {(f32)target_tile_x*TILE_SIZE, (f32)target_tile_y*TILE_SIZE, TILE_SIZE, TILE_SIZE};

                if (target_tile_x > 0 && target_tile_x < TILEMAP_WIDTH-1 &&
                    target_tile_y > 0 && target_tile_y < TILEMAP_HEIGHT-1) {
                    player.target_pos = {(f32)target_tile_x * TILE_SIZE, (f32)target_tile_y * TILE_SIZE};
                    player.is_moving = true;
                    tilemap[current_tile_y][current_tile_x] = (int)TileType_fire;
                } else {
                    GameOver(&player);
                    // TODO: Set a starting tile for the target tile or the player will keep
                    // moving after restart
                }
            }
        } else {
            // MOTE: Move towards target position
            Vector2 direction = VectorSub(player.target_pos, player.pos);
            float distance    = Length(direction);
            if (distance <= player.speed * delta_t) {
                player.pos = player.target_pos;
                player.is_moving  = false;
            } else {
                direction = VectorNorm(direction);
                Vector2 movement = VectorScale(direction, player.speed * delta_t);
                player.pos = VectorAdd(player.pos, movement);
            }
        }
        DrawRectangleV(player.pos, player.size, RED);

        // NOTE: DEBUG
        float thickness = 2.0;
        DrawRectangleLinesEx(player.collider, thickness, GREEN);

        //Vector2 draw_pos = {player_pos.x - half_tile_size, player_pos.y - half_tile_size};
        //DrawRectangleRec(player_collider, GREEN);


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
