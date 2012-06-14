
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <mapper/mapper.h>

struct _agentInfo
{
    char *influence_device_name;
    char *xagora_device_name;

    int linked_influence;
    int connected;

    mapper_admin admin;
    mapper_device dev;
    mapper_monitor mon;
    mapper_db db;
} agentInfo;

mapper_signal sig_pos[2],
              sig_vel[2],
              sig_accel[2],
              sig_force = 0,
              sig_gain = 0,
              sig_spin = 0,
              sig_fade = 0,
              sig_dir = 0,
              sig_flow = 0;

float mass = 1.0;
float gain = 0.0001;
float damping = 0.6;
float limit = 0.1;

#define WIDTH  500
#define HEIGHT 500

int numInstances = 1;
int done = 0;

void make_influence_connections()
{
    char signame1[1024], signame2[1024];
    struct _agentInfo *info = &agentInfo;

    sprintf(signame1, "%s/node/observation", info->influence_device_name);

    sprintf(signame2, "%s/force", mdev_name(info->dev));

    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);

    sprintf(signame1, "%s/position", mdev_name(info->dev));

    sprintf(signame2, "%s/node/position", info->influence_device_name);

    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);
}

void make_xagora_connections()
{
    char signame1[1024], signame2[1024];
    struct _agentInfo *info = &agentInfo;

    sprintf(signame1, "%s/position", mdev_name(info->dev));

    sprintf(signame2, "%s/Butterfly%d",
            info->xagora_device_name, mdev_ordinal(info->dev));

    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);
}

void force_handler(mapper_signal msig,
                   int instance_id,
                   mapper_db_signal props,
                   mapper_timetag_t *timetag,
                   void *value)
{
    if (!value)
        return;
    float *force = (float *)value;
    float accel[2], *paccel;

    paccel = (float *)msig_instance_value(sig_accel[0], instance_id, 0);
    if (!paccel) {
        printf("error: force_handler cannot retrieve acceleration for instance %i\n",
               instance_id);
        return;
    }

    accel[0] = paccel[0] + force[0] / mass * gain;
    accel[1] = paccel[1] + force[1] / mass * gain;

    msig_update_instance(sig_accel[0], instance_id, accel);
}

void dev_db_callback(mapper_db_device record,
                     mapper_db_action_t action,
                     void *user)
{
    // if we see /influence.1 or /XAgora.1, send /link message
    struct _agentInfo *info = (struct _agentInfo*)user;

    if (action == MDB_NEW) {
        if (strcmp(record->name, info->influence_device_name)==0 ||
            strcmp(record->name, info->xagora_device_name)==0) {
            mapper_monitor_link(info->mon, mdev_name(info->dev),
                                record->name);
            mapper_monitor_link(info->mon, record->name,
                                mdev_name(info->dev));
        }
    }
    else if (action == MDB_REMOVE) {
        if (strcmp(record->name, info->influence_device_name)==0) {
            mapper_monitor_unlink(info->mon, mdev_name(info->dev),
                                  record->name);
            info->linked_influence = 0;
        }
    }
}

void link_db_callback(mapper_db_link record,
                      mapper_db_action_t action,
                      void *user)
{
    // if we see our links, send /connect messages
    struct _agentInfo *info = (struct _agentInfo*)user;

    if (action == MDB_NEW) {
        if (((strcmp(record->dest_name, info->influence_device_name)==0) &&
            (strcmp(record->src_name, mdev_name(info->dev))==0)) ||
            ((strcmp(record->src_name, info->influence_device_name)==0) &&
             (strcmp(record->dest_name, mdev_name(info->dev))==0))) {
            info->linked_influence++;
            if (info->linked_influence > 1)
                make_influence_connections();
        }
        else if ((strcmp(record->src_name, mdev_name(info->dev))==0) &&
              (strcmp(record->dest_name, info->xagora_device_name)==0)) {
            make_xagora_connections();
        }
    }
    else if (action == MDB_REMOVE) {
        if ((strcmp(record->src_name, info->influence_device_name)==0) ||
            (strcmp(record->dest_name, info->influence_device_name)==0))
            info->linked_influence--;
    }
}

