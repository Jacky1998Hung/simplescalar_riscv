#include <stdio.h>
#include <stdlib.h>
#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "sim.h"
#include "endian.h"
#include "eio.h"
#include "syscall.h"
#include "encode.def"
#include <sys/syscall.h>
#include <stdint.h>
/* live execution only support on same-endian hosts... */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

/* #include <sys/file.h> */

#include <sys/stat.h>
#include <sys/uio.h>
#include <setjmp.h>
#include <sys/times.h>
#include <limits.h>
#include <sys/ioctl.h>

#if !defined(linux) && !defined(sparc) && !defined(hpux) && !defined(__hpux) && !defined(__CYGWIN32__) && !defined(ultrix)
#ifndef _MSC_VER
#include <sys/select.h>
#endif
#endif
#ifdef linux

#include <utime.h>
#include <sgtty.h>

#endif /* linux */

#if defined(hpux) || defined(__hpux)
#include <sgtty.h>
#endif

#ifdef __svr4__
#include "utime.h"
#endif

#if defined(sparc) && defined(__unix__)
#if defined(__svr4__) || defined(__USLC__)
#include <dirent.h>
#else
#include <sys/dir.h>
#endif


#if defined(hpux) || defined(__hpux)
#undef CR0
#endif

#ifdef __FreeBSD__
#include <termios.h>
/*#include <sys/ioctl_compat.h>*/
#else /* !__FreeBSD__ */
#ifndef _MSC_VER
#include <termio.h>
#endif
#endif

#include <sgtty.h>
#include <utime.h>
#endif

// RISCV system call for linux
#define RV_SYS_clone 220
#define RV_SYS_tgkill 131
#define RV_SYS_statx 291
#define RV_SYS_exit 93
#define RV_SYS_renameat2 276
#define RV_SYS_renameat 38
#define RV_SYS_exit_group 94
#define RV_SYS_getpid 172
#define RV_SYS_kill 129
#define RV_SYS_read 63
#define RV_SYS_write 64
#define RV_SYS_openat 56
#define RV_SYS_close 57
#define RV_SYS_lseek 62
#define RV_SYS_brk 214
#define RV_SYS_linkat 37
#define RV_SYS_unlinkat 35
#define RV_SYS_mkdirat 34
#define RV_SYS_chdir 49
#define RV_SYS_getcwd 17
#define RV_SYS_fstat 80
#define RV_SYS_fstatat 79
#define RV_SYS_faccessat 48
#define RV_SYS_pread 67
#define RV_SYS_pwrite 68
#define RV_SYS_uname 160
#define RV_SYS_getuid 174
#define RV_SYS_geteuid 175
#define RV_SYS_getgid 176
#define RV_SYS_getegid 177
#define RV_SYS_gettid 178
#define RV_SYS_sysinfo 179
#define RV_SYS_mmap 222
#define RV_SYS_munmap 215
#define RV_SYS_mremap 216
#define RV_SYS_mprotect 226
#define RV_SYS_prlimit64 261
#define RV_SYS_getmainvars 2011
#define RV_SYS_rt_sigaction 134
#define RV_SYS_writev 66
#define RV_SYS_gettimeofday 169
#define RV_SYS_times 153
#define RV_SYS_fcntl 25
#define RV_SYS_ftruncate 46
#define RV_SYS_getdents 61
#define RV_SYS_dup 23
#define RV_SYS_readlinkat 78
#define RV_SYS_rt_sigprocmask 135
#define RV_SYS_ioctl 29
#define RV_SYS_getrlimit 163
#define RV_SYS_setrlimit 164
#define RV_SYS_getrusage 165
#define RV_SYS_clock_gettime 113
#define RV_SYS_set_tid_address 96
#define RV_SYS_set_robust_list 99
#define RV_SYS_madvise 233
#define RV_SYS_futex 98
#define RV_SYS_getrandom 278
#define RV_SYS_riscv_hwprobe 258

#define OLD_SYSCALL_THRESHOLD 1024
#define RV_SYS_open 1024
#define RV_SYS_link 1025
#define RV_SYS_unlink 1026
#define RV_SYS_mkdir 1030
#define RV_SYS_access 1033
#define RV_SYS_stat 1038
#define RV_SYS_lstat 1039
#define RV_SYS_time 1062


#undef AT_FDCWD
#define AT_FDCWD -100
#define RISCV_PGSIZE MD_PAGE_SIZE

#define LINUX_O_RDONLY          00
#define LINUX_O_WRONLY          01
#define LINUX_O_RDWR            02
#define LINUX_O_CREAT           0100
#define LINUX_O_EXCL            0200
#define LINUX_O_NOCTTY          0400
#define LINUX_O_TRUNC           01000
#define LINUX_O_APPEND          02000
#define LINUX_O_NONBLOCK        04000
#define LINUX_O_SYNC            010000
#define LINUX_O_ASYNC           020000

#define SYS_GETTID 224
#define SYS_TGKILL 270
#define SYS_STATX 383
char buf[10000];
#define MAXBUFSIZE        1024
struct osf_timeval {
    sqword_t osf_tv_sec;           /* seconds */
    sqword_t osf_tv_usec;          /* microseconds */
};

struct osf_timezone {
    sqword_t osf_tz_minuteswest;   /* minutes west of Greenwich */
    sqword_t osf_tz_dsttime;       /* type of dst correction */
};


typedef struct iovec_rv {
    md_addr_t iov_base;           /* starting address */
    size_t iov_len;               /* length in bytes */
} riscv_iovec;

struct sysinfo_ {
    long uptime;             /* Seconds since boot */
    unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
    unsigned long totalram;  /* Total usable main memory size */
    unsigned long freeram;   /* Available memory size */
    unsigned long sharedram; /* Amount of shared memory */
    unsigned long bufferram; /* Memory used by buffers */
    unsigned long totalswap; /* Total swap space size */
    unsigned long freeswap;  /* Swap space still available */
    unsigned short procs;    /* Number of current processes */
    unsigned short pad;    /* Number of current processes */
    unsigned long totalhigh; /* Total high memory size */
    unsigned long freehigh;  /* Available high memory size */
    unsigned int mem_unit;   /* Memory unit size in bytes */
    char _f[20 - 2 * sizeof(long) - sizeof(int)];
/* Padding to 64 bytes */
};


typedef struct file {
    sword_t kfd;
    word_t refcnt;
} file_t;

