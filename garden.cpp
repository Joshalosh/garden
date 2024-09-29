#include "raylib.h"

int main() {
    // -------------------------------------
    // Initialisation
    // -------------------------------------
    const int base_screen_width  = 320;
    const int base_screen_height = 180;

    const int window_width  = 1280;
    const int window_height = 720;
    InitWindow(window_width, window_height, "Raylib basic window");

    RenderTexture2D target = LoadRenderTexture(base_screen_width, base_screen_height); SetTargetFPS(60);
    // -------------------------------------
    // Main Game Loop
    while (!WindowShouldClose()) {
        // -----------------------------------
        // Update
        // -----------------------------------
        // TODO: put things here
        // -----------------------------------
        // Draw
        // -----------------------------------

        // Draw to render texture
        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawText("It works!", 20, 20, 20, WHITE);
        Vector2 rect_pos = {1, 0.0f};
        Vector2 rect_size = {10, 170};
        DrawRectangleV(rect_pos, rect_size, RED);
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

        // TODO: Delete this todo

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
