//=============================================================================================================
#include "thread.h"

size_t SignalCombo::Add(const Signal* sn)
{
	handles.push_back(sn->handle);
	return handles.size() - 1;
}

size_t SignalCombo::WaitAny()
{
	DWORD ret = WaitForMultipleObjects(handles.size(), handles.data(), 0, INFINITE);
	return (ret - WAIT_OBJECT_0);
}

void SignalCombo::WaitAll()
{
	WaitForMultipleObjects(handles.size(), handles.data(), 1, INFINITE);
}

unsigned long __stdcall Thread::Run(void* param)
{
	emitter_base* emi = reinterpret_cast<emitter_base*>(param);

	emi->emit();
	return 0;
}

Thread::Thread()
{
	emi = NULL;
	handle = INVALID_HANDLE_VALUE;
}

Thread::~Thread()
{
	if( handle != INVALID_HANDLE_VALUE )
		Close();
}

void Thread::Start()
{
	if( handle != INVALID_HANDLE_VALUE )
		ResumeThread(handle);
}

void Thread::Stop()
{
	if( handle != INVALID_HANDLE_VALUE )
		SuspendThread(handle);
}

void Thread::Close()
{
	if( handle != INVALID_HANDLE_VALUE )
	{
		CloseHandle(handle);
		handle = INVALID_HANDLE_VALUE;
	}

	if( emi )
	{
		delete emi;
		emi = NULL;
	}
}
