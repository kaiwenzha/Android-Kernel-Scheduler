## Linux kernel scheduler

### Kernel

- **Definition**

  Kernel is fundamental part of OS that **manages computer's hardwares**, and **allows softwares to run and use hardware resources** in shared manners.

- **Hardware Resources**

  - processors
  - memory
  - I/O devices (keyboard, disk drives, network interface cards)

- **OS  =  kernel  +  some useful utilities and applications (eg. administration tools and GUIs)**

- **Monolithic kernels and modules**


  - Linux has a monolithic kernel that contains all of the code necessary to perform every kernel related task in a single binary file.
  - Linux can extend its functionality by **adding modules**. Here, the modules are pieces of code that can be **loaded and unloaded into the kernel** upon demand while a system is up and running.

### Process

- **Process & Thread**
  - A **process** is an instance of a computer program that is being executed. 
  - A **thread** is also the programmed code in execution. The thread is **the smallest unit** that can be managed independently by a kernel scheduler. (Multiple threads can exist within the same process and share memory)
  - In linux kernel terms, a thread is often called a task. But default Linux kernel creates a process with a single thread, a thread may look like a process. For that reason, **we sometimes refer to a process as a task as well**.

- **Create new process**

  - `fork()` - create a new process
  - `exec()` - replace the current process with a new process specified in its argument directly.

- **Wait until child process terminates**

  - `wait()` - allow parent process to halt until its child process finishes
### Scheduler

- **Priority**
  - Most of scheduling algorithm are priority-based.
  - The general idea, which isn't exactly implemented on Linux, is that threads with a higher priority run before those with a lower priority, whereas, threads with the same priority are scheduled in a round-robin fashion.
- **Preemptive scheduling**
  - Linux kernel features a preemptive scheduling
- **Context switching**
  - A processor changes a thread to execute from one to another.
- **I/O-bound & CPU-bound**
  - Threads can be classified into two major types: I/O-bound and CPU-bound.
  - **I/O-bound**
    - threads that are mostly waiting for arrivals of inputs(eg. keyboard strokes) or the completion of outputs(eg. writing into disks)
    - **Not stay running for very long**, and block themselves voluntarily to **wait for I/O events**.
    - What matters: **they need to be processed in quick** otherwise no good responsiveness.
    - A common rule is that a scheduler puts more urgency into serving the I/O-bound threads.****
  - **CPU-bound**
    - threads that spend much of time doing calculations(e.g. compiling a program)
    - Not many I/O events involved => tend to **run as long as the scheduler allows**
    - **Not expect system to be responsive** 
    - Be picked to run by scheduler less frequently, but once chosen, **holds CPU for a longer time**
- **Real-time & Non-real-time**
  - Threads can be classified into two major types: Real-time and Non-real-time.
  - **Real-time**
    - should be processed with a strict time constraint, often referred to as a deadline.
    - the scheduler take cares of the real-time threads with **a high priority**.
    - can be further classified into: hard real-time and soft real-time.
      - hard real-time: require all ddls to be met with no exception otherwise the system may fall into catastrophic failure.
      - soft real-time: miss ddl results in degraded quality for the intended service, but system can still go on.
  - **Non-real-time**
    - Not associated with ddl
    - could be human-interactive threads or batch threads(threads that process a large amount of data without manual intervention).
    - For batch threads, a fast response time is not critical, and so they can **be scheduled to run as resources allow(low priority)**.

### Core Scheduler

