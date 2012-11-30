/*
 * InfluenceEnvironment.cpp
 *
 * (c) 2012 Sofian Audry -- info(@)sofianaudry(.)com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "InfluenceEnvironment.h"

#include <unistd.h>
#include <stdio.h>

InfluenceEnvironment::InfluenceEnvironment(int observationDim_, int actionDim_, const char *namePrefix, bool autoConnect_, int initialPort)
  : devNamePrefix(namePrefix), autoConnect(autoConnect_), devInitialPort(initialPort), currentObservation(observationDim_), observationDim(observationDim_), actionDim(actionDim_) {
}

InfluenceEnvironment::~InfluenceEnvironment() {
  autoDisconnectDevice();
  if (dev)
    mdev_free(dev);
}

void InfluenceEnvironment::init() {
  dev = mdev_new(devNamePrefix, devInitialPort, 0);

  mdev_add_input(dev, "/observation", observationDim, 'f', 0, 0, 0,
                 (mapper_signal_handler*)updateInput, this);

  // Output "action" is position (x,y)
  outsigX = mdev_add_output(dev, "/position/x", 1, 'i', 0, 0, 0);
  outsigY = mdev_add_output(dev, "/position/y", 1, 'i', 0, 0, 0);

  if (autoConnect)
    autoConnectDevice(dev);
}

Observation* InfluenceEnvironment::start() {
  printf("Starting env\n");
  pos[0] = rand()%WIDTH/2+WIDTH/4;
  pos[1] = rand()%WIDTH/2+WIDTH/4;
  vel[0] = vel[1] = 0;

  // Send position.
  int x = (int)pos[0];
  int y = (int)pos[1];
  msig_update(outsigX, &x, 0, MAPPER_TIMETAG_NOW);
  msig_update(outsigY, &y, 0, MAPPER_TIMETAG_NOW);

  // Wait for response.
  mdev_poll(dev, 1000);
  return &currentObservation;
}

Observation* InfluenceEnvironment::step(const Action* action) {
  printf("Stepping env\n");

  // Update velocity depending on chosen action.
  float gain = 2;
  float limit = 1;

  float magnet = (action->actions[0] == 0 ? -1 : +1);
  //vel[0] = vel[1] = gain;
  magnet = 1;
  vel[0] += magnet * gain * (currentObservation[0] - currentObservation[2]);
  vel[1] += magnet * gain * (currentObservation[1] - currentObservation[3]);

  pos[0] += vel[0];
  pos[1] += vel[1];

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

  // Send position.
  int x = (int)pos[0];
  int y = (int)pos[1];
  msig_update(outsigX, &x, 0, MAPPER_TIMETAG_NOW);
  msig_update(outsigY, &y, 0, MAPPER_TIMETAG_NOW);
  
  // Wait for retroaction.
  while (! mdev_poll(dev, 10) );

  // Compute reward.

  currentObservation.reward = sqrt( vel[0]*vel[0] + vel[1]*vel[1] ); // go fast

  printf("--> receiving reward = %f, data = %f ...\n", currentObservation.reward, currentObservation[0]);

  return &currentObservation;
}

void InfluenceEnvironment::updateInput(mapper_signal sig, mapper_db_signal props,
                                        mapper_timetag_t *timetag, float *value) {
  printf("update input called %f\n", *value);
  RLObservation& obs = ((InfluenceEnvironment*)props->user_data)->currentObservation;
  for (unsigned int i=0; i<obs.dim; i++)
    obs[i] = value[i];
}
