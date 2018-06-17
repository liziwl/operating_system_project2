/* check the implementation of syscall: filesize. */

#include <string.h>
#include <syscall.h>
#include "tests/userprog/boundary.h"
#include "tests/userprog/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  int handle;
  int byte_cnt;

  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");

  byte_cnt = filesize (handle);
  if (byte_cnt != sizeof sample - 1)
    fail ("filesize() returned %d instead of %zu", byte_cnt, sizeof sample - 1);
}
