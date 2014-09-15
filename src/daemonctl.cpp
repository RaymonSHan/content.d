#include <stdio.h>
#include "../include/daemonctl.h"

int main(int argc, char** argv)
{
  if (argc >= 1)
    printf("begin %s\n", *argv);
  printf("second line\n");
}
