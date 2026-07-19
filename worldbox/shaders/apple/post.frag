#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D screenTex;
void main() {
    vec3 c = texture(screenTex, TexCoord).rgb;
    float v = 1.0 - length(TexCoord - 0.5) * 0.4;
    FragColor = vec4(c * v, 1.0);
}
