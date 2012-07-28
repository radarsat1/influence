
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

mapper_signal sig_pos_in[2],
              sig_pos_out[2],
              sig_vel_in[2],
              sig_vel_out[2],
              sig_accel_in[2],
              sig_accel_out[2],
              sig_force[2],
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

    sprintf(signame1, "%s/node/observation/x", info->influence_device_name);
    sprintf(signame2, "%s/force/x", mdev_name(info->dev));
    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);

    sprintf(signame1, "%s/node/observation/y", info->influence_device_name);
    sprintf(signame2, "%s/force/y", mdev_name(info->dev));
    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);

    sprintf(signame1, "%s/position/x", mdev_name(info->dev));
    sprintf(signame2, "%s/node/position/x", info->influence_device_name);
    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);

    sprintf(signame1, "%s/position/y", mdev_name(info->dev));
    sprintf(signame2, "%s/node/position/y", info->influence_device_name);
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
    mapper_signal sig = (mapper_signal)props->user_data;
    float accel, *paccel;

    paccel = (float *)msig_instance_value(sig, instance_id, 0);
    if (!paccel) {
        printf("error: force_handler cannot retrieve acceleration for instance %i\n",
               instance_id);
        return;
    }

    accel = (*paccel) + (*force) / mass * gain;
    msig_update_instance(sig, instance_id, &accel);
}

