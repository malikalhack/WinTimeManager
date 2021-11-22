#include <stdio.h>
#include <Windows.h>
#include "TimeManager.h"

TimeManager::TimeManager() {
    cancel_ = false;
    sync_tick_ = 0;
    Init_();
    list_access_ = CreateBinSemaphore_();
    sync_tick_access_ = CreateBinSemaphore_();
    for (uint8_t i = 0; i < BUF_SIZE; i++) {
        prcs_blocks_[i] = CreateBinSemaphore_(true);
    }
}

TimeManager::~TimeManager() {
    cancel_ = true;
    Sleep(600);
    for (uint8_t i = 0; i < BUF_SIZE; i++) {
        CloseHandle(prcs_blocks_[i]);
    }
    CloseHandle(list_access_);
    CloseHandle(sync_tick_access_);
}

#if SYNC == TICKS
    void TimeManager::Sync() {
        WaitForSingleObject(sync_tick_access_, INFINITE);
        uint16_t curr_time = ++sync_tick_;
#elif SYNC == SECONDS
    void TimeManager::Sync(uint32_t elapsed_time) {
        sync_tick_access.lock();
        {
            uint16_t remainder = UINT16_MAX - sync_tick_;
            if (elapsed_time >= remainder) {
                elapsed_time -= remainder;
                sync_tick_ = elapsed_time;
            }
            else {
                sync_tick_ += elapsed_time;
            }
        }
        uint16_t curr_time = sync_tick_;
#else
    #error NOT implemented
#endif // SYNC
        ReleaseSemaphore(sync_tick_access_, 1, NULL);
        for (uint8_t i = 0; i < BUF_SIZE; i++) {
            if (prcs_list_[i].delay.is_blocked) {
                if (curr_time > prcs_list_[i].delay.cur_time) {
                    if (curr_time - prcs_list_[i].delay.cur_time >= prcs_list_[i].delay.wait_time) {
                        ReleaseSemaphore(prcs_blocks_[i], 1, NULL);
                    }
                }
                else {
                    uint16_t res = UINT32_MAX - prcs_list_[i].delay.cur_time + prcs_list_[i].delay.cur_time;
                    if (res >= prcs_list_[i].delay.wait_time) {
                        ReleaseSemaphore(prcs_blocks_[i], 1, NULL);
                    }
                }
            }
        }
    }

uint16_t TimeManager::get_time_() {
    WaitForSingleObject(sync_tick_access_, INFINITE);
    uint16_t result = sync_tick_;
    ReleaseSemaphore(sync_tick_access_, 1, NULL);
    return result;
}

uint8_t TimeManager::get_dscr_(std::thread::id id) {
    uint8_t result = 0;
    bool found = false;
    for (uint8_t index = 0; index < BUF_SIZE; index++) {
        if (prcs_id_[index] == id) {
            result = index;
            found = true;
            break;
        }
    }
    if (!found) printf("No identifier found.\n");
    return result;
}

std::thread::id TimeManager::GetPrcsId(uint8_t dscr) {
    WaitForSingleObject(list_access_, INFINITE);
    std::thread::id result = prcs_list_[dscr].id;
    ReleaseSemaphore(list_access_, 1, NULL);
    return result;
}

void TimeManager::wait_in_ticks(uint32_t delay) {
    bool repeat = true;
    DWORD dwWaitResult;
    uint16_t cur_time = get_time_();
    uint8_t dscr = get_dscr_(std::this_thread::get_id());

    prcs_list_[dscr].delay.cur_time = cur_time;
    prcs_list_[dscr].delay.wait_time = delay;
    prcs_list_[dscr].delay.is_blocked = true;
    do {
        dwWaitResult = WaitForSingleObject(
            prcs_blocks_[dscr],
            500 //in milliseconds
        );
        switch (dwWaitResult) {
            case WAIT_OBJECT_0:
                repeat = false;
            case WAIT_TIMEOUT:
                break;
            case WAIT_FAILED:
                printf("Something went wrong. Error code %ld\n", GetLastError());
                break;
            default: 
                printf("Error! Unknown result\n");
        }
    } while (repeat && !cancel_);
    prcs_list_[dscr].delay.is_blocked = false;
}

void TimeManager::Init_() {
    prcs_id_[0] = std::this_thread::get_id();
    for (uint8_t index = 0; index < BUF_SIZE; index++) {
        prcs_list_[index].delay.is_blocked = true;
    }
}

semaphore_t TimeManager::CreateBinSemaphore_(bool blocked) {
    uint8_t initial_count = (blocked) ? 0 : 1;
    semaphore_t link = CreateSemaphore(
        NULL,   // default security attributes
        initial_count,
        1,      // maximum count
        NULL    // unnamed semaphore
    );
    if (link == NULL) printf("Error create semaphore\n");
    return link;
}

bool TimeManager::AddPrcs(uint8_t dscr,  std::function<void()> func) {
    bool result = false;
    if (dscr) {
        WaitForSingleObject(list_access_, INFINITE);
        if (dscr < BUF_SIZE - 1) {
            prcs_list_[dscr].thr = std::thread(func);
            prcs_id_[dscr] = prcs_list_[dscr].thr.get_id();
            result = true;
        } else {
            printf("There are no room here\n");
        }
        ReleaseSemaphore(list_access_, 1, NULL);
    } else {
        printf("Reserved by main thread\n");
    }
    return result;
}

void TimeManager::KillPrcs(uint8_t dscr, bool force) {
    WaitForSingleObject(list_access_, INFINITE);
    cancel_ = true;
    if (force) {
        prcs_list_[dscr].thr.detach();
    }
    else {
        prcs_list_[dscr].thr.join();
    }
    ReleaseSemaphore(list_access_, 1, NULL);
}
