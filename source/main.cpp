#include <stdio.h>
#include <functional>
#include "TimeManager.h"
#include <chrono>

enum descriptors {
    MAIN_THREAD = 0,
    THREAD_1,
    THREAD_2,
    THREAD_3,
    MAX_DSCR
};

class TEST {
public:
    uint32_t virtual_time;
    uint8_t virtual_seconds;

    TEST();
    ~TEST();
    void Sync(uint32_t);

private:
    TimeManager *TM_;

    void thread_1_thr_func(void);
    void thread_2_thr_func(void);
    void thread_3_thr_func(void);

    std::function< void() > f_func_1 = [this]() { this->thread_1_thr_func(); };
    std::function< void() > f_func_2 = [this]() { this->thread_2_thr_func(); };
    std::function< void() > f_func_3 = [this]() { this->thread_3_thr_func(); };
};

int main() {
    uint8_t timer = 10; //in seconds
    TEST *test = new TEST;

    while (timer--) {
#if SYNC == TICKS
        for (uint8_t i = 0; i < TICKS_PER_SECOND; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(MILLISECONDS_PER_TICK)); 
            test->Sync(MILLISECONDS_PER_TICK);
        }
#elif SYNC == MILLISECONDS
        const uint16_t delay = 1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        test->Sync(delay);
#else
    #error Type of synchronization is unknown
#endif // SYNC
    }
    printf("Time is up\n");
    getchar();
    return 0;
}


TEST::TEST() {
    virtual_time = 0;
    virtual_seconds = 0;
    TM_ = new TimeManager();
    TM_->AddPrcs(THREAD_1, f_func_1);
    TM_->AddPrcs(THREAD_2, f_func_2);
    TM_->AddPrcs(THREAD_3, f_func_3);
}

TEST::~TEST() {
    TM_->KillPrcs(THREAD_1);
    TM_->KillPrcs(THREAD_2);
    TM_->KillPrcs(THREAD_3);
    delete TM_;
}

void TEST::Sync(uint32_t elapsed_time) {
#if SYNC == TICKS
    virtual_time += elapsed_time;
    while (virtual_time >= 1000u) {
        virtual_time -= 1000u;
        printf("Synchronization! Virtual time equals %d\n", ++virtual_seconds);
    }
    TM_->Sync();
#elif SYNC == MILLISECONDS
    printf("Synchronization! Virtual time equals %d\n", ++virtual_seconds);
    TM_->Sync(elapsed_time);
#else
    #error Type of synchronization is unknown
#endif // SYNC
}

void TEST::thread_1_thr_func( ) {
    printf("THR_1: Wait 1 sec\n");
    this->TM_->wait_in_ticks(1 * TICKS_PER_SECOND);
    printf("THR_1: Wait 6 sec\n");
    this->TM_->wait_in_ticks(6 * TICKS_PER_SECOND);
    printf("THR_1: Finish\n");
}

void TEST::thread_2_thr_func() {
    printf("THR_2: Wait 5 sec\n");
    this->TM_->wait_in_ticks(5 * TICKS_PER_SECOND);
    printf("THR_2: Wait 3 sec\n");
    this->TM_->wait_in_ticks(3 * TICKS_PER_SECOND);
    printf("THR_2: Finish\n");
}

void TEST::thread_3_thr_func() {
    printf("THR_3: Wait 9 sec\n");
    this->TM_->wait_in_ticks(9 * TICKS_PER_SECOND);
    printf("THR_3: Finish\n");
}