struct {
    int linux_flag;
} linux_flag_table[] = {
        {LINUX_O_RDONLY},
        {LINUX_O_WRONLY},
        {LINUX_O_RDWR},
        {LINUX_O_APPEND},
        {LINUX_O_CREAT},
        {LINUX_O_TRUNC},
        {LINUX_O_EXCL},
        {LINUX_O_NONBLOCK},
        {LINUX_O_NOCTTY},
        {LINUX_O_SYNC},
        {LINUX_O_ASYNC},
};
#define LINUX_NFLAGS    (sizeof(linux_flag_table)/sizeof(linux_flag_table[0]))

struct linux_statbuf {
    half_t linux_st_dev;
    half_t pad0;                  /* to match Linux padding... */
    word_t linux_st_ino;
    half_t linux_st_mode;
    half_t linux_st_nlink;
    half_t linux_st_uid;
    half_t linux_st_gid;
    half_t linux_st_rdev;
    half_t pad1;
    word_t linux_st_size;
    word_t linux_st_blksize;
    word_t linux_st_blocks;
    word_t linux_st_atime;
    word_t pad2;
    word_t linux_st_mtime;
    word_t pad3;
    word_t linux_st_ctime;
    word_t pad4;
    word_t pad5;
    word_t pad6;
};
typedef sqword_t __s64;
typedef qword_t __u64;
typedef word_t __u32;
typedef half_t __u16;
struct statx_timestamp {
    __s64 tv_sec;    /* Seconds since the Epoch (UNIX time) */
    __u32 tv_nsec;   /* Nanoseconds since tv_sec */
};

struct linux_statxbuf {
    __u32 stx_mask;        /* Mask of bits indicating
                             filled fields */
    __u32 stx_blksize;     /* Block size for filesystem I/O */
    __u64 stx_attributes;  /* Extra file attribute indicators */
    __u32 stx_nlink;       /* Number of hard links */
    __u32 stx_uid;         /* User ID of owner */
    __u32 stx_gid;         /* Group ID of owner */
    __u16 stx_mode;        /* File type and mode */
    __u64 stx_ino;         /* Inode number */
    __u64 stx_size;        /* Total size in bytes */
    __u64 stx_blocks;      /* Number of 512B blocks allocated */
    __u64 stx_attributes_mask;
    /* Mask to show what's supported
       in stx_attributes */

    /* The following fields are file timestamps */
    struct statx_timestamp stx_atime;  /* Last access */
    struct statx_timestamp stx_btime;  /* Creation */
    struct statx_timestamp stx_ctime;  /* Last status change */
    struct statx_timestamp stx_mtime;  /* Last modification */

    /* If this file represents a device, then the next two
       fields contain the ID of the device */
    __u32 stx_rdev_major;  /* Major ID */
    __u32 stx_rdev_minor;  /* Minor ID */

    /* The next two fields contain the ID of the device
       containing the filesystem where the file resides */
    __u32 stx_dev_major;   /* Major ID */
    __u32 stx_dev_minor;   /* Minor ID */
};


typedef long (*syscall_t)(long, long, long, long, long, long, long);

#if 0
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, unsigned long n)
{
  const static void* syscall_table[] = {
  //  [RV_SYS_exit] = sys_exit,
  //  [RV_SYS_exit_group] = sys_exit,
  //  [RV_SYS_read] = sys_read,
  //  [RV_SYS_pread] = sys_pread,
  //  [RV_SYS_write] = sys_write,
  //  [RV_SYS_openat] = sys_openat,
  //  [RV_SYS_close] = sys_close,
  //  [RV_SYS_fstat] = sys_fstat,
  //  [RV_SYS_lseek] = sys_lseek,
  //  [RV_SYS_fstatat] = sys_fstatat,
  //  [RV_SYS_linkat] = sys_linkat,
  //  [RV_SYS_unlinkat] = sys_unlinkat,
  //  [RV_SYS_mkdirat] = sys_mkdirat,
  //  [RV_SYS_renameat] = sys_renameat,
  //  [RV_SYS_getcwd] = sys_getcwd,
  //  [RV_SYS_brk] = sys_brk,
  //  [SYS_uname] = sys_uname,
  //  [SYS_getpid] = sys_getpid,
  //  [SYS_getuid] = sys_getuid,
  //  [SYS_geteuid] = sys_getuid,
  //  [SYS_getgid] = sys_getuid,
  //  [SYS_getegid] = sys_getuid,
  //  [RV_SYS_mmap] = sys_mmap,
  //  [RV_SYS_munmap] = sys_munmap,
  //  [RV_SYS_mremap] = sys_mremap,
  //  [RV_SYS_mprotect] = sys_mprotect,
  //  [RV_SYS_prlimit64] = sys_stub_nosys,
  //  [RV_SYS_rt_sigaction] = sys_rt_sigaction,
  //  [RV_SYS_gettimeofday] = sys_gettimeofday,
  //  [RV_SYS_times] = sys_times,
  //  [RV_SYS_writev] = sys_writev,
  //  [RV_SYS_faccessat] = sys_faccessat,
  //  [RV_SYS_fcntl] = sys_fcntl,
  //  [RV_SYS_ftruncate] = sys_ftruncate,
  //  [RV_SYS_getdents] = sys_getdents,
  //  [RV_SYS_dup] = sys_dup,
  //  [RV_SYS_readlinkat] = sys_stub_nosys,
  //  [RV_SYS_rt_sigprocmask] = sys_stub_success,
  //  [RV_SYS_ioctl] = sys_stub_nosys,
  //  [RV_SYS_clock_gettime] = sys_clock_gettime,
  //  [RV_SYS_getrusage] = sys_stub_nosys,
  //  [RV_SYS_getrlimit] = sys_stub_nosys,
  //  [RV_SYS_setrlimit] = sys_stub_nosys,
  //  [RV_SYS_chdir] = sys_chdir,
  //  [RV_SYS_set_tid_address] = sys_stub_nosys,
  //  [RV_SYS_set_robust_list] = sys_stub_nosys,
  //  [RV_SYS_madvise] = sys_stub_nosys,
  };

const static void* old_syscall_table[] = {
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_open] = sys_open,
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_link] = sys_link,
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_unlink] = sys_unlink,
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_mkdir] = sys_mkdir,
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_access] = sys_access,
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_stat] = sys_stat,
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_lstat] = sys_lstat,
  //  [-OLD_SYSCALL_THRESHOLD + RV_SYS_time] = sys_time,
  };
  syscall_t f = 0;
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof(arr[0]))
  if (n < ARRAY_SIZE(syscall_table))
    f = syscall_table[n];
  else if (n - OLD_SYSCALL_THRESHOLD < ARRAY_SIZE(old_syscall_table))
    f = old_syscall_table[n - OLD_SYSCALL_THRESHOLD];

  if (!f)
    panic("bad syscall #%ld!",n);

  return f(a0, a1, a2, a3, a4, a5, n);
}

