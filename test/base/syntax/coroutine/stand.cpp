// #include "doctest.h"
//
//
//
// #include <iostream>
// #include <future>
// #include <thread>
// #include <chrono>
// #include <vector>
// #include <queue>
// #include <random>
// #include <stdexcept>
// #include <functional>
// #include <mutex>
// #include <condition_variable>
// #include <atomic>
//
// class ThreadPool {
// public:
//     ThreadPool(size_t threads) : stop(false) {
//         for(size_t i = 0; i < threads; ++i)
//             workers.emplace_back([this] {
//                 for(;;) {
//                     std::function<void()> task;
//                     {
//                         std::unique_lock<std::mutex> lock(this->queue_mutex);
//                         this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
//                         if(this->stop && this->tasks.empty()) return;
//                         task = std::move(this->tasks.front());
//                         this->tasks.pop();
//                     }
//                     task();
//                 }
//             });
//     }
//
//     template<class F>
//     auto enqueue(F&& f) -> std::future<typename std::result_of<F()>::type> {
//         using return_type = typename std::result_of<F()>::type;
//         auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
//         std::future<return_type> res = task->get_future();
//         {
//             std::unique_lock<std::mutex> lock(queue_mutex);
//             if(stop) throw std::runtime_error("enqueue on stopped ThreadPool");
//             tasks.emplace([task](){ (*task)(); });
//         }
//         condition.notify_one();
//         return res;
//     }
//
//     ~ThreadPool() {
//         {
//             std::unique_lock<std::mutex> lock(queue_mutex);
//             stop = true;
//         }
//         condition.notify_all();
//         for(std::thread &worker: workers) worker.join();
//     }
//
// private:
//     std::vector<std::thread> workers;
//     std::queue<std::function<void()>> tasks;
//     std::mutex queue_mutex;
//     std::condition_variable condition;
//     bool stop;
// };
//
// class ServiceOrchestrator {
// public:
//     ServiceOrchestrator() : pool(4), current_load(0) {}
//
//     template<typename Func>
//     auto call_service(const std::string& service_name, int delay_ms, Func&& func)
//     -> std::future<decltype(func())> {
//         return pool.enqueue([this, service_name, delay_ms, func = std::forward<Func>(func)]() {
//             std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
//             if (rand() % 10 == 0) {  // 10% 概率失敗
//                 throw std::runtime_error(service_name + " 調用失敗");
//             }
//             return func();
//         });
//     }
//
//     void adjust_parallelism() {
//         if (current_load > 100) {
//             // 增加執行緒池大小
//         } else if (current_load < 50) {
//             // 減少執行緒池大小
//         }
//     }
//
//     std::atomic<int> current_load;
//
// private:
//     ThreadPool pool;
// };
//
// ServiceOrchestrator orchestrator;
//
// // 使用者數據服務
// std::future<std::string> get_user_data(int user_id) {
//     return orchestrator.call_service("使用者數據服務", 200, []() { return "使用者數據"; });
// }
//
// // 訂單歷史服務
// std::future<std::vector<int>> get_order_history(int user_id) {
//     return orchestrator.call_service("訂單歷史服務", 300, []() { return std::vector<int>{1, 2, 3}; });
// }
//
// // 推薦服務
// std::future<std::vector<std::string>> get_recommendations(const std::vector<int>& order_history) {
//     return orchestrator.call_service("推薦服務", 500, []() { return std::vector<std::string>{"商品A", "商品B", "商品C"}; });
// }
//
// // 庫存服務
// std::future<bool> check_inventory(const std::string& product) {
//     return orchestrator.call_service("庫存服務", 200, []() { return true; });
// }
//
// // 批處理庫存檢查
// std::vector<std::future<bool>> batch_check_inventory(const std::vector<std::string>& products) {
//     std::vector<std::future<bool>> results;
//     for (const auto& product : products) {
//         results.push_back(check_inventory(product));
//     }
//     return results;
// }
//
// TEST_CASE( "coroutine: stand" )
// {
//     try {
//         auto start = std::chrono::high_resolution_clock::now();
//
//         int user_id = 12345;
//         std::atomic<bool> cancelled(false);
//
//         // 啟動取消計時器
//         auto cancel_timer = std::async(std::launch::async, [&cancelled]() {
//             std::this_thread::sleep_for(std::chrono::seconds(2));
//             cancelled.store(true);
//         });
//
//         // 獲取使用者數據
//         auto user_data_future = get_user_data(user_id);
//
//         // 獲取訂單歷史
//         auto order_history_future = get_order_history(user_id);
//
//         // 等待使用者數據和訂單歷史，帶超時
//         std::string user_data;
//         std::vector<int> order_history;
//
//         try {
//             auto timeout = std::chrono::seconds(1);
//             if (user_data_future.wait_for(timeout) == std::future_status::ready) {
//                 user_data = user_data_future.get();
//             } else {
//                 throw std::runtime_error("獲取使用者數據超時");
//             }
//
//             if (order_history_future.wait_for(timeout) == std::future_status::ready) {
//                 order_history = order_history_future.get();
//             } else {
//                 throw std::runtime_error("獲取訂單歷史超時");
//             }
//         } catch (const std::exception& e) {
//             std::cerr << "錯誤: " << e.what() << std::endl;
//             return 1;
//         }
//
//         if (cancelled.load()) {
//             std::cout << "操作已取消" << std::endl;
//             return 0;
//         }
//
//         // 延遲執行獲取推薦
//         auto get_recommendations_delayed = [&order_history]() {
//             return get_recommendations(order_history);
//         };
//         auto recommendations_future = std::async(std::launch::deferred, get_recommendations_delayed);
//
//         // 觸發延遲執行
//         auto recommendations = recommendations_future.get();
//
//         if (cancelled.load()) {
//             std::cout << "操作已取消" << std::endl;
//             return 0;
//         }
//
//         // 批量檢查庫存
//         auto inventory_futures = batch_check_inventory(recommendations);
//
//         // 等待所有庫存檢查完成
//         std::vector<std::string> available_products;
//         for (size_t i = 0; i < recommendations.size(); ++i) {
//             if (cancelled.load()) {
//                 std::cout << "操作已取消" << std::endl;
//                 return 0;
//             }
//
//             try {
//                 if (inventory_futures[i].wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
//                     if (inventory_futures[i].get()) {
//                         available_products.push_back(recommendations[i]);
//                     }
//                 } else {
//                     std::cout << "庫存檢查超時: " << recommendations[i] << std::endl;
//                 }
//             } catch (const std::exception& e) {
//                 std::cerr << "庫存檢查錯誤: " << e.what() << std::endl;
//             }
//
//             // 動態調整並行度
//             orchestrator.adjust_parallelism();
//         }
//
//         // 輸出結果
//         std::cout << "使用者數據: " << user_data << std::endl;
//         std::cout << "訂單歷史: " << order_history.size() << " 個訂單" << std::endl;
//         std::cout << "可用的推薦產品: " << std::endl;
//         for (const auto& product : available_products) {
//             std::cout << " - " << product << std::endl;
//         }
//
//         auto end = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//         std::cout << "總執行時間: " << duration.count() << "ms" << std::endl;
//
//     } catch (const std::exception& e) {
//         std::cerr << "發生錯誤: " << e.what() << std::endl;
//     }
//
// }