#include "raylib.h"

#if defined(GRAPHICS_API_OPENGL_ES2)
static const char *WOBBLE_VS = 
"precision mediump float;\n"
"in vec3 vertexPosition;\n"
"in vec2 vertexTexCoord;\n"
"in vec4 vertexColor;\n"
"uniform mat4 mvp;\n"
"out vec2 fragTexCoord;\n"
"out vec4 fragColor;\n"
"void main(){\n"
"  fragTexCoord = vertexTexCoord;\n"
"  fragColor    = vertexColor;\n"
"  gl_Position  = mvp * vec4(vertexPosition, 1.0);\n"
"}\n";

static const char *WOBBLE_FS =
"precision mediump float;\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform float time;\n"
"uniform float amplitude;\n"
"uniform float frequency;\n"
"uniform float speed;\n"
"out vec4 finalColor;\n"
"void main(){\n"
"  float phase  = fragTexCoord.y * frequency + time * speed;\n"
"  vec2  uv     = fragTexCoord + vec2(sin(phase) * amplitude, 0.0);\n"
"  finalColor   = texture(texture0, uv) * fragColor;\n"
"}\n";
#else
static const char *WOBBLE_VS =
"in vec3 vertexPosition;\n"
"in vec2 vertexTexCoord;\n"
"in vec4 vertexColor;\n"
"uniform mat4 mvp;\n"
"out vec2 fragTexCoord;\n"
"out vec4 fragColor;\n"
"void main(){\n"
"  fragTexCoord = vertexTexCoord;\n"
"  fragColor    = vertexColor;\n"
"  gl_Position  = mvp * vec4(vertexPosition, 1.0);\n"
"}\n";

static const char *WOBBLE_FS =
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform float time;\n"
"uniform float amplitude;\n"
"uniform float frequency;\n"
"uniform float speed;\n"
"out vec4 finalColor;\n"
"void main(){\n"
"  float phase  = fragTexCoord.y * frequency + time * speed;\n"
"  vec2  uv     = fragTexCoord + vec2(sin(phase) * amplitude, 0.0);\n"
"  finalColor   = texture(texture0, uv) * fragColor;\n"
"}\n";
#endif

void WobbleShaderInit(Wobble_Shader *shader, f32 amplitude, f32 frequency, f32 speed) {
    //shader->shader             = LoadShader(0, "../shaders/wobble.fs");
    shader->shader             = LoadShaderFromMemory(WOBBLE_VS, WOBBLE_FS);
    shader->time_location      = GetShaderLocation(shader->shader, "time");
    shader->amplitude_location = GetShaderLocation(shader->shader, "amplitude");
    shader->frequency_location = GetShaderLocation(shader->shader, "frequency");
    shader->speed_location     = GetShaderLocation(shader->shader, "speed");

    shader->amplitude = amplitude;
    shader->frequency = frequency;
    shader->speed     = speed;
}