- **Scheduling classes**

  - `struct sched_class` - a scheduling class is a set of function pointers

    ```c
    struct sched_class {
        const struct sched_class *next;
        ...
        struct task_struct * (*pick_next_task) (struct rq *rq, struct task_struct *prev);
        void (*put_prev_task) (struct rq *rq, struct task_struct *p);
        ...
        void (*task_tick) (struct rq *rq, struct task_struct *p, int queued);
        ...
    };
    ```

    Each scheduling algorithm gets an instance of `struct sched_class` and connects the function pointers with their corresponding implementations.

  - `rt_sched_class` - Real-time scheduler

    ```c
    const struct sched_class rt_sched_class = {
        .next           = &fair_sched_class,
        ...
        .pick_next_task     = pick_next_task_rt,
        .put_prev_task      = put_prev_task_rt,
        ...
        .task_tick      = task_tick_rt,
        ...
    };
    ```

    - RT scheduler assigns a priority to every thread and processes in order of priorities.
    - Only address the soft real-time threads.

  - `fair_sched_class` - Non-real-time scheduler(CFS - Complete Fair Scheduler)

    ```c
    const struct sched_class fair_sched_class = {
        .next           = &idle_sched_class,
        ...
        .pick_next_task     = pick_next_task_fair,
        .put_prev_task      = put_prev_task_fair,
        ...
        .task_tick      = task_tick_fair,
        ...
    };
    ```

    - Still assign priority, but not directly mean the order of being processed. Rather, it decides how long a thread can occupy a processor(proportion of processor time).
    - **Fair** - Each thread is guaranteed to use its own fraction of processor time for a certain time interval according to its priority.

  - **Core priority logics:** **`rt_sched_class` > `fair_sched_class`**   

- **Scheduling policies**

  - **RT scheduler**
    - `SCHED_RR` - Round-robin (run one by one for a pre-defined time interval)
    - `SCHED_FIFO` - First-In-First-Out
  - **CFS scheduler**
    - `SCHED_BATCH` - Handle threads that have a **batch-characteristics **(CPU-bound and non-interactive). Never preempt non-idle threads.
    - `SCHED_NORMAL` - Normal threads fall into this type.

- **Run queues**

  - `struct rq` - manage ready threads by enqueueing them into a **run queue**

    ```c
    struct rq {
        ...
        struct cfs_rq cfs;
        struct rt_rq rt;
        ...
        struct task_struct *curr, *idle, *stop;
        ...
        u64 clock;
        ...
    };
    ```

  - **Details**

    - `struct cfs_rq cfs` and `struct rt_rq rt` - **sub-run queues** for CFS and RT scheduler.
      - Enqueueing a thread to a run queue eventually means enqueueing it into either of these sub-run queues depending on the scheduler class.
    - The thread that is currently running on a CPU is pointed by `struct task_struct *curr` .
    - `clock` - store the lastest time at which the corresponding CPU reads a clock source

  - Each CPU has its own run queue.

  - Each thread can belong to only a single run queue since it's impossible that multi-CPUs process the same thread simultaneously.

  - **Load Balancing**

    - It occurs among CPUs in multi-core processors.
    - Without balancing load, threads may wait in a specific CPU's run queue, while other CPUs have nothing in their run queue. (lead to performance degradation)

- **The main body: `__schedule()`**

  - Functions: **put the previously running thread** into the run queue, **picking a new thread** to run next, and **switching context** between the two threads.

    ```c
    static void __sched __schedule(void)
    {
        ...
        cpu = smp_processor_id();
        rq = cpu_rq(cpu);
        prev = rq->curr;
        ...
        put_prev_task(rq, prev);
        ...
        next = pick_next_task(rq);
        ...
        if (likely(prev != next)) {
            ...
            rq->curr = next;
            ...
            context_switch(rq, prev, next);
            ...
        }
        ...
    }
    ```

  - **Most work in `__schedule()` is delegated to the scheduling classes.**

  - `put_prev_task()` - function **registered to the function pointer** `put_prev_task` **of** **the scheduling class** that the previously running task belongs to.

    ```c
    static void put_prev_task(struct rq *rq, struct task_struct *prev)
    {
        ...
        prev->sched_class->put_prev_task(rq, prev);
    }
    ```

    - As for specific `rt_sched_class` and `fair_sched_class` , this is `put_prev_task_rt()` for RT scheduler and `put_prev_task_fair()` for CFS.

  - `pick_next_task()` - similar

    ```c
    #define for_each_class(class) \
       for (class = sched_class_highest; class; class = class->next)

    static inline struct task_struct *pick_next_task(struct rq *rq)
    {
        const struct sched_class *class;
        struct task_struct *p;
        ...
        for_each_class(class) {
            p = class->pick_next_task(rq);
            if (p)
                return p;
        }
        ...
    }
    ```

    - `pick_next_task()` - seek the next-running thread using the **function pointer** `pick_next_task` **of a scheduling class** **by which** `pick_next_task_fair()` and `pick_next_task_rt()` are called for CFS and RT scheduler.
    - `for_each_class(class)` - scheduling classes are processed one by one in order of their priority, note that `fair_sched_class` can be taken care of only if there is nothing to do with `rt_sched_class`.

