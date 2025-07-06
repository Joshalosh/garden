
#version 330

in vec2 fragTexCoord;
in vec4 fragCol;

out vec3 finalCol;

uniform sampler2D texture0;
uniform float time;
uniform float amplitude;
uniform float amplitude;
uniform float speed;

void main()
{
    vec2 uv = fragTexCoord;

    uv.x += sin(uv.y * frequency + time * speed) * amplitude;

    finalCol = texture(texture0, uv) * fragCol;
}
