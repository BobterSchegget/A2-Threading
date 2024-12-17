#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "arrivals.h"
#include "intersection_time.h"
#include "input.h"

#include <time.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

/* 
 * curr_arrivals[][][]
 *
 * A 3D array that stores the arrivals that have occurred
 * The first two indices determine the entry lane: first index is Side, second index is Direction
 * curr_arrivals[s][d] returns an array of all arrivals for the entry lane on side s for direction d,
 *   ordered in the same order as they arrived
 */
static Arrival curr_arrivals[4][4][20];

/*
 * semaphores[][]
 *
 * A 2D array that defines a semaphore for each entry lane,
 *   which are used to signal the corresponding traffic light that a car has arrived
 * The two indices determine the entry lane: first index is Side, second index is Direction
 */
static sem_t semaphores[4][4];

/**
 * ts
 *
 * A struct that defines the timeout for the semaphore
 */
struct timespec ts;

/**
 * mutex
 *
 * A single mutex to lock the semaphores
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * supply_arrivals()
 *
 * A function for supplying arrivals to the intersection
 * This should be executed by a separate thread
 */
static void* supply_arrivals()
{
  int t = 0;
  int num_curr_arrivals[4][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};

  // for every arrival in the list
  for (int i = 0; i < sizeof(input_arrivals)/sizeof(Arrival); i++)
  {
    // get the next arrival in the list
    Arrival arrival = input_arrivals[i];
    // wait until this arrival is supposed to arrive
    sleep(arrival.time - t);
    t = arrival.time;
    // store the new arrival in curr_arrivals
    curr_arrivals[arrival.side][arrival.direction][num_curr_arrivals[arrival.side][arrival.direction]] = arrival;
    num_curr_arrivals[arrival.side][arrival.direction] += 1;
    // increment the semaphore for the traffic light that the arrival is for
    sem_post(&semaphores[arrival.side][arrival.direction]);
  }

  return(0);
}

/*
 * manage_light(void* arg)
 *
 * A function that implements the behaviour of a traffic light
 */
static void* manage_light(void* arg)
{
  // Get the number of the traffic light
  int num = *(int*)arg;
  free(arg);

  // From the given number, determine the side and direction
  int side;
  int direction;

  if (num < 6) 
  {
    side = (num / 3) + 1;
    direction = num % 3;
  } else if (num == 6)
  {
    side = 2;
    direction = 3;
  } else if (num > 6) 
  {
    side = ((num - 1) / 3) + 1;
    direction = (num - 1) % 3;
  }

  // Wait for the first arrival
  while(1)
  {
    int result = sem_timedwait(&semaphores[side][direction], &ts);
    if (result == -1 && errno == ETIMEDOUT)
    {
      // Exit when time is up
      return 0;
    }
    else
    {
      // Get last car that has arrived at the traffic light
      int nrCars = 0;
      for (int i = 0; i < 20; i++)
      {
        if (curr_arrivals[side][direction][i].id != 0)
        {
          nrCars++;
        }
      }

      // Claim the mutex
      pthread_mutex_lock(&mutex);
      // Turn traffic light green
      printf("traffic light %d %d turns green at time %d for car %d\n",
       side, direction, get_time_passed(), curr_arrivals[side][direction][nrCars - 1].id);
      // Sleep
      sleep(CROSS_TIME);
      // Turn traffic light red
      printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());
      // Release the mutex
      pthread_mutex_unlock(&mutex);
    }
  }
  return(0);
}


int main(int argc, char * argv[])
{
  // create semaphores to wait/signal for arrivals
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      sem_init(&semaphores[i][j], 0, 0);
    }
  }

  // start the timer
  start_time();
  
  pthread_t traffic_lights[10];

  // set the end time
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += END_TIME - get_time_passed();

  for (int i = 0; i < 10; i++)
  {
    int * num = malloc(sizeof(int));
    *num = i;
    pthread_create(&traffic_lights[i], NULL, manage_light, num);
  }
  
  pthread_t arrivals_thread;
  pthread_create(&arrivals_thread, NULL, supply_arrivals, NULL);

  for (int i = 0; i < 10; i++)
  {
    pthread_join(traffic_lights[i], NULL);
  }
  pthread_join(arrivals_thread, NULL);

  // destroy semaphores
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      sem_destroy(&semaphores[i][j]);
    }
  }
}