struct _agentInfo *agentInit()
{
    int i;
    float init[2] = {0, 0};
    struct _agentInfo *info = &agentInfo;
    memset(info, 0, sizeof(struct _agentInfo));

    info->influence_device_name = strdup("/influence.1");
    info->xagora_device_name = strdup("/XAgora_receiver.1");

    info->admin = mapper_admin_new(0, 0, 0);

    // add device
    info->dev = mdev_new("agent", 9000, info->admin);
    while (!mdev_ready(info->dev)) {
        mdev_poll(info->dev, 100);
    }
    printf("ordinal: %d\n", mdev_ordinal(info->dev));
    fflush(stdout);

    // add monitor and monitor callbacks
    info->mon = mapper_monitor_new(info->admin, 0);
    info->db  = mapper_monitor_get_db(info->mon);
    mapper_db_add_device_callback(info->db, dev_db_callback, info);
    mapper_db_add_link_callback(info->db, link_db_callback, info);

    // add signals
    float mn=-1, mx=1;
    sig_force = mdev_add_input(info->dev, "force", 2, 'f', "N", &mn, &mx,
                               force_handler, 0);
    msig_reserve_instances(sig_force, numInstances-1);

    sig_accel[0] = mdev_add_input(info->dev, "acceleration", 2, 'f', 0,
                                &mn, &mx, 0, 0);
    msig_reserve_instances(sig_accel[0], numInstances-1);
    sig_accel[1] = mdev_add_output(info->dev, "acceleration", 2, 'f', 0, &mn, &mx);
    msig_reserve_instances(sig_accel[1], numInstances-1);

    // initialize accelerations to zero
    for (i=0; i<numInstances; i++) {
        msig_update_instance(sig_accel[0], i, init);
    }

    sig_vel[0] = mdev_add_input(info->dev, "velocity", 2, 'f', "m/s",
                                &mn, &mx, 0, 0);
    msig_reserve_instances(sig_vel[0], numInstances-1);
    sig_vel[1] = mdev_add_output(info->dev, "velocity", 2, 'f', "m/s", &mn, &mx);
    msig_reserve_instances(sig_vel[1], numInstances-1);
    
    // initialize velocities to zero
    for (i=0; i<numInstances; i++) {
        msig_update_instance(sig_vel[0], i, init);
    }

    sig_pos[0] = mdev_add_input(info->dev, "position", 2, 'f', 0,
                                &mn, &mx, 0, 0);
    msig_reserve_instances(sig_pos[0], numInstances-1);
    sig_pos[1] = mdev_add_output(info->dev, "position", 2, 'f', 0, &mn, &mx);
    msig_reserve_instances(sig_pos[1], numInstances-1);

    // initialize positions to random values
    for (i=0; i<numInstances; i++) {
        init[0] = rand()%1000*0.002-1.0;
        init[1] = rand()%1000*0.002-1.0;
        msig_update_instance(sig_pos[0], i, init);
    }

    sig_gain = mdev_add_output(info->dev, "gain", 1, 'f', "normalized", &mn, &mx);
    msig_reserve_instances(sig_gain, numInstances-1);

    mx = 0.9;
    sig_fade = mdev_add_output(info->dev, "fade", 1, 'f', "normalized", &mn, &mx);
    msig_reserve_instances(sig_fade, numInstances-1);

    mn = -1.5;
    mx = 1.5;
    sig_spin = mdev_add_output(info->dev, "spin", 1, 'f', "radians", &mn, &mx);
    msig_reserve_instances(sig_spin, numInstances-1);

    mn = -3.1415926;
    mx = 3.1315926;
    sig_dir = mdev_add_output(info->dev, "direction", 1, 'f', "radians", &mn, &mx);
    msig_reserve_instances(sig_dir, numInstances-1);

