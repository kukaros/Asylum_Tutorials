
#ifndef _RENDERINGCORE_H_
#define _RENDERINGCORE_H_

#include "../common/thread.h"
#include "../common/blockingqueue.hpp"
#include "../common/glext.h"

#define SAFE_DELETE(x)		if( (x) ) { delete (x); (x) = 0; }

/**
 * \brief This is what is visible to the public
 *
 * Can be used from the worker thread only.
 */
class IRenderingContext
{
public:
	virtual ~IRenderingContext();

	// factory methods
	virtual OpenGLFramebuffer* CreateFramebuffer(GLuint width, GLuint height) = 0;
	virtual OpenGLScreenQuad* CreateScreenQuad() = 0;
	virtual OpenGLEffect* CreateEffect(const char* vsfile, const char* gsfile, const char* fsfile) = 0;
	virtual OpenGLMesh* CreateMesh(const char* file) = 0;

	// rendering methods
	virtual void Clear() = 0;
};

/**
 * \brief Singleton thread for all OpenGL calls
 */
class RenderingCore
{
	friend RenderingCore* GetRenderingCore();

public:
	class PrivateInterface;

	class IRenderingTask
	{
		friend class RenderingCore;

	protected:
		Signal finished;
		int universeid;

	public:
		IRenderingTask(int universe);
		virtual ~IRenderingTask();

		virtual void Execute(IRenderingContext* context) = 0;

		inline void Wait() {
			finished.Wait();
			finished.Halt();
		}

		inline int GetUniverseID() const {
			return universeid;
		}
	};

	void AddTask(IRenderingTask* task);
	void Shutdown();

	// these methods always block
	int CreateUniverse(HDC hdc);
	void DeleteUniverse(int id);

private:
	static RenderingCore* _inst;
	static Guard singletonguard;

	blockingqueue<IRenderingTask*>	tasks;
	Thread							thread;
	PrivateInterface*				privinterf;

	RenderingCore();
	~RenderingCore();

	bool SetupCoreProfile();
	void THREAD_Run();
};

RenderingCore* GetRenderingCore();

#endif
