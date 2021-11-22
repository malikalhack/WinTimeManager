#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

/****************************** Included files ********************************/
#include <stdint.h>
#include <thread>
#include <functional>
/******************************** Definition **********************************/
#define TICKS   0
#define SECONDS 1
#define SYNC TICKS
#define BUF_SIZE 16
#if ( (BUF_SIZE & BUF_SIZE - 1) || (BUF_SIZE < 16) )
    #error BUF_SIZE must be equal or greater than 16 and must be a power of two. 
#endif // check BUF_SIZE
#define MAX_ID (BUF_SIZE / 16)

typedef void* semaphore_t;

struct delay_t {
    uint16_t cur_time;
    uint16_t wait_time;
    bool is_blocked;
};

struct prcs{
    std::thread thr;
    std::thread::id id;
    struct delay_t delay;
};

/******************************* Class definition *****************************/
class TimeManager {
public:
    TimeManager();
    ~TimeManager();

#if SYNC == TICKS
    void Sync();
#elif SYNC == SECONDS
    void Sync(uint32_t);
#else
    #error NOT implemented
#endif // SYNC

    void wait_in_ticks(uint32_t);
    bool AddPrcs(uint8_t, std::function<void()>);
    void KillPrcs(uint8_t, bool force = false);
    std::thread::id GetPrcsId(uint8_t);

private:
    volatile uint32_t sync_tick_;
    bool cancel_;
    semaphore_t list_access_;
    semaphore_t sync_tick_access_;

    prcs prcs_list_[BUF_SIZE];
    semaphore_t prcs_blocks_[BUF_SIZE];
    std::thread::id prcs_id_[BUF_SIZE];

    void Init_();
    semaphore_t CreateBinSemaphore_(bool blocked = false);
    uint8_t get_dscr_(std::thread::id);
    uint16_t get_time_();
};

#endif // !TIME_MANAGER_H
