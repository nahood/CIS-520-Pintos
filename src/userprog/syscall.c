#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "filesys/filesys.h"
#include "kernel/list.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include <string.h>
#include "devices/input.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);

// Initial file descriptor (0 and 1 are reserved)
static int fd = 2;

struct list fd_list;

struct sys_file {
  int fd;
  struct file *f;
  struct list_elem elem;
};

static struct sys_file* get_file (int fd) {
  struct list_elem *e;
  bool found = false;
  bool flag = false;
  struct sys_file *sf = malloc (sizeof (struct sys_file));

  for (e = list_begin (&fd_list); e != list_end (&fd_list); e = list_next (e)) {
    if (!flag) {
      free (sf);
      flag = true;
    }

    sf = list_entry(e, struct sys_file, elem);

    if (sf->fd == fd) {
      found = true;
      break;
    }
  }

  if (found) {
    return sf;
  } else {
    return NULL;
  }
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&fd_list);
}


// --- SYSCALLS


static int write (int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    printf ("%s", (char*) buffer);
  }

  return size;
}

static void exit (int status) {
  struct thread *t = thread_current ();
  t->exit_code = status;
  printf ("%s: exit(%d)\n", t->name, status);
  thread_exit ();
}

static int filesize (int fd) {
  struct sys_file *sf = get_file (fd);

  return sf->f->inode->data.length;
}

static bool create (const char *file, unsigned initial_size) {
  if (file == NULL) {
    exit (-1);
    return false;
  }

  return filesys_create(file, initial_size);
}

static int open (const char* file) {
  struct file *f;

  if (file != NULL && (f = filesys_open (file)) != NULL) {
    struct sys_file *sf = malloc (sizeof (struct sys_file));
    sf->fd = fd;
    sf->f = f;

    list_push_front (&fd_list, &sf->elem);
    return fd++;
  } else {
    return -1;
  }
}

static void close (int fd) {
  if (fd < 2) {
    return;
  }

  struct sys_file *sf = get_file (fd);

  if (sf != NULL) {
    list_remove (&sf->elem);
  }
}

static int read (int fd, void *buffer, unsigned size) {
  if (fd == 0) {
    int offset = 0;

    while (size > 0) {
      int input = input_getc ();
      memcpy (((int*) buffer + offset), &input, 4);
      size--;
    }

    return size;
  } else {
    struct sys_file *sf = get_file (fd);

    if (sf == NULL) {
      return -1;
    }

    unsigned read_size = inode_read_at (sf->f->inode, buffer, size, 0);

    if (read_size < size) {
      return -1;
    } else {
      return size;
    }
  }
}

static tid_t exec (const char* cmd_line) {
  tid_t result = process_execute (cmd_line);

  if (result != TID_ERROR) {
    return result;
  } else {
    return -1;
  }
}

static int wait (tid_t tid) {
  return process_wait (tid);
}


// --- MEMORY ACCESS


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


// --- SYSCALL HANDLER


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
    case SYS_CREATE: {
      char *file = (char*)(*((int*)kernel_addr ((int*) f->esp + 1)));
      unsigned size = *((unsigned*)kernel_addr ((int*) f->esp + 2));

      f->eax = create (file, size);
      break;
    }
    case SYS_OPEN: {
      char *file = (char*)(*((int*)kernel_addr ((int*) f->esp + 1)));

      f->eax = open (file);
      break;
    }
    case SYS_FILESIZE: {
      int fd = *((int*)kernel_addr ((int*) f->esp + 1));

      f->eax = filesize (fd);
      break;
    }
    case SYS_READ: {
      int fd = *((int*)kernel_addr ((int*) f->esp + 1));
      void *buffer = (void*)(*((int*)kernel_addr ((int*) f->esp + 2)));
      unsigned size = *((unsigned*)kernel_addr ((int*) f->esp + 3));

      f->eax = read (fd, buffer, size);
      break;
    }
    case SYS_CLOSE: {
      int fd = *((int*)kernel_addr ((int*) f->esp + 1));

      close (fd);
      break;
    }
    case SYS_EXEC: {
      char *cmd_line = (char*)(*((int*)kernel_addr ((int*) f->esp + 1)));

      f->eax = exec (cmd_line);
      break;
    }
    case SYS_WAIT: {
      int tid = *((int*)kernel_addr ((int*) f->esp + 1));

      f->eax = wait (tid);
      break;
    }
  }
}

