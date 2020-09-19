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
#include <ucommon/timers.h>
#include <ucommon/thread.h>
#include <ucommon/cpr.h>

namespace ucommon {

#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
extern int _posix_clocking;
#endif

static long _difftime(time_t ref)
{
    time_t now;
    time(&now);

    return (long)difftime(ref, now);
}

#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
static void adj(struct timespec *ts)
{
    assert(ts != NULL);

    if(ts->tv_nsec >= 1000000000l)
        ts->tv_sec += (ts->tv_nsec / 1000000000l);
    ts->tv_nsec %= 1000000000l;
    if(ts->tv_nsec < 0)
        ts->tv_nsec = -ts->tv_nsec;
}
#else
static void adj(struct timeval *ts)
{
    assert(ts != NULL);

    if(ts->tv_usec >= 1000000l)
        ts->tv_sec += (ts->tv_usec / 1000000l);
    ts->tv_usec %= 1000000l;
    if(ts->tv_usec < 0)
        ts->tv_usec = -ts->tv_usec;
}
#endif

TimerQueue::event::event(timeout_t timeout) :
Timer(), DLinkedObject()
{
    set(timeout);
}

TimerQueue::event::event(TimerQueue *tq, timeout_t timeout) :
Timer(), DLinkedObject()
{
    set(timeout);
    Timer::update();
    attach(tq);
}

TimerQueue::event::~event()
{
    detach();
}

Timer::Timer()
{
    clear();
}

Timer::Timer(timeout_t in)
{
    set();
    operator+=(in);
}

Timer::Timer(const Timer& copy)
{
    timer = copy.timer;
}

Timer::Timer(time_t in)
{
    set();
    timer.tv_sec += _difftime(in);
}

#ifdef  _MSWINDOWS_

Timer::tick_t Timer::ticks(void)
{
    ULARGE_INTEGER timer;

    GetSystemTimeAsFileTime((FILETIME*)&timer);
    timer.QuadPart +=
        (tick_t) (6893856000000000);
    return timer.QuadPart;
}

#else

Timer::tick_t Timer::ticks(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tick_t)tv.tv_sec * (tick_t)10000000) +
        ((tick_t)tv.tv_usec * 10) + (((tick_t)0x01B21DD2) << 32) + (tick_t)0x13814000;
}
#endif

void Timer::set(timeout_t timeout)
{
    set();
    operator+=(timeout);
}

void Timer::set(time_t time)
{
    set();
    timer.tv_sec += _difftime(time);
}

void TimerQueue::event::attach(TimerQueue *tq)
{
    if(tq == list())
        return;

    detach();
    if(!tq)
        return;

    tq->modify();
    enlist(tq);
    Timer::update();
    tq->update();
}

void TimerQueue::event::arm(timeout_t timeout)
{
    TimerQueue *tq = list();
    if(tq)
        tq->modify();
    set(timeout);
    if(tq)
        tq->update();
}

void TimerQueue::event::disarm(void)
{
    TimerQueue *tq = list();
    bool flag = is_active();

    if(tq && flag)
        tq->modify();
    clear();
    if(tq && flag)
        tq->update();
}

void TimerQueue::event::update(void)
{
    TimerQueue *tq = list();
    if(Timer::update() && tq) {
        tq->modify();
        tq->update();
    }
}

void TimerQueue::event::detach(void)
{
    TimerQueue *tq = list();
    if(tq) {
        tq->modify();
        clear();
        delist();
        tq->update();
    }
}

void Timer::set(void)
{
#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    clock_gettime(_posix_clocking, &timer);
#else
    gettimeofday(&timer, NULL);
#endif
    updated = true;
}

void Timer::clear(void)
{
    timer.tv_sec = 0;
#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    timer.tv_nsec = 0;
#else
    timer.tv_usec = 0;
#endif
    updated = false;
}

bool Timer::update(void)
{
    bool rtn = updated;
    updated = false;
    return rtn;
}

bool Timer::is_active(void) const
{
#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    if(!timer.tv_sec && !timer.tv_nsec)
        return false;
#else
    if(!timer.tv_sec && !timer.tv_usec)
        return false;
#endif
    return true;
}

