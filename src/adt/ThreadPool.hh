#pragma once

#include <atomic>
#include <threads.h>

#include "Queue.hh"

#ifdef __linux__
    #include <sys/sysinfo.h>
    #define getLogicalCoresCount() get_nprocs()
#elif _WIN32
    #include <windows.h>
    #include <sysinfoapi.h>

inline DWORD
getLogicalCoresCountWIN32()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

    #define getLogicalCoresCount() getLogicalCoresCountWIN32()
#else
    #define getLogicalCoresCount() 4
#endif

namespace adt
{

struct TaskNode
{
    thrd_start_t pfn;
    void* pArgs;
};

struct ThreadPool
{
    Allocator* _pAlloc {};
    Queue<TaskNode> _qTasks;
    thrd_t* _pThreads {};
    u32 _threadCount {};
    cnd_t _cndQ, _cndWait;
    mtx_t _mtxQ, _mtxWait;
    std::atomic<int> _activeTaskCount;
    bool _bDone {};

    ThreadPool() = default;
    ThreadPool(Allocator* p, u32 _threadCount);
    ThreadPool(Allocator* p);

    void start();
    bool busy();
    void submit(thrd_start_t pfnTask, void* pArgs) { submit({pfnTask, pArgs}); }
    void submit(TaskNode task);
    void wait();
    void destroy();

private:
    void stop();
    static int loop(void* _self);
};

inline
ThreadPool::ThreadPool(Allocator* p, u32 _threadCount)
    : _pAlloc(p), _qTasks(p, _threadCount), _threadCount(_threadCount), _activeTaskCount(0), _bDone(false)
{
    _pThreads = (thrd_t*)p->alloc(_threadCount, sizeof(thrd_t));
    cnd_init(&_cndQ);
    mtx_init(&_mtxQ, mtx_plain);
    cnd_init(&_cndWait);
    mtx_init(&_mtxWait, mtx_plain);
}

inline
ThreadPool::ThreadPool(Allocator* p)
    : ThreadPool(p, getLogicalCoresCount()) {}

inline void
ThreadPool::start()
{
    for (size_t i = 0; i < _threadCount; i++)
        thrd_create(&_pThreads[i], ThreadPool::loop, this);
}

inline bool
ThreadPool::busy()
{
    mtx_lock(&_mtxQ);
    bool ret = !_qTasks.empty() || _activeTaskCount > 0;
    mtx_unlock(&_mtxQ);

    return ret;
}

inline int
ThreadPool::loop(void* p)
{
    auto* self = (ThreadPool*)p;

    while (!self->_bDone)
    {
        TaskNode task;
        {
            mtx_lock(&self->_mtxQ);

            while (!(!self->_qTasks.empty() || self->_bDone))
                cnd_wait(&self->_cndQ, &self->_mtxQ);

            if (self->_bDone)
            {
                mtx_unlock(&self->_mtxQ);
                return thrd_success;
            }

            task = *self->_qTasks.popFront();
            self->_activeTaskCount++; /* increment before unlocking mtxQ to avoid 0 tasks and 0 q possibility */

            mtx_unlock(&self->_mtxQ);
        }

        task.pfn(task.pArgs);
        self->_activeTaskCount--;

        if (!self->busy())
            cnd_signal(&self->_cndWait);
    }

    return thrd_success;
}

inline void
ThreadPool::submit(TaskNode task)
{
    mtx_lock(&_mtxQ);
    _qTasks.pushBack(task);
    mtx_unlock(&_mtxQ);

    cnd_signal(&_cndQ);
}

inline void
ThreadPool::wait()
{
    while (busy())
    {
        mtx_lock(&_mtxWait);
        cnd_wait(&_cndWait, &_mtxWait);
        mtx_unlock(&_mtxWait);
    }
}

inline void
ThreadPool::stop()
{
    _bDone = true;
    cnd_broadcast(&_cndQ);
    for (u32 i = 0; i < _threadCount; i++)
        thrd_join(_pThreads[i], nullptr);
}

inline void
ThreadPool::destroy()
{
    stop();

    _pAlloc->free(_pThreads);
    _qTasks.destroy();
    cnd_destroy(&_cndQ);
    mtx_destroy(&_mtxQ);
    cnd_destroy(&_cndWait);
    mtx_destroy(&_mtxWait);
}

} /* namespace adt */
