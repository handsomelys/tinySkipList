#include <iostream>
#include <chrono>
// chrono 是cpp 11 的时间库，最高可达纳秒级别精度
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include "../skiplist.h"

#define NUM_THREADS 2
#define TEST_COUNT 100000
SkipList<int, std::string> skipList(18);

void *insertElement(void* threadid) {
    long tid;
    tid = (long)threadid;
    std::cout << tid << std::endl;
    int tmp = TEST_COUNT / NUM_THREADS;
    for (int i = tid * tmp, count = 0; count < tmp; i++) {
        count++;
        skipList.insert_element(rand() % TEST_COUNT, "a");
    }
    pthread_exit(NULL);
}

void *getElement(void* threadid) {
    long tid;
    tid = (long)threadid;
    std::cout << tid << std::endl;
    // 每个线程承担的任务量
    // 感觉这个i其实没什么卵用
    int tmp = TEST_COUNT / NUM_THREADS;
    for (int i = tid * tmp, count = 0; count < tmp; i++) {
        count++;
        skipList.search_element(rand() % TEST_COUNT);
    }
    pthread_exit(NULL);
}

int main() {
    srand (time(NULL));
    {
        pthread_t threads[NUM_THREADS];
        int rc;
        int i;
        auto start = std::chrono::high_resolution_clock::now();

        for (i = 0; i < NUM_THREADS; i++) {
            std::cout << "main() : creating thread, " << i << std::endl;
            rc = pthread_create(&threads[i], NULL, insertElement, (void *)i);

            if (rc) {
                std::cout << "Error: unable to create thread, " << rc << std::endl;
                exit(-1);
            }
        }

        void *ret;
        for (i = 0; i < NUM_THREADS; i++) {
            if (pthread_join(threads[i], &ret) != 0) {
                perror("pthread_create() error");
                exit(3);
            }
        }
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << "insert elapsed:" << elapsed.count() << std::endl;
    }
    pthread_exit(NULL);
    return 0;
}