timeout_t Timer::get(void) const
{
    timeout_t diff;
#if _POSIX_TIMERS > 0 && POSIX_TIMERS
    struct timespec current;

    clock_gettime(_posix_clocking, &current);
    adj(&current);
    if(current.tv_sec > timer.tv_sec)
        return 0;
    if(current.tv_sec == timer.tv_sec && current.tv_nsec > timer.tv_nsec)
        return 0;
    diff = (timer.tv_sec - current.tv_sec) * 1000;
    diff += ((timer.tv_nsec - current.tv_nsec) / 1000000l);
#else
    struct timeval current;
    gettimeofday(&current, NULL);
    adj(&current);
    if(current.tv_sec > timer.tv_sec)
        return 0;
    if(current.tv_sec == timer.tv_sec && current.tv_usec > timer.tv_usec)
        return 0;
    diff = (timer.tv_sec - current.tv_sec) * 1000;
    diff += ((timer.tv_usec - current.tv_usec) / 1000);
#endif
    return diff;
}

Timer::operator bool() const
{
    if(get())
        return false;

    return true;
}

bool Timer::operator!() const
{
    if(get())
        return true;

    return false;
}

timeout_t Timer::operator-(const Timer& source)
{
    timeout_t tv = get(), dv = source.get();
    if(!tv)
        return 0;

    if(tv == Timer::inf)
        return Timer::inf;

    if(dv == Timer::inf)
        return tv;

    if(dv > tv)
        return 0;

    return tv - dv;
}

bool Timer::operator==(const Timer& source) const
{
    return get() == source.get();
}

bool Timer::operator!=(const Timer& source) const
{
    return get() != source.get();
}

bool Timer::operator<(const Timer& source) const
{
    return get() < source.get();
}

bool Timer::operator<=(const Timer& source) const
{
    return get() <= source.get();
}

bool Timer::operator>(const Timer& source) const
{
    return get() > source.get();
}

bool Timer::operator>=(const Timer& source) const
{
    return get() >= source.get();
}

Timer& Timer::operator=(timeout_t to)
{
#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    clock_gettime(_posix_clocking, &timer);
#else
    gettimeofday(&timer, NULL);
#endif
    operator+=(to);
    return *this;
}

Timer& Timer::operator+=(timeout_t to)
{
    if(!is_active())
        set();

    timer.tv_sec += (to / 1000);
#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    timer.tv_nsec += (to % 1000l) * 1000000l;
#else
    timer.tv_usec += (to % 1000l) * 1000l;
#endif
    adj(&timer);
    updated = true;
    return *this;
}

Timer& Timer::operator-=(timeout_t to)
{
    if(!is_active())
        set();
    timer.tv_sec -= (to / 1000);
#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    timer.tv_nsec -= (to % 1000l) * 1000000l;
#else
    timer.tv_usec -= (to % 1000l) * 1000l;
#endif
    adj(&timer);
    return *this;
}


Timer& Timer::operator+=(time_t abs)
{
    if(!is_active())
        set();
    timer.tv_sec += _difftime(abs);
    updated = true;
    return *this;
}

Timer& Timer::operator-=(time_t abs)
{
    if(!is_active())
        set();
    timer.tv_sec -= _difftime(abs);
    return *this;
}

Timer& Timer::operator=(time_t abs)
{
#if _POSIX_TIMERS > 0 && defined(POSIX_TIMERS)
    clock_gettime(_posix_clocking, &timer);
#else
    gettimeofday(&timer, NULL);
#endif
    if(!abs)
        return *this;

    timer.tv_sec += _difftime(abs);
    updated = true;
    return *this;
}

void Timer::sync(Timer &t)
{
#if _POSIX_TIMERS > 0 && defined(HAVE_CLOCK_NANOSLEEP) && defined(POSIX_TIMERS)
    clock_nanosleep(_posix_clocking, TIMER_ABSTIME, &t.timer, NULL);
#elif defined(_MSWINDOWS_)
    SleepEx(t.get(), FALSE);
#else
    usleep(t.get());
#endif
}


timeout_t TQEvent::timeout(void)
{
    timeout_t timeout = get();
    if(is_active() && !timeout) {
        disarm();
        expired();
        timeout = get();
        Timer::update();
    }
    return timeout;
}

TimerQueue::TimerQueue() : OrderedIndex()
{
}

TimerQueue::~TimerQueue()
{
}

timeout_t TimerQueue::expire(void)
{
    timeout_t first = Timer::inf, next;
    linked_pointer<TimerQueue::event> timer = begin();
    TimerQueue::event *tp;

    while(timer) {
        tp = *timer;
        timer.next();
        next = tp->timeout();
        if(next && next < first)
            first = next;
    }
    return first;
}

void TimerQueue::operator+=(event &te) { te.attach(this); }

void TimerQueue::operator-=(event &te)
{
    if(te.list() == this)
        te.detach();
}

} // namespace ucommon