- **Periodic accounting: `scheduler_tick()`**

  ```c
  void scheduler_tick(void)
  {
      int cpu = smp_processor_id();
      struct rq *rq = cpu_rq(cpu);
      struct task_struct *curr = rq->curr;
      ...
      update_rq_clock(rq);
      curr->sched_class->task_tick(rq, curr, 0);
      ...
  }
  ```

  - periodically called by the kernel with the frequency `HZ`
  - `update_rq_clock()` - read a clock source and update `clock` of the run queue

  - `task_tick` - function pointer that checks if the current thread is running for too long. If so, set a flag that indicates `__schedule()` must be called to replace the current task.

### Completely Fair Scheduler (CFS)

- **Time slice**

  - CFS sets time slice that is an interval for which a thread is allowed to run **without being preempted**.
  - Time slice $\propto$ weight $\propto$ priority $\propto$ running time

- `check_preempt_tick` 

  ```c
  static void check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
  {
      ...
      ideal_runtime = sched_slice(cfs_rq, curr);
      delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
      if (delta_exec > ideal_runtime) {
          resched_task(rq_of(cfs_rq)->curr);
          ...
          return;
      }
      ...
  }
  ```

  - check if the current thread is running longer than its time slice.

  - If so, call `resched_task()` that marks `__schedule()` will be executed to change the running thread.

  - `sched_slice()` - calculate the time slice for the currently running thread.

    ```c
    static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
    {
        u64 slice = __sched_period(cfs_rq->nr_running + !se->on_rq);
        ...
        cfs_rq = cfs_rq_of(se);
        load = &cfs_rq->load;
        ...
        slice = calc_delta_mine(slice, se->load.weight, load);
        ...
        return slice;
    }
    ```

    - `__sched_period` - update the scheduling period considering the number of ready threads.

      ```c
      static u64 __sched_period(unsigned long nr_running)
      {
          u64 period = sysctl_sched_latency;
          unsigned long nr_latency = sched_nr_latency;
          if (unlikely(nr_running > nr_latency)) {
              period = sysctl_sched_min_granularity;
              period *= nr_running;
          }
          return period;
      }
      ```

      - By default, the kernel targets to serve `sched_nr_latency` threads for `sysctl_sched_latency` ms, assuming that a thread is supposed to run at a time for at least `sysctl_min_granularity` ms, which is defined as follows:
        $$
        \text{sysctl_min_granularity} = \frac{\text{sysctl_sched_latency}}{\text{sched_nr_latency}}
        $$
        That is, the default scheduling period is `sysctl_sched_latency` ms. However, if there are more than `sched_nr_latency` threads in a run queue, the scheduling period is set to `sysctl_min_granularity` ms multiplied by the number of ready threads.

      - `calc_delta_mine()` - finalize the time slice for the currently running thread
        $$
        \text{time slice of current thread} = (\text{scheduling period})\times\bigg(\frac{\text{weight of current thread}}{\text{sum of weights of all threads}}\bigg)
        $$

- **Virtual runtime**
  $$
  \text{virtual runtime} = (\text{actual runtime})*1024/\text{weight}
  $$

  - `update_curr()` - update virtual runtime

    ```c
    static void update_curr(struct cfs_rq *cfs_rq)
    {
        struct sched_entity *curr = cfs_rq->curr;
        u64 now = rq_of(cfs_rq)->clock_task;
        unsigned long delta_exec;
        ...
        delta_exec = (unsigned long)(now - curr->exec_start);
        ...
        __update_curr(cfs_rq, curr, delta_exec);
        curr->exec_start = now;
        ...
    }
    ```

    - `__update_curr()` - convert virtual time increment (done by `calc_delta_fair() ` below)

      ```c
      static inline void
      __update_curr(struct cfs_rq *cfs_rq, struct sched_entity *curr,
                unsigned long delta_exec)
      {
          ...
          delta_exec_weighted = calc_delta_fair(delta_exec, curr);
          curr->vruntime += delta_exec_weighted;
          ...
      }
      ```

