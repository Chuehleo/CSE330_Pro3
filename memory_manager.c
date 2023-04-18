#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

#define SHOW_TIMER 1

static unsigned long long WSS =0;
static unsigned long long RSS =0;
static unsigned long long SWAP =0;

static int pid=0;
module_param(pid, int, S_IRUGO);

unsigned long timer_interval_ns = 10e9; // 10-second timer
static struct hrtimer hr_timer;

// declaration of functions
static int __init start_module(void);
static void __exit end_module(void);

void clear_data(void){
	WSS =0;
	RSS=0;
	SWAP=0;
}

int ptep_test_and_clear_young(struct vm_area_struct *vma,
unsigned long addr, pte_t *ptep)
{
	int ret = 0;
	if (pte_young(*ptep))
		ret = test_and_clear_bit(_PAGE_BIT_ACCESSED,(unsigned long *) &ptep->pte);
	return ret;
}

/* check whether a pte points to a swap entry */
static inline int is_swap_pte(pte_t pte)
{
	return !pte_none(pte) && !pte_present(pte);
}



void update(struct vm_area_struct *vma,unsigned long addr,pte_t* p){
	pte_t value =*p;

	if (pte_present(value)){
		++RSS;
		if (ptep_test_and_clear_young(vma,addr,p)){
			++WSS;
		}
	}
	else {//if (is_swap_pte(*p)) {
		++SWAP;
	}
	
	//#endif
}

pte_t *pte_by_address(const struct mm_struct *const mm,
                             const unsigned long address)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pmd_t *pmd;
	pud_t *pud;
	pte_t *ptep=NULL;
	
	pgd = pgd_offset(mm, address); // get pgd from mm and the page address
	
	// check if pgd is bad or does not exist
	if (pgd_none(*pgd) || pgd_bad(*pgd)){
		return NULL;
	}
	
	p4d = p4d_offset(pgd, address); // get p4d from from pgd and the page address
	if (p4d_none(*p4d) || p4d_bad(*p4d)){ // check if p4d is bad or does not exist
	return NULL;}
	
	pud = pud_offset(p4d, address); // get pud from from p4d and the page address
	if (pud_none(*pud) || pud_bad(*pud)){ // check if pud is bad or does not exist
	return NULL;}
	
	pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
	if (pmd_none(*pmd) || pmd_bad(*pmd)){
	// check if pmd is bad or does not exist
	return NULL;}
	
	ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
	return ptep;
}

void update_measure_info(struct task_struct* t){
	struct mm_struct *mm=NULL; 
	
	mm=t->mm;
	if (mm != NULL) {
        struct vm_area_struct *vma = mm->mmap;
        while (vma != NULL) {
            unsigned long address;
            for (address = vma->vm_start; address < vma->vm_end; address += PAGE_SIZE) {
                pte_t *p = pte_by_address(mm,address);
                if (p!=NULL){
                	update(vma,address,p);
                }
            }
            vma = vma->vm_next;
        }
    }
}

void do_measure(void){
	struct task_struct* t;
	clear_data();
	for_each_process(t) {
		if (t->pid==pid){
			update_measure_info(t);
			break;
		}
	}
}

enum hrtimer_restart timer_callback( struct hrtimer *timer_for_restart )
{
	// Resetting the timer
	ktime_t currtime,interval;
	currtime = ktime_get();
	interval = ktime_set(0,timer_interval_ns);
	hrtimer_forward(timer_for_restart, currtime , interval);
	
	if (SHOW_TIMER)
		printk(KERN_ALERT "start of callback\n");
	
	// Do the measurement, like looking into VMA and walking through memory pages
	// And also do the Kernel log printing aka printk per requirements
	do_measure();
	
	printk(KERN_ALERT "PID [%d]: RSS=[%lld] KB, SWAP=[%lld] KB, WSS=[%lld] KB\n",
	pid,RSS*(PAGE_SIZE/1024),SWAP*(PAGE_SIZE/1024),WSS*(PAGE_SIZE/1024));
	
	return HRTIMER_RESTART;
}

static int __init timer_init(void) {
	ktime_t ktime ;
	ktime = ktime_set(0, timer_interval_ns );
	hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	hr_timer.function = &timer_callback;
	hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );
	return 0;
}
static void __exit timer_exit(void) {
	int ret;
	ret = hrtimer_cancel( &hr_timer );
	if (ret) printk("The timer was still in use...\n");
	printk("HR Timer module uninstalling\n");
}


int __init start_module(void){
	printk(KERN_ALERT "start the module with pid=%d\n",pid);
	timer_init();
	#if 0
	hrtimer_init(&task_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	task_timer.function = task1_timer_func;

	
	#endif 
	return 0;
}
void __exit end_module(void){
	timer_exit();
	printk(KERN_ALERT "exit the module\n");		
}

module_init(start_module);
module_exit(end_module);

