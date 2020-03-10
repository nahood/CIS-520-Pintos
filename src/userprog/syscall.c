#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static int write (int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    printf ("%s", (char*) buffer);
  }

  return size;
}

static void exit (int status) {
  struct thread *t = thread_current ();
  printf ("%s: exit(%d)\n", t->name, status);
  thread_exit ();
}

static bool valid_address (void *addr) {
  // Check if the address is a virtual user address and within stack space
  return is_user_vaddr(addr) && (unsigned int) addr > (unsigned int) 0x08084000;
}

static void
syscall_handler (struct intr_frame *f) 
{
  if (!valid_address (f->esp)) {
    exit (-1);
    return;
  }

  int call_num = *((int*) f->esp);

  switch(call_num) {
    case SYS_WRITE: {
      int fd = *((int*) f->esp + 1);
      void *buffer = (void*) (*((int*) f->esp + 2));
      unsigned size = *((unsigned*) f->esp + 3);

      f->eax = write (fd, buffer, size);
      break;
    }
    case SYS_EXIT: {
      int status = *((int*) f->esp + 1);
      exit (status);
      break;
    }
  }
}

