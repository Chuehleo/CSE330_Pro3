#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/mm_types.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

static unsigned int pid = 0;
module_param(pid, unsigned int, 0644);
MODULE_PARM_DESC(pid, "Process ID");

static unsigned int RSS; /*number of pages in the physical memory*/
static unsigned int SWAP; /*number of pages swapped out to disk*/
static unsigned int WSS; /*number of pages in the proces working set*/

static unsigned long timer_interval_ns = 10e9;
static struct hrtimer hr_timer;
struct ktime_t ktime;

struct task_struct *task = NULL;
struct mm_struct *mm_st = NULL;
unsigned long address;

pte_t *pte_address(struct mm_struct *mm, unsigned long address)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pmd_t *pmd;
	pud_t *pud;
	pte_t *ptep, pte;

	pgd = pgd_offset(mm, address); // get pgd from mm and the page address
	if (pgd_none(*pgd) || pgd_bad(*pgd))
	{
		// check if pgd is bad or does not exist
		return;
	}
	p4d = p4d_offset(pgd, address); //get p4d from from pgd and the page address
	if (p4d_none(*p4d) || p4d_bad(*p4d)) 
	{
		// check if p4d is bad or does not exist
		return;
	}
	pud = pud_offset(p4d, address); // get pud from from p4d and the page address
	if (pud_none(*pud) || pud_bad(*pud))
	{
		// check if pud is bad or does not exist
		return;
	}
	pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
	if (pmd_none(*pmd) || pmd_bad(*pmd))
	{
		// check if pmd is bad or does not exist
		return;
	}
	ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
	if (!ptep)
	{
		// check if pte does not exist
		return;
	}
	pte = *ptep;
}	

int ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep)
{
	int ret = 0;
	if (pte_young(*ptep))
	{
		ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
	}
	return ret;
}

void measure()
{
	WSS = 0;
	RSS = 0;
	SWAP = 0;

	for_each_process(task)
	{
		if (task->pid == pid)
		{
			measure_pte(task);
			break;
		}
	}	
}

//iterate through page tables of process
void measure_pte(struct task_struct* task)
{
	mm_st = task->mm;	
	while (vma != NULL)
	{
		for (int addr = vma->vm_start; 
}

enum hrtimer_restart no_restart_callback(struct hrtimer *timer)
{
	ktime_t currtime , interval;
	currtime = ktime_get();
	interval = ktime_set(0, timer_interval_ns);
	hrtimer_forward(timer, currtime , interval);
	
	// Do the measurement
	measure();
	printk("PID [%d]: RSS=[%d] KB, SWAP=[%d] KB, WSS=[%d] KB\n", pid, RSS, SWAP, WSS);

	return HRTIMER_NORESTART;
}


int _init(void)
{
	printk("Process ID number: %d\n",pid);
	
	ktime = ktime_set(0, timer_interval_ns);
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &no_restart_callback;	
	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
	
	return 0;
}

void _exit(void)
{
	hrtimer_cancel(&hr_timer);
	printk("Exiting the module...\n");
}

module_init(_init);
module_exit(_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chun Ting Ho");
