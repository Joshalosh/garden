
struct WobbleShader {
    Shader shader;
    u32 timeLoc;
    u32 amplitudeLoc;
    u32 frequencyLoc;
    u32 speedLoc;

    f32 amplitude;
    f32 frequency;
    f32 speed;
};
