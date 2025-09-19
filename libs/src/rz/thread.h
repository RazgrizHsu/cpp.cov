#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <stdexcept>
#include <optional>

class TaskPool {
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	std::mutex mtx;
	std::condition_variable cond;
	std::atomic<bool> stop{ false };
	std::function<void( const std::exception & )> onErrHandler;

	std::atomic<int> dones{ 0 };
	std::atomic<int> total{ 0 };
	std::atomic<bool> touted{ false };

public:
	using IFnProgress = std::function<void( int, int )>;
	IFnProgress onProgress{ nullptr };

	TaskPool( size_t threads ) : workers( threads ) {}

	size_t size() {
		std::lock_guard<std::mutex> lock( mtx );
		return tasks.size();
	}

	template<class F>
	void enqueue( F &&f ) {
		std::lock_guard<std::mutex> lock( mtx );
		tasks.emplace( std::forward<F>( f ) );
		++total;
	}

	void runWaitAll( IFnProgress progress = nullptr, std::optional<int> timeoutSecs = std::nullopt ) {
		onProgress = progress;
		touted = false;

		auto startTime = std::chrono::steady_clock::now();

		for ( auto &worker: workers ) {
			worker = std::thread( [this, timeoutSecs, startTime] {
				while ( true ) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock( mtx );
						auto predicate = [this] { return stop || !tasks.empty() || touted; };

						if ( timeoutSecs ) {
							auto timeout = std::chrono::seconds( *timeoutSecs );
							if ( !cond.wait_for( lock, timeout, predicate ) ) {
								touted = true;
								return;
							}
						}
						else {
							cond.wait( lock, predicate );
						}

						if ( ( stop || touted ) && tasks.empty() ) return;
						if ( !tasks.empty() ) {
							task = std::move( tasks.front() );
							tasks.pop();
						}
						else continue;
					}

					if ( timeoutSecs && std::chrono::steady_clock::now() - startTime >= std::chrono::seconds( *timeoutSecs ) ) {
						touted = true;
						return;
					}

					try { task(); }
					catch ( const std::exception &e ) {
						if ( onErrHandler ) onErrHandler( e );
					}
					int completed = ++dones;
					if ( onProgress ) onProgress( completed, total.load() );
				}
			} );
		}

		{
			std::unique_lock<std::mutex> lock( mtx );
			stop = true;
		}
		cond.notify_all();

		for ( auto &worker: workers ) {
			if ( worker.joinable() ) worker.join();
		}

		if ( touted ) {
			throw std::runtime_error( "TaskPool operation timed out after " + std::to_string( *timeoutSecs ) + " seconds" );
		}
	}

	void onError( std::function<void( const std::exception & )> onErr ) {
		onErrHandler = std::move( onErr );
	}

	~TaskPool() {
		if ( !stop ) runWaitAll();
	}
};
