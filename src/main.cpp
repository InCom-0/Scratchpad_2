
#include <iostream>
#include <thread>

#include <exec/async_scope.hpp>
#include <exec/execute.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>



#include <settings.hpp>
#include <tableTest.hpp>

int main() {

    using namespace std::chrono_literals;

    exec::static_thread_pool pool{4};
    exec::async_scope        scope{};
    auto                     sch = pool.get_scheduler();

    auto step = [](int jobId, const char *stage, stdexec::inplace_stop_token tk) -> bool {
        for (int i = 0; i < 25; ++i) {
            if (tk.stop_requested()) {
                std::cout << "job " << jobId << " stopped in " << stage << "\n";
                return false;
            }
            std::this_thread::sleep_for(15ms);
        }
        std::cout << "job " << jobId << " finished " << stage << "\n";
        return true;
    };

    auto make_job = [=](int jobId) {
        return stdexec::starts_on(
            sch,
            stdexec::read_env(stdexec::get_stop_token) | stdexec::then([=](stdexec::inplace_stop_token tk) {
                if (! step(jobId, "stage-1", tk)) { return; }
                if (! step(jobId, "stage-2", tk)) { return; }
                if (! step(jobId, "stage-3", tk)) { return; }
            }));
    };

    scope.spawn(make_job(1));
    scope.spawn(make_job(2));
    scope.spawn(make_job(3));

    std::this_thread::sleep_for(600ms);
    scope.request_stop();                 // cancel all jobs in this scope
    stdexec::sync_wait(scope.on_empty()); // wait for all jobs to drain

    return 0;
}