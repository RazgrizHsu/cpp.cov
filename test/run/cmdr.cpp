#include "doctest.h"
#include <thread>
#include <vector>
#include <chrono>
#include <rz/idx.h>
#include <spdlog/idx.h>
#include <spdlog/sinks/ostream_sink.h>
#include <csignal>
#include <future>
#include <atomic>

// class CT {
// public:
//     std::ostringstream os;
//     std::shared_ptr<lg::logger> l;
//
//     CT() {
//         auto s = std::make_shared<lg::sinks::ostream_sink_mt>(os);
//         l = std::make_shared<lg::logger>("test", s);
//         lg::set_default_logger(l);
//         lg::set_level(lg::level::debug);
//     }
//     ~CT() { lg::drop_all(); }
//
//     void log(const std::string &m) {
//         os << m << std::endl;
//         std::cout << m << std::endl;
//         std::cout.flush();
//     }
//
//     bool waitExit(pid_t p, int t) {
//         for (int i = 0; i < t * 10; ++i) {
//             int s; pid_t r = waitpid(p, &s, WNOHANG);
//             if (r == p) {
//                 log("PID " + std::to_string(p) + " exited: " + std::to_string(WEXITSTATUS(s)));
//                 return true;
//             } else if (r == -1) {
//                 log("Error for PID " + std::to_string(p) + ": " + std::string(strerror(errno)));
//                 return false;
//             }
//             std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         }
//         log("Timeout waiting for PID " + std::to_string(p));
//         return false;
//     }
//
//     std::string pInfo(pid_t p) {
//         char c[256]; snprintf(c, sizeof(c), "ps -p %d -o pid,ppid,state,command", p);
//         std::string o; FILE *pipe = popen(c, "r");
//         if (pipe) {
//             char b[128];
//             while (fgets(b, sizeof(b), pipe) != nullptr) o += b;
//             pclose(pipe);
//         }
//         return o;
//     }
// };
//
// TEST_CASE("Cmdr: All") {
//     //=== 基本功能測試 ===
//     SUBCASE("Base") {
//         // 基本命令
//         { std::string o; int ec = rz::exec("echo 'Hello'", [&](bool i, const std::string &l) { o += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(o, "Hello\n"); }
//
//         // 退出碼
//         { int ec = rz::exec("exit 15", [&](bool i, const std::string &l) {}); CHECK_EQ(ec, 15); }
//
//         // 錯誤命令
//         { int ec = rz::exec("not_cmd", [](bool i, const std::string &l) {}); CHECK_NE(ec, 0); }
//
//         // 標準錯誤
//         { std::string e; int ec = rz::exec("echo 'Err' >&2", [&](bool i, const std::string &l) { if (!i) e += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(e, "Err\n"); }
//
//         // 長輸出
//         { std::string o; int ec = rz::exec("for i in {1..1000}; do echo $i; done", [&](bool i, const std::string &l) { o += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(o.substr(0, 4), "1\n2\n"); CHECK(o.find("999\n1000\n") != std::string::npos); }
//
//         // 快速命令
//         { std::string o; int ec = rz::exec("echo 'Quick'", [&](bool i, const std::string &l) { o += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(o, "Quick\n"); }
//
//         // 多行輸出
//         { std::vector<std::string> ls; int ec = rz::exec("echo 'L1'; echo 'L2'; echo 'L3'", [&](bool i, const std::string &l) { ls.push_back(l); }); CHECK_EQ(ec, 0); CHECK_EQ(ls.size(), 3); CHECK_EQ(ls[0], "L1"); CHECK_EQ(ls[1], "L2"); CHECK_EQ(ls[2], "L3"); }
//
//         // 混合輸出
//         { std::string o, e; int ec = rz::exec("echo 'out'; echo 'err' >&2", [&](bool i, const std::string &l) { (i ? o : e) += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(o, "out\n"); CHECK_EQ(e, "err\n"); }
//
//         // 異步執行
//         { rz::Cmdr c; auto f = c.execAsync("echo 'Async'", [](bool i, const std::string &l) { CHECK_EQ(l, "Async"); }); int ec = f.wait(); CHECK_EQ(ec, 0); }
//
//         // 小量輸出
//         { std::string o; int ec = rz::exec("echo 'Small'", [&](bool i, const std::string &l) { o += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(o, "Small\n"); }
//
//         // 多行輸出
//         { std::string o; int ec = rz::exec("echo 'L1' && echo 'L2' && echo 'L3'", [&](bool i, const std::string &l) { o += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(o, "L1\nL2\nL3\n"); }
//
//         // 中等輸出
//         { std::string o; int ec = rz::exec("for i in {1..100}; do echo \"L$i\"; done", [&](bool i, const std::string &l) { o += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(std::count(o.begin(), o.end(), '\n'), 100); }
//
//         // 大量輸出
//         { std::string o; int ec = rz::exec("for i in {1..10000}; do echo \"L$i\"; done", [&](bool i, const std::string &l) { o += l + "\n"; }); CHECK_EQ(ec, 0); CHECK_EQ(std::count(o.begin(), o.end(), '\n'), 10000); }
//
//         // Yes命令
//         { std::string o; int lc = 0; int ec = rz::exec("yes 'test' | head -n 10000", [&](bool i, const std::string &l) { o += l + "\n"; ++lc; }); CHECK_EQ(ec, 0); CHECK_EQ(lc, 10000); CHECK(o.substr(0, 5) == "test\n"); }
//     }
//
//     //=== 短處理程式測試 ===
//     SUBCASE("ShortProc") {
//         CT ct; rz::Cmdr c(true); ct.log("Start ShortProc");
//         std::atomic<bool> ok(false), err(false);
//         auto f = c.execAsync("echo 'Test' && exit 0", [&](bool i, const std::string &l) { ct.log("Out: " + std::string(i ? "out: " : "err: ") + l); ok = true; });
//         try { int r = f.wait(); CHECK_EQ(r, 0); } catch (const std::exception &e) { err = true; ct.log("Exc: " + std::string(e.what())); }
//         CHECK(ok); CHECK(!err);
//         ct.log("ShortProc done");
//     }
//
//     //=== 綜合測試 ===
//     SUBCASE("CompCmdr") {
//         CT ct; rz::Cmdr c(true); ct.log("Start CompCmdr");
//
//         // 基本
//         SUBCASE("Basic") {
//             ct.log("Basic test"); std::string o;
//             auto f = c.execAsync("echo 'Hi'", [&](bool i, const std::string &l) { ct.log("Out: " + string(i ? "out: " : "err: ") + l); if (i) o += l; });
//             int r = f.wait(); CHECK_EQ(r, 0); CHECK_EQ(o, "Hi");
//             ct.log("Basic passed");
//         }
//
//         // 錯誤
//         SUBCASE("Err") {
//             ct.log("Err test");
//             auto f = c.execAsync("bad_cmd", [&](bool i, const std::string &l) { ct.log("Out: " + string(i ? "out: " : "err: ") + l); });
//             int r = f.wait(); CHECK_NE(r, 0);
//             ct.log("Err passed");
//         }
//
//         // 超時
//         SUBCASE("TO") {
//             ct.log("TO test"); auto start = std::chrono::steady_clock::now(); bool caught = false; std::string e;
//             try {
//                 auto f = c.execAsync("sleep 5", [&](bool i, const std::string &l) { ct.log("Out: " + l); }, { .timeout = 1 });
//                 f.wait();
//             } catch (const std::runtime_error &ex) {
//                 caught = true; e = ex.what(); auto end = std::chrono::steady_clock::now();
//                 auto s = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
//                 CHECK_GE(s, 1); CHECK_LT(s, 3); CHECK_EQ(e, "[cmdr] Command execution timed out");
//             }
//             CHECK(caught);
//             ct.log("TO passed");
//         }
//
//         // 多命令
//         SUBCASE("Multi") {
//             ct.log("Multi test"); std::vector<std::string> os;
//             auto f = c.execAsync("echo 'C1' && echo 'C2'", [&](bool i, const std::string &l) { if (i) os.push_back(l); ct.log("Out: " + l); });
//             f.wait(); CHECK_EQ(os.size(), 2); CHECK_EQ(os[0], "C1"); CHECK_EQ(os[1], "C2");
//             ct.log("Multi passed");
//         }
//
//         // 長時間運行
//         SUBCASE("Long") {
//             ct.log("Long test"); std::atomic<bool> d(false);
//             auto f = c.execAsync("sleep 2 && echo 'Done'", [&](bool i, const std::string &l) { ct.log("Out: " + l); d = true; });
//             std::this_thread::sleep_for(std::chrono::seconds(1)); CHECK(!d);
//             f.wait(); CHECK(d);
//             ct.log("Long passed");
//         }
//
//         // 退出碼
//         SUBCASE("ExitCode") {
//             ct.log("ExitCode test"); std::atomic<bool> cp(false); std::atomic<int> code(-1);
//             auto start = std::chrono::steady_clock::now();
//             auto f = c.execAsync("sleep 1 && echo 'Done' && exit 42", [&](bool i, const std::string &l) { ct.log("Out: " + string(i ? "out: " : "err: ") + l); if (l == "Done") cp = true; });
//             std::thread mon([&]() {
//                 ct.log("Mon thread start");
//                 auto st = f.rstFuture.wait_for(std::chrono::seconds(3));
//                 if (st == std::future_status::ready) {
//                     ct.log("Future ready");
//                     try { code = f.wait(); ct.log("Code: " + std::to_string(code)); }
//                     catch (const std::exception &e) { ct.log("Exc: " + std::string(e.what())); }
//                 } else { ct.log("Future TO"); }
//             });
//             mon.join();
//             auto end = std::chrono::steady_clock::now();
//             auto dur = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
//             ct.log("ExitCode finished in " + std::to_string(dur) + "s");
//             CHECK(cp); CHECK_EQ(code, 42);
//             ct.log("ExitCode passed");
//         }
//
//         // 大輸出
//         SUBCASE("LargeOut") {
//             ct.log("LargeOut test"); const int N = 1000; std::atomic<int> cnt(0);
//             auto f = c.execAsync("for i in {1.." + std::to_string(N) + "}; do echo \"L$i\"; done", [&](bool i, const std::string &l) { cnt++; if (cnt % 200 == 0) ct.log("Got " + std::to_string(cnt)); });
//             f.wait(); CHECK_EQ(cnt, N);
//             ct.log("LargeOut passed");
//         }
//
//         // 殘留檢查
//         SUBCASE("Leftover") {
//             ct.log("Leftover test"); std::string ps;
//             FILE *p = popen("pgrep -f 'sleep'", "r");
//             if (p) { char b[128]; while (fgets(b, sizeof(b), p) != nullptr) ps += b; pclose(p); }
//             CHECK(ps.empty());
//             ct.log("No leftover procs");
//         }
//
//         ct.log("CompCmdr done");
//     }
//
//     //=== 信號處理與終止 ===
//     SUBCASE("SigTerm") {
//         CT ct; rz::Cmdr c(true); ct.log("Start SigTerm");
//
//         // 正常終止
//         SUBCASE("NormTerm") {
//             ct.log("NormTerm test"); std::atomic<bool> tm(false), cp(false);
//             auto t = c.execAsync("sleep 10 && echo 'Done'", [&](bool i, const std::string &l) { ct.log("Out: " + l); cp = true; });
//             int tid = t.taskId; std::this_thread::sleep_for(std::chrono::seconds(1));
//             bool tr = rz::terminateTask(tid); tm = true;
//             try { int ec = t.wait(); ct.log("Task term: " + std::to_string(ec)); CHECK_NE(ec, 0); }
//             catch (const std::exception &e) { ct.log("Exc: " + std::string(e.what())); }
//             CHECK(tm); CHECK(!cp); CHECK(tr);
//             ct.log("NormTerm passed");
//         }
//
//         // 強制終止
//         SUBCASE("ForceTerm") {
//             ct.log("ForceTerm test");
//             auto t = c.execAsync("trap '' TERM; sleep 10; echo 'Should not see'", [&](bool i, const std::string &l) { ct.log("Out: " + l); });
//             int tid = t.taskId; std::this_thread::sleep_for(std::chrono::seconds(1));
//             rz::terminateTask(tid); std::this_thread::sleep_for(std::chrono::milliseconds(500));
//             bool sr = !t.isDone(); CHECK(sr);
//             bool kr = rz::killTask(tid); CHECK(kr);
//             try { int ec = t.wait(); ct.log("Task killed: " + std::to_string(ec)); CHECK_EQ(ec, 137); }
//             catch (const std::exception &e) { ct.log("Exc: " + std::string(e.what())); CHECK(false); }
//             ct.log("ForceTerm passed");
//         }
//
//         // 終止所有
//         SUBCASE("TermAll") {
//             ct.log("TermAll test"); std::vector<rz::ICmdRst> ts; const int tc = 5;
//             for (int i = 0; i < tc; i++) {
//                 auto t = c.execAsync("sleep " + std::to_string(5 + i) + " && echo 'Task " + std::to_string(i) + " done'",
//                     [&, i](bool is, const std::string &l) { ct.log("Task " + std::to_string(i) + " out: " + l); });
//                 ts.push_back(std::move(t));
//             }
//             std::this_thread::sleep_for(std::chrono::seconds(2));
//             rz::terminateAllTasks(); std::this_thread::sleep_for(std::chrono::seconds(1));
//             bool at = true;
//             for (int i = 0; i < tc; i++) {
//                 try { int ec = ts[i].wait(); ct.log("Task " + std::to_string(i) + " ec: " + std::to_string(ec)); if (ec == 0) at = false; }
//                 catch (const std::exception &e) { ct.log("Exc for task " + std::to_string(i) + ": " + std::string(e.what())); }
//             }
//             CHECK(at);
//             ct.log("TermAll passed");
//         }
//
//         ct.log("SigTerm done");
//     }
//
//     //=== 超時邊界 ===
//     SUBCASE("TOEdge") {
//         CT ct; rz::Cmdr c(true); ct.log("Start TOEdge");
//
//         // 邊界完成
//         SUBCASE("CompAtBound") {
//             ct.log("CompAtBound test"); std::atomic<bool> cp(false);
//             rz::opts op; op.timeout = 2; auto start = std::chrono::steady_clock::now();
//             auto t = c.execAsync("sleep 1.9 && echo 'Just in time'", [&](bool i, const std::string &l) { ct.log("Out: " + l); cp = true; }, op);
//             try {
//                 int ec = t.wait(); auto end = std::chrono::steady_clock::now();
//                 auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//                 ct.log("Task done in " + std::to_string(dur) + "ms, ec: " + std::to_string(ec));
//                 CHECK_EQ(ec, 0); CHECK(cp); CHECK_LT(dur, 2100);
//             } catch (const std::exception &e) { ct.log("Exc: " + std::string(e.what())); CHECK(false); }
//             ct.log("CompAtBound passed");
//         }
//
//         // 超時後輸出
//         SUBCASE("OutAfterTO") {
//             ct.log("OutAfterTO test"); std::atomic<int> oc(0); rz::opts op; op.timeout = 1;
//             try {
//                 auto t = c.execAsync("sleep 3", [&](bool i, const std::string &l) { oc++; }, op);
//                 t.wait(); CHECK(false);
//             } catch (const std::runtime_error &e) {
//                 ct.log("Expected exc: " + std::string(e.what()));
//                 CHECK_EQ(std::string(e.what()), "[cmdr] Command execution timed out");
//             }
//             std::this_thread::sleep_for(std::chrono::seconds(1)); CHECK_EQ(oc, 0);
//             ct.log("OutAfterTO passed");
//         }
//
//         // 零超時
//         SUBCASE("ZeroTO") {
//             ct.log("ZeroTO test"); rz::opts op; op.timeout = 0;
//             try {
//                 auto t = c.execAsync("echo 'Should not see'", [&](bool i, const std::string &l) { ct.log("Out: " + l); }, op);
//                 t.wait(); CHECK(false);
//             } catch (const std::runtime_error &e) {
//                 ct.log("Expected exc: " + std::string(e.what()));
//                 CHECK_EQ(std::string(e.what()), "[cmdr] Command execution timed out");
//             }
//             ct.log("ZeroTO passed");
//         }
//
//         ct.log("TOEdge done");
//     }
//
//     //=== 資源管理 ===
//     SUBCASE("ResMgmt") {
//         CT ct; ct.log("Start ResMgmt");
//
//         // 重複創建銷毀
//         SUBCASE("RepCre") {
//             ct.log("RepCre test"); const int its = 10;
//             for (int i = 0; i < its; i++) { rz::Cmdr *c = new rz::Cmdr(true); delete c; }
//             CHECK(true);
//             ct.log("RepCre passed");
//         }
//
//         // 多命令清理
//         SUBCASE("ManyCmdClean") {
//             ct.log("ManyCmdClean test"); rz::Cmdr c(true); const int cc = 20; std::vector<rz::ICmdRst> ts;
//             for (int i = 0; i < cc; i++) {
//                 auto t = c.execAsync("echo 'Task " + std::to_string(i) + "'", [&](bool i, const std::string &l) {});
//                 ts.push_back(std::move(t));
//             }
//             for (auto &t : ts) t.wait();
//             std::string ps = ct.pInfo(getpid()); ct.log("Proc info: " + ps);
//             std::string cps;
//             FILE *pipe = popen(("ps -o pid,ppid,command | grep " + std::to_string(getpid())).c_str(), "r");
//             if (pipe) { char buf[256]; while (fgets(buf, sizeof(buf), pipe) != nullptr) cps += buf; pclose(pipe); }
//             ct.log("Children: " + cps); CHECK(cps.find("sleep") == std::string::npos);
//             std::string fdi;
//             pipe = popen(("ls -l /proc/" + std::to_string(getpid()) + "/fd | wc -l").c_str(), "r");
//             if (pipe) { char buf[256]; if (fgets(buf, sizeof(buf), pipe) != nullptr) fdi = buf; pclose(pipe); }
//             ct.log("FD count: " + fdi); int fc = std::stoi(fdi); CHECK(fc < 100);
//             ct.log("ManyCmdClean passed");
//         }
//
//         ct.log("ResMgmt done");
//     }
//
//     //=== 並發與多執行緒 ===
//     SUBCASE("Concur") {
//         CT ct; ct.log("Start Concur");
//
//         // 同時執行
//         SUBCASE("SimCmd") {
//             ct.log("SimCmd test"); rz::Cmdr c(true); const int cc = 5; std::atomic<int> cpc(0);
//             std::vector<std::future<void>> fs;
//             for (int i = 0; i < cc; i++) {
//                 fs.push_back(std::async(std::launch::async, [&c, i, &cpc]() {
//                     auto t = c.execAsync("sleep " + std::to_string(0.2 + i * 0.1) + " && echo 'Task " + std::to_string(i) + " done'",
//                         [i](bool is, const std::string &l) { std::cout << "Task " << i << " out: " << l << std::endl; });
//                     try { int ec = t.wait(); if (ec == 0) cpc++; }
//                     catch (const std::exception &e) { std::cerr << "Task " << i << " exc: " << e.what() << std::endl; }
//                 }));
//             }
//             for (auto &f : fs) f.wait();
//             ct.log("Done count: " + std::to_string(cpc)); CHECK_EQ(cpc, cc);
//             ct.log("SimCmd passed");
//         }
//
//         // 共享實例
//         SUBCASE("SharedInst") {
//             ct.log("SharedInst test"); rz::Cmdr c(true); std::atomic<int> sc(0), fc(0);
//             const int tc = 5, cpt = 5; std::vector<std::thread> ts;
//             for (int t = 0; t < tc; t++) {
//                 ts.push_back(std::thread([&c, t, &sc, &fc, cpt]() {
//                     for (int cmd = 0; cmd < cpt; cmd++) {
//                         try {
//                             auto task = c.execAsync("echo 'T" + std::to_string(t) + " C" + std::to_string(cmd) + "'", [](bool i, const std::string &l) {});
//                             int ec = task.wait(); (ec == 0) ? sc++ : fc++;
//                         } catch (const std::exception &e) { fc++; std::cerr << "T" << t << " C" << cmd << " exc: " << e.what() << std::endl; }
//                     }
//                 }));
//             }
//             for (auto &t : ts) t.join();
//             ct.log("Success: " + std::to_string(sc) + ", Fail: " + std::to_string(fc));
//             CHECK_EQ(sc, tc * cpt); CHECK_EQ(fc, 0);
//             ct.log("SharedInst passed");
//         }
//
//         // 取消與重啟
//         SUBCASE("CanRestart") {
//             ct.log("CanRestart test"); rz::Cmdr c(true); std::atomic<bool> fc(false), sc(false);
//             auto t1 = c.execAsync("sleep 5 && echo 'Should not see'", [&](bool i, const std::string &l) { ct.log("T1 out: " + l); });
//             int tid1 = t1.taskId; std::this_thread::sleep_for(std::chrono::seconds(1));
//             bool cr = rz::terminateTask(tid1); CHECK(cr); fc = true;
//             auto t2 = c.execAsync("sleep 1 && echo 'Second done'", [&](bool i, const std::string &l) { ct.log("T2 out: " + l); sc = true; });
//             int ec2 = t2.wait(); CHECK_EQ(ec2, 0); CHECK(sc);
//             try { int ec1 = t1.wait(); ct.log("T1 ec: " + std::to_string(ec1)); CHECK_NE(ec1, 0); }
//             catch (const std::exception &e) { ct.log("T1 exc: " + std::string(e.what())); }
//             CHECK(fc);
//             ct.log("CanRestart passed");
//         }
//
//         ct.log("Concur done");
//     }
//
//     //=== 系統環境與信號 ===
//     SUBCASE("SysEnv") {
//         CT ct; ct.log("Start SysEnv");
//
//         // 環境變數
//         SUBCASE("EnvVar") {
//             ct.log("EnvVar test"); setenv("CMDR_TEST_VAR", "test_val", 1);
//             std::string o; rz::Cmdr c(true);
//             auto t = c.execAsync("echo $CMDR_TEST_VAR", [&](bool i, const std::string &l) { o = l; });
//             int ec = t.wait(); CHECK_EQ(ec, 0); CHECK_EQ(o, "test_val");
//             unsetenv("CMDR_TEST_VAR");
//             ct.log("EnvVar passed");
//         }
//
//         // SIGPIPE
//         SUBCASE("SigPipe") {
//             ct.log("SigPipe test"); rz::Cmdr c(true);
//             auto t = c.execAsync("yes | head -n 10", [&](bool i, const std::string &l) { ct.log("Out: " + l); });
//             int ec = t.wait(); CHECK_EQ(ec, 0);
//             std::string ps;
//             FILE *pipe = popen("pgrep -f '^yes$'", "r");
//             if (pipe) { char buf[128]; while (fgets(buf, sizeof(buf), pipe) != nullptr) ps += buf; pclose(pipe); }
//             CHECK(ps.empty());
//             ct.log("SigPipe passed");
//         }
//
//         // 重新導向
//         SUBCASE("Redirect") {
//             ct.log("Redirect test"); rz::Cmdr c(true); std::string so, se;
//             auto t = c.execAsync("echo 'stdout' > /dev/null; echo 'stderr' >&2", [&](bool i, const std::string &l) { (i ? so : se) += l; });
//             int ec = t.wait(); CHECK_EQ(ec, 0); CHECK(so.empty()); CHECK_EQ(se, "stderr");
//             ct.log("Redirect passed");
//         }
//
//         ct.log("SysEnv done");
//     }
//
//     //=== 邊界案例 ===
//     SUBCASE("Edge") {
//         CT ct; ct.log("Start Edge");
//
//         // 空命令
//         SUBCASE("EmptyCmd") {
//             ct.log("EmptyCmd test"); rz::Cmdr c(true);
//             auto t = c.execAsync("", [&](bool i, const std::string &l) { ct.log("Out: " + l); });
//             int ec = t.wait(); CHECK_EQ(ec, 0);
//             ct.log("EmptyCmd passed");
//         }
//
//         // 無效命令
//         SUBCASE("InvalidCmd") {
//             ct.log("InvalidCmd test"); rz::Cmdr c(true);
//             auto t = c.execAsync("not_exist_cmd", [&](bool i, const std::string &l) { ct.log("Out: " + l); });
//             int ec = t.wait(); CHECK_NE(ec, 0);
//             ct.log("InvalidCmd passed");
//         }
//
//         // 極長命令
//         SUBCASE("LongCmd") {
//             ct.log("LongCmd test"); std::string lcmd = "echo '"; for (int i = 0; i < 10000; i++) lcmd += "a"; lcmd += "'";
//             rz::Cmdr c(true); std::string o;
//             auto t = c.execAsync(lcmd, [&](bool i, const std::string &l) { o = l; });
//             int ec = t.wait(); CHECK_EQ(ec, 0); CHECK_EQ(o.length(), 10000);
//             ct.log("LongCmd passed");
//         }
//
//         // 二進制輸出
//         SUBCASE("BinOut") {
//             ct.log("BinOut test"); rz::Cmdr c(true); std::atomic<bool> ro(false);
//             auto t = c.execAsync("dd if=/dev/urandom bs=1024 count=1", [&](bool i, const std::string &l) { ct.log("Bin len: " + std::to_string(l.length())); ro = true; });
//             int ec = t.wait(); CHECK_EQ(ec, 0); CHECK(ro);
//             ct.log("BinOut passed");
//         }
//
//         ct.log("Edge done");
//     }
//
// }
