
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <mapper/mapper.h>

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#endif

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

mapper_signal sig_pos = 0,
              sig_gain = 0,
              sig_spin = 0,
              sig_fade = 0,
              sig_dir = 0,
              sig_flow = 0;

float obs[2] = {0,0};

#define WIDTH  500
#define HEIGHT 500

int id = 0;
int done = 0;

void make_influence_connections()
{
    char signame1[1024], signame2[1024];
    struct _agentInfo *info = &agentInfo;

    sprintf(signame1, "%s/node/%d/observation",
            info->influence_device_name, mdev_ordinal(info->dev));

    sprintf(signame2, "%s/observation", mdev_name(info->dev));

    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);

    sprintf(signame1, "%s/position", mdev_name(info->dev));

    sprintf(signame2, "%s/node/%d/position",
            info->influence_device_name, mdev_ordinal(info->dev));

    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);
}

void make_xagora_connections()
{
    char signame1[1024], signame2[1024];
    struct _agentInfo *info = &agentInfo;

    sprintf(signame2, "%s/Butterfly%d",
            info->xagora_device_name, mdev_ordinal(info->dev));

    mapper_monitor_connect(info->mon, signame1, signame2, 0, 0);
}

void signal_handler(mapper_signal msig,
                    mapper_db_signal props,
                    mapper_timetag_t *timetag,
                    void *value)
{
    memcpy(obs, value, sizeof(float)*2);
    printf("observation: %f, %f\n", obs[0], obs[1]);
}

void dev_db_callback(mapper_db_device record,
                     mapper_db_action_t action,
                     void *user)
{
    // if we see /influence.1 or /XAgora.1, send /link message
    struct _agentInfo *info = (struct _agentInfo*)user;

    if (action == MDB_NEW) {
        if (strcmp(record->name, info->influence_device_name)==0) {
            mapper_monitor_link(info->mon, mdev_name(info->dev),
                                record->name);
            mapper_monitor_link(info->mon, record->name,
                                mdev_name(info->dev));
        }
        else if (strcmp(record->name, info->xagora_device_name)==0)
            mapper_monitor_link(info->mon, mdev_name(info->dev),
                                record->name);
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
        if (((strcmp(record->src_name, info->influence_device_name)==0) &&
             (strcmp(record->dest_name, mdev_name(info->dev))==0)) ||
            ((strcmp(record->dest_name, info->influence_device_name)==0) &&
             (strcmp(record->src_name, mdev_name(info->dev))==0))) {
            info->linked_influence++;
            if (info->linked_influence >= 2)
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
    struct _agentInfo *info = &agentInfo;
    memset(info, 0, sizeof(struct _agentInfo));

    info->influence_device_name = strdup("/influence.1");
    info->xagora_device_name = strdup("/XAgora_receiver.1");

    info->admin = mapper_admin_new(0, 0, 0);

    // add device
    info->dev = mdev_new("agent", 9000 + id, info->admin);
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
    mdev_add_input(info->dev, "observation", 2, 'f', "norm", &mn, &mx,
                   signal_handler, 0);
    int imn=0, imx=WIDTH;
    sig_pos = mdev_add_output(info->dev, "position", 2, 'i', 0, &imn, &imx);
    sig_gain = mdev_add_output(info->dev, "gain", 1, 'f',
                               "normalized", &mn, &mx);
    mx = 0.9;
    sig_fade = mdev_add_output(info->dev, "fade", 1, 'f', "normalized", &mn, &mx);
    mn = -1.5;
    mx = 1.5;
    sig_spin = mdev_add_output(info->dev, "spin", 1, 'f', "radians", &mn, &mx);
    mn = -3.1415926;
    mx = 3.1315926;
    sig_dir = mdev_add_output(info->dev, "direction", 1, 'f', "radians", &mn, &mx);
    mn = -1;
    sig_flow = mdev_add_output(info->dev, "flow", 1, 'f', "noramlized", &mn, &mx);

    return info;
}

void agentLogout()
{
    struct _agentInfo *info = &agentInfo;

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
    if (argc > 1)
        id = atoi(argv[1]);

    signal(SIGINT, ctrlc);

    struct _agentInfo *info = agentInit();
    if (!info->dev)
        goto done;

    float pos[2];
    pos[0] = rand()%WIDTH/2+WIDTH/4;
    pos[1] = rand()%HEIGHT/2+HEIGHT/4;
    float gain = 0.2;
    float damping = 0.9;
    float limit = 1;
    float vel[2] = {0,0};

    while (!done) {
        mapper_monitor_poll(info->mon, 0);
        if (mdev_poll(info->dev, 10)) {
            vel[0] += obs[0] * gain;
            vel[1] += obs[1] * gain;
            pos[0] += vel[0];
            pos[1] += vel[1];
            vel[0] *= damping;
            vel[1] *= damping;

            if (vel[0] >  limit) vel[0] =  limit;
            if (vel[0] < -limit) vel[0] = -limit;
            if (vel[1] >  limit) vel[1] =  limit;
            if (vel[1] < -limit) vel[1] = -limit;

            if (pos[0] < 0) {
                pos[0] = 0;
                vel[0] *= -0.95;
            }

            if (pos[0] >= WIDTH) {
                pos[0] = WIDTH-1;
                vel[0] *= -0.95;
            }

            if (pos[1] < 0) {
                pos[1] = 0;
                vel[1] *= -0.95;
            }

            if (pos[1] >= HEIGHT) {
                pos[1] = HEIGHT-1;
                vel[1] *= -0.95;
            }

            int p[2];
            p[0] = (int)pos[0];
            p[1] = (int)pos[1];
            msig_update(sig_pos, p);
        }
        else {
            usleep(500);
            int p[2];
            p[0] = (int)pos[0];
            p[1] = (int)pos[1];
            msig_update(sig_pos, p);
        }
    }

done:
    agentLogout();
    return 0;
}
