
uniform sampler2D field;
uniform float kernels[25];
uniform float gain;

void main()
{
    int i,j;
    vec2 t;
    vec4 a=vec4(0,0,0,0), b;
    for (i=0; i<5; i++) {
        for (j=0; j<5; j++) {
            t = texture2D(field,
                          vec2((gl_FragCoord.x+float(i)-2.0)/500.0,
                               (gl_FragCoord.y+float(j)-2.0)/500.0)).rg;
            t *= kernels[i+j*5];
            a.rg += t;
        }
    }
    a.rg *= vec2(gain);
    b = texture2D(field,vec2(gl_FragCoord/500.0));
    b.rgb *= vec3(b.b);
    a += b;

    gl_FragColor = a;
}
