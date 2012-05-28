
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

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

void on_signal_border_gain(mapper_signal msig,
                           mapper_db_signal props,
                           mapper_timetag_t *timetag,
                           void *value)
{
    float *gain = (float*)value;
    borderGain = *gain;
}

void on_signal(mapper_signal msig,
               mapper_db_signal props,
               mapper_timetag_t *timetag,
               void *value)
{
    int *pos = (int*)value;
    mapper_db_signal p = msig_properties(msig);
    int index = (int)(long)p->user_data;
    agents[index].pos[0] = pos[0];
    agents[index].pos[1] = pos[1];
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
    int mn = 0, mx = field_width;
    float fmn = 0, fmx = 1.0;

    dev = mdev_new("influence", 9000, 0);

    for (i = 0; i < maxAgents; i++)
    {
        fmn = -1.0, fmx = 1.0;
        char str[256];
        sprintf(str, "/node/%ld/observation", i+1);
        sigobs[i] = mdev_add_output(dev, str, 2, 'f', 0,
                                    &fmn, &fmx);
        fmn = 0.0;
        mdev_add_input(dev, "/border_gain", 1, 'f', 0,
                       &fmn, &fmx, on_signal_border_gain, 0);
        sprintf(str, "/node/%ld/position", i+1);
        mdev_add_input(dev, str, 2, 'i', 0,
                       &mn, &mx, on_signal, (void*)(i));
        sprintf(str, "/node/%ld/gain", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_gain, (void*)(i));
        fmx = 0.9;
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
        sprintf(str, "/node/%ld/direction", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_dir, (void*)(i));
        fmn = -1.0;
        fmx = 1.0;
        sprintf(str, "/node/%ld/flow", i+1);
        mdev_add_input(dev, str, 1, 'f', 0,
                       &fmn, &fmx, on_signal_flow, (void*)(i));
    }
}

void CmdLine(int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, argv, "hfr:p:x:s:")) != -1)
    {
        switch (c)
        {
        case 'h': // Help
            printf("Usage: influence [-h] [-r <rate>] [-p <passes>] "
                   "[-x <offset>] [-s <size>] [-f]\n");
            printf("  -h  Help\n");
            printf("  -r  Update rate, default=100\n");
            printf("  -p  Number of passes per frame, default=1\n");
            printf("  -x  \"X,Y\" offsets, glReadPixel work-around\n");
            printf("  -s  Field size in pixels, default = 500\n");
            printf("  -f  Begin in full-screen mode\n");
            exit(0);
        case 'r': // Rate
            update_rate = atoi(optarg);
            break;
        case 'p': // Passes
            number_of_passes = atoi(optarg);
            break;
        case 'x': // X/Y Offset
            x_offset = atoi(optarg);
            if (strchr(optarg,',')!=0)
                y_offset = atoi(strchr(optarg,',')+1);
            break;
        case 's': // Field Size
            field_width = atoi(optarg);
            field_height = atoi(optarg);
            break;
        case 'f': // Full screen
            fullscreen = 1;
            break;
        case '?': // Unknown
            printf("influence: Bad options, use -h for help.\n");
            exit(1);
            break;
        default:
            abort();
        }
    }
}

void mapperLogout()
{
    printf("Cleaning up...\n");
    mdev_free(dev);
}

int main(int argc, char** argv)
{
    CmdLine(argc, argv);

    initMapper();

    vfgl_Init(argc, argv);
    vfgl_DrawCallback = on_draw;
    vfgl_Run();

    return 0;
}
