#version 120
uniform sampler2D model_tex;
uniform sampler2D black;
uniform float time;
uniform int u_isRotating;
varying vec2 texCoord;

void main(void)
{
    float transitionTime = 30.0;
    float faktor = 0.0;
    
    if (u_isRotating == 1) {
        faktor = smoothstep(0.0, 1.0, clamp(min(time, transitionTime) / transitionTime, 0.0, 1.0));
    }
    
    gl_FragColor = vec4(
        mix(
            texture2D(model_tex, texCoord).rgb,
            texture2D(black, texCoord).rgb,
            faktor
        ),
        1.0
    );
}