    mn = -1;
    mx = 1;
    sig_flow = mdev_add_output(info->dev, "flow", 1, 'f', "normalized", &mn, &mx);
    msig_reserve_instances(sig_flow, numInstances-1);

    return info;
}

void agentLogout()
{
    printf("Cleaning up...\n");
    struct _agentInfo *info = &agentInfo;

    int i;
    for (i=0; i<numInstances; i++)
        msig_release_instance(sig_pos[1], i);

    mdev_poll(info->dev, 100);

    if (info->influence_device_name) {
        mapper_monitor_unlink(info->mon,
                              info->influence_device_name,
                              mdev_name(info->dev));
        free(info->influence_device_name);
    }
    if (info->xagora_device_name) {
        mapper_monitor_unlink(info->mon,
                              mdev_name(info->dev),
                              info->xagora_device_name);
        free(info->xagora_device_name);
    }
    if (info->dev)
        mdev_free(info->dev);
    if (info->db) {
        mapper_db_remove_device_callback(info->db, dev_db_callback, info);
        mapper_db_remove_link_callback(info->db, link_db_callback, info);
    }
    if (info->mon)
        mapper_monitor_free(info->mon);
    if (info->admin) {
        mapper_admin_free(info->admin);
    }
    memset(info, 0, sizeof(struct _agentInfo));
}

void ctrlc(int sig)
{
    done = 1;
}

int main(int argc, char *argv[])
{
    int i;
    if (argc > 1)
        numInstances = atoi(argv[1]);

    signal(SIGINT, ctrlc);

    srand(100);

    struct _agentInfo *info = agentInit();
    if (!info->dev)
        goto done;

    float *paccel, *pvel, *ppos;
    float accel[2], vel[2], pos[2];

    while (!mdev_ready(info->dev)) {
        mapper_monitor_poll(info->mon, 0);
        mdev_poll(info->dev, 10);
    }

    while (!done) {
        mapper_monitor_poll(info->mon, 0);
        mdev_poll(info->dev, 10);

        for (i=0; i<numInstances; i++) {
            paccel = (float *)msig_instance_value(sig_accel[0], i, 0);
            pvel = (float *)msig_instance_value(sig_vel[0], i, 0);
            if (!pvel) {
                printf("couldn't retrieve vel value for instance %i\n", i);
                continue;
            }
            if (!paccel) {
                printf("couldn't retrieve accel value for %p instance %i\n", sig_accel[0], i);
                continue;
            }
            
            ppos = (float *)msig_instance_value(sig_pos[0], i, 0);
            if (!ppos) {
                printf("couldn't retrieve pos value for instance %i\n", i);
                continue;
            }

            accel[0] = paccel[0] * 0.9;
            accel[1] = paccel[1] * 0.9;
            vel[0] = pvel[0] * 0.9 + accel[0];
            vel[1] = pvel[1] * 0.9 + accel[1];
            pos[0] = ppos[0] + vel[0];
            pos[1] = ppos[1] + vel[1];

            if (pos[0] < -1) {
                pos[0] = -1;
                vel[0] *= -0.95;
            }
            if (pos[0] >= 1) {
                pos[0] = 1;
                vel[0] *= -0.95;
            }
            if (pos[1] < -1) {
                pos[1] = -1;
                vel[1] *= -0.95;
            }
            if (pos[1] >= 1) {
                pos[1] = 1;
                vel[1] *= -0.95;
            }
            accel[0] = accel[1] = 0;

            msig_update_instance(sig_accel[0], i, accel);
            msig_update_instance(sig_accel[1], i, accel);
            msig_update_instance(sig_vel[0], i, vel);
            msig_update_instance(sig_vel[1], i, vel);
            msig_update_instance(sig_pos[0], i, pos);
            msig_update_instance(sig_pos[1], i, pos);
        }
    }

done:
    agentLogout();
    return 0;
}
