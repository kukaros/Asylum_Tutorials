//=============================================================================================================
#ifndef _THREAD_H_
#define _THREAD_H_

#include <windows.h>
#include <vector>

/**
 * \brief Fast lock
 */
class Guard
{
private:
	CRITICAL_SECTION critsec;

public:
	Guard() {
		InitializeCriticalSection(&critsec);
	}

	~Guard() {
		DeleteCriticalSection(&critsec);
	}

	inline void Lock() {
		EnterCriticalSection(&critsec);
	}

	inline void Unlock() {
		LeaveCriticalSection(&critsec);
	}
};

/**
 * \brief Condition
 */
class Signal
{
	friend class SignalCombo;

private:
	HANDLE handle;

public:
	Signal() {
		handle = CreateEvent(NULL, TRUE, FALSE, NULL); 
	}

	~Signal() {
		Close();
	}

	inline void Fire() {
		SetEvent(handle);
	}

	inline void Halt() {
		ResetEvent(handle);
	}

	inline void Close() {
		CloseHandle(handle);
		handle = NULL;
	}

	inline void Wait() {
		WaitForSingleObject(handle, INFINITE);
	}
};

/**
 * \brief Multiple signals
 */
class SignalCombo
{
private:
	std::vector<HANDLE> handles;

public:
	SignalCombo() {
		handles.reserve(10);
	}

	~SignalCombo() {
		handles.clear();
	}

	size_t Add(const Signal* sn);
	size_t WaitAny();

	void WaitAll();
};

/**
 * \brief Represents a thread
 */
class Thread
{
	class emitter_base
	{
	public:
		virtual ~emitter_base() {}
		virtual void emit() = 0;
	};

	template <typename callable_type>
	class emitter : public emitter_base
	{
	private:
		callable_type* obj;
		void (callable_type::*memfunc)();

	public:
		emitter(callable_type* _obj, void (callable_type::*_memfunc)())
			: obj(_obj), memfunc(_memfunc) {}

		inline void emit() {
			(obj->*memfunc)();
		}
	};

private:
	HANDLE			handle;
	emitter_base*	emi;
	unsigned long	id;
	
	static unsigned long __stdcall Run(void* param);

public:
	Thread();
	~Thread();

	template <typename callable_type>
	bool Attach(callable_type* obj, void (callable_type::*memfunc)());

	void Start();
	void Stop();
	void Close();

	inline int GetID() const {
		return id;
	}

	inline HANDLE GetHandle() const {
		return handle;
	}
};

template <typename callable_type>
bool Thread::Attach(callable_type* obj, void (callable_type::*memfunc)())
{
	if( emi )
		delete emi;

	emi = new emitter<callable_type>(obj, memfunc);

	handle = CreateThread(
		NULL, 0, (LPTHREAD_START_ROUTINE)&Thread::Run, emi, CREATE_SUSPENDED, &id);

	return (handle != INVALID_HANDLE_VALUE);
}

#endif
//=============================================================================================================
