/**
 * @file    BlockQueue.hpp
 * @author  XTU@Kzzz3
 * @brief   封装一个线程安全队列
 * @version 0.1
 * @date    2022-04-13
 *
 * @copyright Copyright (c) 2022 Kzzz3
 */
#ifndef BLOCKQUEUE_HPP
#define BLOCKQUEUE_HPP

#include <queue>
#include <mutex>
#include <semaphore>

/**
 * @brief 封装一个线程安全队列  (gcc的模板类不允许分文件编写)
 *
 * @tparam T 存储数据类型
 */
template <class T>
class BlockQueue
{
public:
    std::mutex _rw_mutex;                        // 队列的读写锁
    std::queue<T> _task_queue;                   // 任务队列
    std::counting_semaphore<INT32_MAX> _cnt_sem; // 阻塞外部对队列中资源请求

public:
    BlockQueue();
    ~BlockQueue();
    T pop();
    int size();
    bool empty();
    void clear();
    void push(T job);
};

/**
 * @brief Construct a new Safe Queue< T>:: Safe Queue object
 *
 * @tparam T 存储数据类型
 */
template <class T>
BlockQueue<T>::BlockQueue() : _cnt_sem(0) {}

/**
 * @brief Destroy the Safe Queue< T>:: Safe Queue object
 *
 * @tparam T
 */
template <class T>
BlockQueue<T>::~BlockQueue() {}

/**
 * @brief       向队列中存入一个数据
 *
 * @tparam T    存储数据类型
 * @param job   数据对象
 */
template <class T>
void BlockQueue<T>::push(T job)
{
    std::lock_guard<std::mutex> lock(_rw_mutex);

    _task_queue.push(job);
    _cnt_sem.release();
}

/**
 * @brief      从队列中弹出一个数据
 *
 * @tparam T   存储数据类型
 * @return T   数据对象
 */
template <class T>
T BlockQueue<T>::pop()
{
    _cnt_sem.acquire();
    std::lock_guard<std::mutex> lock(_rw_mutex);

    T retJob = nullptr;

    retJob = _task_queue.front();
    _task_queue.pop();

    return retJob;
}

/**
 * @brief           对立是否为空
 *
 * @tparam T        存储数据类型
 * @return true     队列空
 * @return false    队列非空
 */
template <class T>
bool BlockQueue<T>::empty()
{
    std::lock_guard<std::mutex> lock(_rw_mutex);
    return _task_queue.empty();
}

/**
 * @brief           返回队列数据数量
 *
 * @tparam T        存储数据类型
 * @return int      数据数量
 */
template <class T>
int BlockQueue<T>::size()
{
    std::lock_guard<std::mutex> lock(_rw_mutex);
    return _task_queue.size();
}

#endif // BLOCKQUEUE_HPP