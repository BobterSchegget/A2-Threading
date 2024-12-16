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
 * Mutexes for 'conflict zone' for the advanced solution
 */
static pthread_mutex_t mutex12;
static pthread_mutex_t mutex11;
static pthread_mutex_t mutex10;
static pthread_mutex_t mutex22;
static pthread_mutex_t mutex21;
static pthread_mutex_t mutex20;
static pthread_mutex_t mutex23;
static pthread_mutex_t mutex32;
static pthread_mutex_t mutex31;
static pthread_mutex_t mutex30;

int is_mutex_locked(pthread_mutex_t *mutex) {
    int lock_status = pthread_mutex_trylock(mutex);
    if (lock_status == 0) {
        pthread_mutex_unlock(mutex);
        return 0; // Not locked
    } else if (lock_status == EBUSY) {
        return 1; // Locked
    }
    return -1; // Error case
}

void wait_until_unlocked(pthread_mutex_t *mutex) {
    while (is_mutex_locked(mutex)) {
        usleep(100); // Sleep briefly to avoid busy waiting
    }

}

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

    

      
     
    if (side == 1 && direction == 2) { 
    pthread_mutex_lock(&mutex12);

    wait_until_unlocked(&mutex21);
    wait_until_unlocked(&mutex30);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());

   

    pthread_mutex_unlock(&mutex12);
}
else if (side == 1 && direction == 1) { 
    pthread_mutex_lock(&mutex11);
    wait_until_unlocked(&mutex21);
    wait_until_unlocked(&mutex20);
    wait_until_unlocked(&mutex30); 

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());


    pthread_mutex_unlock(&mutex11);
}
else if (side == 1 && direction == 0) { 
   pthread_mutex_lock(&mutex10);
    wait_until_unlocked(&mutex21);
    wait_until_unlocked(&mutex20);
    wait_until_unlocked(&mutex23);
    wait_until_unlocked(&mutex32);
    wait_until_unlocked(&mutex31);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());


     pthread_mutex_unlock(&mutex10);
}
else if (side == 2 && direction == 2) {
   pthread_mutex_lock(&mutex22);
    wait_until_unlocked(&mutex31);
    

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());

    // Now unlock

     pthread_mutex_unlock(&mutex22);
}
else if (side == 2 && direction == 1) {
    pthread_mutex_lock(&mutex21);
    wait_until_unlocked(&mutex12);
    wait_until_unlocked(&mutex11);
    wait_until_unlocked(&mutex10);
    wait_until_unlocked(&mutex31);
    wait_until_unlocked(&mutex30);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());


   
    pthread_mutex_unlock(&mutex21);
}
else if (side == 2 && direction == 0) {
   pthread_mutex_lock(&mutex20);
    wait_until_unlocked(&mutex11);
    wait_until_unlocked(&mutex10);
    wait_until_unlocked(&mutex31);
    wait_until_unlocked(&mutex30);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());



     pthread_mutex_unlock(&mutex20);
}
else if (side == 2 && direction == 3) {
   pthread_mutex_lock(&mutex23);
    wait_until_unlocked(&mutex10);
    wait_until_unlocked(&mutex32);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());


     pthread_mutex_unlock(&mutex23);
}
else if (side == 3 && direction == 2) {
   pthread_mutex_lock(&mutex32);
    wait_until_unlocked(&mutex10);
    wait_until_unlocked(&mutex23);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());

     pthread_mutex_unlock(&mutex32);
}
else if (side == 3 && direction == 1) {
   pthread_mutex_lock(&mutex31);
    wait_until_unlocked(&mutex10);
    wait_until_unlocked(&mutex22);
    wait_until_unlocked(&mutex21);
    wait_until_unlocked(&mutex20);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());

     pthread_mutex_unlock(&mutex31);
}
else if (side == 3 && direction == 0) {
   pthread_mutex_lock(&mutex30);
    wait_until_unlocked(&mutex12);
    wait_until_unlocked(&mutex11);
    wait_until_unlocked(&mutex21);
    wait_until_unlocked(&mutex20);

    // Turn traffic light green
    printf("traffic light %d %d turns green at time %d for car %d\n",
        side, direction, get_time_passed(), current_car.id);

    // Sleep
    sleep(CROSS_TIME);

    // Turn traffic light red
    printf("traffic light %d %d turns red at time %d\n", side, direction, get_time_passed());


    pthread_mutex_unlock(&mutex30);
}

     
  }

  // TODO: (DONE)
  // while not all arrivals have been handled, repeatedly:
  //  - wait for an arrival using the semaphore for this traffic light
  //  - lock the right mutex(es)
  //  - make the traffic light turn green
  //  - sleep for CROSS_TIME seconds
  //  - make the traffic light turn red
  //  - unlock the right mutex(es)

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
  
  // TODO: create a thread per traffic light that executes manage_light
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

  // TODO: create a thread that executes supply_arrivals
  pthread_t arrivals_thread;
  pthread_create(&arrivals_thread, NULL, supply_arrivals, NULL);

  // TODO: wait for all threads to finish
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
