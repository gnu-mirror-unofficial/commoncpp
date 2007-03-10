#ifndef _UCOMMON_THREAD_H_
#define	_UCOMMON_THREAD_H_

#if UCOMMON_THREADING > 0

#ifndef _UCOMMON_ACCESS_H_
#include <ucommon/access.h>
#endif

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT Barrier 
{
private:
	pthread_barrier_t barrier;

public:
	Barrier(unsigned count);
	~Barrier();

	void wait(void);
};

class __EXPORT Spinlock : public Exclusive
{
private:
	pthread_spinlock_t spin;

public:
	Spinlock();
	~Spinlock();

	bool operator!(void);

	void Exlock(void);
	void Unlock(void);	

	inline void lock(void)
		{pthread_spin_lock(&spin);};

	inline void unlock(void)
		{pthread_spin_unlock(&spin);};

	inline void release(void)
		{pthread_spin_unlock(&spin);};
};
	
class __EXPORT Conditional : public Exclusive
{
private:
	friend class __EXPORT Semaphore;
	friend class __EXPORT Event;

	class __EXPORT attribute
	{
	public:
		pthread_condattr_t attr;
		attribute();
	};
	static attribute attr;

	volatile pthread_t locker;
	pthread_cond_t cond;
	pthread_mutex_t mutex;

public:
	Conditional();
	~Conditional();

	void Exlock(void);
	void Unlock(void);

	inline void lock(void)
		{Conditional::Exlock();};

	inline void unlock(void)
		{Conditional::Unlock();};

	inline void release(void)
		{Conditional::Unlock();};

	bool wait(timeout_t timeout = 0);
	bool wait(Timer &timer);
	void signal(bool broadcast);
};

class __EXPORT Semaphore : public Shared
{
private:
	unsigned count, waits;
	pthread_cond_t cond;
	pthread_mutex_t mutex;

public:
	Semaphore(unsigned limit);
	~Semaphore();

	void Shlock(void);
	void Unlock(void);

	bool wait(timeout_t timeout = 0);
	bool wait(Timer &timer);

	inline void release(void)
		{Semaphore::Unlock();};
};

class __EXPORT Event
{
private:
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned count;
	bool signalled;

public:
	Event();
	~Event();

	void signal(void);
	void reset(void);

	bool wait(timeout_t timeout);
	bool wait(Timer &t);
	void wait(void);
};

class __EXPORT Mutex : public Exclusive
{
private:
	class __EXPORT attribute
	{
	public:
		pthread_mutexattr_t attr;
		attribute();
	};

	static attribute attr;
	pthread_mutex_t mutex;
		
public:
	Mutex();
	~Mutex();

	void Exlock(void);
	void Unlock(void);

	inline void lock(void)
		{pthread_mutex_lock(&mutex);};

	inline void unlock(void)
		{pthread_mutex_unlock(&mutex);};

	inline void release(void)
		{pthread_mutex_unlock(&mutex);};
};

class __EXPORT LockedPointer
{
private:
	friend class __EXPORT locked_release;
	pthread_mutex_t mutex;
	Object *pointer;

protected:
	LockedPointer();

	void set(Object *ptr);
	Object *get(void);

	LockedPointer &operator=(Object *o);
};

class __EXPORT SharedPointer
{
private:
	friend class __EXPORT shared_release;
	pthread_rwlock_t lock;
	Object *pointer;

protected:
	SharedPointer();
	~SharedPointer();

	void set(Object *ptr);
	Object *get(void);

public:
	void release(void);
};

class __EXPORT Threadlock : public Exclusive, public Shared
{
private:
	pthread_rwlock_t lock;

public:
	Threadlock();
	~Threadlock();

	void Exlock(void);
	void Shlock(void);
	void Unlock(void);

	inline void shared(void)
		{pthread_rwlock_rdlock(&lock);};

	inline void exclusive(void)
		{pthread_rwlock_wrlock(&lock);};

	inline void release(void)
		{pthread_rwlock_unlock(&lock);};
};

class __EXPORT Thread
{
private:
	static int policy; 
	size_t stack;
	unsigned priority;

	pthread_t tid;
	volatile bool running, detached;

	void start(bool detached);

protected:
	Thread(unsigned pri = 0, size_t stack = 0);

public:
	virtual void run(void) = 0;
	
	virtual ~Thread();

