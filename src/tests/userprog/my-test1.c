/* Opens a many files, which must succeed. */

#include <stdio.h>
#include <syscall.h>
#include "tests/userprog/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  char child_cmd[128];
  int handle;
  int i;
  int test_num = 500;
  for(i = 0; i< test_num; i++ ){
    CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");

    snprintf (child_cmd, sizeof child_cmd, "child-close %d", handle);
    msg ("wait(exec()) = %d", wait (exec (child_cmd)));
  }
}
