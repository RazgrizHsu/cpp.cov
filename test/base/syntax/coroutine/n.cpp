// #include <iostream>
// #include <coroutine>
// #include <chrono>
// #include <thread>
// #include <vector>
// #include <queue>
// #include <random>
// #include <stdexcept>
// #include <optional>
// #include <atomic>
// #include <future>
// #include "doctest.h"
//
// template<typename T>
// struct Task {
//     struct promise_type {
//         std::optional<T> result;
//         std::exception_ptr exception;
//
//         Task get_return_object() { return Task(std::coroutine_handle<promise_type>::from_promise(*this)); }
//         std::suspend_never initial_suspend() { return {}; }
//         std::suspend_never final_suspend() noexcept { return {}; }
//         void unhandled_exception() { exception = std::current_exception(); }
//         template<typename U>
//         void return_value(U&& value) { result = std::forward<U>(value); }
//     };
//
//     std::coroutine_handle<promise_type> handle;
//
//     Task(std::coroutine_handle<promise_type> h) : handle(h) {}
//     ~Task() { if (handle) handle.destroy(); }
//
//     T get_result() {
//         if (handle.promise().exception) {
//             std::rethrow_exception(handle.promise().exception);
//         }
//         return std::move(handle.promise().result.value());
//     }
// };
//
// template<typename T>
// struct Awaitable {
//     std::chrono::milliseconds delay;
//     std::string service_name;
//     std::function<T()> operation;
//     std::atomic<bool>& cancelled;
//     std::chrono::milliseconds timeout;
//
//     bool await_ready() const { return false; }
//     void await_suspend(std::coroutine_handle<> h) {
//         std::thread([this, h] {
//             auto start = std::chrono::high_resolution_clock::now();
//             while (std::chrono::high_resolution_clock::now() - start < delay) {
//                 if (cancelled.load()) {
//                     throw std::runtime_error("操作已取消");
//                 }
//                 if (std::chrono::high_resolution_clock::now() - start > timeout) {
//                     throw std::runtime_error(service_name + " 操作超時");
//                 }
//                 std::this_thread::sleep_for(std::chrono::milliseconds(50));
//             }
//             if (rand() % 10 == 0) {  // 10% 概率失敗
//                 throw std::runtime_error(service_name + " 調用失敗");
//             }
//             h.resume();
//         }).detach();
//     }
//     T await_resume() { return operation(); }
// };
//
// class ServiceOrchestrator {
// public:
//     ServiceOrchestrator() : current_load(0) {}
//
//     template<typename T>
//     Awaitable<T> call_service(const std::string& service_name, int delay_ms, std::function<T()> operation,
//                               std::atomic<bool>& cancelled, std::chrono::milliseconds timeout = std::chrono::seconds(1)) {
//         return Awaitable<T>{std::chrono::milliseconds(delay_ms), service_name, operation, cancelled, timeout};
//     }
//
//     void adjust_parallelism() {
//         if (current_load > 100) {
//             std::cout << "增加並行度" << std::endl;
//         } else if (current_load < 50) {
//             std::cout << "減少並行度" << std::endl;
//         }
//     }
//
//     std::atomic<int> current_load;
// };
//
// ServiceOrchestrator orchestrator;
//
// Task<std::string> get_user_data(int user_id, std::atomic<bool>& cancelled) {
//     co_return co_await orchestrator.call_service<std::string>(
//         "使用者數據服務", 200, []() { return "使用者數據"; }, cancelled
//     );
// }
//
// Task<std::vector<int>> get_order_history(int user_id, std::atomic<bool>& cancelled) {
//     co_return co_await orchestrator.call_service<std::vector<int>>(
//         "訂單歷史服務", 300, []() { return std::vector<int>{1, 2, 3}; }, cancelled
//     );
// }
//
// Task<std::vector<std::string>> get_recommendations(const std::vector<int>& order_history, std::atomic<bool>& cancelled) {
//     co_return co_await orchestrator.call_service<std::vector<std::string>>(
//         "推薦服務", 500, []() { return std::vector<std::string>{"商品A", "商品B", "商品C"}; }, cancelled
//     );
// }
//
// Task<bool> check_inventory(const std::string& product, std::atomic<bool>& cancelled) {
//     co_return co_await orchestrator.call_service<bool>(
//         "庫存服務", 200, []() { return true; }, cancelled
//     );
// }
//
// Task<std::vector<bool>> batch_check_inventory(const std::vector<std::string>& products, std::atomic<bool>& cancelled) {
//     std::vector<Task<bool>> tasks;
//     for (const auto& product : products) {
//         tasks.push_back(check_inventory(product, cancelled));
//     }
//
//     std::vector<bool> results;
//     for (auto& task : tasks) {
//         results.push_back(co_await task);
//         orchestrator.adjust_parallelism();
//     }
//     co_return results;
// }
//
// Task<void> process_user_data(int user_id) {
//     auto start = std::chrono::high_resolution_clock::now();
//     std::atomic<bool> cancelled(false);
//
//     std::thread cancel_timer([&cancelled]() {
//         std::this_thread::sleep_for(std::chrono::seconds(2));
//         cancelled.store(true);
//     });
//
//     try {
//         auto user_data_task = get_user_data(user_id, cancelled);
//         auto order_history_task = get_order_history(user_id, cancelled);
//
//         std::string user_data = co_await user_data_task;
//         std::vector<int> order_history = co_await order_history_task;
//
//         if (cancelled.load()) {
//             std::cout << "操作已取消" << std::endl;
//             co_return;
//         }
//
//         auto recommendations_task = get_recommendations(order_history, cancelled);
//         auto recommendations = co_await recommendations_task;
//
//         if (cancelled.load()) {
//             std::cout << "操作已取消" << std::endl;
//             co_return;
//         }
//
//         auto inventory_results = co_await batch_check_inventory(recommendations, cancelled);
//
//         std::vector<std::string> available_products;
//         for (size_t i = 0; i < recommendations.size(); ++i) {
//             if (cancelled.load()) {
//                 std::cout << "操作已取消" << std::endl;
//                 co_return;
//             }
//
//             if (inventory_results[i]) {
//                 available_products.push_back(recommendations[i]);
//             }
//         }
//
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
//     cancel_timer.join();
// }
//
// TEST_CASE( "coroutine: base" ) {
//     auto task = process_user_data(12345);
//     // 等待協程完成
//     while (!task.handle.done()) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
//     // 這裡可以添加一些斷言來檢查結果
//     CHECK_TRUE(true);  // 示例斷言，實際應根據具體邏輯添加有意義的斷言
// }