	inline bool isRunning(void)
		{return running;};

	inline bool isDetached(void)
		{return detached;};

	void release(void);
	
	inline void detach(void)
		{return start(true);};

	inline void start(void)
		{return start(false);};

	inline static void setPolicy(int pol)
		{policy = pol;};

	inline static int getPolicy(void)
		{return policy;};

	void setPriority(unsigned pri);

	inline unsigned getPriority(void)
		{return priority;};

	static unsigned maxPriority(void);
};

class __EXPORT Buffer : public Conditional
{
private:
	size_t size, used, objsize;
	char *buf, *head, *tail;

protected:
	virtual size_t onPeek(void *buf);
	virtual size_t onWait(void *buf);
	virtual size_t onPost(void *buf);

public:
	static const size_t timeout;

	Buffer(size_t capacity, size_t osize = 0);
	~Buffer();

	inline size_t getSize(void)
		{return size;};

	inline size_t getUsed(void)
		{return used;};

	size_t wait(void *buf, timeout_t timeout = 0);
	size_t post(void *buf, timeout_t timeout = 0);
	size_t peek(void *buf);

	virtual bool operator!();
};

class __EXPORT locked_release
{
protected:
	Object *object;

	locked_release();
	locked_release(const locked_release &copy);

public:
	locked_release(LockedPointer &p);
	~locked_release();

	void release(void);

	locked_release &operator=(LockedPointer &p);
};

class __EXPORT shared_release
{
protected:
	Object *object;
	SharedPointer *ptr;

	shared_release();
	shared_release(const shared_release &copy);

public:
	shared_release(SharedPointer &p);
	~shared_release();

	void release(void);

	shared_release &operator=(SharedPointer &p);
};

class __EXPORT auto_cancellation
{
private:
	int type, mode;

public:
	auto_cancellation(int type, int mode);
	~auto_cancellation();
};

template<class T>
class buffer : public Buffer
{
public:
	inline buffer(size_t size) : Buffer(size, sizeof(T)) {};

	inline size_t wait(T *obj, timeout_t timeout = 0)
		{return Buffer::wait(obj, timeout);};

	inline size_t post(T *obj, timeout_t timeout = 0)
		{return Buffer::post(obj, timeout);};
};
 
template<class T>
class shared_pointer : public SharedPointer
{
public:
	inline shared_pointer() : SharedPointer() {};

	inline T *get(void)
		{return static_cast<T*>(SharedPointer::get());};

	inline void set(T *p)
		{SharedPointer::set(p);};
};	

template<class T>
class locked_pointer : public LockedPointer
{
public:
	inline locked_pointer() : LockedPointer() {};

	inline T* get(void)
		{return static_cast<T *>(LockedPointer::get());};

	inline void set(T *p)
		{LockedPointer::set(p);};

	inline locked_pointer<T>& operator=(T *obj)
		{LockedPointer::operator=(obj); return *this;};

	inline T *operator*()
		{return get();};
};

template<class T>
class locked_instance : public locked_release
{
public:
    inline locked_instance() : locked_release() {};

    inline locked_instance(locked_pointer<T> &p) : locked_release(p) {};

    inline T& operator*() const
        {return *(static_cast<T *>(object));};

    inline T* operator->() const
        {return static_cast<T*>(object);};

    inline T* get(void) const
        {return static_cast<T*>(object);};
};

template<class T>
class shared_instance : public shared_release
{
public:
	inline shared_instance() : shared_release() {};

	inline shared_instance(shared_pointer<T> &p) : shared_release(p) {};

	inline T& operator*() const
		{return *(static_cast<T *>(object));};

	inline T* operator->() const
		{return static_cast<T*>(object);};

	inline T* get(void) const
		{return static_cast<T*>(object);};
};

#define	cancel_immediate()	auto_cancellation \
	__cancel__(PTHREAD_CANCEL_ASYNCHRONOUS, PTHREAD_CANCEL_ENABLE)

#define	cancel_deferred() auto_cancellation \
	__cancel__(PTHREAD_CANCEL_DEFERRED, PTHREAD_CANCEL_ENABLE)

#define	cancel_disabled() auto_cancellation \
	__cancel__(PTHREAD_CANCEL_DEFERRED, PTHREAD_CANCEL_DISABLE)

END_NAMESPACE

extern "C" {
};

#endif
#endif
