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

int PID = 2;

// Data
char *planet_names[] = {"Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"};

typedef struct {
    int pid;
    int size;
    int date_start;
    char data[50];
} PROCESS;

// 64KB, divided in 8 frames of 8KB
PROCESS mmu_pmem[8];

// 1MB, divided in 124 pages of 8KB
PROCESS mmu_vmem[124];

PROCESS empty_space = {.pid=0};
PROCESS used_space = {.pid=1};
PROCESS buffer;

#define PMEM_ARR_SIZE sizeof(mmu_pmem) / sizeof(PROCESS)
#define VMEM_ARR_SIZE sizeof(mmu_vmem) / sizeof(PROCESS)

sem_t s0;

// 64KB - Expressed here in Kb, 512
void mmu_init_pmem() {
    for (int i = 0; i < PMEM_ARR_SIZE; i++)
        mmu_pmem[i] = empty_space;
}

// 1MB
void mmu_init_vmem() {
    for (int i = 0; i < VMEM_ARR_SIZE; i++)
        mmu_vmem[i] = empty_space;
}

int mmu_search_for_empty_block(PROCESS arr[], int arr_size, int new_process_block_size) {
    int i = 0, j = 0;
    while (i <= arr_size) {
        if (arr[i].pid == empty_space.pid) {
            if (j == new_process_block_size) {
//                printf("Found empty block of %d from %d ~ %d \n", j, i - new_process_block_size, i);
                return i - new_process_block_size;
            }

            j++;
        } else
            j = 0;
        i++;
    }
    return -1;
}

void mmu_store_process(PROCESS arr[], int arr_size, PROCESS new_process, int initial_position) {
    for (int i = initial_position; i < initial_position + (new_process.size / 8); i++)
        mmu_vmem[i] = new_process;
}

int mmu_search_for_oldest_block(PROCESS *arr, int arr_size) {
    int oldest = (int) time(NULL);
    int oldest_pos = 0;
    for (int i = 0; i < arr_size; i++) {
        if (arr[i].pid == empty_space.pid)
            continue;
        else if (oldest > arr[i].date_start) {
            oldest = arr[i].date_start;
            oldest_pos = i;
        }
    }
    return oldest_pos;
}


void mmu_free_mem(PROCESS memory[], int memory_size, PROCESS new_process) {

    // While there's not enough space for the new process
    while (mmu_search_for_empty_block(memory, memory_size, new_process.size / 8) < 0) {
        int initial_pos = mmu_search_for_oldest_block(memory, memory_size);

        PROCESS process = memory[initial_pos];

        printf("[FREE] pid=%d creation=%d from=%d~%d\n", process.pid, process.date_start,
               initial_pos, initial_pos + (process.size / 8));

        for (int i = initial_pos; i < initial_pos + (process.size / 8); i++)
            memory[i] = empty_space;
    }
}

void mmu_new_process(PROCESS new_process) {
    int vmem_initial_position = 0, pmem_initial_position = 0;

    // If theres not enough PAGES, free some
    if (mmu_search_for_empty_block(mmu_vmem, VMEM_ARR_SIZE, new_process.size / 8) < 0) {
        printf("VMEM Page Fault - Removing the oldest\n");
        mmu_free_mem(mmu_vmem, VMEM_ARR_SIZE, new_process);
    }

    vmem_initial_position = mmu_search_for_empty_block(mmu_vmem, VMEM_ARR_SIZE, new_process.size / 8);
    mmu_store_process(mmu_vmem, VMEM_ARR_SIZE, new_process, vmem_initial_position);

// TODO PMEM
//    pmem_initial_position = mmu_search_for_empty_block(mmu_pmem, PMEM_ARR_SIZE, new_process.size / 8);
//    mmu_store_process(new_process, pmem_initial_position);

    printf("[MMU] New Process stored vmem_pages=%d~%d\n", vmem_initial_position,
           vmem_initial_position + new_process.size / 8);
}

void mmu_management() {
    while (1) {
        // Wait for a new process to be created
        sem_wait(&s0);
        mmu_new_process(buffer);
        buffer = empty_space;
    }
}

void mmu_printf(PROCESS new_process) {
    printf("[PMEM] PID=%d DATA=%d\n", new_process.pid, new_process.data);
}

// min 1 byte max 1MB
void *Processor() {
    PROCESS new_process;
    while (1) {
        sleep(rand() % 5 + 4);

        new_process.pid = PID++;
        new_process.size = rand() % 1024;
        new_process.date_start = (int) time(NULL); // Use Epoch time
        strcpy(new_process.data, planet_names[rand() % 8]);

        printf("\n[NEW] pid=%d size=%d block=%d date_start=%d data=%s\n", new_process.pid, new_process.size,
               new_process.size / 8, new_process.date_start, new_process.data);

        buffer = new_process;
        sem_post(&s0);
    }
}

void main(void) {
    printf("MMU Simulator\n\n");
    srand(time(NULL));
    sem_init(&s0, 0, 0);
    pthread_t processor, mmu;

    mmu_init_vmem();
    mmu_init_pmem();

    (void) pthread_create(&processor, NULL, Processor, NULL);
    (void) pthread_create(&mmu, NULL, mmu_management, NULL);

    (void) pthread_join(&processor, NULL);
    (void) pthread_join(&mmu_management, NULL);
}

#pragma clang diagnostic pop