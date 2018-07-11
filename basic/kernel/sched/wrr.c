#include "sched.h"
#include <linux/slab.h>

#define PATH_MAX 4096
static char group_path[PATH_MAX];

static void enqueue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se, bool head)
{
    struct list_head *queue = &rq->wrr.queue;

    if (head)
        list_add(&wrr_se->run_list, queue);
    else
        list_add_tail(&wrr_se->run_list, queue);

    rq->wrr.wrr_nr_running++;
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
    printk("YOU ARE IN: enqueue_task_wrr @ WRR\n");

    struct sched_wrr_entity *wrr_se = &p->wrr;

    if (flags & ENQUEUE_WAKEUP)
    {
        wrr_se->timeout = 0;
    } 

    enqueue_wrr_entity(rq, wrr_se, flags & ENQUEUE_HEAD);
    inc_nr_running(rq);
}

/*
 * Update the current task's runtime statistics. Skip current tasks that
 * are not in our scheduling class.
 */
static void update_curr_wrr(struct rq *rq)
{
    struct task_struct *curr = rq->curr;
    u64 delta_exec;

    if (curr->sched_class != &wrr_sched_class)
		return;

    delta_exec = rq->clock_task - curr->se.exec_start;
    if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

    schedstat_set(curr->se.statistics.exec_max,
		      max(curr->se.statistics.exec_max, delta_exec));

    curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);
}

static void dequeue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se)
{
    list_del_init(&wrr_se->run_list);
    rq->wrr.wrr_nr_running--;
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct* p, int flags)
{
    printk("YOU ARE IN: dequeue_task_wrr @ WRR\n");

    struct sched_wrr_entity *wrr_se = &p->wrr;

    update_curr_wrr(rq);
    dequeue_wrr_entity(rq, wrr_se);
    dec_nr_running(rq);
}

static void requeue_task_wrr(struct rq *rq, struct task_struct *p, int head)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	struct list_head *queue = &rq->wrr.queue;

	if (head)
        list_move(&wrr_se->run_list, queue);
    else
        list_move_tail(&wrr_se->run_list, queue);
}

static void yield_task_wrr(struct rq *rq)  //task gives up
{
    printk("YOU ARE IN: yield_task_wrr @ WRR\n");

	requeue_task_wrr(rq, rq->curr, 0);
}

static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
    printk("YOU ARE IN: pick_next_task_wrr @ WRR\n");

    struct sched_wrr_entity* head;
    struct task_struct *p;

    if (unlikely(!rq->wrr.wrr_nr_running))
        return NULL;

    head = list_first_entry(&rq->wrr.queue, struct sched_wrr_entity, run_list);  //sched_wrr_entity
    p = container_of(head, struct task_struct, wrr);  //task_struct
    
    if (!p) return NULL;
    p->se.exec_start = rq->clock_task;  //current clock

	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
    printk("YOU ARE IN: put_prev_task_wrr @ WRR\n");

    update_curr_wrr(rq);
    p->se.exec_start = 0;
}

static void task_fork_wrr(struct task_struct* p)
{
    printk("YOU ARE IN: task_fork_wrr @ WRR\n");

    p->wrr.time_slice = p->wrr.parent->time_slice;
}

static void set_curr_task_wrr(struct rq *rq)
{
    printk("YOU ARE IN: set_curr_task_wrr @ WRR\n");

    struct task_struct *p = rq->curr;

    p->se.exec_start = rq->clock_task;
}

