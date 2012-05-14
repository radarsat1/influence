
uniform sampler2D field;
uniform float kernels[25];
uniform float gain;

void main()
{
    vec4 t;
    float a = kernels[0] * 0.0;
    t = texture2D(field,
                   vec2((gl_FragCoord.x)/640.0,
                        (gl_FragCoord.y)/480.0));
    t += texture2D(field,
                  vec2((gl_FragCoord.x-1.0)/640.0,
                       (gl_FragCoord.y)/480.0));
    t += texture2D(field,
                   vec2((gl_FragCoord.x+1.0)/640.0,
                        (gl_FragCoord.y)/480.0));
    t += texture2D(field,
                   vec2((gl_FragCoord.x)/640.0,
                        (gl_FragCoord.y-1.0)/480.0));
    t += texture2D(field,
                   vec2((gl_FragCoord.x)/640.0,
                        (gl_FragCoord.y+1.0)/480.0));
 
    t *= vec4(gain * 0.2 + a);

    gl_FragColor = t;
}
