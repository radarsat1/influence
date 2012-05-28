
#ifndef _VFGL_H_
#define _VFGL_H_

void vfgl_Init(int argc, char** argv);
void vfgl_CmdLine(int argc, char **argv);
void vfgl_Run();

#define maxAgents 20
struct _agent
{
    float   obs[4];
    int     pos[2];
    float   gain;
    float   spin;
    float   fade;
    float   dir[2];
    float   flow;
} agent;

extern struct _agent agents[];
extern float borderGain;
extern void mapperLogout();
extern void (*vfgl_DrawCallback)();

// Options
extern int update_rate;
extern int number_of_passes;
extern int x_offset;
extern int y_offset;
extern int field_width;
extern int field_height;
extern int fullscreen;

#endif // _VFGL_H_
