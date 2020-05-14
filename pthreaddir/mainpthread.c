#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>

#define LINES 1000000
#define LINE_MAX     2003
#define NUM_THREADS  32

char data[LINES][LINE_MAX];
int sums[LINES];
int diffs[LINES - 1];

void *sum_array(void *myID) {
  int i, j;

  int startPos = ((int) myID) * (LINES / NUM_THREADS);
  int endPos = startPos + (LINES / NUM_THREADS);

  for (i = startPos; i < endPos; i++) {
    j = 0;
    while (data[i][j] != '\0') {
      sums[i] += data[i][j];
      j++;
    }
  }
}

void *compute_diff(void *myID) {
  int i;

  int startPos = ((int) myID) * (LINES / NUM_THREADS);
  int endPos = startPos + (LINES / NUM_THREADS);

  for (i = startPos; i < endPos; i++) {
    diffs[i] = sums[i] - sums[i + 1];
  }
}

int main() {
  int i, rc;
  pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;
	void *status;

  struct timeval t1, t2, t3, t4, t5;

  gettimeofday(&t1, NULL);
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  FILE *fp = fopen("/homes/dan/625/wiki_dump.txt", "r");
  // FILE *fp = fopen("../test.txt", "r");

  if (fp == NULL) {
    printf("Error opening file\n");
    return -1;
  }

  // Read file
  for (i = 0; i < LINES; i++) {
    fgets(data[i], LINE_MAX, fp);
    sums[i] = 0;
  }

  gettimeofday(&t2, NULL);

  for (i = 0; i < NUM_THREADS; i++ ) {
    rc = pthread_create(&threads[i], &attr, sum_array, (void *)i);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  pthread_attr_destroy(&attr);

  for(i = 0; i < NUM_THREADS; i++) {
    rc = pthread_join(threads[i], &status);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }

  gettimeofday(&t3,NULL);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (i = 0; i < NUM_THREADS; i++ ) {
    rc = pthread_create(&threads[i], &attr, compute_diff, (void *)i);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  pthread_attr_destroy(&attr);

  for(i = 0; i < NUM_THREADS; i++) {
    rc = pthread_join(threads[i], &status);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }

  gettimeofday(&t4,NULL);

  // Print the differences
  for (i = 0; i < LINES - 1; i++) {
     printf("%d-%d: %d \n", i, i+1, diffs[i]);
  }
  gettimeofday(&t5, NULL);

  //Print number of lines
  printf("Number of Lines: %d\n", LINES);


  //Calculate times and print
  double time = (t2.tv_sec - t1.tv_sec) * 1000.0;
  time += (t2.tv_usec - t1.tv_usec) / 1000.0;
  printf("Time to read data: %f\n", time);

  time = (t3.tv_sec - t2.tv_sec) * 1000.0;
  time += (t3.tv_usec - t2.tv_usec) / 1000.0;
  printf("Time to compute sums: %f\n", time);

  time = (t4.tv_sec - t3.tv_sec) * 1000.0;
  time += (t4.tv_usec - t3.tv_usec) / 1000.0;
  printf("Time to compute diffs: %f\n", time);

  time = (t5.tv_sec - t4.tv_sec) * 1000.0;
  time += (t5.tv_usec - t4.tv_usec) / 1000.0;
  printf("Time to print results: %f\n", time);

  time = (t5.tv_sec - t1.tv_sec) * 1000.0;
  time += (t5.tv_usec - t1.tv_usec) / 1000.0;
  printf("Total running time: %f\n", time);

  fclose(fp);
}

