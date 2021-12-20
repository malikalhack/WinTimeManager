/**
 * @file    TimeManager.cpp
 * @version 2.0.0
 * @authors Anton Chernov
 * @date    19/10/2021
 * @date    24/11/2021
 */

/****************************** Included files ********************************/
#include <stdio.h>
#include <Windows.h>
#include "TimeManager.h"
/******************************************************************************/
static CRITICAL_SECTION  list_access_;     ///< Exclusive access to the list
static CRITICAL_SECTION  sync_tick_access_;///< Exclusive access to the sync counter

TimeManager::TimeManager() {
    cancel_ = false;
    sync_tick_ = 0;
    Init_();
    InitializeCriticalSectionAndSpinCount(&list_access_, 1024);
    InitializeCriticalSectionAndSpinCount(&sync_tick_access_, 1024);
    start_allowed_ = CreateTmEvent_(L"Start");
    for (uint8_t i = 0; i < BUF_SIZE; i++) {
        prcs_blocks_[i] = CreateTmEvent_();
    }
}
/*----------------------------------------------------------------------------*/
TimeManager::~TimeManager() {
    cancel_ = true;
    Sleep(600);
    CloseTmEvent_(start_allowed_);
    for (uint8_t i = 0; i < BUF_SIZE; i++) {
        CloseTmEvent_(prcs_blocks_[i]);
    }
    DeleteCriticalSection(&list_access_);
    DeleteCriticalSection(&sync_tick_access_);
}
/*----------------------------------------------------------------------------*/
#if SYNC == TICKS
    void TimeManager::Sync() {
        EnterCriticalSection(&sync_tick_access_);
        uint16_t curr_time = ++sync_tick_;
#elif SYNC == MILLISECONDS
    void TimeManager::Sync(uint32_t elapsed_time) {
        EnterCriticalSection(&sync_tick_access_);
        {
            uint32_t remainder = UINT32_MAX - sync_tick_;
            if (elapsed_time >= remainder) {
                elapsed_time -= remainder;
                sync_tick_ = elapsed_time;
            }
            else {
                sync_tick_ += elapsed_time;
            }
        }
        uint32_t curr_time = sync_tick_;
#else
    #error Type of synchronization is unknown
#endif // SYNC
        LeaveCriticalSection(&sync_tick_access_);
        for (uint8_t i = 0; i < BUF_SIZE; i++) {
            if (prcs_list_[i].delay.is_blocked) {
                if (curr_time > prcs_list_[i].delay.cur_time) {
                    if (curr_time - prcs_list_[i].delay.cur_time >= prcs_list_[i].delay.wait_time) {
                        SetEvent(prcs_blocks_[i]);
                    }
                }
                else {
                    uint32_t res = UINT32_MAX - prcs_list_[i].delay.cur_time + prcs_list_[i].delay.cur_time;
                    if (res >= prcs_list_[i].delay.wait_time) {
                        SetEvent(prcs_blocks_[i]);
                    }
                }
            }
        }
    }
/*----------------------------------------------------------------------------*/
uint16_t TimeManager::get_time_() {
    EnterCriticalSection(&sync_tick_access_);
    uint16_t result = sync_tick_;
    LeaveCriticalSection(&sync_tick_access_);
    return result;
}
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
std::thread::id TimeManager::GetPrcsId(uint8_t dscr) {
    EnterCriticalSection(&list_access_);
    std::thread::id result = prcs_list_[dscr].id;
    LeaveCriticalSection(&list_access_);
    return result;
}
/*----------------------------------------------------------------------------*/
void TimeManager::wait_for_start() {
    WaitForSingleObject(start_allowed_, INFINITE);
}
/*----------------------------------------------------------------------------*/
void TimeManager::wait_in_ticks(uint32_t delay) {
    bool repeat = true;
    DWORD dwWaitResult;
    uint16_t cur_time = get_time_();
    uint8_t dscr = get_dscr_(std::this_thread::get_id());

    prcs_list_[dscr].delay.cur_time = cur_time;
#if SYNC == TICKS
    prcs_list_[dscr].delay.wait_time = delay;
#elif SYNC == MILLISECONDS
    prcs_list_[dscr].delay.wait_time = delay * MILLISECONDS_PER_TICK;
#else
    #error Type of synchronization is unknown
#endif // SYNC
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
/*----------------------------------------------------------------------------*/
void TimeManager::Init_() {
    prcs_id_[0] = std::this_thread::get_id();
    for (uint8_t index = 0; index < BUF_SIZE; index++) {
        prcs_list_[index].delay.cur_time = 0u;
        prcs_list_[index].delay.wait_time = 0u;
        prcs_list_[index].delay.is_blocked = false;
    }
}
/*----------------------------------------------------------------------------*/
event_t TimeManager::CreateTmEvent_(const wchar_t * name) {
    event_t link = CreateEvent(
        NULL,   // default security attributes
        FALSE,  // auto-reset event
        FALSE,  // initial state is nonsignaled
        name    // name of event
    );
    if (link == NULL) printf("Error create event\n");
    return link;
}
/*----------------------------------------------------------------------------*/
void TimeManager::CloseTmEvent_(event_t evt) {
    CloseHandle(evt);
}
/*----------------------------------------------------------------------------*/
bool TimeManager::AddPrcs(uint8_t dscr,  std::function<void()> func) {
    bool result = false;
    if (dscr) {
        EnterCriticalSection(&list_access_);
        if (dscr < BUF_SIZE - 1) {
            ResetEvent(start_allowed_);
            result = true;
            prcs_list_[dscr].thr = std::thread(func);
            prcs_id_[dscr] = prcs_list_[dscr].thr.get_id();
            SetEvent(start_allowed_);
        } else {
            printf("There are no room here\n");
        }
        LeaveCriticalSection(&list_access_);
    } else {
        printf("Reserved by main thread\n");
    }
    return result;
}
/*----------------------------------------------------------------------------*/
void TimeManager::KillPrcs(uint8_t dscr, bool force) {
    EnterCriticalSection(&list_access_);
    cancel_ = true;
    if (force) {
        prcs_list_[dscr].thr.detach();
    }
    else {
        prcs_list_[dscr].thr.join();
    }
    LeaveCriticalSection(&list_access_);
}
/******************************************************************************/
