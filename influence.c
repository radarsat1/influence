
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <mapper/mapper.h>

#include "influence_opengl.h"

mapper_device dev = 0;
mapper_signal sigobs[maxAgents];

void on_draw()
{
    mdev_poll(dev, 0);

    int i;
    for (i=0; i < maxAgents; i++)
    {
        if (agents[i].pos[0] > -1 && agents[i].pos[1] > -1)
            msig_update(sigobs[i], agents[i].obs);
    }
}

void on_signal_x(mapper_signal msig,
               mapper_db_signal props,
               mapper_timetag_t *timetag,
               void *value)
{
    int *pos = (int*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].pos[0] = pos[0];
}

void on_signal_y(mapper_signal msig,
                 mapper_db_signal props,
                 mapper_timetag_t *timetag,
                 void *value)
{
    int *pos = (int*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].pos[1] = pos[0];
}

void on_signal_gain(mapper_signal msig,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    float *gain = (float*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].gain = *gain;
}

void on_signal_spin(mapper_signal msig,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    float *spin = (float*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].spin = *spin;
}

void on_signal_fade(mapper_signal msig,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    float *fade = (float*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].fade = *fade;
}

void on_signal_dir(mapper_signal msig,
                   mapper_db_signal props,
                   mapper_timetag_t *timetag,
                   void *value)
{
    float *dir = (float*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].dir[0] = cos(*dir);
    agents[index].dir[1] = sin(*dir);
}

void on_signal_flow(mapper_signal msig,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    float *flow = (float*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].flow = *flow;
}

void initMapper()
{
    long i;
    int mn = 0, mxx = WIDTH, mxy = HEIGHT;
    float fmn = 0, fmx = 1.0;

    dev = mdev_new("vector", 9000, 0);

    for (i = 0; i < maxAgents; i++)
    {
        char str[256];
        sprintf(str, "/node/%ld/observation", i+1);
        sigobs[i] = mdev_add_output(dev, str, 4, 'f', 0,
                                    &fmn, &fmx);
        sprintf(str, "/node/%ld/position/x", i+1);
        mdev_add_input(dev, str, 1, 'i', 0,
                       &mn, &mxx, on_signal_x, (void*)(i));
        sprintf(str, "/node/%ld/position/y", i+1);
        mdev_add_input(dev, str, 1, 'i', 0,
                       &mn, &mxy, on_signal_y, (void*)(i));
        sprintf(str, "/node/%ld/gain", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_gain, (void*)(i));
        sprintf(str, "/node/%ld/fade", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_fade, (void*)(i));
        fmn = -1.5;
        fmx = 1.5;
        sprintf(str, "/node/%ld/spin", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_spin, (void*)(i));
        fmn = -3.1415926;
        fmx = 3.1415926;
        sprintf(str, "/node/%ld/dir", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_dir, (void*)(i));
        fmn = -1.0;
        fmx = 1.0;
        sprintf(str, "/node/%ld/flow", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_flow, (void*)(i));
    }
}

int main(int argc, char** argv)
{
    initMapper();

    vfgl_Init(argc, argv);
    vfgl_DrawCallback = on_draw;
    vfgl_Run();

    return 0;
}
