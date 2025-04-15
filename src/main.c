/*
 * PREMISE: RAID 5
 *
 * Data is striped across multiple disks along with parity. Parity is a
 * special form of data that is used to recover data if disk failure occurs.
 *
 * DISCREPANCY: A whole disk worth of storage is used to store parity. That
 * is, if the size of each drive is 1 TB/drive, and there are four drives, than
 * 1TB of storage is used to store parity.
 *
 * */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * INPUTS
 *
 * param: B -> Block Size (from 1 to 2^12)
 * param: J -> Number of bytes; multiple of B
 * param: PATH -> path to hard disk contents
 * param: K -> Number of bytes K stored in each disk; multiple of B
 * param: N1,N2,...,Nn -> path to desirable number of drives arranged in RAID 5
 *
 * */

int main(int argc, char **argv) {

  if (argc <= 5) {
    fprintf(stderr, "Invalid Usage. Must have at least 5 arguments.");
    return EXIT_FAILURE;
  }

  int B, J;
  long tmp[2];
  for (int i = 0; i < 2; i++) {
    errno = 0;
    char *end = NULL;
    tmp[i] = strtol(argv[i + 1], &end, 10);

    if (errno == ERANGE || tmp[i] > INT_MAX || tmp[i] < INT_MIN) {
      perror("strtol error");
      return EXIT_FAILURE;
    }

    if (*end != '\0') {
      fprintf(stderr, "Invalid integer input");
      return EXIT_FAILURE;
    }
  }

  B = (int)tmp[0];
  J = (int)tmp[1];

  printf("DEBUG: B(%d) and J(%d) are successfully initialized\n", B, J);

  FILE *fp;
  char *PATH = argv[3];
  fp = fopen(PATH, "r");
  if (!fp) {
    fprintf(stderr, "Error opening file %s", PATH);
    return EXIT_FAILURE;
  } else
    printf("DEBUG: File of path %s SUCCESS.", PATH);

  return EXIT_SUCCESS;
}
