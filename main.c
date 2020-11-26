// Lucas Marcon Zampieri - 1837873

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <sys/time.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int PID = 1;

// Data
char *planet_names[] = {"Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"};

typedef struct {
    int pid;
    int size;
    int block;
    int score;
    char data[50];
} PROCESS;

// 64KB, divided in 8 block of 8KB
PROCESS mmu_pmem[8];

PROCESS process_table[124];

PROCESS empty_space = {.pid=0};

PROCESS write_buffer;
PROCESS read_buffer;

long currentTimeMillis() {
    struct timeval time;
    gettimeofday(&time, NULL);
    int64_t s1 = (int64_t) (time.tv_sec) * 1000;
    int64_t s2 = (time.tv_usec / 1000);
    return s1 + s2;
}

#define PMEM_ARR_SIZE sizeof(mmu_pmem) / sizeof(PROCESS)

sem_t s0;

// 64KB in frames of 8KB
void mmu_init_pmem() {
    for (int i = 0; i < PMEM_ARR_SIZE; i++)
        mmu_pmem[i] = empty_space;
}

void mmu_init_process_table() {
    for (int i = 0; i < 124; i++)
        process_table[i] = empty_space;
}

int mmu_search_for_empty_frame() {
    int i = 0;

    while (i < PMEM_ARR_SIZE) {
        if (mmu_pmem[i].pid == empty_space.pid)
            return i;
        i++;
    }
    return -1;
}

int mmu_search_for_oldest_page() {
    int oldest = (int) time(NULL);
    int oldest_pos = 0;
    for (int i = 0; i < PMEM_ARR_SIZE; i++) {
        if (oldest > mmu_pmem[i].score) {
            oldest = mmu_pmem[i].score;
            oldest_pos = i;
        }
    }
    return oldest_pos;
}


void mmu_free_mem() {
    int initial_pos = mmu_search_for_oldest_page();
    printf("[MMU] Page Fault, free pid=%d score=%d p_mem_pos=%d\n", mmu_pmem[initial_pos].pid,
           mmu_pmem[initial_pos].score, initial_pos);
    mmu_pmem[initial_pos] = empty_space;
}

void mmu_load_mem(PROCESS new_process) {
    printf("[MMU] Loading to memory pid=%d\n", new_process.pid);
    // Store all blocks of process
    for (int i = 0; i < new_process.block; i++) {
        int empty_frame = mmu_search_for_empty_frame();

        if (empty_frame >= 0) {
            mmu_pmem[empty_frame] = new_process;
        } else {
            mmu_free_mem();
            mmu_pmem[mmu_search_for_empty_frame()] = new_process;
        }
        // Process Ages per cicle
        new_process.score++;
    }

    // Update score on process table
    process_table[new_process.pid] = new_process;
}

void mmu_write_process(PROCESS new_process) {

    mmu_load_mem(new_process);

    printf("[MMU][WRITE] pid=%d data=%s\n\n", new_process.pid, new_process.data);
}

int mmu_process_is_loaded(PROCESS process) {
    int i = 0, j = 0;

    if (process.block <= PMEM_ARR_SIZE) {
        while (i < PMEM_ARR_SIZE) {
            if (mmu_pmem[i].pid == process.pid)
                j++;
            i++;
        }
        if (j == process.block) {
            printf("[MMU] Already on memory pid=%d\n", process.pid);
            return 1;
        }
    }
    return 0;
}

void mmu_read_process(PROCESS read_process) {

    if (!mmu_process_is_loaded(read_process))
        mmu_load_mem(read_process);

    printf("[MMU][READ] pid=%d data=%s\n\n", read_process.pid, read_process.data);
}

void mmu_management() {
    while (1) {
        // Wait for a new process to be created
        sem_wait(&s0);
        if (write_buffer.pid != empty_space.pid) {
            mmu_write_process(write_buffer);
            write_buffer = empty_space;
        }
        if (read_buffer.pid != empty_space.pid) {
            mmu_read_process(read_buffer);
            read_buffer = empty_space;
        }
    }
}

// min 1 byte max 1MB
void process_management() {
    PROCESS new_process;
    int flip = 1;
    while (1) {
        sleep(rand() % 5 + 4);
        if (flip) {
            // Write a new process
            new_process.pid = PID++;
            new_process.size = (rand() % 1000 + 1 - 8) + 8;
            new_process.block = new_process.size / 8;
            new_process.score = currentTimeMillis() - 125;
            strcpy(new_process.data, planet_names[rand() % 8]);

            printf("[PROCESS][NEW] pid=%d size=%d pages=%d score=%d data=%s\n\n", new_process.pid,
                   new_process.size,
                   new_process.block, new_process.score, new_process.data);

            process_table[new_process.pid] = new_process;
            write_buffer = new_process;
            flip = 0;
            sem_post(&s0);
        } else {
            // Read an existing process
            int read_pid = (rand() % PID + 1 - 1) + 1;
            read_buffer = process_table[read_pid];
            printf("[PROCESS][READ] pid=%d\n\n", read_pid);
            flip = 1;
            sem_post(&s0);
        }
    }
}


void main(void) {
    struct timeval now;

    printf("MMU Simulator\n\n");

    srand(time(NULL));
    sem_init(&s0, 0, 0);
    pthread_t processor, mmu;

    mmu_init_process_table();
    mmu_init_pmem();

    (void) pthread_create(&processor, NULL, process_management, NULL);
    (void) pthread_create(&mmu, NULL, (void *(*)(void *)) mmu_management, NULL);

    (void) pthread_join((pthread_t) &processor, NULL);
    (void) pthread_join((pthread_t) &mmu_management, NULL);
}

#pragma clang diagnostic pop