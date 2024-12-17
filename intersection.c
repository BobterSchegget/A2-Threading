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



#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif


// Handles the turning on and off of the traffic light
static void lighthandler(int side, int directionm, Arrival current_car);


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
 * Mutexes for 'conflict zone' for the advanced solution
 */
static pthread_mutex_t mutex1;
static pthread_mutex_t mutex2;
static pthread_mutex_t mutex3;
static pthread_mutex_t mutex4;
static pthread_mutex_t mutex5;
static pthread_mutex_t mutex6;
static pthread_mutex_t mutex7;



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

  // Track the next car to be processed for this light
  int next_car_index = 0;

  // Wait for the first arrival
  while(1)
  {
    int result = sem_timedwait(&semaphores[side][direction], &ts);
    if (result == -1 && errno == ETIMEDOUT)
    {
      // Exit when time is up
      return 0;
    }
    
    // get current car
    Arrival current_car = curr_arrivals[side][direction][next_car_index];
    next_car_index++;

    pthread_mutex_t* required_mutexes[4]; // Array to hold the required mutexes
    int mutex_count = 0;    
     

  // mutex logic
   if (side == 1 && direction == 2) {
            required_mutexes[0] = &mutex4;
            mutex_count = 1;
        } else if (side == 1 && direction == 1) {
            required_mutexes[0] = &mutex5;
            required_mutexes[1] = &mutex6;
            mutex_count = 2;
        } else if (side == 1 && direction == 0) {
            required_mutexes[0] = &mutex2;
            required_mutexes[1] = &mutex3;
            required_mutexes[2] = &mutex7;
            mutex_count = 3;
        } else if (side == 2 && direction == 2) {
            required_mutexes[0] = &mutex1;
            mutex_count = 1;
        } else if (side == 2 && direction == 1) {
            required_mutexes[0] = &mutex3;
            required_mutexes[1] = &mutex4;
            required_mutexes[2] = &mutex5;
            mutex_count = 3;
        } else if (side == 2 && direction == 0) {
            required_mutexes[0] = &mutex2;
            required_mutexes[1] = &mutex6;
            mutex_count = 2;
        } else if (side == 2 && direction == 3) {
            required_mutexes[0] = &mutex7;
            mutex_count = 1;
        } else if (side == 3 && direction == 2) {
            required_mutexes[0] = &mutex7;
            mutex_count = 1;
        } else if (side == 3 && direction == 1) {
            required_mutexes[0] = &mutex1;
            required_mutexes[1] = &mutex2;
            required_mutexes[2] = &mutex3;
            mutex_count = 3;
        } else if (side == 3 && direction == 0) {
            required_mutexes[0] = &mutex4;
            required_mutexes[1] = &mutex5;
            required_mutexes[2] = &mutex6;
            mutex_count = 3;
        }

          while (1)
        {
            bool locked = true;

            // Attempt to lock all required mutexes
            for (int i = 0; i < mutex_count; i++) {
                if (pthread_mutex_trylock(required_mutexes[i]) != 0) {
                    // If locking fails, release previously acquired locks
                    for (int j = 0; j < i; j++) {
                        pthread_mutex_unlock(required_mutexes[j]);
                    }
                    locked = false;
                    break;
                }
            }

            if (locked) {
                // Successfully acquired all locks
                break;
            } else {
                // Failed to acquire locks, sleep before retrying
                usleep(100);
            }
        }

   lighthandler(side, direction, current_car);
  
   for (int i = 0; i < mutex_count; i++) {
            pthread_mutex_unlock(required_mutexes[i]);
        }

  }
  return(0);
}

void lighthandler(int side, int direction, Arrival current_car)
{
  // Turn traffic light green
  printf("traffic light %d %d turns green at time %d for car %d\n",
         side, direction, get_time_passed(), current_car.id);

  // Sleep
  sleep(CROSS_TIME);

  // Turn traffic light red
  printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());
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

 // wait for all threads to finish
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