#endif


#define __NEW_UTS_LEN 64
struct new_utsname {
    char sysname[__NEW_UTS_LEN + 1];
    char nodename[__NEW_UTS_LEN + 1];
    char release[__NEW_UTS_LEN + 1];
    char version[__NEW_UTS_LEN + 1];
    char machine[__NEW_UTS_LEN + 1];
    char domainname[__NEW_UTS_LEN + 1];
};

/* open(2) flags for SimpleScalar target, syscall.c automagically maps
   between these codes to/from host open(2) flags */
#define SS_O_RDONLY        0
#define SS_O_WRONLY        1
#define SS_O_RDWR        2
#define SS_O_APPEND        0x0008
#define SS_O_CREAT        0x0200
#define SS_O_TRUNC        0x0400
#define SS_O_EXCL        0x0800
#define SS_O_NONBLOCK        0x4000
#define SS_O_NOCTTY        0x8000
#define SS_O_SYNC        0x2000

/* open(2) flags translation table for SimpleScalar target */
struct {
    int ss_flag;
    int local_flag;
} ss_flag_table[] = {
        /* target flag */    /* host flag */
/* !_MSC_VER */
        {SS_O_RDONLY, O_RDONLY},
        {SS_O_WRONLY, O_WRONLY},
        {SS_O_RDWR, O_RDWR},
        {SS_O_APPEND, O_APPEND},
        {SS_O_CREAT, O_CREAT},
        {SS_O_TRUNC, O_TRUNC},
        {SS_O_EXCL, O_EXCL},
        {SS_O_NONBLOCK, O_NONBLOCK},
        {SS_O_NOCTTY, O_NOCTTY},
#ifdef O_SYNC
        {SS_O_SYNC, O_SYNC},
#endif
};
#define SS_NFLAGS    (sizeof(ss_flag_table)/sizeof(ss_flag_table[0]))


struct task_struct {
    int tgid;
    int pid;
    struct regs_t *regs;
    struct mem_t *mem;
    struct task_struct *parent;
    struct task_struct *son;
    struct task_struct *next;
};

struct task_struct *create_task_struct(int pid, int tgid, struct regs_t *regs, struct mem_t *mem) {
    struct task_struct *new_struct = (struct task_struct *) malloc(sizeof(struct task_struct));
    new_struct->mem = (struct mem_t *) malloc(sizeof(struct mem_t));
    new_struct->regs = (struct regs_t *) malloc(sizeof(struct regs_t));
    new_struct->parent = NULL;
    new_struct->son = NULL;
    new_struct->next = NULL;

    return new_struct;
}

void copy_mem(struct mem_t *from, struct mem_t *to) {

}

void free_task_struct(int pid) {

}

// Partial implementation on riscv_hwprobe from Linux
// See: https://www.kernel.org/doc/html/latest/arch/riscv/hwprobe.html

#define RISCV_HWPROBE_KEY_IMA_EXT_0 4
#define     RISCV_HWPROBE_IMA_FD        (1 << 0)
#define     RISCV_HWPROBE_IMA_C         (1 << 1)
#define     RISCV_HWPROBE_IMA_V         (1 << 2)
#define     RISCV_HWPROBE_EXT_ZBA       (1 << 3)
#define     RISCV_HWPROBE_EXT_ZBB       (1 << 4)
#define     RISCV_HWPROBE_EXT_ZBS       (1 << 5)
#define     RISCV_HWPROBE_EXT_ZICBOZ    (1 << 6)

struct riscv_hwprobe {
    int64_t  key;
    uint64_t value;
};

/* syscall proxy handler, architect registers and memory are assumed to be
   precise when this function is called, register and memory are updated with
   the results of the sustem call */
