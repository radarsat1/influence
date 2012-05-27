
uniform sampler2D field;
uniform float kernels[25];
uniform float gain;

void main()
{
    int i,j;
    float pos[2];
    vec3 t;
    vec4 a=vec4(0,0,0,0), b;
    for (i=0; i<5; i++) {
        for (j=0; j<5; j++) {
            pos[0] = float(i)-2.0;
            pos[1] = float(j)-2.0;
            t = texture2D(field,
                          vec2((gl_FragCoord.x+pos[0])/500.0,
                               (gl_FragCoord.y+pos[1])/500.0)).rgb;
            t *= vec3(kernels[i+j*5]);
            t.r += t.b * pos[0] * -0.5;
            t.g += t.b * pos[1] * -0.5;
            a.rgb += t;
        }
    }
    a.rgb *= vec3(gain);
    b = texture2D(field,vec2(gl_FragCoord/500.0));
    b *= vec4(b.a);
    a += b;

    gl_FragColor = a;
}
