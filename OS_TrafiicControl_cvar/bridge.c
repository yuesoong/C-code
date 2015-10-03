#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

//direction
#define TO_HANOVER 6
#define TO_NORWICH 1

//position
#define HANOVER 2
#define BRIDGE 3
#define NORWICH 4

#define THRESHOD 5

pthread_cond_t cvar_hanover = PTHREAD_COND_INITIALIZER;
pthread_cond_t cvar_norwich = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int location;
  int direction;
  int id;
}Car;

int max_car;
int count = 0;
Car *cars;
int toNorwich;
int toHanover;
int wentToHanover;
int wentToNorwich;
int waitToHanover;
int waitToNorwich;
int waitDirection; //the direction that needs to pause
int dir;

void *oneVehicle(void *vargp);
void arriveBridge(Car *car);
void onBridge(Car *car);
void exitBridge(Car *car);
int condition(Car *car);
char *direction(int num);

int main(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Please provide the max number of cars on bridge, number of \n");
    printf("cars to Norwich, and number of cars to Hanover\n");
    exit(0);
  }
  max_car = atoi(argv[1]);
  toNorwich = atoi(argv[2]);
  toHanover = atoi(argv[3]);

  time_t t;
  srand((unsigned) time(&t));
  int i = toNorwich, j = toHanover, idx = 0, rc;
  double ratio = (double)toNorwich / (toNorwich + toHanover);

  pthread_t threads[toNorwich + toHanover];
  cars = calloc((toNorwich + toHanover), sizeof(Car));
  
  while (i > 0 || j > 0) {
    //randomize the sequence of the cars.
    double probability = (double)(rand() % 100) / 100;

    if (i <= 0) {
      cars[idx].location = NORWICH;
      cars[idx].direction = TO_HANOVER;
      cars[idx].id = toHanover - j + 1;
      j--;
    } else if (j <= 0) {
      cars[idx].location = HANOVER;
      cars[idx].direction = TO_NORWICH;
      cars[idx].id = toNorwich - i + 1;
      i--;
    } else if (probability < ratio) {
      cars[idx].location = HANOVER;
      cars[idx].direction = TO_NORWICH;
      cars[idx].id = toNorwich - i + 1;
      i--;
    } else {
      cars[idx].location = NORWICH;
      cars[idx].direction = TO_HANOVER;
      cars[idx].id = toHanover - j + 1;
      j--;
    }

    rc = pthread_create(&threads[idx], NULL, oneVehicle, (void *)&cars[idx]);
    if (rc) {
      printf("Failed to create a new thread\n");
      exit(-1);
    }
    
    idx++;
    usleep(100);
  }

  for (int i = 0; i < toNorwich + toHanover; i++) {
    rc = pthread_join(threads[i], NULL);
    if (rc) {
      fprintf(stderr, "pthread_join failed for thread %d\n", i);
      exit(-1);
    }
  }
  free(cars);
}

void *oneVehicle(void *vargp) {
  Car *car = (Car *)vargp;

  arriveBridge(car);
  onBridge(car);
  exitBridge(car);
  
  return NULL;
}

int condition(Car *car) {
  if (car -> direction == waitDirection) {
    return 0;
  } else if (count == 0) {
    return 1;
  } else if (count >= max_car) {
    return 0;
  } else if (dir == car -> direction) {
    return 1;
  } else {
    return 0;
  }
}

void arriveBridge(Car *car) {
  int rc;
  rc = pthread_mutex_lock(&mutex);
  if (rc) {
    printf("failed to mutex_lock.\n");
    exit(-1);
  }

  while (!condition(car)) {
    //update the number of waiting cars
    if (car -> direction == TO_HANOVER) {
      waitToHanover++;
      pthread_cond_wait(&cvar_hanover, &mutex);
      waitToHanover--;
    } else {
      waitToNorwich++;
      pthread_cond_wait(&cvar_norwich, &mutex);
      waitToNorwich--;
    }
  }

  //update the number of passed cars
  if (car -> direction == TO_HANOVER) {
    wentToHanover++;
  } else {
    wentToNorwich++;
  }

  dir = car -> direction;
  count++;
  //update the location of the car
  car -> location = BRIDGE;

  rc = pthread_mutex_unlock(&mutex);
  if (rc) {
    printf("fail to mutex_unlock.\n");
    exit(-1);
  }
}

void onBridge(Car *car) {
  int rc;
  rc = pthread_mutex_lock(&mutex);
  if (rc) {
    printf("fail to mutex_lock.\n");
    exit(-1);
  }
  int num = 0;
  for (int i = 0; i < toNorwich + toHanover; i++) {
    if (cars[i].location == BRIDGE) {
      num++;
      printf("No.%d %s on bridge.\n", cars[i].id, direction(cars[i].direction));
    }
  }
  printf("-------Total %d cars on bridge--------\n", num);

  rc = pthread_mutex_unlock(&mutex);
  if (rc) {
    printf("fail to mutex_unlock.\n");
    exit(-1);
  }

  usleep(1000);
}

void exitBridge(Car *car) {
  int rc;
  rc = pthread_mutex_lock(&mutex);
  if (rc) {
    printf("fail to mutex_lock.\n");
    exit(-1);
  }

  count--;
  if (count == 0) {
    wentToHanover = 0;
    wentToNorwich = 0;
    waitDirection = 0;
  }

  if (car -> direction == TO_HANOVER) {
    car -> location = HANOVER;
    //if there are cars in the opposite direction waiting
    //only tell the opposite direction to go
    if (count == 0 && waitToNorwich > 0) {
      rc = pthread_cond_broadcast(&cvar_norwich);
    } else if (wentToHanover > THRESHOD && waitToNorwich > 0) {
      rc = pthread_cond_broadcast(&cvar_norwich);
      wentToHanover = 0;
      waitDirection = TO_HANOVER;
    } else {
      rc = pthread_cond_signal(&cvar_hanover);
    }
  } else {
    car -> location = NORWICH;
    if (count == 0 && waitToHanover > 0) {
      rc = pthread_cond_broadcast(&cvar_hanover);
    } else if (wentToNorwich > THRESHOD && waitToHanover > 0) {
      rc = pthread_cond_broadcast(&cvar_hanover);
      wentToNorwich = 0;
      waitDirection = TO_NORWICH;
    } else {
      pthread_cond_signal(&cvar_norwich);
    }
  }

  rc = pthread_mutex_unlock(&mutex);
  if (rc) {
    printf("fail to mutex_unlock.\n");
    exit(-1);
  }
}

char *direction(int num) {
  if (num == TO_HANOVER) {
    char* str = "to Hanover";
    return str;
  } else {
    char *str = "to Norwich";
    return str;
  }
}