void
sys_syscall(struct regs_t *regs,    /* registers to access */
            mem_access_fn mem_fn,    /* generic memory accessor */
            struct mem_t *mem,        /* memory space to access */
            md_inst_t inst,        /* system call inst */
            int traceable)        /* traceable system call? */
{
    debug("syscall: %d", regs->regs_R[17]);
    word_t syscode = regs->regs_R[17];
    static struct task_struct *task_list;
    qword_t _a0 = regs->regs_R[MD_REG_A0];
    qword_t _a1 = regs->regs_R[MD_REG_A1];
    qword_t _a2 = regs->regs_R[MD_REG_A2];
    qword_t _a3 = regs->regs_R[MD_REG_A3];
    qword_t _a4 = regs->regs_R[MD_REG_A4];
    qword_t _a5 = regs->regs_R[MD_REG_A5];
    if (syscode == RV_SYS_exit || syscode == RV_SYS_exit_group) {

        
        longjmp(sim_exit_buf, (regs->regs_R[MD_REG_A0] & 0xff) + 1);
        return;
    }
    static md_addr_t mmap_brk_point = 0xd0000000;
    switch (syscode) {
        case RV_SYS_brk: {
            md_addr_t addr = regs->regs_R[MD_REG_A0];
            if (addr != 0) {
                ld_brk_point = addr;
                //regs->regs_R[MD_REG_A0] = 0;
                regs->regs_R[MD_REG_A0] = ld_brk_point;

                /* check whether heap area has merged with stack area */
                if (addr >= regs->regs_R[MD_REG_SP]) {
                    /* out of address space, indicate error */
                    fatal("out of address space");
                    regs->regs_R[MD_REG_A0] = -ENOMEM;
                }
            } else /* just return break point */
                regs->regs_R[MD_REG_A0] = ld_brk_point;

            if (verbose) {
                myfprintf(stderr, "brk(0x%08p) = 0x%08p\n", addr, ld_brk_point);
            }
        }
            break;
        case RV_SYS_mmap: {

            //if (/* flags */regs->regs_R[MD_REG_A3] !=
            //    0x22 /* (MAP_PRIVATE|MAP_ANONYMOUS) */) {
            //  fprintf(stderr, "mmap flag:%d",regs->regs_R[MD_REG_A3]);
            //  fatal("non-anonymous MMAP's not yet implemented");
            // }

            regs->regs_R[MD_REG_A0] = mmap_brk_point;
            mmap_brk_point += regs->regs_R[MD_REG_A1];

            if (verbose)
                myfprintf(stderr, "MMAP: 0x%08x -> 0x%08x, %d bytes...\n",
                          regs->regs_R[MD_REG_A0], mmap_brk_point,
                          regs->regs_R[MD_REG_A1]);

        }
            break;
        case RV_SYS_mremap: {
            unsigned long int addr = regs->regs_R[MD_REG_A0];
            uint32_t old_length = regs->regs_R[MD_REG_A1];
            uint32_t new_length = regs->regs_R[MD_REG_A2];
            uint32_t flags = regs->regs_R[MD_REG_A3];

            if (new_length > old_length) {
                if ((addr + old_length) == mmap_brk_point) {
                    unsigned int diff = new_length - old_length;
                    mmap_brk_point += diff;
                    regs->regs_R[MD_REG_A0] = 0;
                } else {
//			if (!(flags & 1)) {
                    char *buf;
                    buf = (char *) calloc(old_length, sizeof(char));
                    mem_bcopy(mem_fn, mem, Read,
                            /*buf*/addr, buf, /*nread*/old_length);
                    mem_bcopy(mem_fn, mem, Write,
                            /*buf*/mmap_brk_point, buf, /*nread*/old_length);
                    regs->regs_R[MD_REG_A0] = mmap_brk_point;
                    mmap_brk_point += new_length;
                    //warn("can't remap here and MREMAP_MAYMOVE flag not set\n");
                    free(buf);
                }
            } else {
                regs->regs_R[MD_REG_A0] = regs->regs_R[MD_REG_A0];
            }
//FIXME	
            if (verbose)
                myfprintf(stderr, "MREMAP: 0x%08x -> 0x%08x, %d bytes...\n",
                          regs->regs_R[MD_REG_A0], mmap_brk_point,
                          regs->regs_R[MD_REG_A1]);

        }
            break;
        case RV_SYS_link: {
            char old_path[MAXBUFSIZE];
            char new_path[MAXBUFSIZE];
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], old_path);
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A1], new_path);
            regs->regs_R[MD_REG_A0] = link(old_path, new_path);

        }
            break;
        case RV_SYS_unlinkat: {
            char pathname[MAXBUFSIZE];
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A1], pathname);
            regs->regs_R[MD_REG_A0] = unlinkat(regs->regs_R[MD_REG_A0], pathname, regs->regs_R[MD_REG_A2]);


        }
            break;
        case RV_SYS_unlink: {
            char buf[MAXBUFSIZE];
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);
            regs->regs_R[MD_REG_A0] = unlink(buf);
        }
            break;

        case RV_SYS_munmap: {
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_mprotect: {
            // panic("invalid/unimplemented system call encountered, code %d", syscode);
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_open : {
            char buf[MAXBUFSIZE];
            int error_return;
            int linux_flags = regs->regs_R[MD_REG_A1], local_flags = 0;

            /* copy filename to host memory */
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);
            error_return = open(buf, linux_flags, /*mode*/regs->regs_R[MD_REG_A2]);
            fprintf(stderr, " nomarl: %lld\n", regs->regs_R[MD_REG_A1]);
            if (error_return == -1) {
                fprintf(stderr, " openerrpr: %lld\n", regs->regs_R[MD_REG_A1]);
            }

            // create
            if (error_return == -1 && regs->regs_R[MD_REG_A1] == 1537) {

                creat(buf, regs->regs_R[MD_REG_A2]);
                error_return = open(buf, linux_flags, /*mode*/regs->regs_R[MD_REG_A2]);
            }
            /* open the file */

            /* check for an error condition */
            if (error_return != -1) {
                regs->regs_R[MD_REG_A0] = error_return;
            } /* no error */
            else {
                regs->regs_R[MD_REG_A0] = -(errno); /* negative of the error number is returned in r0 */
            }

        }
            break;
        case RV_SYS_openat: {
            char buf[MAXBUFSIZE];
            int error_return;
            int linux_flags = regs->regs_R[MD_REG_A2], local_flags = 0;

            /* copy filename to host memory */
            //if(_a1 != 0){
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A1], buf);

            /* open the file */
            /*fd*/error_return =
                          openat(regs->regs_R[MD_REG_A0], buf, linux_flags, /*mode*/regs->regs_R[MD_REG_A3]);
            //} else {error_return = -1;}

            /* check for an error condition */
            //if (error_return != -1) {
            regs->regs_R[MD_REG_A0] = error_return;
            //} /* no error */
            //else {
            //      regs->regs_R[MD_REG_A0] = -1; /* negative of the error number is returned in r0 */
            //}

            if (verbose) {
                myfprintf(stderr, "openat(%d, %d, %d) = %d \n", _a0, _a1, _a2, regs->regs_R[MD_REG_A0]);
            }
        }
            break;

// case RV_SYS_write:
// {
//     char *buf;
//     int error_return;
//     /* allocate same-sized output buffer in host memory */
//     if (!(buf = (char *) calloc(/*nbytes*/regs->regs_R[MD_REG_A2], sizeof(char))))
//         fatal("out of memory in RV_SYS_write");

//     /* copy inputs into host memory */
//     mem_bcopy(mem_fn, mem, Read, /*buf*/regs->regs_R[MD_REG_A1], buf,
//               /*nbytes*/regs->regs_R[MD_REG_A2]);

//     // print debug info
//     if (verbose) {
//         printf("syscall_write: fd=%ld, buf=%p, count=%ld\n", 
//                regs->regs_R[MD_REG_A0], buf, regs->regs_R[MD_REG_A2]);
//         printf("Memory content before write: ");
//         for (int i = 0; i < regs->regs_R[MD_REG_A2]; i++) {
//             printf("%02x ", (unsigned char)buf[i]);
//         }
//         printf("\n");

//         // print floating point value
//         double *double_buf = (double *)buf;
//         for (int i = 0; i < regs->regs_R[MD_REG_A2] / sizeof(double); i++) {
//             printf("Double value at buf[%d]: %f\n", i, double_buf[i]);
//         }
//     }

//     /* write() the data */
//     error_return = write(/*fd*/regs->regs_R[MD_REG_A0], buf,
//                          /*nbytes*/regs->regs_R[MD_REG_A2]);