static void watchdog(struct rq *rq, struct task_struct *p)
{
	unsigned long soft, hard;

	/* max may change after cur was read, this will be fixed next tick */
	soft = task_rlimit(p, RLIMIT_RTTIME);
	hard = task_rlimit_max(p, RLIMIT_RTTIME);

	if (soft != RLIM_INFINITY) {
		unsigned long next;

		p->wrr.timeout++;
		next = DIV_ROUND_UP(min(soft, hard), USEC_PER_SEC/HZ);
		if (p->wrr.timeout > next)
			p->cputime_expires.sched_exp = p->se.sum_exec_runtime;
	}
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
    printk("YOU ARE IN: task_tick_wrr @ WRR\n");

    struct list_head *queue = &rq->wrr.queue;

	update_curr_wrr(rq);

	watchdog(rq, p);

    if (p->policy != SCHED_WRR)
		return;

	if (--p->wrr.time_slice)
		return;
    
    cgroup_path(task_group(p)->css.cgroup, group_path, PATH_MAX);

    if (strcmp(group_path, FOREGROUP) == 0)
        p->wrr.time_slice = WRR_FORE_TIMESLICE;
    else if (strcmp(group_path, BACKGROUP) == 0)
        p->wrr.time_slice = WRR_BACK_TIMESLICE;

    if (queue->prev != queue->next) {
		requeue_task_wrr(rq, p, 0);
		resched_task(p);
	}
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
    printk("YOU ARE IN: get_rr_interval_wrr @ WRR\n");

    cgroup_path(task_group(task)->css.cgroup, group_path, PATH_MAX);

    printk("GROUP_INFO: %s\n", group_path);

	if (task->policy == SCHED_WRR && strcmp(group_path, FOREGROUP) == 0)
		return WRR_FORE_TIMESLICE;
	else if (task->policy == SCHED_WRR && strcmp(group_path, BACKGROUP) == 0)
        return WRR_BACK_TIMESLICE;
    else
        return 0;
}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
    printk("YOU ARE IN: switched_to_wrr @ WRR\n");

    if (p->on_rq && rq->curr != p)
		if (rq == task_rq(p) && rq->curr->policy != SCHED_WRR)
			resched_task(rq->curr);
}

void free_wrr_sched_group(struct task_group *tg)
{
}

int alloc_wrr_sched_group(struct task_group *tg, struct task_group *parent)
{
	return 1;
}

static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
}

static int select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
}

static void rq_online_wrr(struct rq *rq)
{
}

static void rq_offline_wrr(struct rq *rq)
{
}

static void pre_schedule_wrr(struct rq *rq, struct task_struct *prev)
{
}

static void post_schedule_wrr(struct rq *rq)
{
}

static void task_woken_wrr(struct rq *rq, struct task_struct *p)
{
}

static void switched_from_wrr (struct rq *this_rq, struct task_struct *task)
{

}

static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
}

const struct sched_class wrr_sched_class = {
    .next               = &fair_sched_class,            /*Never need impl*/
    .enqueue_task       = enqueue_task_wrr,             /*Required*/
    .dequeue_task       = dequeue_task_wrr,             /*Required*/
    .yield_task         = yield_task_wrr,               /*Required*/
    .check_preempt_curr = check_preempt_curr_wrr,       /*Never need impl*/
    .pick_next_task     = pick_next_task_wrr,           /*Required*/
    .put_prev_task      = put_prev_task_wrr,            /*Required*/
    .task_fork          = task_fork_wrr,                /*Required*/
#ifdef CONFIG_SMP
    .select_task_rq     = select_task_rq_wrr,           /*Never need impl*/
    .set_cpus_allowed   = set_cpus_allowed_wrr,         /*Never need impl*/
    .rq_online          = rq_online_wrr,                /*Never need impl*/
    .rq_offline         = rq_offline_wrr,               /*Never need impl*/
    .pre_schedule       = pre_schedule_wrr,             /*Never need impl*/
    .post_schedule      = post_schedule_wrr,            /*Never need impl*/
    .task_woken         = task_woken_wrr,               /*Never need impl*/
#endif
    .switched_from      = switched_from_wrr,            /*Never need impl*/
    .set_curr_task      = set_curr_task_wrr,            /*Required*/
    .task_tick          = task_tick_wrr,                /*Required*/
    .get_rr_interval    = get_rr_interval_wrr,          /*Required*/
    .prio_changed       = prio_changed_wrr,             /*Never need impl*/
    .switched_to        = switched_to_wrr,              /*Required*/
};