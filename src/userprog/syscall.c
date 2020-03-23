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
#include "devices/shutdown.h"


static void syscall_handler (struct intr_frame *);

// List of sys_file structs
// Used for keeping track of open file descriptors and the associated file
struct list fd_list;

// File lock for 
struct lock file_lock;

// Struct that associates an open file with a fd
struct sys_file {
  int fd;                   // File descriptor
  struct file *f;           // Pointer to file
  struct list_elem elem;    // List element
  struct thread *owner;     // Thread that opened the file
};

// Returns a thread's file from fd
static struct sys_file* get_file (int fd, struct thread *t) {
  struct list_elem *e;
  bool found = false;
  struct sys_file *sf;

  for (e = list_begin (&fd_list); e != list_end (&fd_list); e = list_next (e)) {
    sf = list_entry(e, struct sys_file, elem);

    if (sf->fd == fd && t == sf->owner) {
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
  lock_init (&file_lock);
}


// --- SYSCALLS


static int write (int fd, const void *buffer, unsigned size_) {
  unsigned size = 0;

  // fd 1 for console output
  if (fd == 1) {
    printf ("%s", (char*) buffer);
    size = size_;
  } else {
    struct sys_file *sf = get_file (fd, thread_current ());

    if (sf != NULL) {
      lock_acquire (&file_lock);
      size = file_write (sf->f, buffer, size_);
      lock_release (&file_lock);
    }
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
  struct sys_file *sf = get_file (fd, thread_current ());

  return sf->f->inode->data.length;
}

static bool create (const char *file, unsigned initial_size) {
  if (file == NULL) {
    exit (-1);
    return false;
  }

  lock_acquire (&file_lock);
  bool result = filesys_create(file, initial_size);
  lock_release (&file_lock);

  return result;
}

static int open (const char* file) {
  struct file *f;
  struct thread *t = thread_current ();

  if (file == NULL) {
    return -1;
  }

  lock_acquire (&file_lock);
  f = filesys_open (file);
  lock_release (&file_lock);

  if (f != NULL) {
    struct sys_file *sf = malloc (sizeof (struct sys_file));
    sf->fd = t->fd;
    sf->f = f;
    sf->owner = t;

    list_push_front (&fd_list, &sf->elem);
    return t->fd++;
  } else {
    return -1;
  }
}

static void close (int fd) {
  if (fd < 2) {
    return;
  }

  struct sys_file *sf = get_file (fd, thread_current ());

  if (sf != NULL && sf->owner == thread_current ()) {
    lock_acquire (&file_lock);
    file_close (sf->f);
    list_remove (&sf->elem);
    lock_release (&file_lock);
  }
}

static void halt (void) {
  shutdown_power_off();
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
    struct sys_file *sf = get_file (fd, thread_current ());

    if (sf == NULL) {
      return -1;
    }

    lock_acquire (&file_lock);
    unsigned read_size = file_read (sf->f, buffer, size);
    lock_release (&file_lock);

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


// Check if the address is a virtual user address and within stack space
static bool valid_address (void *addr) {
  return is_user_vaddr(addr) && (unsigned int) addr > (unsigned int) 0x08048000;
}

// Validates a syscall argument address
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

static bool remove (const char* file) {
  lock_acquire (&file_lock);
  bool result = filesys_remove (file);
  lock_release (&file_lock);

  return result;
}

static void seek (int fd, unsigned position) {
  struct sys_file *sf = get_file (fd, thread_current ());

  lock_acquire (&file_lock);
  if ((off_t) position >= 0) {
    file_seek (sf->f, position);
  }
  lock_release (&file_lock);
}

static unsigned tell (int fd) {
  struct sys_file *sf = get_file (fd, thread_current ());

  lock_acquire (&file_lock);
  unsigned result = file_tell (sf->f);
  lock_release (&file_lock);

  return result;
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
    case SYS_REMOVE: {
      char *file = (char*)(*((int*)kernel_addr ((int*) f->esp + 1)));

      f->eax = remove (file);
      break;
    }
    case SYS_HALT: {
      halt ();
      break;
    }
    case SYS_TELL: {
      int fd = *((int*)kernel_addr ((int*) f->esp + 1));

      f->eax = tell (fd);
      break;
    }
    case SYS_SEEK: {
      int fd = *((int*)kernel_addr ((int*) f->esp + 1));
      unsigned position = *((unsigned*)kernel_addr ((int*) f->esp + 2));

      seek (fd, position);
      break;
    }
  }
}

