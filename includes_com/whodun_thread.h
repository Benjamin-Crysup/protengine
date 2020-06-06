#ifndef WHODUN_THREAD_H
#define WHODUN_THREAD_H 1

#include <deque>
#include <vector>
#include <stdint.h>

/**A pool of reusable threads.*/
class ThreadPool{
public:
	/**
	 * Set up the threads.
	 * @param numThread The number of threads.
	 */
	ThreadPool(int numThread);
	/**Kill the threads.*/
	~ThreadPool();
	/**
	 * Add a task to run.
	 * @param toDo The function to run.
	 * @param toPass The thing to add.
	 */
	void addTask(void (*toDo)(void*), void* toPass);
	/**The number of threads in this pool.*/
	int numThr;
	/**The task mutex.*/
	void* taskMut;
	/**The task conditions.*/
	void* taskCond;
	/**The wating tasks.*/
	std::deque< std::pair<void(*)(void*), void*> > waitTask;
	/**Whether the pool is live.*/
	bool poolLive;
	/**The live threads.*/
	std::vector<void*> liveThread;
};

#endif
