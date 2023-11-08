#include <linux/kernel.h> /* We are doing kernel work */
#include <linux/module.h> /* Specifically, a module */
#include <linux/proc_fs.h> /* Necessary because we use proc fs */
#include <linux/seq_file.h> /* for seq_file */
#include <linux/version.h>
#include <generated/utsrelease.h>

/* TIME */
#include <linux/time.h>
#include <linux/timekeeping32.h>
#include <linux/kernel_stat.h>

/* CPU */
#include <asm/processor.h>
#include <linux/padata.h>
#include <asm/x86_init.h>


/* MEMORY */
#include <linux/sysinfo.h>
#include <linux/mmzone.h> //active
#include <linux/vmstat.h> //dirty
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/mm.h>


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define PROC_NAME "my_info"
#define Kilo(x) ((x) << (PAGE_SHIFT - 10))
static int version_printed = 0;

/* This function is called at the beginning of a sequence.
 * ie, when:
 *   - the /proc file is read (first time)
 *   - after the function stop (end of sequence)
 */
static void *my_seq_start(struct seq_file *s, loff_t *pos)
{
    // never use in this hw
    /*
      static unsigned long counter = 0;

      // beginning a new sequence?
      if (*pos == 0) {
          // yes => return a non null value to begin the sequence
          return &counter;
      }

      // no => it is the end of the sequence, return end to stop reading
      *pos = 0;
      return NULL; */
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
    *pos = cpumask_next(*pos - 1, cpu_online_mask);
    if ((*pos) < nr_cpu_ids)
    {
        return &cpu_data(*pos);
    }
    return NULL;
}



/* This function is called after the beginning of a sequence.
 * It is called untill the return is NULL (this ends the sequence).
 */
static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    //never use in this hw
    /*
     * unsigned long *tmp_v = (unsigned long *)v;
     * (*tmp_v)++;
     * (*pos)++;
     * return NULL;
    */

}
static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
    (*pos)++;
    return c_start(m, pos);
}


/* This function is called at the end of a sequence. */
static void my_seq_stop(struct seq_file *m, void *v)
{
    /* nothing to do, we use a static value in start() */
    struct sysinfo i;
    unsigned long pages[NR_LRU_LISTS];
    si_meminfo(&i);

    int lru;
    for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
        pages[lru] = global_node_page_state(NR_LRU_BASE + lru);

    seq_printf(m, "\n============Memory===========\n");

    seq_printf(m, "MemTotal\t: %lu kB\n", Kilo(i.totalram));
    seq_printf(m, "MemFree\t\t: %lu kB\n", Kilo(i.freeram));
    seq_printf(m, "Buffers\t\t: %lu kB\n", Kilo(i.bufferram));
    seq_printf(m, "Active\t\t: %lu kB\n", Kilo(pages[LRU_ACTIVE_ANON] +
               pages[LRU_ACTIVE_FILE]));
    seq_printf(m, "Inactive\t: %lu kB\n", Kilo(pages[LRU_INACTIVE_ANON] +
               pages[LRU_INACTIVE_FILE]));
    seq_printf(m, "Shmem\t\t: %lu kB\n",Kilo(i.sharedram));
    seq_printf(m, "Dirty\t\t: %lu kB\n",Kilo(
                   global_node_page_state(NR_FILE_DIRTY)));
    seq_printf(m, "Writeback\t: %lu kB\n",
               Kilo(global_node_page_state(NR_WRITEBACK)));
    seq_printf(m, "KernelStack\t: %lu kB\n",
               global_zone_page_state(NR_KERNEL_STACK_KB));
    seq_printf(m, "PageTables\t: %lu kB\n",
               Kilo(global_zone_page_state(NR_PAGETABLE)));

    // time
    seq_printf(m, "\n============Time===========\n");
    u64 nsec;
    u32 rem;
    struct timespec uptime;
    struct timespec idle;

    int u;
    nsec = 0;
    for_each_possible_cpu(u)
    nsec += (__force u64) kcpustat_cpu(u).cpustat[CPUTIME_IDLE];
    get_monotonic_boottime(&uptime);
    idle.tv_sec = div_u64_rem(nsec, NSEC_PER_SEC, &rem);
    idle.tv_nsec = rem;
    seq_printf(m, "Uptime\t\t: %lu.%02lu (s)\n",
               (unsigned long) uptime.tv_sec,
               (uptime.tv_nsec / (NSEC_PER_SEC / 100)));
    seq_printf(m, "Idletime\t: %lu.%02lu (s)\n\n",
               (unsigned long) idle.tv_sec,
               (idle.tv_nsec / (NSEC_PER_SEC / 100)));

}

/* This function is called for each "step" of a sequence. */
static int my_seq_show(struct seq_file *m, void *v)
{
    /* version */
    if(!version_printed)
    {
        seq_printf(m, "\n============Version========\n");
        seq_printf(m, "Linux version %s\n",UTS_RELEASE);
        seq_printf(m, "\n============CPU===========");
        version_printed = 1;
    }

    /* cpu */
    struct cpuinfo_x86 *c = v;
    unsigned int cpu;
    cpu = c->cpu_index;

    // ==cpu== only print once when version is printed
    seq_printf(m, "\nprocessor\t: %u\n", cpu);
    seq_printf(m,"model name\t: %s\n",
               c->x86_model_id[0] ? c->x86_model_id : "unknown");
    seq_printf(m, "physical id\t: %d\n", c->phys_proc_id);
    seq_printf(m, "core id\t\t: %d\n", c->cpu_core_id);
    if (c->x86_cache_size >= 0)
        seq_printf(m, "cache size\t: %d KB\n", c->x86_cache_size);
    seq_printf(m, "clflush size\t: %u\n", c->x86_clflush_size);
    seq_printf(m, "cache_alignment\t: %d\n", c->x86_cache_alignment);
    seq_printf(m, "address sizes\t: %u bits physical, %u bits virtual\n",
               c->x86_phys_bits, c->x86_virt_bits);

    return 0;
}



/* This structure gather "function" to manage the sequence */
static struct seq_operations my_seq_ops =
{
    .start = c_start,
    .next = c_next,
    .stop = my_seq_stop,
    .show = my_seq_show,
};



/* This function is called when the /proc file is open. */
static int my_open(struct inode *inode, struct file *file)
{
    version_printed = 0;
//	return single_open(file, my_seq_show,NULL);
    return seq_open(file, &my_seq_ops);
};



/* This structure gather "function" that manage the /proc file */
#ifdef HAVE_PROC_OPS
static const struct proc_ops my_file_ops =
{
    .proc_open = my_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release,
};
#else
static const struct file_operations my_file_ops =
{
    .open = my_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};
#endif



static int __init procfs4_init(void)
{
    struct proc_dir_entry *entry;
    entry = proc_create(PROC_NAME, 0644, NULL, &my_file_ops);
    if (entry == NULL)
    {
        remove_proc_entry(PROC_NAME, NULL);
        pr_debug("Error: Could not initialize /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    return 0;
}



static void __exit procfs4_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    pr_debug("/proc/%s removed\n", PROC_NAME);
}



module_init(procfs4_init);
module_exit(procfs4_exit);
MODULE_LICENSE("GPL");
