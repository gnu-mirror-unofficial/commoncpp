// Copyright (C) 2006-2014 David Sugar, Tycho Softworks.
// Copyright (C) 2015-2020 Cherokees of Idaho.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <ucommon-config.h>
#include <ucommon/export.h>
#include <ucommon/object.h>
#include <ucommon/condition.h>
#include <ucommon/thread.h>
#include <ucommon/timers.h>
#include <ucommon/linked.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

namespace ucommon {

#if !defined(_MSTHREADS_)
Conditional::attribute Conditional::attr;
#endif

unsigned ConditionalAccess::max_sharing = 0;

void ConditionalAccess::limit_sharing(unsigned max)
{
    max_sharing = max;
}

void Conditional::set(struct timespec *ts, timeout_t msec)
{
    assert(ts != NULL);

#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    clock_gettime(_posix_clocking, ts);
#else
    timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000l;
#endif
    ts->tv_sec += msec / 1000;
    ts->tv_nsec += (msec % 1000) * 1000000l;
    while(ts->tv_nsec >= 1000000000l) {
        ++ts->tv_sec;
        ts->tv_nsec -= 1000000000l;
    }
}

#ifdef  _MSTHREADS_

ConditionMutex::ConditionMutex()
{
	InitializeCriticalSection(&mutex);
}

ConditionMutex::~ConditionMutex()
{
    DeleteCriticalSection(&mutex);
}

Conditional::Conditional()
{
    InitializeConditionVariable(&cond);
}

Conditional::~Conditional()
{
}

void Conditional::wait(void)
{
    SleepConditionVariableCS(&cond, &mutex, INFINITE);
}

bool Conditional::wait(timeout_t timeout)
{
    if(SleepConditionVariableCS(&cond, &mutex, timeout))
        return true;

    return false;
}

void Conditional::signal(void)
{
    WakeConditionVariable(&cond);
}

void Conditional::broadcast(void)
{
    WakeAllConditionVariable(&cond);
}

bool Conditional::wait(struct timespec *ts)
{
    assert(ts != NULL);

    return wait((timeout_t)(ts->tv_sec * 1000 + (ts->tv_nsec / 1000000l)));
}

ConditionVar::ConditionVar(ConditionMutex *m)
{
	shared = m;
    InitializeConditionVariable(&cond);
}

ConditionVar::~ConditionVar()
{
}

void ConditionVar::wait(void)
{
    SleepConditionVariableCS(&cond, &shared->mutex, INFINITE);
}

bool ConditionVar::wait(timeout_t timeout)
{
    if(SleepConditionVariableCS(&cond, &shared->mutex, timeout))
        return true;

    return false;
}

void ConditionVar::signal(void)
{
    WakeConditionVariable(&cond);
}

void ConditionVar::broadcast(void)
{
    WakeAllConditionVariable(&cond);
}

bool ConditionVar::wait(struct timespec *ts)
{
    assert(ts != NULL);

    return wait((timeout_t)(ts->tv_sec * 1000 + (ts->tv_nsec / 1000000l)));
}

#else

#include <stdio.h>

Conditional::attribute::attribute()
{
    Thread::init();
    pthread_condattr_init(&attr);
#if _POSIX_TIMERS > 0 && defined(HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined(POSIX_TIMERS)
#if defined(_POSIX_MONOTONIC_CLOCK)
    if(!pthread_condattr_setclock(&attr, CLOCK_MONOTONIC))
        _posix_clocking = CLOCK_MONOTONIC;
#else
    pthread_condattr_setclock(&attr, CLOCK_REALTIME);
#endif
#endif
}

ConditionMutex::ConditionMutex()
{
	if(pthread_mutex_init(&mutex, NULL))
		__THROW_RUNTIME("mutex init failed");
}

ConditionMutex::~ConditionMutex()
{
	pthread_mutex_destroy(&mutex);
}

Conditional::Conditional()
{
	if(pthread_cond_init(&cond, &Conditional::attr.attr))
		__THROW_RUNTIME("conditional init failed");
}

Conditional::~Conditional()
{
    pthread_cond_destroy(&cond);
}

bool Conditional::wait(timeout_t timeout)
{
    struct timespec ts;
    set(&ts, timeout);
    return wait(&ts);
}

bool Conditional::wait(struct timespec *ts)
{
    assert(ts != NULL);

    if(pthread_cond_timedwait(&cond, &mutex, ts) == ETIMEDOUT)
        return false;

    return true;
}

ConditionVar::ConditionVar(ConditionMutex *m)
{
	shared = m;
    if(pthread_cond_init(&cond, &Conditional::attr.attr))
		__THROW_RUNTIME("conditional init failed");
}

ConditionVar::~ConditionVar()
{
    pthread_cond_destroy(&cond);
}

bool ConditionVar::wait(timeout_t timeout)
{
    struct timespec ts;
    Conditional::set(&ts, timeout);
    return wait(&ts);
}

bool ConditionVar::wait(struct timespec *ts)
{
    assert(ts != NULL);

    if(pthread_cond_timedwait(&cond, &shared->mutex, ts) == ETIMEDOUT)
        return false;

    return true;
}

#endif

#if defined(_MSTHREADS_)

ConditionalAccess::ConditionalAccess()
{
    waiting = pending = sharing = 0;
    InitializeConditionVariable(&bcast);
}

ConditionalAccess::~ConditionalAccess()
{
}

bool ConditionalAccess::waitBroadcast(timeout_t timeout)
{
    if(SleepConditionVariableCS(&bcast, &mutex, timeout))
        return true;

    return false;
}

void ConditionalAccess::waitBroadcast()
{
    SleepConditionVariableCS(&bcast, &mutex, INFINITE);
}

void ConditionalAccess::waitSignal()
{
    SleepConditionVariableCS(&cond, &mutex, INFINITE);
}

bool ConditionalAccess::waitSignal(timeout_t timeout)
{
    if(SleepConditionVariableCS(&cond, &mutex, timeout))
        return true;

    return false;
}

bool ConditionalAccess::waitBroadcast(struct timespec *ts)
{
    assert(ts != NULL);

    return waitBroadcast((timeout_t)(ts->tv_sec * 1000 + (ts->tv_nsec / 1000000l)));
}

bool ConditionalAccess::waitSignal(struct timespec *ts)
{
    assert(ts != NULL);

    return waitSignal((timeout_t)(ts->tv_sec * 1000 + (ts->tv_nsec / 1000000l)));
}

#else

ConditionalAccess::ConditionalAccess()
{
    waiting = pending = sharing = 0;
    if(pthread_cond_init(&bcast, &attr.attr))
		__THROW_RUNTIME("conditional init failed");
}

ConditionalAccess::~ConditionalAccess()
{
    pthread_cond_destroy(&bcast);
}

bool ConditionalAccess::waitSignal(timeout_t timeout)
{
    struct timespec ts;
    set(&ts, timeout);
    return waitSignal(&ts);
}

bool ConditionalAccess::waitBroadcast(struct timespec *ts)
{
    assert(ts != NULL);

    if(pthread_cond_timedwait(&bcast, &mutex, ts) == ETIMEDOUT)
        return false;

    return true;
}

bool ConditionalAccess::waitBroadcast(timeout_t timeout)
{
    struct timespec ts;
    set(&ts, timeout);
    return waitBroadcast(&ts);
}

bool ConditionalAccess::waitSignal(struct timespec *ts)
{
    assert(ts != NULL);

    if(pthread_cond_timedwait(&cond, &mutex, ts) == ETIMEDOUT)
        return false;

    return true;
}

#endif

void ConditionalAccess::modify(void)
{
    lock();
    while(sharing) {
        ++pending;
        waitSignal();
        --pending;
    }
}

void ConditionalAccess::commit(void)
{
    if(pending)
        signal();
    else if(waiting)
        broadcast();
    unlock();
}

void ConditionalAccess::access(void)
{
    lock();
    assert(!max_sharing || sharing < max_sharing);
    while(pending) {
        ++waiting;
        waitBroadcast();
        --waiting;
    }
    ++sharing;
    unlock();
}

void ConditionalAccess::release(void)
{
   lock();
    assert(sharing);

    --sharing;
    if(pending && !sharing)
        signal();
    else if(waiting && !pending)
        broadcast();
    unlock();
}

ConditionalLock::ConditionalLock() :
ConditionalAccess()
{
    contexts = NULL;
}

ConditionalLock::~ConditionalLock()
{
    linked_pointer<Context> cp = contexts, next;
    while(cp) {
        next = cp->getNext();
        delete *cp;
        cp = next;
    }
}

ConditionalLock::Context *ConditionalLock::getContext(void)
{
    Context *slot = NULL;
    pthread_t tid = Thread::self();
    linked_pointer<Context> cp = contexts;

    while(cp) {
        if(cp->count && Thread::equal(cp->thread, tid))
            return *cp;
        if(!cp->count)
            slot = *cp;
        cp.next();
    }
    if(!slot) {
        slot = new Context(&this->contexts);
        slot->count = 0;
    }
    slot->thread = tid;
    return slot;
}

void ConditionalLock::_share(void)
{
    access();
}

void ConditionalLock::_unshare(void)
{
    release();
}

void ConditionalLock::modify(void)
{
    Context *context;

    lock();
    context = getContext();

    assert(context && sharing >= context->count);

    sharing -= context->count;
    while(sharing) {
        ++pending;
        waitSignal();
        --pending;
    }
    ++context->count;
}

void ConditionalLock::commit(void)
{
    Context *context = getContext();
    --context->count;

    if(context->count) {
        sharing += context->count;
        unlock();
    }
    else
        ConditionalAccess::commit();
}

void ConditionalLock::release(void)
{
    Context *context;

    lock();
    context = getContext();
    assert(sharing && context && context->count > 0);
    --sharing;
    --context->count;
    if(pending && !sharing)
        signal();
    else if(waiting && !pending)
        broadcast();
    unlock();
}

void ConditionalLock::access(void)
{
    Context *context;
    lock();
    context = getContext();
    assert(context && (!max_sharing || sharing < max_sharing));

    // reschedule if pending exclusives to make sure modify threads are not
    // starved.

    ++context->count;

    while(context->count < 2 && pending) {
        ++waiting;
        waitBroadcast();
        --waiting;
    }
    ++sharing;
    unlock();
}

void ConditionalLock::exclusive(void)
{
    Context *context;

    lock();
    context = getContext();
    assert(sharing && context && context->count > 0);
    sharing -= context->count;
    while(sharing) {
        ++pending;
        waitSignal();
        --pending;
    }
}

void ConditionalLock::share(void)
{
    Context *context = getContext();
    assert(!sharing && context && context->count);
    sharing += context->count;
    unlock();
}


Barrier::Barrier(unsigned limit) :
Conditional()
{
    count = limit;
    waits = 0;
}

Barrier::~Barrier()
{
    lock();
    if(waits)
        broadcast();
    unlock();
}

void Barrier::set(unsigned limit)
{
    assert(limit > 0);

    lock();
    count = limit;
    if(count <= waits) {
        waits = 0;
        broadcast();
    }
    unlock();
}

void Barrier::dec(void)
{
    lock();
    if(count)
        --count;
    unlock();
}

unsigned Barrier::operator--(void)
{
    unsigned result;
    lock();
    if(count)
        --count;
    result = count;
    unlock();
    return result;
}

void Barrier::inc(void)
{
    lock();
    count++;
    if(count <= waits) {
        waits = 0;
        broadcast();
    }
    unlock();
}

unsigned Barrier::operator++(void)
{
    unsigned result;
    lock();
    count++;
    if(count <= waits) {
        waits = 0;
        broadcast();
    }
    result = count;
    unlock();
    return result;
}

bool Barrier::wait(timeout_t timeout)
{
    bool result;

    Conditional::lock();
    if(!count) {
        Conditional::unlock();
        return true;
    }
    if(++waits >= count) {
        waits = 0;
        Conditional::broadcast();
        Conditional::unlock();
        return true;
    }
    result = Conditional::wait(timeout);
    Conditional::unlock();
    return result;
}

void Barrier::wait(void)
{
    Conditional::lock();
    if(!count) {
        Conditional::unlock();
        return;
    }
    if(++waits >= count) {
        waits = 0;
        Conditional::broadcast();
        Conditional::unlock();
        return;
    }
    Conditional::wait();
    Conditional::unlock();
}

Semaphore::Semaphore(unsigned limit) :
Conditional()
{
	waits = 0;
	count = limit;
	used = 0;
}

Semaphore::Semaphore(unsigned limit, unsigned avail) :
Conditional()
{
	assert(limit > 0);
	assert(avail <= limit);

	waits = 0;
	count = limit;
	used = limit - avail;
}

void Semaphore::_share(void)
{
    wait();
}

void Semaphore::_unshare(void)
{
    release();
}

bool Semaphore::wait(timeout_t timeout)
{
    bool result = true;
    struct timespec ts;
    Conditional::set(&ts, timeout);

    lock();
    while(used >= count && result) {
        ++waits;
        result = Conditional::wait(&ts);
        --waits;
		if(!count)
			break;
    }
    if(result && count)
        ++used;
    unlock();
    return result;
}

void Semaphore::wait(void)
{
    lock();
    if(used >= count) {
        ++waits;
        Conditional::wait();
        --waits;
    }
	if(count)
	    ++used;
    unlock();
}

void Semaphore::release(void)
{
    lock();
    if(used)
        --used;
    if(waits) {
		if(count)
	        signal();
		else
			broadcast();
	}
    unlock();
}

void Semaphore::set(unsigned value)
{
    assert(value > 0);

    unsigned diff;

    lock();
    count = value;
    if(used >= count || !waits) {
        unlock();
        return;
    }
    diff = count - used;
    if(diff > waits)
        diff = waits;
    unlock();
    while(diff--) {
        lock();
        signal();
        unlock();
    }
}

} // namespace ucommon
