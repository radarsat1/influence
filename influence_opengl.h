
#ifndef _VFGL_H_
#define _VFGL_H_

void vfgl_Init(int argc, char** argv);
void vfgl_Run();

#define maxAgents 20
struct _agent
{
    float   obs[4];
    int     pos[2];
    float   gain;
    float   spin;
    float   fade;
    float   dir;
    float   flow;
} agent;

extern struct _agent agents[];

extern void (*vfgl_DrawCallback)();

#define WIDTH 640.0
#define HEIGHT 480.0

#endif // _VFGL_H_
