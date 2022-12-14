//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include <vector>
#include <functional>
#include <atomic>
#include <thread>

#include "progress_bar.h"

//simple thread pool implementation
//updateFun should be thread-safe!
template <class T>
void processInParallel(const std::vector<T>& scheduledTasks,
					   std::function<void(const T&)> updateFun,
					   size_t maxThreads, bool progressBar)
{
	if (scheduledTasks.empty()) return;

	std::atomic<size_t> jobId(0);
	ProgressPercent progress(scheduledTasks.size());
	if (progressBar) progress.advance(0);

	#pragma omp parallel for
	for (size_t i = 0; i < std::min(maxThreads, scheduledTasks.size()); ++i)
	{
		bool finished = false;
		while (!finished)
		{
			size_t expected = 0;
			while(true)
			{
				expected = jobId;
				if (jobId == scheduledTasks.size()) 
				{
					finished = true;
					break;
				}
				if (jobId.compare_exchange_weak(expected, expected + 1))
				{
					break;
				}
			}
			if (!finished) {
				updateFun(scheduledTasks[expected]);
				if (progressBar) progress.advance();
			}
		}
	}
}

template <class T>
void processInParallel2(const std::vector<T>& scheduledTasks,
					   std::function<void(const T&, const size_t, const size_t)> updateFun,
					   size_t maxThreads, bool progressBar)
{
	if (scheduledTasks.empty()) return;

	ProgressPercent progress(scheduledTasks.size());
	if (progressBar) progress.advance(0);

	auto threadWorker = [](const size_t nthreads, const size_t thread, 
						 const std::vector<T>& scheduledTasks, 
						 std::function<void(const T&, const size_t, const size_t)> updateFun, 
						 ProgressPercent &progress, const bool progressBar)
	{
		for (const auto& task : scheduledTasks) {
			updateFun(task, nthreads, thread);

			if (progressBar) progress.advance();
		}
	};

	std::vector<std::thread> threads(std::min(maxThreads, 
											  scheduledTasks.size()));
	for (size_t i = 0; i < threads.size(); ++i)
	{
		threads[i] = std::thread(threadWorker, threads.size(), i, 
								 std::ref(scheduledTasks), updateFun, 
								 std::ref(progress), progressBar);
	}
	for (size_t i = 0; i < threads.size(); ++i)
	{
		threads[i].join();
	}
}
