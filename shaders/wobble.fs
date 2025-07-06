
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float amplitude;
uniform float frequency;
uniform float speed;

void main()
{
    vec2 uv = fragTexCoord;
    uv.x += sin(uv.y * frequency + time * speed) * amplitude;
    finalColor = texture(texture0, uv) * fragColor;
}
