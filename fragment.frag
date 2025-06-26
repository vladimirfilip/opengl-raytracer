#version 330 core

uniform float u_ScreenWidth;
uniform float u_ScreenHeight;
uniform float u_FOV;

#define WHITE vec4(1.0f, 1.0f, 1.0f, 1.0f)
#define BLACK vec4(0.0f, 0.0f, 0.0f, 1.0f)

out vec4 FragColor;
void main() {
    float dAlpha = u_FOV / u_ScreenWidth;
    float alpha = -u_FOV / 2.0f + dAlpha * gl_FragCoord.xy.x;
    float beta = (gl_FragCoord.xy.y - u_ScreenHeight / 2) * dAlpha;
    vec3 dir = {sin(alpha), sin(beta), 1.0f};
    FragColor = vec4(abs(sin(alpha)), abs(sin(beta)), 1.0f, 1.0f);
}