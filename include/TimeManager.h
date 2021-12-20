/**
 * @file    TimeManager.h
 * @version 2.0.0
 * @authors Anton Chernov
 * @date    19/10/2021
 * @date    24/11/2021
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

/****************************** Included files ********************************/
#include <stdint.h>
#include <thread>
#include <functional>
/******************************** Definition **********************************/
#define BUF_SIZE                16u
#define MILLISECONDS_PER_TICK   10u
#define TICKS_PER_SECOND        1000u / MILLISECONDS_PER_TICK
#define TICKS                   0u
#define MILLISECONDS            1u
#define SYNC                    MILLISECONDS

typedef void* event_t;

struct delay_t {
    uint32_t cur_time;
    uint32_t wait_time;
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
/**
 @brief Synchronization function
 @fn void TimeManager::Sync()
 */
    void Sync();
#elif SYNC == MILLISECONDS
/**
 @brief Synchronization function
 @fn void TimeManager::Sync(uint32_t elapsed_time)
 @param[in] elapsed_time passed argument of elapsed time
 */
    void Sync(uint32_t);
#else
    #error Type of synchronization is unknown
#endif // SYNC

/**
 * @brief Public method to suspend execution of a thread until its ID is obtained
 * @fn void TimeManager::wait_for_start()
 */
    void wait_for_start();

/**
 * @brief Public method to stop a thread and waiting for the counter
 * @fn void TimeManager::wait_in_ticks(uint32_t delay)
 * @param[in] delay time to sleep (in ticks)
 */
    void wait_in_ticks(uint32_t);

/**
 * @brief Public method to add a process to prcs_list_ array.
 * @fn void TimeManager::AddPrcs(uint8_t dscr, std::function<void()> func)
 * @param[in] dscr - description of the thread from the descriptor-enumerator.
 * @param[in] func - function for thread.
 */
    bool AddPrcs(uint8_t, std::function<void()>);

/**
 * @brief Public method to remove a process from prcs_list_ array.
 * @fn void TimeManager::KillPrcs(uint8_t dscr)
 * @param[in] dscr - description of the thread from the descriptor-enumerator.
 * @param[in] force - flag that define the end of a thread: false - join, true - detach.
 */
    void KillPrcs(uint8_t, bool force = false);

/**
 * @brief Public method for getting the thread identifier.
 * @fn std::thread::id TimeManager::GetPrcsId(uint8_t dscr)
 * @return the thread identifier
 */
    std::thread::id GetPrcsId(uint8_t);

private:
    volatile uint32_t sync_tick_;       ///< Sync counter
    bool cancel_;                       ///< Cancel waiting for a sync object
    event_t start_allowed_;             ///< Event to start the thread

    prcs prcs_list_[BUF_SIZE];          ///< Array of process objects
    event_t prcs_blocks_[BUF_SIZE];     ///< Array of synchronization objects
    std::thread::id prcs_id_[BUF_SIZE]; ///< Array of process IDs

    void Init_();
    void CloseTmEvent_(event_t);
    event_t CreateTmEvent_(const wchar_t* name = NULL);
    uint8_t get_dscr_(std::thread::id);
    uint16_t get_time_();
};
/******************************************************************************/
#endif // !TIME_MANAGER_H
