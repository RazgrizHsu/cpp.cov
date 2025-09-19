#include "doctest.h"
#include <rz/thread.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>



TEST_CASE( "TaskPool: ComprehensiveTest" ) {
	// Test 1: EnqueueAndRunTasks
	{
		TaskPool pool( 4 );
		std::vector<int> results( 10, 0 );
		std::atomic<int> executedTasks( 0 );

		for ( int i = 0; i < 10; ++i ) {
			pool.enqueue( [&results, &executedTasks, i] {
				std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
				results[i] = i * i;
				executedTasks++;
			} );
		}

		CHECK_EQ( executedTasks, 0 ); // 任務還沒開始執行

		pool.runWaitAll();

		CHECK_EQ( executedTasks, 10 ); // 所有任務都應該執行完畢
		for ( int i = 0; i < 10; ++i ) {
			CHECK_EQ( results[i], i * i );
		}
	}

	// Test 2: Delayed Execution
	{
		TaskPool pool( 2 );
		std::atomic<int> counter( 0 );

		for ( int i = 0; i < 5; ++i ) {
			pool.enqueue( [&counter] {
				counter++;
			} );
		}

		CHECK_EQ( counter, 0 ); // 任務還沒開始執行
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
		CHECK_EQ( counter, 0 ); // 仍然沒有執行

		pool.runWaitAll();

		CHECK_EQ( counter, 5 ); // 所有任務都應該執行完畢
	}

	// Test 3: EnqueueAfterRunWaitAll
	{
		TaskPool pool( 2 );
		std::atomic<int> counter( 0 );

		pool.runWaitAll();

		for ( int i = 0; i < 5; ++i ) {
			pool.enqueue( [&counter] {
				counter++;
			} );
		}

		CHECK_EQ( counter, 0 ); // 任務不應該執行，因為runWaitAll已經被調用

		pool.runWaitAll(); // 這次調用應該執行新入隊的任務

		CHECK_EQ( counter, 5 );
	}

	// Test 4: Concurrent Enqueue and RunWaitAll
	{
		TaskPool pool( 4 );
		std::atomic<int> counter( 0 );
		std::atomic<bool> stop( false );
		std::atomic<bool> enqueue_done( false );

		std::thread enqueue_thread( [&]() {
			for ( int i = 0; i < 1000 && !stop; ++i ) {
				pool.enqueue( [&counter] {
					std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
					counter++;
				} );
				std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
			}
			enqueue_done = true;
		} );

		// 等待一些任務被加入隊列
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

		pool.runWaitAll();
		stop = true;
		enqueue_thread.join();

		int final_count = counter.load();
		CHECK_GT( final_count, 0 );
		CHECK_LE( final_count, 1000 );
	}

	// Test 5: Exception Handling
	{
		TaskPool pool( 2 );
		std::atomic<int> exception_count( 0 );

		pool.onError( [&exception_count]( const std::exception & ) {
			exception_count++;
		} );

		for ( int i = 0; i < 10; ++i ) {
			pool.enqueue( [i] {
				if ( i % 3 == 0 ) {
					throw std::runtime_error( "Test exception" );
				}
				std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
			} );
		}

		pool.runWaitAll();

		CHECK_EQ( exception_count, 4 ); // 應該有4個異常被捕獲
	}

	// Test 6: Large Number of Tasks
	{
		TaskPool pool( 8 );
		std::atomic<int> counter( 0 );
		const int num_tasks = 10000;

		for ( int i = 0; i < num_tasks; ++i ) {
			pool.enqueue( [&counter] {
				counter++;
			} );
		}

		CHECK_EQ( counter, 0 ); // 任務還沒開始執行

		pool.runWaitAll();

		CHECK_EQ( counter, num_tasks );
	}
}
