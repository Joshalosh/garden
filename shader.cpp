
void WobbleShaderInit(Wobble_Shader *shader, f32 amplitude, f32 frequency, f32 speed) {
    shader->shader             = LoadShader(0, "../shaders/wobble.fs");
    shader->time_location      = GetShaderLocation(shader->shader, "time");
    shader->amplitude_location = GetShaderLocation(shader->shader, "amplitude");
    shader->frequency_location = GetShaderLocation(shader->shader, "frequency");
    shader->speed_location     = GetShaderLocation(shader->shader, "speed");

    shader->amplitude = amplitude;
    shader->frequency = frequency;
    shader->speed     = speed;
}
