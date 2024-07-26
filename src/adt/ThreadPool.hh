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
_getLogicalCoresCountWIN32()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

    #define getLogicalCoresCount() _getLogicalCoresCountWIN32()
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
    Allocator* pAlloc {};
    Queue<TaskNode> qTasks;
    thrd_t* pThreads {};
    u32 threadCount {};
    cnd_t cndQ, cndWait;
    mtx_t mtxQ, mtxWait;
    std::atomic<int> activeTaskCount;
    bool bDone {};

    ThreadPool() = default;
    ThreadPool(Allocator* p, u32 _threadCount);
    ThreadPool(Allocator* p);

    void start();
    bool busy();
    void submit(thrd_start_t pfnTask, void* pArgs) { submit({pfnTask, pArgs}); }
    void submit(TaskNode task);
    void wait();
    void stop();
    void destroy();

private:
    static int loop(void* _self);
};

inline
ThreadPool::ThreadPool(Allocator* p, u32 _threadCount)
    : pAlloc(p), qTasks(p, _threadCount), threadCount(_threadCount), activeTaskCount(0), bDone(false)
{
    this->pThreads = (thrd_t*)p->alloc(_threadCount, sizeof(thrd_t));
    cnd_init(&this->cndQ);
    mtx_init(&this->mtxQ, mtx_plain);
    cnd_init(&this->cndWait);
    mtx_init(&this->mtxWait, mtx_plain);
}

inline
ThreadPool::ThreadPool(Allocator* p)
    : ThreadPool(p, getLogicalCoresCount()) {}

inline void
ThreadPool::start()
{
    for (size_t i = 0; i < this->threadCount; i++)
        thrd_create(&this->pThreads[i], ThreadPool::loop, this);
}

inline bool
ThreadPool::busy()
{
    mtx_lock(&this->mtxQ);
    bool ret = !this->qTasks.empty() || this->activeTaskCount > 0;
    mtx_unlock(&this->mtxQ);

    return ret;
}

inline int
ThreadPool::loop(void* _self)
{
    auto* self = reinterpret_cast<ThreadPool*>(_self);

    while (!self->bDone)
    {
        TaskNode task;
        {
            mtx_lock(&self->mtxQ);

            while (!(!self->qTasks.empty() || self->bDone))
                cnd_wait(&self->cndQ, &self->mtxQ);

            if (self->bDone)
            {
                mtx_unlock(&self->mtxQ);
                return thrd_success;
            }

            task = *self->qTasks.popFront();
            self->activeTaskCount++; /* increment before unlocking mtxQ to avoid 0 tasks and 0 q possibility */

            mtx_unlock(&self->mtxQ);
        }

        task.pfn(task.pArgs);
        self->activeTaskCount--;

        if (!self->busy())
            cnd_signal(&self->cndWait);
    }

    return thrd_success;
}

inline void
ThreadPool::submit(TaskNode task)
{
    mtx_lock(&this->mtxQ);
    this->qTasks.pushBack(task);
    mtx_unlock(&this->mtxQ);

    cnd_signal(&this->cndQ);
}

inline void
ThreadPool::wait()
{
    while (this->busy())
    {
        mtx_lock(&this->mtxWait);
        cnd_wait(&this->cndWait, &this->mtxWait);
        mtx_unlock(&this->mtxWait);
    }
}

inline void
ThreadPool::stop()
{
    this->bDone = true;
    cnd_broadcast(&this->cndQ);
    for (u32 i = 0; i < this->threadCount; i++)
        thrd_join(this->pThreads[i], nullptr);
}

inline void
ThreadPool::destroy()
{
    this->pAlloc->free(this->pThreads);
    this->qTasks.destroy();
    cnd_destroy(&this->cndQ);
    mtx_destroy(&this->mtxQ);
    cnd_destroy(&this->cndWait);
    mtx_destroy(&this->mtxWait);
}

} /* namespace adt */
