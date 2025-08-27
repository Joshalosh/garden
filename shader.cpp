#include "raylib.h"

#if defined(GRAPHICS_API_OPENGL_ES2)
/*
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
*/

static const char *WOBBLE_VS =
"precision mediump float;\n"
"attribute vec3 vertexPosition;\n"
"attribute vec2 vertexTexCoord;\n"
"attribute vec4 vertexColor;\n"
"uniform mat4 mvp;\n"
"varying vec2 fragTexCoord;\n"
"varying vec4 fragColor;\n"
"void main(){\n"
"  fragTexCoord = vertexTexCoord;\n"
"  fragColor    = vertexColor;\n"
"  gl_Position  = mvp * vec4(vertexPosition, 1.0);\n"
"}\n";

static const char *WOBBLE_FS =
"precision mediump float;\n"
"varying vec2 fragTexCoord;\n"
"varying vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform float time;\n"
"uniform float amplitude;\n"
"uniform float frequency;\n"
"uniform float speed;\n"
"void main(){\n"
"  float phase = fragTexCoord.y * frequency + time * speed;\n"
"  vec2 uv     = fragTexCoord + vec2(sin(phase) * amplitude, 0.0);\n"
"  gl_FragColor = texture2D(texture0, uv) * fragColor;\n"
"}\n";

#else   // Desktop -> GLSL 330

static const char *WOBBLE_VS =
"#version 330\n"
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
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform float time;\n"
"uniform float amplitude;\n"
"uniform float frequency;\n"
"uniform float speed;\n"
"out vec4 finalColor;\n"
"void main(){\n"
"  float phase = fragTexCoord.y * frequency + time * speed;\n"
"  vec2 uv     = fragTexCoord + vec2(sin(phase) * amplitude, 0.0);\n"
"  finalColor  = texture(texture0, uv) * fragColor;\n"
"}\n";

#endif

void WobbleShaderInit(Wobble_Shader *shader, float amplitude, float frequency, float speed)
{
    shader->shader = LoadShaderFromMemory(WOBBLE_VS, WOBBLE_FS);

    shader->shader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(shader->shader, "texture0");

    int texSlot = 0;

    SetShaderValue(shader->shader, shader->shader.locs[SHADER_LOC_MAP_DIFFUSE], &texSlot, SHADER_UNIFORM_INT);

    shader->time_location      = GetShaderLocation(shader->shader, "time");
    shader->amplitude_location = GetShaderLocation(shader->shader, "amplitude");
    shader->frequency_location = GetShaderLocation(shader->shader, "frequency");
    shader->speed_location     = GetShaderLocation(shader->shader, "speed");

    shader->amplitude = amplitude;
    shader->frequency = frequency;
    shader->speed     = speed;
}
