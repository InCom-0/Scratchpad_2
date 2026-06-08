
#include <iostream>
#include <thread>

#include <exec/async_scope.hpp>
#include <exec/execute.hpp>
#include <exec/repeat_n.hpp>
#include <exec/repeat_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <exec/task.hpp>
#include <exec/unless_stop_requested.hpp>
#include <stdexec/execution.hpp>


#include <readerwriterqueue.h>


#include <settings.hpp>

#include <tbb/tbb.h>


class Job {
public:
    exec::async_scope                  ascope = {};
    moodycamel::ReaderWriterQueue<int> m_spsc_q;


    template <typename SCHD, typename LAM, typename... Args>
    static Job spawn(SCHD const &sch, LAM, Args &&...args) {
        return Job(LAM{}, sch, std::forward<Args>(args)...);
    }

private:
    template <typename SCHD, typename LAM, typename... Args>
    Job(LAM, SCHD const &sch, Args... args) {
        ascope.spawn(stdexec::starts_on(sch, LAM{}(std::forward<Args>(args)..., m_spsc_q)));
    }
};

template <typename... Qs>
class Job_PRE {
public:
    exec::async_scope ascope = {};
    std::tuple<Qs...> m_qs;

    template <typename LAM, typename... Tail, std::size_t... I1, std::size_t... I2>
    explicit Job_PRE(LAM, std::index_sequence<I1...>, std::index_sequence<I2...>, Tail &&...tail)
        : m_qs(std::forward<Tail...[sizeof...(I1) + 1 + I2]>(tail...[sizeof...(I1) + 1 + I2])...) {
        ascope.spawn(LAM{}(tail...[I1]..., std::get<I2>(m_qs)...));
    }


    Job_PRE()                = delete;
    Job_PRE(Job_PRE &&)      = delete;
    Job_PRE(Job_PRE &)       = delete;
    Job_PRE(Job_PRE const &) = delete;

    ~Job_PRE() = default;

private:
};


struct Separator {};

template <typename LAM, typename... Tail>
auto spawn(LAM, Tail &&...tail) {

    constexpr std::size_t count = (0u + ... + (std::is_same_v<std::remove_cvref_t<Tail>, Separator> ? 1u : 0u));
    static_assert(count < 2, "Pass one or none Job_PRE::Separator{}.");

    constexpr std::size_t sep = []() -> std::size_t {
        constexpr std::size_t count = (0u + ... + (std::is_same_v<std::remove_cvref_t<Tail>, Separator> ? 1u : 0u));
        static_assert(count == 1, "Pass exactly one Job_PRE::Separator{}.");

        constexpr bool is_sep[] = {std::is_same_v<std::remove_cvref_t<Tail>, Separator>...};
        for (std::size_t i = 0; i < sizeof...(Tail); ++i) {
            if (is_sep[i]) { return i; }
        }
        return sizeof...(Tail);
    }();

    return Job_PRE(LAM{}, std::make_index_sequence<sep>{},
                   std::make_index_sequence<count == 0 ? 0 : sizeof...(Tail) - sep - 1>{}, std::forward<Tail>(tail)...);
}

template <typename LAM, typename... Tail, std::size_t... I1, std::size_t... I2>
Job_PRE(LAM, std::index_sequence<I1...>, std::index_sequence<I2...>, Tail &&...tail)
    -> Job_PRE<std::remove_cvref_t<Tail...[sizeof...(I1) + 1 + I2]>...>;


int main() {

    using namespace std::chrono_literals;

    exec::static_thread_pool pool{4};
    exec::async_scope        scope{};
    auto                     sch = pool.get_scheduler();

    moodycamel::ReaderWriterQueue<int> qqq;


    auto qqq2 = std::move(qqq);


    auto lam_1 = [](int i, double d, auto &&sched, moodycamel::ReaderWriterQueue<int> const &q,
                    int e) -> exec::basic_task<void, experimental::execution::__task::inline_task_context<void>> {
        stdexec::schedule(sched);
        co_return;
    };


    auto one_iter = stdexec::schedule(sch) | stdexec::let_value([] {
                        return stdexec::just() | stdexec::then([]() {
                                   // cooperative cancellation points inside one iteration
                                   for (int i = 0; i < 11; ++i) {
                                       std::cout << "Iter: " << i << "\n";

                                       std::this_thread::sleep_for(5ms);
                                   }
                                   //    if (tk.stop_requested()) { return; }
                               }) |
                               exec::unless_stop_requested();
                    }) |
                    exec::repeat_n(4);


    auto spawnIn = [](exec::async_scope &ascope, auto task) { ascope.spawn(std::move(task)); };


    Job_PRE job_1 = spawn(lam_1, 7, 15.5, sch, Separator{}, moodycamel::ReaderWriterQueue<int>{}, 5);


    // scope.spawn(std::move(one_iter));

    std::this_thread::sleep_for(200ms);
    scope.request_stop();                 // cancel all jobs in this scope
    stdexec::sync_wait(scope.on_empty()); // wait for all jobs to drain

    return 0;
}