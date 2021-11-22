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
    uint8_t virtual_time;

    TEST();
    ~TEST();
    void Sync(uint16_t);

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
    uint8_t timer = 10;
    TEST *test = new TEST;

    while (timer--) {
        const uint16_t delay = 1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        test->Sync(delay);
    }
    printf("Time is up\n");
    getchar();
    return 0;
}


TEST::TEST() {
    virtual_time = 0;
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

void TEST::Sync(uint16_t elapsed_time) {
    printf("Synchronization! Virtual time equals %d\n", ++virtual_time);
#if SYNC == TICKS
    TM_->Sync();
#elif SYNC == SECONDS
    TM_->Sync(elapsed_time);
#else
    #error NOT implemented
#endif // SYNC
}

void TEST::thread_1_thr_func( ) {
    printf("THR_1: Wait 1 sec\n");
    this->TM_->wait_in_ticks(1);
    printf("THR_1: Wait 6 sec\n");
    this->TM_->wait_in_ticks(6);
    printf("THR_1: Finish\n");
}

void TEST::thread_2_thr_func() {
    printf("THR_2: Wait 5 sec\n");
    this->TM_->wait_in_ticks(5);
    printf("THR_2: Finish\n");
}

void TEST::thread_3_thr_func() {
    printf("THR_3: Wait 9 sec\n");
    this->TM_->wait_in_ticks(9);
    printf("THR_3: Finish\n");
}
