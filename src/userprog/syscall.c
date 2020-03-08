#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static int write (int fd, const void *buffer, unsigned size) {
  printf ("%s", (char*) buffer);

  return size;
}

static void
syscall_handler (struct intr_frame *f) 
{
  int call_num = *((int*) f->esp);

  switch(call_num) {
    case SYS_WRITE: {
      int fd = *((int*) f->esp + 1);
      void *buffer = (void*) (*((int*) f->esp + 2));
      unsigned size = *((unsigned*) f->esp + 3);

      f->eax = write(fd, buffer, size);
      break;
    }
  }
}

