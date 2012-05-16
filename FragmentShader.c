
uniform sampler2D field;
uniform float kernels[25];
uniform float gain;

void main()
{
    int i,j;
    vec4 t,a=vec4(0,0,0,0);
    for (i=0; i<5; i++) {
        for (j=0; j<5; j++) {
            t = texture2D(field,
                          vec2((gl_FragCoord.x+float(i)-2.0)/500.0,
                               (gl_FragCoord.y+float(j)-2.0)/500.0));
            t *= kernels[i+j*5];
            a += t;
        }
    }
    a *= vec4(gain);

    gl_FragColor = a;
}