//     if (error_return == -1)
//         /* got an error, return details */
//         regs->regs_R[MD_REG_A0] = -errno;
//     else
//         /* no error, return bytes written */
//         regs->regs_R[MD_REG_A0] = error_return;

//     /* free the host memory buffer */
//     free(buf);

//     /* simulate a register writeback */
//     mem_bcopy(mem_fn, mem, Write, /*buf*/regs->regs_R[MD_REG_A1], buf,
//               /*nbytes*/regs->regs_R[MD_REG_A2]);

//     if (verbose) {
//         printf("Memory content after write: ");
//         for (int i = 0; i < regs->regs_R[MD_REG_A2]; i++) {
//             printf("%02x ", (unsigned char)buf[i]);
//         }
//         printf("\n");

//         // print flaoting point value
//         double *double_buf = (double *)buf;
//         for (int i = 0; i < regs->regs_R[MD_REG_A2] / sizeof(double); i++) {
//             printf("Double value at buf[%d] after write: %f\n", i, double_buf[i]);
//         }
//     }
// }
// break;

        case RV_SYS_write: {
            char *buf;
            int error_return;
            /* allocate same-sized output buffer in host memory */
            if (!(buf =
                          (char *) calloc(/*nbytes*/regs->regs_R[MD_REG_A2], sizeof(char))))
                fatal("out of memory in RV_SYS_write");

            /* copy inputs into host memory */
            mem_bcopy(mem_fn, mem, Read, /*buf*/regs->regs_R[MD_REG_A1], buf,
                    /*nbytes*/regs->regs_R[MD_REG_A2]);
	    if (verbose) {
		printf("%s", buf);
	    }
            int len = write(/*fd*/regs->regs_R[MD_REG_A0],
                                  buf, /*nbytes*/regs->regs_R[MD_REG_A2]);
            regs->regs_R[MD_REG_A0] = len;
            /* done with output buffer */
            free(buf);
        }
            break;
            
        case RV_SYS_close: {
            int error_return;
            /* don't close stdin, stdout, or stderr as this messes up sim logs */
            if (/*fd*/regs->regs_R[MD_REG_A0] == 0
                      || /*fd*/regs->regs_R[MD_REG_A0] == 1
                      || /*fd*/regs->regs_R[MD_REG_A0] == 2) {
                regs->regs_R[MD_REG_A0] = 0;
                if (verbose) {
                    myfprintf(stderr, "close(%d) = %d\n", _a0, regs->regs_R[MD_REG_A0]);
                }
                break;
            }

            /* close the file */
            error_return = close(/*fd*/regs->regs_R[MD_REG_A0]);

            /* check for an error condition */
            if (error_return != -1) {
                regs->regs_R[MD_REG_A0] = error_return;
            } /* no error */
            else {
                regs->regs_R[MD_REG_A0] = -(errno); /* negative of the error number is returned in r0 */
            }

            if (verbose) {
                myfprintf(stderr, "close(%d) = %d\n", _a0, regs->regs_R[MD_REG_A0]);
            }
        }
            break;
        case RV_SYS_fstat: {
            struct linux_statbuf linux_sbuf;
            struct stat sbuf;
            /* fstat() the file */
            /*result*/regs->regs_R[MD_REG_A0] =
                              fstat(/*fd*/regs->regs_R[MD_REG_A0], &sbuf);

            /* check for an error condition */
            if (regs->regs_R[MD_REG_A0] == (qword_t) -1)
                /* got an error, return details */
                regs->regs_R[MD_REG_A0] = -errno;

            /* Debug: print the original stat structure */
            // fprintf(stderr, "sbuf: dev=%ld\n"
            //                 "ino=%ld\n"
            //                 "mode=%o\n"
            //                 "nlink=%ld\n"
            //                 "uid=%d\n"
            //                 "gid=%d\n"
            //                 "rdev=%ld\n"
            //                 "size=%ld\n"
            //                 "blksize=%ld\n"
            //                 "blocks=%ld\n"
            //                 "atime=%ld\n"
            //                 "mtime=%ld\n"
            //                 "ctime=%ld\n",
            // sbuf.st_dev, sbuf.st_ino, sbuf.st_mode, sbuf.st_nlink, sbuf.st_uid,
            // sbuf.st_gid, sbuf.st_rdev, sbuf.st_size, sbuf.st_blksize,
            // sbuf.st_blocks, sbuf.st_atime, sbuf.st_mtime, sbuf.st_ctime);

            /* translate the stat structure to host format */
            linux_sbuf.linux_st_dev = MD_SWAPH(sbuf.st_dev);
            linux_sbuf.linux_st_ino = MD_SWAPW(sbuf.st_ino);
            linux_sbuf.linux_st_mode = MD_SWAPH(sbuf.st_mode);
            linux_sbuf.linux_st_nlink = MD_SWAPH(sbuf.st_nlink);
            linux_sbuf.linux_st_uid = MD_SWAPH(sbuf.st_uid);
            linux_sbuf.linux_st_gid = MD_SWAPH(sbuf.st_gid);
            linux_sbuf.linux_st_rdev = MD_SWAPH(sbuf.st_rdev);
            linux_sbuf.linux_st_size = MD_SWAPW(sbuf.st_size);
            linux_sbuf.linux_st_blksize = MD_SWAPW(sbuf.st_blksize);
            linux_sbuf.linux_st_blocks = MD_SWAPW(sbuf.st_blocks);
            linux_sbuf.linux_st_atime = MD_SWAPW(sbuf.st_atime);
            linux_sbuf.linux_st_mtime = MD_SWAPW(sbuf.st_mtime);
            linux_sbuf.linux_st_ctime = MD_SWAPW(sbuf.st_ctime);

            /* Debug: print the translated linux_statbuf structure */
            // fprintf(stderr, "\nlinux_sbuf: dev=%ld\n"
            // "ino=%ld\n"
            // "mode=%o\n"
            // "nlink=%ld\n"
            //  "uid=%d\n"
            //   "gid=%d\n"
            //    "rdev=%ld\n"
            //     "size=%ld\n"
            //     "blksize=%ld\n"
            //     "blocks=%ld\n"
            //     "atime=%ld\n"
            //     " mtime=%ld\n"
            //      "ctime=%ld\n\n",
            // linux_sbuf.linux_st_dev, linux_sbuf.linux_st_ino, linux_sbuf.linux_st_mode,
            // linux_sbuf.linux_st_nlink, linux_sbuf.linux_st_uid, linux_sbuf.linux_st_gid,
            // linux_sbuf.linux_st_rdev, linux_sbuf.linux_st_size, linux_sbuf.linux_st_blksize,
            // linux_sbuf.linux_st_blocks, linux_sbuf.linux_st_atime, linux_sbuf.linux_st_mtime,
            // linux_sbuf.linux_st_ctime);

            /* copy fstat() results to simulator memory */
            mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[MD_REG_A1],
                      &linux_sbuf, sizeof(struct linux_statbuf));

        }
            break;
        case RV_SYS_read: {
            char *buf;
            int len;
            if (verbose) {
                myfprintf(stderr, "read(%d,0x%08p,%d)", _a0, _a1, _a2);
            }
            if (!(buf =
                          (char *) calloc(/*nbytes*/_a2, sizeof(char)))) {
                fatal("out of memory in RV_SYS_read");
            }
            len = read(/*fd*/regs->regs_R[MD_REG_A0], buf, /*nbytes*/_a2);

            if (len > 0) {
                mem_bcopy(mem_fn, mem, Write,
                        /*buf*/regs->regs_R[MD_REG_A1], buf, len);
            }
            free(buf);

            regs->regs_R[MD_REG_A0] = len;
            if (verbose) {
                myfprintf(stderr, "%d = read(%d,0x%08p,%d)", regs->regs_R[MD_REG_A0],_a0, _a1, _a2);
            }
        }
            break;
        case RV_SYS_exit: {
            /* exit jumps to the target set in main() */
            longjmp(sim_exit_buf,
                    /* exitcode + fudge */(regs->regs_R[MD_REG_A0] & 0xff) + 1);
            break;
        }
        case RV_SYS_lseek : {
            //panic("invalid/unimplemented system call encountered, code %d", syscode);
            /* seek into file */
            regs->regs_R[MD_REG_A0] =
                    lseek(/*fd*/regs->regs_R[MD_REG_A0],
                            /*off*/regs->regs_R[MD_REG_A1], /*dir*/regs->regs_R[MD_REG_A2]);

            /* check for an error condition */
            if (regs->regs_R[MD_REG_A0] == (qword_t) -1)
                /* got an error, return details */
                regs->regs_R[MD_REG_A0] = -errno;

        }
            break;
        case RV_SYS_futex: {
            myfprintf(stderr, "============FUTEX==========\n");
            int tmp, tmp2, tmp3;
            qword_t result;
            mem_bcopy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], &tmp, sizeof(int));
            mem_bcopy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A2], &tmp2, sizeof(int));
            mem_bcopy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A4], &tmp3, sizeof(int));
            myfprintf(stderr, "futex A0: %x\n", regs->regs_R[MD_REG_A0]);
            myfprintf(stderr, "futex A1: %x\n", regs->regs_R[MD_REG_A1]);
            myfprintf(stderr, "futex A2: %x\n", regs->regs_R[MD_REG_A2]);
            myfprintf(stderr, "futex A3: %x\n", regs->regs_R[MD_REG_A3]);
            myfprintf(stderr, "futex A4: %x\n", regs->regs_R[MD_REG_A4]);
            myfprintf(stderr, "futex A5: %x\n", regs->regs_R[MD_REG_A5]);
            if ((regs->regs_R[MD_REG_A0] % 4) != 0) {
                myfprintf(stderr, "futex address is not aligned: %x\n", regs->regs_R[MD_REG_A0]);
            } else {
                myfprintf(stderr, "futex address is aligned: %x\n", regs->regs_R[MD_REG_A0]);
            }
            result = syscall(240, &tmp/*A0*/, regs->regs_R[MD_REG_A1], &tmp2/*A2*/, regs->regs_R[MD_REG_A3], &tmp3/*A4*/,
                             regs->regs_R[MD_REG_A5]);
            myfprintf(stderr, "futex syscall result: %ld, errno: %d\n", result, errno);
            mem_bcopy(mem_fn, mem, Write, /*fname*/regs->regs_R[MD_REG_A0], &tmp, sizeof(int));
            regs->regs_R[MD_REG_A0] = result;
            myfprintf(stderr, "============END FUTEX==========\n");
        }
            break;
        case RV_SYS_gettimeofday: {
            struct osf_timeval osf_tv;
            struct timeval tv, *tvp;
            struct osf_timezone osf_tz;
            struct timezone tz, *tzp;

            if (/*timeval*/regs->regs_R[MD_REG_A0] != 0) {
                /* copy timeval into host memory */
                mem_bcopy(mem_fn, mem, Read, /*timeval*/regs->regs_R[MD_REG_A0],
                          &osf_tv, sizeof(struct osf_timeval));

                /* convert target timeval structure to host format */
                tv.tv_sec = MD_SWAPW(osf_tv.osf_tv_sec);
                tv.tv_usec = MD_SWAPW(osf_tv.osf_tv_usec);
                tvp = &tv;
            } else
                tvp = NULL;

            if (/*timezone*/regs->regs_R[MD_REG_A1] != 0) {
                /* copy timezone into host memory */
                mem_bcopy(mem_fn, mem, Read, /*timezone*/regs->regs_R[MD_REG_A1],
                          &osf_tz, sizeof(struct osf_timezone));

                /* convert target timezone structure to host format */
                tz.tz_minuteswest = MD_SWAPW(osf_tz.osf_tz_minuteswest);
                tz.tz_dsttime = MD_SWAPW(osf_tz.osf_tz_dsttime);
                tzp = &tz;
            } else
                tzp = NULL;

            /* get time of day */
            /*result*/regs->regs_R[MD_REG_A0] = gettimeofday(tvp, tzp);

            /* check for an error condition */
            if (regs->regs_R[MD_REG_A0] == (qword_t) -1)
                /* got an error, return details */
                regs->regs_R[MD_REG_A0] = -errno;

            if (/*timeval*/regs->regs_R[MD_REG_A0] != 0) {
                /* convert host timeval structure to target format */
                osf_tv.osf_tv_sec = MD_SWAPW(tv.tv_sec);
                osf_tv.osf_tv_usec = MD_SWAPW(tv.tv_usec);

                /* copy timeval to target memory */
                mem_bcopy(mem_fn, mem, Write, /*timeval*/regs->regs_R[MD_REG_A0],
                          &osf_tv, sizeof(struct osf_timeval));
            }

            if (/*timezone*/regs->regs_R[MD_REG_A1] != 0) {
                /* convert host timezone structure to target format */
                osf_tz.osf_tz_minuteswest = MD_SWAPW(tz.tz_minuteswest);
                osf_tz.osf_tz_dsttime = MD_SWAPW(tz.tz_dsttime);

                /* copy timezone to target memory */
                mem_bcopy(mem_fn, mem, Write, /*timezone*/regs->regs_R[MD_REG_A1],
                          &osf_tz, sizeof(struct osf_timezone));
            }


        }
            break;
        case RV_SYS_rt_sigaction : {
            struct sigaction act;
            struct sigaction oldact;
            int sig = regs->regs_R[MD_REG_A0];
            mem_bcopy(mem_fn, mem, Read, /*act*/regs->regs_R[MD_REG_A1],
                      &act, sizeof(struct sigaction));
            mem_bcopy(mem_fn, mem, Read, /*act*/regs->regs_R[MD_REG_A2],
                      &oldact, sizeof(struct sigaction));
            regs->regs_R[MD_REG_A0] = sigaction(regs->regs_R[MD_REG_A0], &act, &oldact);
            if (verbose) {
                // FIXME change sig from interger to string
                myfprintf(stderr, "rt_sigaction(%d,0x%08p,0x%08p) = %d\n", sig, regs->regs_R[MD_REG_A1],
                          regs->regs_R[MD_REG_A2], regs->regs_R[MD_REG_A0]);
            }
        };
            break;
        case RV_SYS_rt_sigprocmask: {
            sigset_t set;
            sigset_t oldset;
            mem_bcopy(mem_fn, mem, Read, /*act*/regs->regs_R[MD_REG_A1],
                      &set, sizeof(sigset_t));
            mem_bcopy(mem_fn, mem, Read, /*act*/regs->regs_R[MD_REG_A2],
                      &oldset, sizeof(sigset_t));
            regs->regs_R[MD_REG_A0] = sigprocmask(regs->regs_R[MD_REG_A0], &set, &oldset);
            mem_bcopy(mem_fn, mem, Write, /*act*/regs->regs_R[MD_REG_A1],
                      &set, sizeof(sigset_t));
            mem_bcopy(mem_fn, mem, Write, /*act*/regs->regs_R[MD_REG_A2],
                      &set, sizeof(sigset_t));
        }
            break;

        case RV_SYS_getuid: {
            regs->regs_R[MD_REG_A0] = 1000;
        }
            break;
        case RV_SYS_geteuid: {
            regs->regs_R[MD_REG_A0] = 1000;
        }
            break;
        case RV_SYS_gettid: {
            regs->regs_R[MD_REG_A0] = syscall(SYS_GETTID);
        }
            break;
        case RV_SYS_getgid: {
            regs->regs_R[MD_REG_A0] = getgid();
        }
            break;
        case RV_SYS_getegid: {
            regs->regs_R[MD_REG_A0] = 1000;
        }
            break;
        case RV_SYS_getpid: {
            regs->regs_R[MD_REG_A0] = getpid();
        }
            break;
        case RV_SYS_uname: {
            struct new_utsname _uname;
            qword_t addr = regs->regs_R[MD_REG_A0];
            strcpy(_uname.sysname, "Proxy Kernel");             // modeling linux OS
            strcpy(_uname.nodename, "");
            strcpy(_uname.release, "4.15.0");
            strcpy(_uname.version, "");
            strcpy(_uname.machine, "");
            strcpy(_uname.domainname, "");
            mem_bcopy(mem_fn, mem, Write,
                    /*buf*/addr, &_uname, /*nread*/sizeof(_uname));
            regs->regs_R[MD_REG_A0] = 0;
            if (verbose) {
                myfprintf(stderr, "uname(0x%08p) = %d\n", addr, 0);
            }
        }
            break;
        case RV_SYS_tgkill: {
            regs->regs_R[MD_REG_A0] = syscall(SYS_TGKILL, regs->regs_R[MD_REG_A0], regs->regs_R[MD_REG_A1],
                                              regs->regs_R[MD_REG_A2]);
        }
            break;
        case RV_SYS_writev: {
            qword_t fd = regs->regs_R[MD_REG_A0];
            md_addr_t iovptr = regs->regs_R[MD_REG_A1];
            qword_t iovcnt = regs->regs_R[MD_REG_A2];
            struct iovec *iov;
            riscv_iovec a_iov;
            int i, res;
            if (fd < 0)
                regs->regs_R[MD_REG_A0] = EBADF;
            else {
                iov = malloc(/* len */ sizeof(struct iovec) * iovcnt);
                for (i = 0; i < iovcnt; i++) {
                    // read a_iov out of simulator memory
                    mem_bcopy(mem_fn, mem, Read,
                            /*buf*/iovptr + i * sizeof(a_iov), &a_iov, /*nread*/sizeof(a_iov));
                    // copy into local iov
                    iov[i].iov_len = a_iov.iov_len;
                    // allocate local buffer
                    iov[i].iov_base = malloc(/* len */ sizeof(char) * (iov[i].iov_len));
                    mem_bcopy(mem_fn, mem, Read,
                            /*buf*/a_iov.iov_base, iov[i].iov_base, /*nread*/a_iov.iov_len);
                }
                // perform writev
                res = writev(fd, iov, iovcnt);

                if (res < 0)
                    regs->regs_R[MD_REG_A0] = errno;
                else {
                    regs->regs_R[MD_REG_A0] = 0;
                }

                // cleanup
                for (i = 0; i < iovcnt; i++)
                    free(iov[i].iov_base);
                free(iov);
            }

        }
            break;
        case RV_SYS_readlinkat: {
            int dirfd = regs->regs_R[MD_REG_A0];
            char *buf;
            size_t bufsize = regs->regs_R[MD_REG_A3];
            char pathname[MAXBUFSIZE];
            if (!(buf =
                          (char *) calloc(/*nbytes*/regs->regs_R[MD_REG_A3], sizeof(char)))) {
                fatal("out of memory in RV_SYS_read");
            }
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A1], pathname);
            regs->regs_R[MD_REG_A0] = readlinkat(dirfd, pathname, buf, bufsize);
            mem_bcopy(mem_fn, mem, Write,
                    /*buf*/regs->regs_R[MD_REG_A2], buf, /*nread*/regs->regs_R[MD_REG_A0]);
            if (verbose) {
                char dirfd_[20] = "unknown\0";
                if (dirfd == -100)
                    strcpy(dirfd_, "AT_FDCWD\0");
                myfprintf(stderr, "readlinkat(%s,\"%s\",0x%08p,%d) = %d\n", dirfd_, pathname, buf, bufsize,
                          regs->regs_R[MD_REG_A0]);
            }
            free(buf);


        }
            break;
        case RV_SYS_prlimit64: { //261
            if (verbose) {
                myfprintf(stderr, "prlimit64(%d,%d) = %d\n", regs->regs_R[MD_REG_A0], regs->regs_R[MD_REG_A1], 0);
            }
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_clock_gettime: {
            struct timespec buf[0];
            clock_gettime(regs->regs_R[MD_REG_A0], buf);
            mem_bcopy(mem_fn, mem, Write,
                    /*buf*/regs->regs_R[MD_REG_A1], buf, sizeof(struct timespec));
            if (verbose) {
                myfprintf(stderr, "clock_gettime(%d, %d) = %d\n", regs->regs_R[MD_REG_A0], regs->regs_R[MD_REG_A1], 0);
            }
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_getrlimit: { //163
            if (verbose) {
                myfprintf(stderr, "getrlimit(%d,%d) = %d\n", regs->regs_R[MD_REG_A0], regs->regs_R[MD_REG_A1], 0);
            }
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_sysinfo: {// 179
            struct sysinfo_ *info = malloc(sizeof(struct sysinfo_));
            sysinfo(&info);
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;

        case RV_SYS_getcwd: {
            char *buf, *res;
            md_addr_t vaddr = regs->regs_R[MD_REG_A0];
            qword_t size = regs->regs_R[MD_REG_A1];
            buf = (char *) calloc(size, sizeof(char));
            res = getcwd(buf, size);
            if (res < 0) regs->regs_R[MD_REG_A0] = -1;
            else {
                regs->regs_R[MD_REG_A0] = strlen(res);
                mem_strcpy(mem_fn, mem, Write, vaddr, buf);
            }
            free(buf);
        }
            break;
        case RV_SYS_fcntl :
        case RV_SYS_ioctl : {
            if (_a0 == 0)
                regs->regs_R[MD_REG_A0] = -1;
            if (_a0 == 2)
                regs->regs_R[MD_REG_A0] = 0;
            //regs->regs_R[MD_REG_A0] = ioctl(_a0, _a1, _a2,_a3,_a4);
            if (verbose) {
                myfprintf(stderr, "ioctl(%d,%d,%d,%d,%d) = %d\n", _a0, _a1, _a2, _a3, _a4, regs->regs_R[MD_REG_A0]);
            }
        }
            break;
        case RV_SYS_statx: {
            char path_[MAXBUFSIZE];
            struct linux_statxbuf sbuf;
            regs->regs_R[MD_REG_A0] = -38;
            if (verbose) {
                myfprintf(stderr, "statx(%d,%d,%d,%d,%d) = %d\n", _a0, _a1, _a2, _a3, _a4, regs->regs_R[MD_REG_A0]);
            }
            //regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_times: {
            struct tms buf[0];
            regs->regs_R[MD_REG_A0] = times(buf);
            mem_bcopy(mem_fn, mem, Write, _a0, buf, sizeof(struct tms));
            if (verbose) {
                myfprintf(stderr, "times(%d)= %d\n", _a0, regs->regs_R[MD_REG_A0]);
            }


        }
            break;
        case RV_SYS_chdir: {
            char pathname[MAXBUFSIZE];
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], pathname);
            regs->regs_R[MD_REG_A0] = chdir(pathname);
            if (verbose) {
                myfprintf(stderr, "chdir(%d)= %d\n", _a0, regs->regs_R[MD_REG_A0]);
            }
        }
            break;
        case RV_SYS_ftruncate: {
            regs->regs_R[MD_REG_A0] = ftruncate(_a0, _a1);
        }
            break;
        case RV_SYS_renameat2:
        case RV_SYS_renameat: {
            char old_path[MAXBUFSIZE];
            char new_path[MAXBUFSIZE];
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A1], old_path);
            mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A3], new_path);
            if (syscode == RV_SYS_renameat)
                regs->regs_R[MD_REG_A0] = renameat(_a0, old_path, _a2, new_path);
            if (syscode == RV_SYS_renameat2)
                regs->regs_R[MD_REG_A0] = syscall(RV_SYS_renameat2, _a0, old_path, _a2, new_path, _a4);

        }
            break;
        case RV_SYS_clone: {
            panic("invalid/unimplemented system call encountered, code %d", syscode);
            regs->regs_R[MD_REG_A0] = clone(_a0, _a1, _a2, _a3);
        }
            break;
        case RV_SYS_faccessat: {
            char pathname[MAXBUFSIZE];
            mem_strcpy(mem_fn, mem, Read, /*pathname*/regs->regs_R[MD_REG_A1], pathname);
            regs->regs_R[MD_REG_A0] = faccessat(_a0, pathname, _a2, _a3);
        }
            break;
        case RV_SYS_stat: 
        case RV_SYS_fstatat: {
            regs->regs_R[MD_REG_A0] = 0;
	    }
            break;
        case RV_SYS_set_tid_address : { /* no.96 need to fix, unimplement */
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_set_robust_list: { /* no.99 need to fix, unimplement */
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_getrandom: { /* no.278 need to fix, unimplement */
            regs->regs_R[MD_REG_A0] = 0;
        }
            break;
        case RV_SYS_riscv_hwprobe: {       
            if((regs->regs_R[MD_REG_A2] != 0) || (regs->regs_R[MD_REG_A3] != 0) || (regs->regs_R[MD_REG_A4] != 0)) {
                regs->regs_R[MD_REG_A0] = -EBADF;
            }
            else {
                int count = regs->regs_R[MD_REG_A1];
                for(int i=0; i < count; i++) {
                    struct riscv_hwprobe kv;
                    mem_bcopy(mem_fn, mem, Read, regs->regs_R[MD_REG_A0], &kv, sizeof(kv));

                    if(kv.key == RISCV_HWPROBE_KEY_IMA_EXT_0) {
                        kv.value = 0;
                        kv.value |= RISCV_HWPROBE_IMA_FD | RISCV_HWPROBE_IMA_C;
                    }
                    else {
                        kv.key = -1;
                        kv.value = 0;
                    }
                     mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A0], &kv, sizeof(kv));
                }             
            }
            /*panic("invalid/unimplemented system call encountered, code %d", syscode);*/
        }
            break;
        default:
            panic("invalid/unimplemented system call encountered, code %d", syscode);
    }
}
