#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 1024
#define Q_LEN 10

struct msg { int seq; int value; };

K_MSGQ_DEFINE(work_q, sizeof(struct msg), Q_LEN, 4);
K_MUTEX_DEFINE(state_mutex);

static int total_sum;
static int total_count;

K_THREAD_STACK_DEFINE(prod_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(work_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(mon_stack,  STACK_SIZE);

static struct k_thread prod_thread_data;
static struct k_thread work_thread_data;
static struct k_thread mon_thread_data;

void producer(void *a, void *b, void *c)
{
    int seq = 0;
    while (1) {
        struct msg m = { .seq = seq, .value = (seq % 10) + 1 };

        if (k_msgq_put(&work_q, &m, K_NO_WAIT) == 0) {
            printk("[PROD] sent  seq=%d val=%d\n", m.seq, m.value);
            seq++;
        } else {
            printk("[PROD] queue full -> drop seq=%d\n", m.seq);
        }

        k_msleep(500);
    }
}

void worker(void *a, void *b, void *c)
{
    struct msg m;
    while (1) {
        k_msgq_get(&work_q, &m, K_FOREVER);

        int c_local, s_local;

        k_mutex_lock(&state_mutex, K_FOREVER);
        total_sum += m.value;
        total_count += 1;
        c_local = total_count;
        s_local = total_sum;
        k_mutex_unlock(&state_mutex);

        printk("[WORK] got   seq=%d val=%d -> count=%d sum=%d\n",
               m.seq, m.value, c_local, s_local);

        k_msleep(700);
    }
}

void monitor(void *a, void *b, void *c)
{
    while (1) {
        k_msleep(2000);

        k_mutex_lock(&state_mutex, K_FOREVER);
        int c = total_count, s = total_sum;
        k_mutex_unlock(&state_mutex);

        printk("[MON ] status count=%d sum=%d\n", c, s);
    }
}

int main(void)
{
    /* Smaller number = higher priority */
    k_thread_create(&work_thread_data, work_stack, STACK_SIZE,
                    worker, NULL, NULL, NULL,
                    1, 0, K_NO_WAIT);

    k_thread_create(&prod_thread_data, prod_stack, STACK_SIZE,
                    producer, NULL, NULL, NULL,
                    2, 0, K_NO_WAIT);

    k_thread_create(&mon_thread_data, mon_stack, STACK_SIZE,
                    monitor, NULL, NULL, NULL,
                    3, 0, K_NO_WAIT);

    k_thread_name_set(&work_thread_data, "worker");
    k_thread_name_set(&prod_thread_data, "producer");
    k_thread_name_set(&mon_thread_data, "monitor");

    return 0;
}
