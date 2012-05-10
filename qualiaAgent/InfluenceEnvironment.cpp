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

  mdev_add_input(dev, "/observation", observationDim + 1, 'f', 0, 0, 0,
                 (mapper_signal_handler*)updateInput, this);

  // TODO: Actions range (min/max) should be known in advance somehow (this is a limitation of the current implemenation)
  outsig = mdev_add_output(dev, "/action", actionDim, 'i', 0, 0, 0);

  if (autoConnect)
    autoConnectDevice(dev);
}

Observation* InfluenceEnvironment::start() {
  printf("Starting env\n");
  mdev_poll(dev, 1000);
  return &currentObservation;
}

Observation* InfluenceEnvironment::step(const Action* action) {
  printf("Stepping env\n");
  printf("--> sending %d ...\n", action->actions[0]);
  msig_update(outsig, action->actions);
  while (!mdev_poll(dev, 1) );
  printf("--> receiving reward = %f, data = %f ...\n", currentObservation.reward, currentObservation[0]);
  //usleep(100);
  return &currentObservation;
}

void InfluenceEnvironment::updateInput(mapper_signal sig, mapper_db_signal props,
                                        mapper_timetag_t *timetag, float *value) {
  printf("update input called %f\n", *value);
  RLObservation& obs = ((InfluenceEnvironment*)props->user_data)->currentObservation;
  unsigned int k;
  for (k=0; k<obs.dim; k++)
    obs[k] = value[k];
  obs.reward = value[k];
}
