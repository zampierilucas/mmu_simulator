// Lucas Marcon Zampieri - 1837873

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int PID = 2;

// Data
char *planet_names[] = {"Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"};

typedef struct {
    int pid;
    int value;
    int size;
    int date_start;
    int date_expiration;
    char data[50];
} PROCESS;

// 64KB, divided in 8 frames
PROCESS mmu_pmem[8];
// 1MB, divided in 1000 pages
PROCESS mmu_vmem[1000];

PROCESS empty_space = {.pid=0};
PROCESS used_space = {.pid=1};

#define PMEM_ARR_SIZE sizeof(mmu_pmem) / sizeof(PROCESS)
#define VMEM_ARR_SIZE sizeof(mmu_vmem) / sizeof(PROCESS)

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

//
int mmu_vmem_search_for_gap(int size) {
    int search = 1, i = 0;
    while (search && i < VMEM_ARR_SIZE) {
        if (mmu_vmem[i].pid == empty_space.pid) {
            int j = 0;
            while (j < size) {
                if (mmu_vmem[i].pid != empty_space.pid)
                    break;
                j++;
            }
            if (j >= size) {
                printf("Found empty block of %d at %d\n", size, (i+j) - size);
                return i;
            }
        }
        i++;
    }
    printf("No gap found\n");
}

void mmu_vmem_process_store(PROCESS write_to_vmem, int initial_position) {
    mmu_vmem[initial_position] = write_to_vmem;
    for (int i = initial_position + 1; i < initial_position + write_to_vmem.size; i++)
        mmu_vmem[i] = used_space;
}

void mmu_vmem_process_free(PROCESS write_to_vmem, int initial_position) {
    for (int i = initial_position; i < initial_position + write_to_vmem.size; i++)
        mmu_vmem[i] = empty_space;
}

void mmu_new_process(PROCESS new_process) {
    int initial_position = 0;

    initial_position = mmu_vmem_search_for_gap(new_process.size);
    mmu_vmem_process_store(new_process, initial_position);

}



//void mmu_vmem_manager() {
//    // if 10 sendes have passes
////    mmu_vmem_remove_unused()
//}

// min 1 byte max 1MB
void node() {
    PROCESS new_process;

    new_process.pid = PID++;
    new_process.value = rand() % 10;
    new_process.size = rand() % 1024;
    new_process.date_start = (int) time(NULL);
    new_process.date_expiration = new_process.date_start + 60;
    strcpy(new_process.data, planet_names[rand() % 8]);

    printf("[NEW] pid=%d value=%d size=%d date_start=%d data=%s\n", \
    new_process.pid, new_process.value, new_process.size, new_process.date_start, new_process.data);

    mmu_new_process(new_process);
}

void *node_mngr(void *id) {
    while (1) {
        printf("%s - Creating new process\n", (char *) id);
        sleep(rand() % 5 + 2);
        node();
    }
}

void main(void) {
    pthread_t processor_1, processor_2, mmu;

    mmu_init_pmem();
    mmu_init_vmem();

    (void) pthread_create(&processor_1, NULL, node_mngr, "1");
//    (void) pthread_create(&t2, NULL, node_mngr, "2");
//    (void) pthread_create(&mmu, NULL, mmu_vmem_manager, "2");

    (void) pthread_join(&processor_1, NULL);
    (void) pthread_join(&mmu, NULL);
//    (void) pthread_join(&t2, NULL);
}

#pragma clang diagnostic pop