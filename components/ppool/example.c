#include <stdio.h>
#include <unistd.h>

#include "ember/ppool.h"

void thread_entry(void *args)
{
    printf("tid=%ld args=%ld\n", (long)getpid(), (long)args);
    sleep(1);
}

int main(void)
{
    pool_t *pool = ppool_init(5);
    if (!pool) {
        fprintf(stderr, "ppool_init failed\n");
        return 1;
    }

    pool_task tasks[5] = {
        {thread_entry, (void *)1, 1},
        {thread_entry, (void *)2, 2},
        {thread_entry, (void *)3, 3},
        {thread_entry, (void *)4, 4},
        {thread_entry, (void *)5, 5},
    };

    for (int i = 0; i < 30; i++) {
        printf("add the %d task to ppool\n", i + 1);
        if (!ppool_add(pool, &tasks[i % 5])) {
            fprintf(stderr, "ppool_add failed at %d\n", i + 1);
        }
    }

    sleep(5);
    ppool_destroy(pool);
    return 0;
}
