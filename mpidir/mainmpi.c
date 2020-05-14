#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <stdlib.h>

#define LINES 1000000
#define LINE_MAX 2003

int NUM_THREADS;

char data[LINES][LINE_MAX];
int sums[LINES];
int local_sums[LINES];
int diffs[LINES - 1];
int local_diffs[LINES - 1];

void *sum_array(void *rank) {
  int i, j;

  int myID =  *((int*) rank);
  int startPos = ((long) myID) * (LINES / NUM_THREADS);
  int endPos = startPos + (LINES / NUM_THREADS);

  for (i = 0; i < LINES; i++) {
  	local_sums[i] = 0;
  }

  for (i = startPos; i < endPos; i++) {
    j = 0;
    while (data[i][j] != '\0') {
      local_sums[i] += data[i][j];
      j++;
    }
  }
}

void *compute_diff(void *rank) {
  int i;

  int myID =  *((int*) rank);
  int startPos = ((long) myID) * (LINES / NUM_THREADS);
  int endPos = startPos + (LINES / NUM_THREADS);

  for ( i = 0; i < LINES - 1; i++ ) {
  	local_diffs[i] = 0;
  }

  for (i = startPos; i < endPos; i++) {
    local_diffs[i] = sums[i] - sums[i + 1];
  }
}

int main(int argc, char* argv[]) {
  int i, rc;
  int numtasks, rank;
  MPI_Status Status;
  struct timeval t1, t2, t3, t4, t5;

  gettimeofday(&t1, NULL);

  // Initialize MPI
  rc = MPI_Init(&argc, &argv);
	if (rc != MPI_SUCCESS) {
	  printf ("Error starting MPI program. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }

  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	NUM_THREADS = numtasks;
  //printf("size = %d rank = %d\n", numtasks, rank);
  //fflush(stdout);


  FILE *fp = fopen("/homes/dan/625/wiki_dump.txt", "r"); 
  //FILE *fp = fopen("../test.txt", "r");

  if (fp == NULL) {
    printf("Error opening file\n");
    return -1;
  }

  if (rank == 0) {
    // Read file
    for (i = 0; i < LINES; i++) {
      fgets(data[i], LINE_MAX, fp);
      sums[i] = 0;
    }
  }

  // Sum the lines
  gettimeofday(&t2, NULL);

	MPI_Bcast(data, LINES * LINE_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);
  sum_array(&rank);
	MPI_Reduce(local_sums, sums, LINES, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  // Compute the diffs
  gettimeofday(&t3,NULL);

	MPI_Bcast(sums, LINES * LINE_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);
  compute_diff(&rank);
	MPI_Reduce(local_diffs, diffs, LINES, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  gettimeofday(&t4,NULL);

  // Print the differences
  if (rank == 0) {
    for (i = 0; i < LINES - 1; i++) {
      printf("%d-%d: %d \n", i, i+1, diffs[i]);
    }
  }
  gettimeofday(&t5, NULL);

  //Print number of lines
  if (rank == 0) {
    printf("Number of Lines: %d\n", LINES);
  }

  MPI_Finalize();

  //Calculate times and print
  if (rank == 0) {
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
  }

  fclose(fp);
}