- **Putting a running thread back into a runqueue**

  - CFS uses `put_prev_task_fair()` that calls `put_prev_entity()`.

    ```c
    static void put_prev_entity(struct cfs_rq *cfs_rq, struct sched_entity *prev)
    {
        if (prev->on_rq)
            update_curr(cfs_rq);
        ...
        if (prev->on_rq) {
            ...
            __enqueue_entity(cfs_rq, prev);
            ...
        }
        cfs_rq->curr = NULL;
    }
    ```

    - `prev->on_rq` - check if the thread is already on a run queue.
    - Otherwise, `update_curr()` to update its virtual runtime; `__enqueue_entity()` to enqueue it into `cfs_rq` (implemented with RBT where threads are sorted according to their virtual runtime).

- **Choosing the thread to run next**

  - `pick_next_task_fair()` (main body is `pick_next_entity()`)

    ```c
    static struct sched_entity *pick_next_entity(struct cfs_rq *cfs_rq)
    {
        struct sched_entity *se = __pick_first_entity(cfs_rq);
        struct sched_entity *left = se;
        if (cfs_rq->skip == se) {
            struct sched_entity *second = __pick_next_entity(se);
            if (second && wakeup_preempt_entity(second, left) < 1)
                se = second;
        }
        if (cfs_rq->last && wakeup_preempt_entity(cfs_rq->last, left) < 1)
            se = cfs_rq->last;

        if (cfs_rq->next && wakeup_preempt_entity(cfs_rq->next, left) < 1)
            se = cfs_rq->next;
        ...
        return se;
    }
    ```

    - `wakeup_preempt_entity()` - balance fairness in terms of virtual time among threads
    - Orders:
      1. Pick the thread that has the smallest virtual runtime.
      2. Pick the “next” thread that woke last but failed to preempt on wake-up, since it may need to run in a hurry.
      3. Pick the “last” thread that ran last for cache locality.
      4. Do not run the “skip” process, if something else is available.

### Real-Time (RT) Scheduler

- **The run queue of RT scheduler**

  - `struct rt_rq` - an array where **each element is the head of a linked list that manages the threads of a particular priority.**

    ```c
    struct rt_prio_array {
        DECLARE_BITMAP(bitmap, MAX_RT_PRIO+1);
        struct list_head queue[MAX_RT_PRIO];
    };

    struct rt_rq {
        struct rt_prio_array active;
        ...
        int curr;
        ...
    }
    ```

    - All real-time threads whose priority is `x` are inserted into a linked list headed by `active.queue[x]`. When there exists at least one thread in`active.queue[x]`, the `x`-th bit of `active.bitmap` is set.

- **Execution and scheduling policies**

  - A newly queued thread is always placed at the end of each list of a corresponding priority in the run queue. The first task on the list of the highest priority available is taken out to run.

  - Two policies: **`SCHED_FIFO`** and **`SCHED_RR`**

    - Difference between the two policies in `task_tick_rt()`

      ```c
      static void task_tick_rt(struct rq *rq, struct task_struct *p, int queued)
      {
          struct sched_rt_entity *rt_se = &p->rt;

          update_curr_rt(rq);

          watchdog(rq, p);

          /*
           * RR tasks need a special form of timeslice management.
           * FIFO tasks have no timeslices.
           */
          if (p->policy != SCHED_RR)
              return;

          if (--p->rt.time_slice)
              return;

          p->rt.time_slice = sched_rr_timeslice;

          /*
           * Requeue to the end of queue if we (and all of our ancestors) are the
           * only element on the queue
           */
          for_each_sched_rt_entity(rt_se) {
              if (rt_se->run_list.prev != rt_se->run_list.next) {
                  requeue_task_rt(rq, p, 0);
                  set_tsk_need_resched(p);
                  return;
              }
          }
      }
      ```

      - The threads with `SCHED_FIFO` can run until they stop or yield. There is nothing to be done every tick interrupt. The `SCHED_RR` threads are given a time slice, which is **decremented by 1 on the tick interrupt**. When this time slice becomes zero, `SCHED_RR` threads are enqueued again.