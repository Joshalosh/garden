
struct Wobble_Shader {
    Shader shader;
    u32 time_location;
    u32 amplitude_location;
    u32 frequency_location;
    u32 speed_location;

    f32 amplitude;
    f32 frequency;
    f32 speed;
};
