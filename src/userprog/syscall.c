#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "pagedir.h"

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

static void *kernel_addr (void *addr) {
  if (!valid_address (addr)) {
    exit (-1);
  }

  void *paddr = pagedir_get_page (thread_current ()->pagedir, addr);
  if (!paddr) {
    exit (-1);
  }

  return paddr;
}

static void
syscall_handler (struct intr_frame *f) 
{
  int call_num = *((int*)kernel_addr (f->esp));

  switch(call_num) {
    case SYS_WRITE: {
      int fd = *((int*)kernel_addr ((int*) f->esp + 1));
      void *buffer = (void*)(*((int*)kernel_addr ((int*) f->esp + 2)));
      unsigned size = *((unsigned*)kernel_addr ((int*) f->esp + 3));

      f->eax = write (fd, buffer, size);
      break;
    }
    case SYS_EXIT: {
      int status = *((int*)kernel_addr ((int*) f->esp + 1));
      exit (status);
      break;
    }
  }
}