void dev_db_callback(mapper_db_device record,
                     mapper_db_action_t action,
                     void *user)
{
    // if we see /influence.1 or /XAgora.1, send /link message
    struct _agentInfo *info = (struct _agentInfo*)user;

    if (action == MDB_NEW) {
        if (strcmp(record->name, info->influence_device_name)==0) {
            mapper_db_link_t props;
            props.num_scopes = 1;
            char *name = (char *)mdev_name(info->dev);
            props.scope_names = &name;
            mapper_monitor_link(info->mon, mdev_name(info->dev), record->name, 0, 0);
            mapper_monitor_link(info->mon, record->name, mdev_name(info->dev),
                                &props, LINK_NUM_SCOPES | LINK_SCOPE_NAMES);
        }
        else if (strcmp(record->name, info->xagora_device_name)==0) {
            // make links to XAgora
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
            (strcmp(record->src_name, mdev_name(info->dev))==0)))
            info->linked_influence |= 0x01;
        else if (((strcmp(record->src_name, info->influence_device_name)==0) &&
             (strcmp(record->dest_name, mdev_name(info->dev))==0))) {
            info->linked_influence |= 0x02;
            if (info->linked_influence & 0x03)
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
    float init = 0;
    struct _agentInfo *info = &agentInfo;
    memset(info, 0, sizeof(struct _agentInfo));

    info->influence_device_name = strdup("/influence.1");
    info->xagora_device_name = strdup("/XAgora_receiver.1");

    info->admin = mapper_admin_new(0, 0, 0);

    // add device
    info->dev = mdev_new("agent", 0, info->admin);
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

    sig_accel_in[0] = mdev_add_input(info->dev, "acceleration/x", 1, 'f', 0,
                                     &mn, &mx, 0, 0);
    msig_reserve_instances(sig_accel_in[0], numInstances-1);
    sig_accel_in[1] = mdev_add_input(info->dev, "acceleration/y", 1, 'f', 0,
                                     &mn, &mx, 0, 0);
    msig_reserve_instances(sig_accel_in[1], numInstances-1);
    sig_accel_out[0] = mdev_add_output(info->dev, "acceleration/x", 1, 'f', 0, &mn, &mx);
    msig_reserve_instances(sig_accel_out[0], numInstances-1);
    sig_accel_out[0] = mdev_add_output(info->dev, "acceleration/y", 1, 'f', 0, &mn, &mx);
    msig_reserve_instances(sig_accel_out[1], numInstances-1);

    // initialize accelerations to zero
    for (i=0; i<numInstances; i++) {
        msig_update_instance(sig_accel_in[0], i, &init);
        msig_update_instance(sig_accel_in[1], i, &init);
    }

    sig_force[0] = mdev_add_input(info->dev, "force/x", 1, 'f', "N", &mn, &mx,
                                  force_handler, sig_accel_in[0]);
    msig_reserve_instances(sig_force[0], numInstances-1);
    sig_force[1] = mdev_add_input(info->dev, "force/y", 1, 'f', "N", &mn, &mx,
                                  force_handler, sig_accel_in[1]);
    msig_reserve_instances(sig_force[1], numInstances-1);

    sig_vel_in[0] = mdev_add_input(info->dev, "velocity/x", 1, 'f', "m/s",
                                   &mn, &mx, 0, 0);
    msig_reserve_instances(sig_vel_in[0], numInstances-1);
    sig_vel_in[1] = mdev_add_input(info->dev, "velocity/y", 1, 'f', "m/s",
                                   &mn, &mx, 0, 0);
    msig_reserve_instances(sig_vel_in[1], numInstances-1);
    sig_vel_out[0] = mdev_add_output(info->dev, "velocity/x", 1, 'f', "m/s", &mn, &mx);
    msig_reserve_instances(sig_vel_out[0], numInstances-1);
    sig_vel_out[1] = mdev_add_output(info->dev, "velocity/y", 1, 'f', "m/s", &mn, &mx);
    msig_reserve_instances(sig_vel_out[1], numInstances-1);
    
    // initialize velocities to zero
    for (i=0; i<numInstances; i++) {
        msig_update_instance(sig_vel_in[0], i, &init);
        msig_update_instance(sig_vel_in[1], i, &init);
    }

    sig_pos_in[0] = mdev_add_input(info->dev, "position/x", 1, 'f', 0,
                                   &mn, &mx, 0, 0);
    msig_reserve_instances(sig_pos_in[0], numInstances-1);
    sig_pos_in[1] = mdev_add_input(info->dev, "position/y", 1, 'f', 0,
                                   &mn, &mx, 0, 0);
    msig_reserve_instances(sig_pos_in[1], numInstances-1);
    sig_pos_out[0] = mdev_add_output(info->dev, "position/x", 1, 'f', 0, &mn, &mx);
    msig_reserve_instances(sig_pos_out[0], numInstances-1);
    sig_pos_out[1] = mdev_add_output(info->dev, "position/y", 1, 'f', 0, &mn, &mx);
    msig_reserve_instances(sig_pos_out[1], numInstances-1);

    // initialize positions to random values
    for (i=0; i<numInstances; i++) {
        init = rand()%1000*0.002-1.0;
        msig_update_instance(sig_pos_in[0], i, &init);
        init = rand()%1000*0.002-1.0;
        msig_update_instance(sig_pos_in[1], i, &init);
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
    for (i=0; i<numInstances; i++) {
        msig_release_instance(sig_pos_out[0], i);
        msig_release_instance(sig_pos_out[1], i);
        msig_release_instance(sig_vel_out[0], i);
        msig_release_instance(sig_vel_out[1], i);
        msig_release_instance(sig_accel_out[0], i);
        msig_release_instance(sig_accel_out[1], i);
    }

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
    int i, j;
    if (argc > 1)
        numInstances = atoi(argv[1]);

    signal(SIGINT, ctrlc);

    srand(100);

    struct _agentInfo *info = agentInit();
    if (!info->dev)
        goto done;

    float *paccel, *pvel, *ppos;
    float accel, vel, pos;

    while (!mdev_ready(info->dev)) {
        mapper_monitor_poll(info->mon, 0);
        mdev_poll(info->dev, 10);
    }

    while (!done) {
        mapper_monitor_poll(info->mon, 0);
        mdev_poll(info->dev, 20);

        for (i=0; i<numInstances; i++) {
            for (j=0; j<2; j++) {
                paccel = (float *)msig_instance_value(sig_accel_in[j], i, 0);
                pvel = (float *)msig_instance_value(sig_vel_in[j], i, 0);
                if (!pvel) {
                    printf("couldn't retrieve vel value for instance %i\n", i);
                    continue;
                }
                if (!paccel) {
                    printf("couldn't retrieve accel value for instance %i\n", i);
                    continue;
                }

                ppos = (float *)msig_instance_value(sig_pos_in[j], i, 0);
                if (!ppos) {
                    printf("couldn't retrieve pos value for instance %i\n", i);
                    continue;
                }

                accel = *paccel * 0.9;
                vel = *pvel * 0.9 + accel;
                pos = *ppos + vel;

                if (pos < -1) {
                    pos = -1;
                    vel *= -0.95;
                }
                if (pos >= 1) {
                    pos = 1;
                    vel *= -0.95;
                }

                accel = 0;

                msig_update_instance(sig_accel_in[j], i, &accel);
                msig_update_instance(sig_accel_out[j], i, &accel);
                msig_update_instance(sig_vel_in[j], i, &vel);
                msig_update_instance(sig_vel_out[j], i, &vel);
                msig_update_instance(sig_pos_in[j], i, &pos);
                msig_update_instance(sig_pos_out[j], i, &pos);
            }
        }
    }

done:
    agentLogout();
    return 0;
}
