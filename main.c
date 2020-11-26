// Lucas Marcon Zampieri - 1837873

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int PID = 1;

// Data
char *planet_names[] = {"Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"};

typedef struct {
    int pid;
    int size;
    int block;
    int date_start;
    char data[50];
} PROCESS;

// 64KB, divided in 8 block of 8KB
PROCESS mmu_pmem[8];

PROCESS process_table[124];

PROCESS empty_space = {.pid=0};

PROCESS write_buffer;
int read_buffer;

#define PMEM_ARR_SIZE sizeof(mmu_pmem) / sizeof(PROCESS)

sem_t s0;

// 64KB - Expressed here in Kb, 512
void mmu_init_pmem() {
    for (int i = 0; i < PMEM_ARR_SIZE; i++)
        mmu_pmem[i] = empty_space;
}

// 1MB
void mmu_init_process_table() {
    for (int i = 0; i < 124; i++)
        process_table[i] = empty_space;
}

int mmu_search_for_empty_frame(PROCESS *arr) {
    int i = 0;

    while (i < PMEM_ARR_SIZE) {
        if (arr[i].pid == empty_space.pid)
            return i;
        i++;
    }
    return -1;
}

int mmu_search_for_oldest_page(PROCESS *arr) {
    int oldest = (int) time(NULL);
    int oldest_pos = 0;
    for (int i = 0; i < PMEM_ARR_SIZE; i++) {
        if (arr[i].pid == empty_space.pid)
            continue;
        else if (oldest > arr[i].date_start) {
            oldest = arr[i].date_start;
            oldest_pos = i;
        }
    }
    return oldest_pos;
}


void mmu_free_mem(PROCESS memory[]) {
    int initial_pos = mmu_search_for_oldest_page(memory);
    printf("[MMU] Page Fault, free pid=%d date=%d p_mem_pos=%d\n", mmu_pmem[initial_pos].pid, mmu_pmem[initial_pos].date_start,
           initial_pos);
    mmu_pmem[initial_pos] = empty_space;
}

void mmu_load_mem(PROCESS new_process){
    // Store all blocks of process
    for (int i = 0; i < new_process.block; i++) {
        int empty_frame = mmu_search_for_empty_frame(mmu_pmem);

        if (empty_frame >= 0) {
            mmu_pmem[empty_frame] = new_process;
        } else {
            mmu_free_mem(mmu_pmem);
            mmu_pmem[mmu_search_for_empty_frame(mmu_pmem)] = new_process;
        }
        // Process Ages per cicle
        new_process.date_start++;
    }
}
void mmu_write_process(PROCESS new_process) {

    mmu_load_mem(new_process);

    printf("[MMU][WRITE] pid=%d data=%s\n\n", new_process.pid, new_process.data);
}

void mmu_read_process(int PID) {
    PROCESS new = process_table[PID];

    // Load to memory
    mmu_load_mem(new);

    printf("[MMU][READ] pid=%d data=%s\n\n", new.pid, new.data);
}

void mmu_management() {
    while (1) {
        // Wait for a new process to be created
        sem_wait(&s0);
        if (write_buffer.pid != empty_space.pid) {
            mmu_write_process(write_buffer);
            write_buffer = empty_space;
        }
        if (read_buffer != empty_space.pid) {
            mmu_read_process(read_buffer);
            read_buffer = empty_space.pid;
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
            new_process.size = rand() % 1000;
            new_process.block = new_process.size / 8;
            new_process.date_start = (int) time(NULL) / 2;
            strcpy(new_process.data, planet_names[rand() % 8]);

            printf("[PROCESS][NEW] pid=%d size=%d pages=%d date_start=%d data=%s\n\n", new_process.pid, new_process.size,
                   new_process.block, new_process.date_start, new_process.data);

            process_table[new_process.pid] = new_process;
            write_buffer = new_process;
            flip = 0;
            sem_post(&s0);
        } else {
            // Read an existing process
            read_buffer = (rand() % PID + 1 - 1) + 1;
            printf("[PROCESS][READ] pid=%d\n\n",read_buffer);
            flip = 1;
            sem_post(&s0);
        }
    }
}

void main(void) {
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