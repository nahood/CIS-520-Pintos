#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <sys/time.h>

#define LINES 1000000
#define LINE_MAX 2003

char data[LINES][LINE_MAX];
int sums[LINES];
int diffs[LINES];

int main() {
  int i, j;

  struct timeval t1, t2, t3, t4, t5;

  gettimeofday(&t1, NULL);
  FILE *fp = fopen("/homes/dan/625/wiki_dump.txt", "r");
  //FILE *fp = fopen("../test.txt", "r");

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

  #pragma omp parallel private(i,j)
  {
    // Compute sums of ASCII values
    #pragma omp for
      for (i = 0; i < LINES; i++) {
        j = 0;
        while (data[i][j] != '\0') {
          sums[i] += data[i][j];
          j++;
        }
      }
  }

  gettimeofday(&t3,NULL);

  #pragma omp parallel private(i)
  {
    // Compute diffs between lines
    #pragma omp for
      for (i = 0; i < LINES; i++) {
        diffs[i] = sums[i] - sums[i + 1];
      }
  }

  gettimeofday(&t4, NULL);

  // Print the differences
  for (i = 0; i < LINES - 1; i++) {
     printf("%d-%d: %d \n", i, i+1, diffs[i]);
  }

  gettimeofday(&t5, NULL);

  //Print out number of lines to read
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

