
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <mapper/mapper.h>

#include "influence_opengl.h"

mapper_device dev = 0;
mapper_signal sigpos[2];
mapper_signal sigobs[2];

void on_draw()
{
    while (mdev_poll(dev, 0)) {}

    int i;
    for (i=0; i < maxAgents; i++)
    {
        if (agents[i].active) {
            msig_update_instance(sigobs[0], i, &agents[i].obs[0], 1);
            msig_update_instance(sigobs[1], i, &agents[i].obs[1], 1);
        }
    }
}

void on_signal_border_gain(mapper_signal msig,
                           int instance_id,
                           mapper_db_signal props,
                           mapper_timetag_t *timetag,
                           void *value)
{
    if (!value)
        return;

    float *gain = (float*)value;
    borderGain = *gain;
}

void on_signal_pos(mapper_signal msig,
                   int instance_id,
                   mapper_db_signal props,
                   mapper_timetag_t *timetag,
                   void *value)
{
    if (value) {
        int offset = (long)props->user_data;
        if (!agents[instance_id].active) {
            // need to init new instance
            msig_match_instances(msig, sigpos[1-offset], instance_id);
            msig_match_instances(msig, sigobs[0], instance_id);
            msig_match_instances(msig, sigobs[1], instance_id);
            agents[instance_id].active = 1;
        }
        int *pos = (int*)value;
        agents[instance_id].pos[offset] = *pos;
    }
    else {
        agents[instance_id].active = 0;
    }
}

void on_signal_gain(mapper_signal msig,
                    int instance_id,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    if (!value)
        return;
    float *gain = (float*)value;
    agents[instance_id].gain = *gain;
}

void on_signal_spin(mapper_signal msig,
                    int instance_id,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    if (!value)
        return;
    float *spin = (float*)value;
    agents[instance_id].spin = *spin;
}

void on_signal_fade(mapper_signal msig,
                    int instance_id,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    if (!value)
        return;
    float *fade = (float*)value;
    agents[instance_id].fade = *fade;
}

void on_signal_dir(mapper_signal msig,
                   int instance_id,
                   mapper_db_signal props,
                   mapper_timetag_t *timetag,
                   void *value)
{
    if (!value)
        return;
    float *dir = (float*)value;
    agents[instance_id].dir[0] = cos(*dir);
    agents[instance_id].dir[1] = sin(*dir);
}

void on_signal_flow(mapper_signal msig,
                    int instance_id,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    if (!value)
        return;
    float *flow = (float*)value;
    agents[instance_id].flow = *flow;
}

void initMapper()
{
    printf("initMapper()\n");
    int mn = 0, mx = field_width;
    float fmn = 0, fmx = 1.0;

    dev = mdev_new("influence", 0, 0);
    mapper_signal input;

    fmn = 0.0, fmx = 1.0;
    mdev_add_input(dev, "/border_gain", 1, 'f', 0, &fmn,
                   &fmx, on_signal_border_gain, 0);

    fmn = -1.0, fmx = 1.0;
    sigobs[0] = mdev_add_output(dev, "/node/observation/x",
                                1, 'f', 0, &fmn, &fmx);
    msig_reserve_instances(sigobs[0], maxAgents-1);
    sigobs[1] = mdev_add_output(dev, "/node/observation/y",
                                1, 'f', 0, &fmn, &fmx);
    msig_reserve_instances(sigobs[1], maxAgents-1);

    sigpos[0] = mdev_add_input(dev, "/node/position/x", 1, 'i', 0, &mn,
                               &mx, on_signal_pos, (void*)(0));
    msig_reserve_instances(sigpos[0], maxAgents-1);
    sigpos[1] = mdev_add_input(dev, "/node/position/y", 1, 'i', 0, &mn,
                               &mx, on_signal_pos, (void*)(1));
    msig_reserve_instances(sigpos[1], maxAgents-1);

    input = mdev_add_input(dev, "/node/gain", 1, 'f', 0, &fmn,
                           &fmx, on_signal_gain, 0);
    msig_reserve_instances(input, maxAgents-1);

    fmx = 0.9;
    input = mdev_add_input(dev, "/node/fade", 1, 'f', 0, &fmn,
                           &fmx, on_signal_fade, 0);
    msig_reserve_instances(input, maxAgents-1);

    fmn = -1.5;
    fmx = 1.5;
    input = mdev_add_input(dev, "/node/spin", 1, 'f', 0, &fmn,
                           &fmx, on_signal_spin, 0);
    msig_reserve_instances(input, maxAgents-1);

    fmn = -3.1415926;
    fmx = 3.1415926;
    input = mdev_add_input(dev, "/node/direction", 1, 'f', 0, &fmn,
                           &fmx, on_signal_dir, 0);
    msig_reserve_instances(input, maxAgents-1);

    fmn = -1.0;
    fmx = 1.0;
    input = mdev_add_input(dev, "/node/flow", 1, 'f', 0, &fmn,
                           &fmx, on_signal_flow, 0);
    msig_reserve_instances(input, maxAgents-1);
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
    int i;
    printf("Cleaning up...\n");
    for (i=0; i<maxAgents; i++) {
        msig_release_instance(sigobs[0], i);
        msig_release_instance(sigobs[1], i);
    }
    mdev_poll(dev, 100);
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
