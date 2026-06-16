#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

struct CustomerTask {
    std::string task_id;
    std::string customer_name;
    std::string customer_type;
    int arrival_time;
    int burst_time;
    int priority;
};

struct ExecutionSlice {
    std::string task_id;
    int start;
    int end;
};

struct TaskMetrics {
    int waiting_time;
    int turnaround_time;
    int response_time;
    int completion_time;
};

struct ScheduleResult {
    std::string algorithm;
    std::vector<ExecutionSlice> slices;
    std::map<std::string, TaskMetrics> metrics;
    double average_waiting_time = 0.0;
    double average_turnaround_time = 0.0;
    double average_response_time = 0.0;
    int total_time = 0;
};

struct SharedOperation {
    std::string actor;
    std::string type;
    int amount;
};

struct SharedAccountResult {
    int initial_balance = 0;
    int final_balance = 0;
    int successful_withdrawals = 0;
    int failed_withdrawals = 0;
    std::vector<std::string> history;
    std::vector<std::string> log;
};

struct PayrollResult {
    int employee_count = 0;
    int salary = 0;
    int semaphore_limit = 0;
    int max_concurrency_seen = 0;
    int total_paid = 0;
    std::map<std::string, int> balances;
    std::vector<std::string> log;
};

struct SynchronizationResult {
    SharedAccountResult shared_account;
    PayrollResult payroll;
};

struct BankerEvaluation {
    bool granted = false;
    bool safe = false;
    std::vector<std::string> safe_sequence;
    std::vector<int> available;
    std::string message;
};

struct BankerDemoResult {
    BankerEvaluation safe_state;
    BankerEvaluation safe_request;
    BankerEvaluation denied_request;
    std::map<std::string, std::vector<int>> need_matrix;
};

struct TransactionRequest {
    std::string request_id;
    std::string customer_name;
    std::string operation;
    std::string account_id;
    int amount = 0;
};

struct TransactionResponse {
    std::string request_id;
    std::string customer_name;
    std::string account_id;
    std::string status;
    std::string message;
    int balance = 0;
};

struct IpcResult {
    std::vector<TransactionRequest> requests;
    std::vector<TransactionResponse> responses;
    std::map<std::string, int> final_accounts;
    int processed_requests = 0;
    std::vector<std::string> log;
};

struct MemoryStep {
    std::string page;
    std::vector<std::string> frames;
    bool hit = false;
    std::string replaced;
};

struct MemoryResult {
    std::string algorithm;
    int frames_count = 0;
    std::vector<std::string> reference_string;
    std::vector<MemoryStep> steps;
    int page_faults = 0;
    int hits = 0;
    double hit_ratio = 0.0;
    double fault_ratio = 0.0;
};

struct MemoryDemoResult {
    std::vector<std::string> reference;
    MemoryResult fifo;
    MemoryResult lru;
};

struct ProjectSummary {
    std::vector<CustomerTask> tasks;
    ScheduleResult fcfs;
    ScheduleResult priority;
    ScheduleResult round_robin;
    SynchronizationResult synchronization;
    BankerDemoResult banker;
    IpcResult ipc;
    MemoryDemoResult memory;
};

std::string formatDouble(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

int readInt(const std::string& prompt) {
    int value = 0;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            return value;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "invalid input, enter an integer.\n";
    }
}

std::string readToken(const std::string& prompt) {
    std::string value;
    std::cout << prompt;
    std::cin >> value;
    return value;
}

void pressEnterToContinue() {
    std::cout << "\npress 0 and enter to return to menu: ";
    int dummy = 0;
    std::cin >> dummy;
}

std::vector<CustomerTask> buildDemoTasks() {
    return {
        {"R1", "Regular-A", "Regular", 0, 4, 1},
        {"P1", "Premium-B", "Premium", 1, 3, 3},
        {"V1", "VIP-C", "VIP", 2, 2, 5},
        {"C1", "Corporate-D", "Corporate", 3, 5, 4},
        {"R2", "Regular-E", "Regular", 4, 2, 1},
    };
}

ScheduleResult buildScheduleResult(const std::string& algorithm,
                                   const std::vector<CustomerTask>& tasks,
                                   const std::vector<ExecutionSlice>& slices,
                                   const std::map<std::string, int>& completion_times,
                                   const std::map<std::string, int>& first_starts) {
    ScheduleResult result;
    result.algorithm = algorithm;
    result.slices = slices;

    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;

    for (const auto& task : tasks) {
        TaskMetrics metrics;
        metrics.completion_time = completion_times.at(task.task_id);
        metrics.turnaround_time = metrics.completion_time - task.arrival_time;
        metrics.waiting_time = metrics.turnaround_time - task.burst_time;
        metrics.response_time = first_starts.at(task.task_id) - task.arrival_time;
        result.metrics[task.task_id] = metrics;

        total_waiting += metrics.waiting_time;
        total_turnaround += metrics.turnaround_time;
        total_response += metrics.response_time;
    }

    if (!tasks.empty()) {
        const double count = static_cast<double>(tasks.size());
        result.average_waiting_time = total_waiting / count;
        result.average_turnaround_time = total_turnaround / count;
        result.average_response_time = total_response / count;
    }

    for (const auto& slice : slices) {
        result.total_time = std::max(result.total_time, slice.end);
    }

    return result;
}

void appendOrExtend(std::vector<ExecutionSlice>& slices, const std::string& task_id, int start, int end) {
    // merge continuous execution of the same task
    if (!slices.empty() && slices.back().task_id == task_id && slices.back().end == start) {
        slices.back().end = end;
        return;
    }
    slices.push_back({task_id, start, end});
}

ScheduleResult runFcfs(const std::vector<CustomerTask>& tasks) {
    std::vector<CustomerTask> ordered = tasks;
    std::sort(ordered.begin(), ordered.end(), [](const CustomerTask& left, const CustomerTask& right) {
        if (left.arrival_time != right.arrival_time) {
            return left.arrival_time < right.arrival_time;
        }
        return left.task_id < right.task_id;
    });

    int time = 0;
    std::vector<ExecutionSlice> slices;
    std::map<std::string, int> completion_times;
    std::map<std::string, int> first_starts;

    for (const auto& task : ordered) {
        const int start = std::max(time, task.arrival_time);
        const int end = start + task.burst_time;
        slices.push_back({task.task_id, start, end});
        completion_times[task.task_id] = end;
        first_starts[task.task_id] = start;
        time = end;
    }

    return buildScheduleResult("FCFS", ordered, slices, completion_times, first_starts);
}

ScheduleResult runPriority(const std::vector<CustomerTask>& tasks) {
    std::vector<CustomerTask> ordered = tasks;
    std::sort(ordered.begin(), ordered.end(), [](const CustomerTask& left, const CustomerTask& right) {
        if (left.arrival_time != right.arrival_time) {
            return left.arrival_time < right.arrival_time;
        }
        return left.task_id < right.task_id;
    });

    std::map<std::string, int> remaining;
    std::map<std::string, int> completion_times;
    std::map<std::string, int> first_starts;
    for (const auto& task : ordered) {
        remaining[task.task_id] = task.burst_time;
    }

    std::vector<ExecutionSlice> slices;
    int time = ordered.empty() ? 0 : ordered.front().arrival_time;

    while (completion_times.size() < ordered.size()) {
        std::vector<CustomerTask> ready;
        for (const auto& task : ordered) {
            if (task.arrival_time <= time && remaining[task.task_id] > 0) {
                ready.push_back(task);
            }
        }

        if (ready.empty()) {
            for (const auto& task : ordered) {
                if (remaining[task.task_id] > 0) {
                    time = std::max(time + 1, task.arrival_time);
                    break;
                }
            }
            continue;
        }

        const auto selected = *std::max_element(ready.begin(), ready.end(), [](const CustomerTask& left,
                                                                               const CustomerTask& right) {
            if (left.priority != right.priority) {
                return left.priority < right.priority;
            }
            if (left.arrival_time != right.arrival_time) {
                return left.arrival_time > right.arrival_time;
            }
            return left.task_id > right.task_id;
        });

        if (first_starts.count(selected.task_id) == 0) {
            first_starts[selected.task_id] = time;
        }

        appendOrExtend(slices, selected.task_id, time, time + 1);
        --remaining[selected.task_id];
        ++time;

        if (remaining[selected.task_id] == 0) {
            completion_times[selected.task_id] = time;
        }
    }

    return buildScheduleResult("Priority", ordered, slices, completion_times, first_starts);
}

ScheduleResult runRoundRobin(const std::vector<CustomerTask>& tasks, int quantum) {
    if (quantum <= 0) {
        throw std::invalid_argument("quantum must be positive");
    }

    std::vector<CustomerTask> ordered = tasks;
    std::sort(ordered.begin(), ordered.end(), [](const CustomerTask& left, const CustomerTask& right) {
        if (left.arrival_time != right.arrival_time) {
            return left.arrival_time < right.arrival_time;
        }
        return left.task_id < right.task_id;
    });

    std::deque<CustomerTask> ready;
    std::map<std::string, int> remaining;
    std::map<std::string, int> completion_times;
    std::map<std::string, int> first_starts;
    for (const auto& task : ordered) {
        remaining[task.task_id] = task.burst_time;
    }

    std::size_t next_index = 0;
    int time = ordered.empty() ? 0 : ordered.front().arrival_time;
    std::vector<ExecutionSlice> slices;

    while (completion_times.size() < ordered.size()) {
        while (next_index < ordered.size() && ordered[next_index].arrival_time <= time) {
            ready.push_back(ordered[next_index]);
            ++next_index;
        }

        if (ready.empty()) {
            time = ordered[next_index].arrival_time;
            continue;
        }

        const CustomerTask current = ready.front();
        ready.pop_front();

        if (first_starts.count(current.task_id) == 0) {
            first_starts[current.task_id] = time;
        }

        const int start = time;
        const int run_time = std::min(quantum, remaining[current.task_id]);
        for (int step = 0; step < run_time; ++step) {
            ++time;
            --remaining[current.task_id];
            while (next_index < ordered.size() && ordered[next_index].arrival_time <= time) {
                ready.push_back(ordered[next_index]);
                ++next_index;
            }
            if (remaining[current.task_id] == 0) {
                break;
            }
        }

        slices.push_back({current.task_id, start, time});
        if (remaining[current.task_id] == 0) {
            completion_times[current.task_id] = time;
        } else {
            ready.push_back(current);
        }
    }

    return buildScheduleResult("Round Robin (q=" + std::to_string(quantum) + ")",
                               ordered,
                               slices,
                               completion_times,
                               first_starts);
}

std::string renderGanttText(const ScheduleResult& result) {
    if (result.slices.empty()) {
        return "no tasks scheduled";
    }

    std::ostringstream stream;
    for (const auto& slice : result.slices) {
        stream << "| " << slice.task_id << ' ';
    }
    stream << "|\n" << result.slices.front().start;
    for (const auto& slice : result.slices) {
        stream << ' ' << slice.end;
    }
    return stream.str();
}

void printSchedulingResult(const ScheduleResult& result) {
    std::cout << "\n" << result.algorithm << "\n";
    std::cout << renderGanttText(result) << "\n";
    std::cout << "average waiting time: " << formatDouble(result.average_waiting_time) << "\n";
    std::cout << "average turnaround time: " << formatDouble(result.average_turnaround_time) << "\n";
    std::cout << "average response time: " << formatDouble(result.average_response_time) << "\n";
    std::cout << "task metrics:\n";
    for (const auto& entry : result.metrics) {
        std::cout << "  " << entry.first
                  << " -> wait=" << entry.second.waiting_time
                  << ", tat=" << entry.second.turnaround_time
                  << ", response=" << entry.second.response_time
                  << ", complete=" << entry.second.completion_time << "\n";
    }
}

class EventLogger {
  public:
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.push_back(timeStamp() + " " + message);
    }

    std::vector<std::string> entries() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_;
    }

  private:
    std::string timeStamp() const {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
        return std::to_string(micros);
    }

    mutable std::mutex mutex_;
    std::vector<std::string> entries_;
};

class CountingSemaphore {
  public:
    explicit CountingSemaphore(int capacity) : available_(capacity) {}

    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&] { return available_ > 0; });
        --available_;
    }

    void release() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++available_;
        }
        condition_.notify_one();
    }

  private:
    int available_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

class BankAccount {
  public:
    BankAccount(std::string account_id, int balance, EventLogger& logger)
        : account_id_(std::move(account_id)), balance_(balance), logger_(logger) {}

    int deposit(int amount, const std::string& actor) {
        std::lock_guard<std::mutex> lock(mutex_);
        const int before = balance_;
        logger_.log(actor + " acquired mutex for deposit on " + account_id_);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        balance_ += amount;
        const int after = balance_;
        const std::string message =
            actor + " deposited " + std::to_string(amount) + " on " + account_id_ + ": " +
            std::to_string(before) + " -> " + std::to_string(after);
        history_.push_back(message);
        logger_.log(message);
        return after;
    }

    bool withdraw(int amount, const std::string& actor) {
        std::lock_guard<std::mutex> lock(mutex_);
        const int before = balance_;
        logger_.log(actor + " acquired mutex for withdraw on " + account_id_);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (before < amount) {
            const std::string message =
                actor + " failed withdrawal of " + std::to_string(amount) + " on " + account_id_ +
                ": balance " + std::to_string(before);
            history_.push_back(message);
            logger_.log(message);
            return false;
        }

        balance_ -= amount;
        const int after = balance_;
        const std::string message =
            actor + " withdrew " + std::to_string(amount) + " on " + account_id_ + ": " +
            std::to_string(before) + " -> " + std::to_string(after);
        history_.push_back(message);
        logger_.log(message);
        return true;
    }

    int balance() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return balance_;
    }

    std::vector<std::string> history() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return history_;
    }

  private:
    std::string account_id_;
    mutable std::mutex mutex_;
    int balance_;
    EventLogger& logger_;
    std::vector<std::string> history_;
};

SharedAccountResult runSharedAccountScenario(int initial_balance, const std::vector<SharedOperation>& operations) {
    EventLogger logger;
    BankAccount account("ACC-100", initial_balance, logger);
    std::mutex outcomes_mutex;
    int successful_withdrawals = 0;
    int failed_withdrawals = 0;

    std::vector<std::thread> threads;
    for (const auto& operation : operations) {
        threads.emplace_back([&, operation] {
            if (operation.type == "deposit") {
                account.deposit(operation.amount, operation.actor);
                return;
            }

            const bool success = account.withdraw(operation.amount, operation.actor);
            std::lock_guard<std::mutex> lock(outcomes_mutex);
            if (success) {
                ++successful_withdrawals;
            } else {
                ++failed_withdrawals;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    SharedAccountResult result;
    result.initial_balance = initial_balance;
    result.final_balance = account.balance();
    result.successful_withdrawals = successful_withdrawals;
    result.failed_withdrawals = failed_withdrawals;
    result.history = account.history();
    result.log = logger.entries();
    return result;
}

SharedAccountResult runSharedAccountDemo() {
    return runSharedAccountScenario(
        1000,
        {
            {"Regular-A", "deposit", 300},
            {"Regular-B", "withdraw", 150},
            {"Premium-C", "withdraw", 400},
            {"VIP-D", "deposit", 200},
        });
}

PayrollResult runCorporatePayrollScenario(int employee_count, int salary, int semaphore_limit) {
    EventLogger logger;
    CountingSemaphore semaphore(semaphore_limit);
    std::mutex concurrency_mutex;
    int active = 0;
    int max_concurrency_seen = 0;

    std::vector<std::unique_ptr<BankAccount>> accounts;
    accounts.reserve(employee_count);
    for (int index = 0; index < employee_count; ++index) {
        std::ostringstream stream;
        stream << "EMP-" << std::setw(3) << std::setfill('0') << index;
        accounts.push_back(std::make_unique<BankAccount>(stream.str(), 0, logger));
    }

    std::vector<std::thread> threads;
    for (int index = 0; index < employee_count; ++index) {
        threads.emplace_back([&, index] {
            std::ostringstream actor;
            actor << "CorporatePayroll-" << std::setw(3) << std::setfill('0') << index;
            // limit parallel deposits with a semaphore
            semaphore.acquire();
            {
                std::lock_guard<std::mutex> lock(concurrency_mutex);
                ++active;
                if (active > max_concurrency_seen) {
                    max_concurrency_seen = active;
                }
                logger.log(actor.str() + " entered deposit semaphore; active=" + std::to_string(active));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            accounts[index]->deposit(salary, actor.str());

            {
                std::lock_guard<std::mutex> lock(concurrency_mutex);
                --active;
                logger.log(actor.str() + " left deposit semaphore; active=" + std::to_string(active));
            }
            semaphore.release();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    PayrollResult result;
    result.employee_count = employee_count;
    result.salary = salary;
    result.semaphore_limit = semaphore_limit;
    result.max_concurrency_seen = max_concurrency_seen;
    for (int index = 0; index < employee_count; ++index) {
        std::ostringstream stream;
        stream << "EMP-" << std::setw(3) << std::setfill('0') << index;
        result.balances[stream.str()] = accounts[index]->balance();
        result.total_paid += accounts[index]->balance();
    }
    result.log = logger.entries();
    return result;
}

PayrollResult runCorporatePayrollDemo() {
    return runCorporatePayrollScenario(50, 200, 2);
}

SynchronizationResult runSynchronizationDemo() {
    SynchronizationResult result;
    result.shared_account = runSharedAccountDemo();
    result.payroll = runCorporatePayrollDemo();
    return result;
}

void printSharedAccountResult(const SharedAccountResult& result) {
    std::cout << "\nshared account result\n";
    std::cout << "initial balance: " << result.initial_balance << "\n";
    std::cout << "final balance: " << result.final_balance << "\n";
    std::cout << "successful withdrawals: " << result.successful_withdrawals << "\n";
    std::cout << "failed withdrawals: " << result.failed_withdrawals << "\n";
    std::cout << "transaction history:\n";
    for (const auto& line : result.history) {
        std::cout << "  " << line << "\n";
    }
}

void printPayrollResult(const PayrollResult& result) {
    std::cout << "\ncorporate payroll result\n";
    std::cout << "employees: " << result.employee_count << "\n";
    std::cout << "salary per employee: " << result.salary << "\n";
    std::cout << "semaphore limit: " << result.semaphore_limit << "\n";
    std::cout << "max concurrency seen: " << result.max_concurrency_seen << "\n";
    std::cout << "total payroll paid: " << result.total_paid << "\n";
}

class BankersAlgorithm {
  public:
    BankersAlgorithm(std::vector<int> available,
                     std::map<std::string, std::vector<int>> maximum,
                     std::map<std::string, std::vector<int>> allocation)
        : available_(std::move(available)),
          maximum_(std::move(maximum)),
          allocation_(std::move(allocation)) {}

    std::map<std::string, std::vector<int>> calculateNeed() const {
        std::map<std::string, std::vector<int>> need;
        for (const auto& entry : maximum_) {
            const auto& client = entry.first;
            const auto& maximum_row = entry.second;
            const auto& allocation_row = allocation_.at(client);
            std::vector<int> need_row(maximum_row.size(), 0);
            for (std::size_t index = 0; index < maximum_row.size(); ++index) {
                need_row[index] = maximum_row[index] - allocation_row[index];
            }
            need[client] = need_row;
        }
        return need;
    }

    BankerEvaluation isSafeState() const {
        std::vector<std::string> sequence;
        const bool safe = checkSafety(available_, allocation_, calculateNeed(), sequence);
        return {safe, safe, sequence, available_, safe ? "system is safe." : "system is unsafe."};
    }

    BankerEvaluation requestResources(const std::string& client, const std::vector<int>& request) {
        auto need = calculateNeed();
        const auto& current_need = need.at(client);

        for (std::size_t index = 0; index < request.size(); ++index) {
            if (request[index] > current_need[index]) {
                return {false, false, {}, available_, "request exceeds the declared maximum need."};
            }
            if (request[index] > available_[index]) {
                return {false, false, {}, available_, "request cannot be granted immediately."};
            }
        }

        std::vector<int> trial_available = available_;
        auto trial_allocation = allocation_;
        auto trial_need = need;
        for (std::size_t index = 0; index < request.size(); ++index) {
            trial_available[index] -= request[index];
            trial_allocation[client][index] += request[index];
            trial_need[client][index] -= request[index];
        }

        std::vector<std::string> sequence;
        const bool safe = checkSafety(trial_available, trial_allocation, trial_need, sequence);
        if (safe) {
            available_ = trial_available;
            allocation_ = trial_allocation;
            return {true, true, sequence, available_, "request granted for " + client + "."};
        }

        return {false, false, sequence, available_,
                "request denied for " + client + " because it leads to an unsafe state."};
    }

  private:
    bool checkSafety(const std::vector<int>& available,
                     const std::map<std::string, std::vector<int>>& allocation,
                     const std::map<std::string, std::vector<int>>& need,
                     std::vector<std::string>& safe_sequence) const {
        std::vector<int> work = available;
        std::map<std::string, bool> finish;
        for (const auto& entry : allocation) {
            finish[entry.first] = false;
        }

        while (safe_sequence.size() < allocation.size()) {
            bool progressed = false;
            for (const auto& entry : allocation) {
                const auto& client = entry.first;
                if (finish[client]) {
                    continue;
                }

                bool can_finish = true;
                for (std::size_t index = 0; index < work.size(); ++index) {
                    if (need.at(client)[index] > work[index]) {
                        can_finish = false;
                        break;
                    }
                }

                if (!can_finish) {
                    continue;
                }

                for (std::size_t index = 0; index < work.size(); ++index) {
                    work[index] += allocation.at(client)[index];
                }
                finish[client] = true;
                safe_sequence.push_back(client);
                progressed = true;
            }

            if (!progressed) {
                return false;
            }
        }

        return true;
    }

    std::vector<int> available_;
    std::map<std::string, std::vector<int>> maximum_;
    std::map<std::string, std::vector<int>> allocation_;
};

BankersAlgorithm buildDemoBanker() {
    return BankersAlgorithm(
        {3, 3, 2},
        {
            {"Loan-A", {7, 5, 3}},
            {"Loan-B", {3, 2, 2}},
            {"Loan-C", {9, 0, 2}},
            {"Loan-D", {2, 2, 2}},
            {"Loan-E", {4, 3, 3}},
        },
        {
            {"Loan-A", {0, 1, 0}},
            {"Loan-B", {2, 0, 0}},
            {"Loan-C", {3, 0, 2}},
            {"Loan-D", {2, 1, 1}},
            {"Loan-E", {0, 0, 2}},
        });
}

BankerDemoResult runBankerDemo() {
    auto banker = buildDemoBanker();
    BankerDemoResult result;
    result.safe_state = banker.isSafeState();
    result.safe_request = banker.requestResources("Loan-B", {1, 0, 2});
    auto denied_banker = buildDemoBanker();
    result.denied_request = denied_banker.requestResources("Loan-A", {0, 0, 2});
    result.need_matrix = buildDemoBanker().calculateNeed();
    return result;
}

void printBankerEvaluation(const BankerEvaluation& result) {
    std::cout << "message: " << result.message << "\n";
    std::cout << "safe sequence: ";
    for (std::size_t index = 0; index < result.safe_sequence.size(); ++index) {
        if (index != 0) {
            std::cout << ", ";
        }
        std::cout << result.safe_sequence[index];
    }
    if (result.safe_sequence.empty()) {
        std::cout << "none";
    }
    std::cout << "\n";
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

std::string trim(const std::string& value) {
    std::size_t start = 0;
    std::size_t end = value.size();
    while (start < end && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string serializeRequest(const TransactionRequest& request) {
    std::ostringstream stream;
    stream << request.request_id << '|'
           << request.customer_name << '|'
           << request.operation << '|'
           << request.account_id << '|'
           << request.amount << '\n';
    return stream.str();
}

TransactionRequest parseRequest(const std::string& line) {
    const auto parts = split(trim(line), '|');
    if (parts.size() != 5) {
        throw std::runtime_error("invalid request line");
    }
    return {parts[0], parts[1], parts[2], parts[3], std::stoi(parts[4])};
}

std::string serializeResponse(const TransactionResponse& response) {
    std::ostringstream stream;
    stream << response.request_id << '|'
           << response.customer_name << '|'
           << response.account_id << '|'
           << response.status << '|'
           << response.message << '|'
           << response.balance << '\n';
    return stream.str();
}

TransactionResponse parseResponse(const std::string& line) {
    const auto parts = split(trim(line), '|');
    if (parts.size() != 6) {
        throw std::runtime_error("invalid response line");
    }
    return {parts[0], parts[1], parts[2], parts[3], parts[4], std::stoi(parts[5])};
}

TransactionResponse processTransaction(std::map<std::string, int>& accounts, const TransactionRequest& request) {
    if (accounts.count(request.account_id) == 0) {
        return {request.request_id, request.customer_name, request.account_id, "failed",
                "unknown account.", -1};
    }

    if (request.operation == "deposit") {
        accounts[request.account_id] += request.amount;
        return {request.request_id, request.customer_name, request.account_id, "success",
                "transaction successful.", accounts[request.account_id]};
    }

    if (request.operation == "withdraw") {
        if (accounts[request.account_id] < request.amount) {
            return {request.request_id, request.customer_name, request.account_id, "failed",
                    "insufficient funds.", accounts[request.account_id]};
        }
        accounts[request.account_id] -= request.amount;
        return {request.request_id, request.customer_name, request.account_id, "success",
                "transaction successful.", accounts[request.account_id]};
    }

    return {request.request_id, request.customer_name, request.account_id, "failed",
            "unsupported operation.", accounts[request.account_id]};
}

std::vector<TransactionRequest> demoRequests() {
    return {
        {"REQ-1", "Regular-A", "deposit", "ACC-100", 500},
        {"REQ-2", "Regular-B", "withdraw", "ACC-100", 200},
        {"REQ-3", "Premium-C", "deposit", "ACC-200", 350},
        {"REQ-4", "VIP-D", "withdraw", "ACC-200", 100},
    };
}

#ifdef _WIN32

std::uintptr_t parseHandle(const char* value) {
    return static_cast<std::uintptr_t>(std::strtoull(value, nullptr, 10));
}

std::string quote(const std::string& value) {
    return "\"" + value + "\"";
}

void writeAll(HANDLE handle, const std::string& value) {
    DWORD total_written = 0;
    while (total_written < value.size()) {
        DWORD written = 0;
        const BOOL ok = WriteFile(handle,
                                  value.data() + total_written,
                                  static_cast<DWORD>(value.size() - total_written),
                                  &written,
                                  nullptr);
        if (!ok) {
            throw std::runtime_error("WriteFile failed");
        }
        total_written += written;
    }
}

std::string readAll(HANDLE handle) {
    std::string output;
    char buffer[256];
    DWORD read = 0;
    while (ReadFile(handle, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        output.append(buffer, buffer + read);
    }
    return output;
}

PROCESS_INFORMATION spawnProcess(const std::string& command) {
    STARTUPINFOA startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};
    std::vector<char> mutable_command(command.begin(), command.end());
    mutable_command.push_back('\0');
    const BOOL ok = CreateProcessA(nullptr,
                                   mutable_command.data(),
                                   nullptr,
                                   nullptr,
                                   TRUE,
                                   0,
                                   nullptr,
                                   nullptr,
                                   &startup_info,
                                   &process_info);
    if (!ok) {
        throw std::runtime_error("CreateProcess failed");
    }
    return process_info;
}

void closeProcessInfo(PROCESS_INFORMATION& info) {
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
}

void waitProcess(PROCESS_INFORMATION& info) {
    WaitForSingleObject(info.hProcess, INFINITE);
    closeProcessInfo(info);
}

void serverLoopWindows(HANDLE request_read, HANDLE response_write) {
    std::map<std::string, int> accounts{{"ACC-100", 1000}, {"ACC-200", 500}};
    const std::string request_text = readAll(request_read);
    std::stringstream stream(request_text);
    std::string line;

    while (std::getline(stream, line)) {
        if (trim(line).empty()) {
            continue;
        }
        const auto request = parseRequest(line);
        const auto response = processTransaction(accounts, request);
        writeAll(response_write, serializeResponse(response));
    }

    CloseHandle(request_read);
    CloseHandle(response_write);
}

void producerLoopWindows(HANDLE request_write, const TransactionRequest& request) {
    writeAll(request_write, serializeRequest(request));
    CloseHandle(request_write);
}

#else

void writeAll(int fd, const std::string& value) {
    std::size_t written_total = 0;
    while (written_total < value.size()) {
        const ssize_t written = write(fd, value.data() + written_total, value.size() - written_total);
        if (written < 0) {
            throw std::runtime_error("write failed");
        }
        written_total += static_cast<std::size_t>(written);
    }
}

std::string readAll(int fd) {
    std::string output;
    char buffer[256];
    ssize_t count = 0;
    while ((count = read(fd, buffer, sizeof(buffer))) > 0) {
        output.append(buffer, buffer + count);
    }
    return output;
}

void serverLoopPosix(int request_read, int response_write) {
    std::map<std::string, int> accounts{{"ACC-100", 1000}, {"ACC-200", 500}};
    const std::string request_text = readAll(request_read);
    std::stringstream stream(request_text);
    std::string line;

    while (std::getline(stream, line)) {
        if (trim(line).empty()) {
            continue;
        }
        const auto request = parseRequest(line);
        const auto response = processTransaction(accounts, request);
        writeAll(response_write, serializeResponse(response));
    }

    close(request_read);
    close(response_write);
    std::_Exit(0);
}

void producerLoopPosix(int request_write, const TransactionRequest& request) {
    writeAll(request_write, serializeRequest(request));
    close(request_write);
    std::_Exit(0);
}

#endif

bool handleIpcChildMode(int argc, char** argv) {
#ifdef _WIN32
    if (argc >= 4 && std::string(argv[1]) == "--ipc-server") {
        serverLoopWindows(reinterpret_cast<HANDLE>(parseHandle(argv[2])),
                          reinterpret_cast<HANDLE>(parseHandle(argv[3])));
        return true;
    }

    if (argc >= 8 && std::string(argv[1]) == "--ipc-producer") {
        TransactionRequest request{argv[3], argv[4], argv[5], argv[6], std::stoi(argv[7])};
        producerLoopWindows(reinterpret_cast<HANDLE>(parseHandle(argv[2])), request);
        return true;
    }
#else
    (void)argc;
    (void)argv;
#endif
    return false;
}

IpcResult runIpcScenario(const std::string& executable_path, const std::vector<TransactionRequest>& requests) {
    IpcResult result;
    result.requests = requests;
    result.final_accounts = {{"ACC-100", 1000}, {"ACC-200", 500}};

#ifdef _WIN32
    SECURITY_ATTRIBUTES security_attributes{};
    security_attributes.nLength = sizeof(security_attributes);
    security_attributes.bInheritHandle = TRUE;

    HANDLE request_read = nullptr;
    HANDLE request_write = nullptr;
    HANDLE response_read = nullptr;
    HANDLE response_write = nullptr;

    if (!CreatePipe(&request_read, &request_write, &security_attributes, 0) ||
        !CreatePipe(&response_read, &response_write, &security_attributes, 0)) {
        throw std::runtime_error("CreatePipe failed");
    }

    SetHandleInformation(request_write, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(response_read, HANDLE_FLAG_INHERIT, 0);

    auto server = spawnProcess(quote(executable_path) + " --ipc-server " +
                               std::to_string(reinterpret_cast<std::uintptr_t>(request_read)) + " " +
                               std::to_string(reinterpret_cast<std::uintptr_t>(response_write)));
    CloseHandle(request_read);
    CloseHandle(response_write);
    SetHandleInformation(request_write, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    std::vector<PROCESS_INFORMATION> producers;
    for (const auto& request : requests) {
        result.log.push_back("producer sent " + request.request_id + " -> " + request.operation + " " +
                             std::to_string(request.amount) + " on " + request.account_id);
        producers.push_back(spawnProcess(
            quote(executable_path) + " --ipc-producer " +
            std::to_string(reinterpret_cast<std::uintptr_t>(request_write)) + " " + request.request_id + " " +
            request.customer_name + " " + request.operation + " " + request.account_id + " " +
            std::to_string(request.amount)));
    }

    for (auto& producer : producers) {
        waitProcess(producer);
    }

    CloseHandle(request_write);
    const std::string responses_text = readAll(response_read);
    CloseHandle(response_read);
    waitProcess(server);

    std::stringstream stream(responses_text);
    std::string line;
    while (std::getline(stream, line)) {
        if (trim(line).empty()) {
            continue;
        }
        const auto response = parseResponse(line);
        result.responses.push_back(response);
        result.final_accounts[response.account_id] = response.balance;
        ++result.processed_requests;
        result.log.push_back("server responded " + response.request_id + " -> " + response.status +
                             " (balance=" + std::to_string(response.balance) + ")");
    }
#else
    int request_pipe[2];
    int response_pipe[2];
    if (pipe(request_pipe) != 0 || pipe(response_pipe) != 0) {
        throw std::runtime_error("pipe creation failed");
    }

    const pid_t server_pid = fork();
    if (server_pid == 0) {
        close(request_pipe[1]);
        close(response_pipe[0]);
        serverLoopPosix(request_pipe[0], response_pipe[1]);
    }

    close(request_pipe[0]);
    close(response_pipe[1]);

    std::vector<pid_t> producers;
    for (const auto& request : requests) {
        result.log.push_back("producer sent " + request.request_id + " -> " + request.operation + " " +
                             std::to_string(request.amount) + " on " + request.account_id);
        const pid_t producer_pid = fork();
        if (producer_pid == 0) {
            producerLoopPosix(request_pipe[1], request);
        }
        producers.push_back(producer_pid);
    }

    for (const auto producer_pid : producers) {
        int status = 0;
        waitpid(producer_pid, &status, 0);
    }

    close(request_pipe[1]);
    const std::string responses_text = readAll(response_pipe[0]);
    close(response_pipe[0]);

    int server_status = 0;
    waitpid(server_pid, &server_status, 0);

    std::stringstream stream(responses_text);
    std::string line;
    while (std::getline(stream, line)) {
        if (trim(line).empty()) {
            continue;
        }
        const auto response = parseResponse(line);
        result.responses.push_back(response);
        result.final_accounts[response.account_id] = response.balance;
        ++result.processed_requests;
        result.log.push_back("server responded " + response.request_id + " -> " + response.status +
                             " (balance=" + std::to_string(response.balance) + ")");
    }
#endif

    return result;
}

IpcResult runIpcDemo(const std::string& executable_path) {
    return runIpcScenario(executable_path, demoRequests());
}

void printIpcResult(const IpcResult& result) {
    std::cout << "\nipc result\n";
    std::cout << "processed requests: " << result.processed_requests << "\n";
    std::cout << "final balances:\n";
    for (const auto& entry : result.final_accounts) {
        std::cout << "  " << entry.first << " -> " << entry.second << "\n";
    }
    std::cout << "request/response flow:\n";
    for (const auto& line : result.log) {
        std::cout << "  " << line << "\n";
    }
}

MemoryResult buildMemoryResult(const std::string& algorithm,
                               int frames_count,
                               const std::vector<std::string>& reference_string,
                               const std::vector<MemoryStep>& steps,
                               int page_faults,
                               int hits) {
    MemoryResult result;
    result.algorithm = algorithm;
    result.frames_count = frames_count;
    result.reference_string = reference_string;
    result.steps = steps;
    result.page_faults = page_faults;
    result.hits = hits;
    if (!reference_string.empty()) {
        result.hit_ratio = static_cast<double>(hits) / reference_string.size();
        result.fault_ratio = static_cast<double>(page_faults) / reference_string.size();
    }
    return result;
}

MemoryResult runFifo(const std::vector<std::string>& reference_string, int frames_count) {
    if (frames_count <= 0) {
        throw std::invalid_argument("frames_count must be positive");
    }

    std::vector<std::string> frames;
    std::vector<std::string> queue;
    std::vector<MemoryStep> steps;
    int hits = 0;
    int faults = 0;

    for (const auto& page : reference_string) {
        if (std::find(frames.begin(), frames.end(), page) != frames.end()) {
            ++hits;
            steps.push_back({page, frames, true, ""});
            continue;
        }

        ++faults;
        std::string replaced;
        if (static_cast<int>(frames.size()) < frames_count) {
            frames.push_back(page);
            queue.push_back(page);
        } else {
            replaced = queue.front();
            queue.erase(queue.begin());
            const auto position = std::find(frames.begin(), frames.end(), replaced);
            *position = page;
            queue.push_back(page);
        }
        steps.push_back({page, frames, false, replaced});
    }

    return buildMemoryResult("FIFO", frames_count, reference_string, steps, faults, hits);
}

MemoryResult runLru(const std::vector<std::string>& reference_string, int frames_count) {
    if (frames_count <= 0) {
        throw std::invalid_argument("frames_count must be positive");
    }

    std::vector<std::string> frames;
    std::vector<MemoryStep> steps;
    std::map<std::string, int> last_used;
    int hits = 0;
    int faults = 0;

    for (int index = 0; index < static_cast<int>(reference_string.size()); ++index) {
        const auto& page = reference_string[index];
        if (std::find(frames.begin(), frames.end(), page) != frames.end()) {
            ++hits;
            last_used[page] = index;
            steps.push_back({page, frames, true, ""});
            continue;
        }

        ++faults;
        std::string replaced;
        if (static_cast<int>(frames.size()) < frames_count) {
            frames.push_back(page);
        } else {
            replaced = frames.front();
            int oldest = last_used[replaced];
            for (const auto& frame : frames) {
                if (last_used[frame] < oldest) {
                    oldest = last_used[frame];
                    replaced = frame;
                }
            }
            const auto position = std::find(frames.begin(), frames.end(), replaced);
            *position = page;
        }
        last_used[page] = index;
        steps.push_back({page, frames, false, replaced});
    }

    return buildMemoryResult("LRU", frames_count, reference_string, steps, faults, hits);
}

MemoryDemoResult runMemoryDemo() {
    const std::vector<std::string> reference = {"7", "0", "1", "2", "0", "3", "0",
                                                "4", "2", "3", "0", "3", "2"};
    return {reference, runFifo(reference, 3), runLru(reference, 3)};
}

void printMemoryResult(const MemoryResult& result) {
    std::cout << "\n" << result.algorithm << "\n";
    std::cout << "page faults: " << result.page_faults << "\n";
    std::cout << "hits: " << result.hits << "\n";
    std::cout << "hit ratio: " << formatDouble(result.hit_ratio) << "\n";
    std::cout << "fault ratio: " << formatDouble(result.fault_ratio) << "\n";
    std::cout << "steps:\n";
    for (const auto& step : result.steps) {
        std::cout << "  page=" << step.page << " frames=[";
        for (std::size_t index = 0; index < step.frames.size(); ++index) {
            if (index != 0) {
                std::cout << ' ';
            }
            std::cout << step.frames[index];
        }
        std::cout << "] " << (step.hit ? "hit" : "fault");
        if (!step.replaced.empty()) {
            std::cout << " replaced=" << step.replaced;
        }
        std::cout << "\n";
    }
}

void ensureDirectory(const std::filesystem::path& path) {
    std::filesystem::create_directories(path);
}

void writeTextFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
}

void writeGanttSvg(const std::filesystem::path& path, const std::string& title, const ScheduleResult& result) {
    const int unit_width = 70;
    const int left_margin = 80;
    const int width = left_margin + (result.total_time == 0 ? 1 : result.total_time) * unit_width + 30;
    const int height = 130;
    const std::vector<std::string> colors = {"#0b6e4f", "#b80c09", "#3a86ff", "#ff9f1c", "#8338ec", "#2d6a4f"};
    std::map<std::string, std::string> palette;
    std::ostringstream body;

    for (const auto& slice : result.slices) {
        if (palette.count(slice.task_id) == 0) {
            palette[slice.task_id] = colors[palette.size() % colors.size()];
        }
        const int x = left_margin + slice.start * unit_width;
        const int block_width = (slice.end - slice.start) * unit_width;
        body << "<rect x=\"" << x << "\" y=\"40\" width=\"" << block_width
             << "\" height=\"30\" fill=\"" << palette[slice.task_id] << "\" rx=\"6\"/>\n";
        body << "<text x=\"" << x + block_width / 2
             << "\" y=\"60\" text-anchor=\"middle\" font-size=\"12\" fill=\"#ffffff\">"
             << slice.task_id << "</text>\n";
    }

    for (int tick = 0; tick <= result.total_time; ++tick) {
        const int x = left_margin + tick * unit_width;
        body << "<line x1=\"" << x << "\" y1=\"75\" x2=\"" << x
             << "\" y2=\"82\" stroke=\"#1f2937\" stroke-width=\"1\"/>\n";
        body << "<text x=\"" << x << "\" y=\"100\" text-anchor=\"middle\" font-size=\"11\" fill=\"#1f2937\">"
             << tick << "</text>\n";
    }

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height
        << "\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"#f8fafc\"/>\n"
        << "<text x=\"20\" y=\"24\" font-size=\"18\" fill=\"#0f172a\">" << title << "</text>\n"
        << "<text x=\"20\" y=\"60\" font-size=\"12\" fill=\"#334155\">cpu timeline</text>\n"
        << body.str() << "</svg>\n";
    writeTextFile(path, svg.str());
}

void writeMemorySvg(const std::filesystem::path& path, const MemoryDemoResult& memory) {
    const int width = 640;
    const int height = 210;
    const int chart_x = 220;
    const int max_width = 260;
    const double max_value = static_cast<double>(std::max(memory.fifo.page_faults, memory.lru.page_faults));

    auto bar_width = [&](double value) {
        if (max_value == 0.0) {
            return 0;
        }
        return static_cast<int>((value / max_value) * max_width);
    };

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height
        << "\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"#fffaf0\"/>\n"
        << "<text x=\"20\" y=\"26\" font-size=\"18\" fill=\"#111827\">fifo vs lru comparison</text>\n"
        << "<text x=\"20\" y=\"46\" font-size=\"12\" fill=\"#4b5563\">page faults and hit ratios</text>\n"
        << "<text x=\"20\" y=\"90\" font-size=\"13\" fill=\"#1f2937\">fifo page faults</text>\n"
        << "<rect x=\"" << chart_x << "\" y=\"70\" width=\"" << bar_width(memory.fifo.page_faults)
        << "\" height=\"28\" fill=\"#006d77\" rx=\"6\"/>\n"
        << "<text x=\"" << chart_x + bar_width(memory.fifo.page_faults) + 12
        << "\" y=\"89\" font-size=\"12\" fill=\"#1f2937\">" << memory.fifo.page_faults << "</text>\n"
        << "<text x=\"20\" y=\"140\" font-size=\"13\" fill=\"#1f2937\">lru page faults</text>\n"
        << "<rect x=\"" << chart_x << "\" y=\"120\" width=\"" << bar_width(memory.lru.page_faults)
        << "\" height=\"28\" fill=\"#bc4749\" rx=\"6\"/>\n"
        << "<text x=\"" << chart_x + bar_width(memory.lru.page_faults) + 12
        << "\" y=\"139\" font-size=\"12\" fill=\"#1f2937\">" << memory.lru.page_faults << "</text>\n"
        << "<text x=\"20\" y=\"184\" font-size=\"13\" fill=\"#1f2937\">fifo hit ratio: "
        << formatDouble(memory.fifo.hit_ratio) << " | lru hit ratio: " << formatDouble(memory.lru.hit_ratio)
        << "</text>\n"
        << "</svg>\n";
    writeTextFile(path, svg.str());
}

void writeSchedulingFiles(const ScheduleResult& fcfs,
                          const ScheduleResult& priority,
                          const ScheduleResult& round_robin,
                          const std::filesystem::path& output_dir) {
    ensureDirectory(output_dir);
    writeGanttSvg(output_dir / "fcfs_gantt.svg", "FCFS Gantt Chart", fcfs);
    writeGanttSvg(output_dir / "priority_gantt.svg", "Priority Gantt Chart", priority);
    writeGanttSvg(output_dir / "round_robin_gantt.svg", "Round Robin Gantt Chart", round_robin);

    std::ofstream file(output_dir / "scheduling_metrics.csv");
    file << "Algorithm,Task,Waiting Time,Turnaround Time,Response Time,Completion Time\n";
    const std::vector<std::pair<std::string, const ScheduleResult*>> schedules = {
        {"FCFS", &fcfs},
        {"Priority", &priority},
        {"Round Robin", &round_robin},
    };
    for (const auto& schedule : schedules) {
        for (const auto& metric : schedule.second->metrics) {
            file << schedule.first << ','
                 << metric.first << ','
                 << metric.second.waiting_time << ','
                 << metric.second.turnaround_time << ','
                 << metric.second.response_time << ','
                 << metric.second.completion_time << '\n';
        }
    }
}

void writeSynchronizationFile(const SynchronizationResult& result, const std::filesystem::path& output_dir) {
    ensureDirectory(output_dir);
    std::ofstream file(output_dir / "synchronization.log");
    for (const auto& line : result.shared_account.log) {
        file << line << '\n';
    }
    file << '\n';
    for (const auto& line : result.payroll.log) {
        file << line << '\n';
    }
}

void writeBankerFile(const BankerDemoResult& result, const std::filesystem::path& output_dir) {
    ensureDirectory(output_dir);
    std::ofstream file(output_dir / "bankers.log");
    file << "safe sequence: ";
    for (std::size_t index = 0; index < result.safe_state.safe_sequence.size(); ++index) {
        if (index != 0) {
            file << ", ";
        }
        file << result.safe_state.safe_sequence[index];
    }
    file << '\n' << result.safe_request.message << '\n' << result.denied_request.message << "\n\nneed matrix:\n";
    for (const auto& entry : result.need_matrix) {
        file << entry.first << ": ";
        for (std::size_t index = 0; index < entry.second.size(); ++index) {
            if (index != 0) {
                file << ' ';
            }
            file << entry.second[index];
        }
        file << '\n';
    }
}

void writeIpcFile(const IpcResult& result, const std::filesystem::path& output_dir) {
    ensureDirectory(output_dir);
    std::ofstream file(output_dir / "ipc_flow.log");
    for (const auto& line : result.log) {
        file << line << '\n';
    }
}

void writeMemoryFiles(const MemoryDemoResult& result, const std::filesystem::path& output_dir) {
    ensureDirectory(output_dir);
    std::ofstream file(output_dir / "memory_metrics.csv");
    file << "Algorithm,Page Faults,Hits,Hit Ratio,Fault Ratio\n";
    file << "FIFO," << result.fifo.page_faults << ',' << result.fifo.hits << ','
         << formatDouble(result.fifo.hit_ratio) << ',' << formatDouble(result.fifo.fault_ratio) << '\n';
    file << "LRU," << result.lru.page_faults << ',' << result.lru.hits << ','
         << formatDouble(result.lru.hit_ratio) << ',' << formatDouble(result.lru.fault_ratio) << '\n';
    writeMemorySvg(output_dir / "memory_comparison.svg", result);
}

std::string buildReportText(const ProjectSummary& summary) {
    std::ostringstream report;
    report << "# Concurrent Banking System with Scheduling, Synchronization, IPC, and Memory Management\n\n"
           << "## Objective\n"
           << "This single-file project implements scheduling, synchronization, deadlock avoidance, ipc, and "
              "memory management for the required banking simulation.\n\n"
           << "## Scheduling\n"
           << "- FCFS average waiting time: " << formatDouble(summary.fcfs.average_waiting_time) << '\n'
           << "- Priority average waiting time: " << formatDouble(summary.priority.average_waiting_time) << '\n'
           << "- Round Robin average waiting time: " << formatDouble(summary.round_robin.average_waiting_time)
           << "\n\n"
           << "Sample FCFS Gantt chart:\n```text\n" << renderGanttText(summary.fcfs) << "\n```\n\n"
           << "## Synchronization\n"
           << "- Shared account initial balance: " << summary.synchronization.shared_account.initial_balance << '\n'
           << "- Shared account final balance: " << summary.synchronization.shared_account.final_balance << '\n'
           << "- Successful withdrawals: " << summary.synchronization.shared_account.successful_withdrawals << '\n'
           << "- Failed withdrawals: " << summary.synchronization.shared_account.failed_withdrawals << '\n'
           << "- Payroll threads: " << summary.synchronization.payroll.employee_count << '\n'
           << "- Observed semaphore limit: " << summary.synchronization.payroll.max_concurrency_seen << '\n'
           << "- Total payroll paid: " << summary.synchronization.payroll.total_paid << "\n\n"
           << "## Banker\n"
           << "- Safe request: " << summary.banker.safe_request.message << '\n'
           << "- Unsafe request: " << summary.banker.denied_request.message << "\n\n"
           << "## IPC\n"
           << "- Requests processed: " << summary.ipc.processed_requests << '\n'
           << "- Final balances: ";

    bool first = true;
    for (const auto& entry : summary.ipc.final_accounts) {
        if (!first) {
            report << ", ";
        }
        report << entry.first << "=" << entry.second;
        first = false;
    }

    report << "\n\n## Memory\n"
           << "- FIFO page faults: " << summary.memory.fifo.page_faults << '\n'
           << "- FIFO hit ratio: " << formatDouble(summary.memory.fifo.hit_ratio) << '\n'
           << "- LRU page faults: " << summary.memory.lru.page_faults << '\n'
           << "- LRU hit ratio: " << formatDouble(summary.memory.lru.hit_ratio) << "\n";
    return report.str();
}

ProjectSummary runCompleteProject(const std::string& executable_path) {
    ProjectSummary summary;
    summary.tasks = buildDemoTasks();
    summary.fcfs = runFcfs(summary.tasks);
    summary.priority = runPriority(summary.tasks);
    summary.round_robin = runRoundRobin(summary.tasks, 2);
    summary.synchronization = runSynchronizationDemo();
    summary.banker = runBankerDemo();
    summary.ipc = runIpcDemo(executable_path);
    summary.memory = runMemoryDemo();
    return summary;
}

void generateCompleteOutput(const ProjectSummary& summary) {
    const auto output_dir = std::filesystem::current_path() / "artifacts";
    ensureDirectory(output_dir);
    writeSchedulingFiles(summary.fcfs, summary.priority, summary.round_robin, output_dir);
    writeSynchronizationFile(summary.synchronization, output_dir);
    writeBankerFile(summary.banker, output_dir);
    writeIpcFile(summary.ipc, output_dir);
    writeMemoryFiles(summary.memory, output_dir);
    writeTextFile(std::filesystem::current_path() / "REPORT.md", buildReportText(summary));
}

std::vector<CustomerTask> inputSchedulingTasks() {
    const int task_count = readInt("enter number of tasks: ");
    std::vector<CustomerTask> tasks;
    tasks.reserve(task_count);

    for (int index = 0; index < task_count; ++index) {
        std::cout << "\ntask " << index + 1 << "\n";
        CustomerTask task;
        task.task_id = readToken("task id: ");
        task.customer_name = readToken("customer name (no spaces): ");
        task.customer_type = readToken("customer type (Regular/Premium/VIP/Corporate): ");
        task.arrival_time = readInt("arrival time: ");
        task.burst_time = readInt("burst time: ");
        task.priority = readInt("priority: ");
        tasks.push_back(task);
    }

    return tasks;
}

SharedAccountResult inputSharedAccountScenario() {
    const int initial_balance = readInt("initial balance: ");
    const int operation_count = readInt("number of operations: ");
    std::vector<SharedOperation> operations;
    operations.reserve(operation_count);

    for (int index = 0; index < operation_count; ++index) {
        std::cout << "\noperation " << index + 1 << "\n";
        SharedOperation operation;
        operation.actor = readToken("actor name (no spaces): ");
        operation.type = readToken("type (deposit/withdraw): ");
        operation.amount = readInt("amount: ");
        operations.push_back(operation);
    }

    return runSharedAccountScenario(initial_balance, operations);
}

PayrollResult inputPayrollScenario() {
    const int employee_count = readInt("employee count: ");
    const int salary = readInt("salary per employee: ");
    const int semaphore_limit = readInt("semaphore limit: ");
    return runCorporatePayrollScenario(employee_count, salary, semaphore_limit);
}

BankerEvaluation inputCustomBankerScenario() {
    const int resource_types = readInt("number of resource types: ");
    const int process_count = readInt("number of clients/processes: ");

    std::vector<int> available(resource_types, 0);
    std::cout << "enter available vector\n";
    for (int index = 0; index < resource_types; ++index) {
        available[index] = readInt("available[" + std::to_string(index) + "]: ");
    }

    std::map<std::string, std::vector<int>> maximum;
    std::map<std::string, std::vector<int>> allocation;
    std::vector<std::string> names;

    for (int process = 0; process < process_count; ++process) {
        const std::string name = readToken("client name: ");
        names.push_back(name);
        maximum[name] = std::vector<int>(resource_types, 0);
        allocation[name] = std::vector<int>(resource_types, 0);

        std::cout << "maximum for " << name << "\n";
        for (int index = 0; index < resource_types; ++index) {
            maximum[name][index] = readInt("max[" + std::to_string(index) + "]: ");
        }

        std::cout << "allocation for " << name << "\n";
        for (int index = 0; index < resource_types; ++index) {
            allocation[name][index] = readInt("allocation[" + std::to_string(index) + "]: ");
        }
    }

    BankersAlgorithm banker(available, maximum, allocation);
    std::cout << "\ninitial state check\n";
    printBankerEvaluation(banker.isSafeState());

    const std::string request_client = readToken("enter client name for request: ");
    std::vector<int> request(resource_types, 0);
    for (int index = 0; index < resource_types; ++index) {
        request[index] = readInt("request[" + std::to_string(index) + "]: ");
    }
    return banker.requestResources(request_client, request);
}

std::vector<TransactionRequest> inputIpcRequests() {
    const int request_count = readInt("number of requests: ");
    std::vector<TransactionRequest> requests;
    requests.reserve(request_count);

    for (int index = 0; index < request_count; ++index) {
        std::cout << "\nrequest " << index + 1 << "\n";
        TransactionRequest request;
        request.request_id = readToken("request id: ");
        request.customer_name = readToken("customer name (no spaces): ");
        request.operation = readToken("operation (deposit/withdraw): ");
        request.account_id = readToken("account id (ACC-100 or ACC-200): ");
        request.amount = readInt("amount: ");
        requests.push_back(request);
    }

    return requests;
}

MemoryDemoResult inputMemoryScenario() {
    const int frames_count = readInt("number of frames: ");
    const int reference_count = readInt("number of page references: ");
    std::vector<std::string> reference_string;
    reference_string.reserve(reference_count);

    for (int index = 0; index < reference_count; ++index) {
        reference_string.push_back(readToken("page " + std::to_string(index + 1) + ": "));
    }

    return {reference_string, runFifo(reference_string, frames_count), runLru(reference_string, frames_count)};
}

void showSchedulingMenu() {
    std::cout << "\n1. use default scheduling data\n2. enter custom scheduling data\n";
}

void handleSchedulingOption() {
    showSchedulingMenu();
    const int mode = readInt("choice: ");
    std::vector<CustomerTask> tasks = (mode == 2) ? inputSchedulingTasks() : buildDemoTasks();
    const int quantum = (mode == 2) ? readInt("round robin quantum: ") : 2;

    const auto fcfs = runFcfs(tasks);
    const auto priority = runPriority(tasks);
    const auto round_robin = runRoundRobin(tasks, quantum);

    printSchedulingResult(fcfs);
    printSchedulingResult(priority);
    printSchedulingResult(round_robin);

    writeSchedulingFiles(fcfs, priority, round_robin, std::filesystem::current_path() / "artifacts");
    std::cout << "\nscheduling files saved in artifacts/\n";
}

void handleSynchronizationOption() {
    std::cout << "\n1. use default synchronization demo\n2. custom shared account\n3. custom payroll\n";
    const int mode = readInt("choice: ");

    if (mode == 2) {
        const auto result = inputSharedAccountScenario();
        printSharedAccountResult(result);
        SynchronizationResult wrapper;
        wrapper.shared_account = result;
        writeSynchronizationFile(wrapper, std::filesystem::current_path() / "artifacts");
        std::cout << "\nsynchronization log saved in artifacts/\n";
        return;
    }

    if (mode == 3) {
        const auto result = inputPayrollScenario();
        printPayrollResult(result);
        SynchronizationResult wrapper;
        wrapper.payroll = result;
        writeSynchronizationFile(wrapper, std::filesystem::current_path() / "artifacts");
        std::cout << "\nsynchronization log saved in artifacts/\n";
        return;
    }

    const auto result = runSynchronizationDemo();
    printSharedAccountResult(result.shared_account);
    printPayrollResult(result.payroll);
    writeSynchronizationFile(result, std::filesystem::current_path() / "artifacts");
    std::cout << "\nsynchronization log saved in artifacts/\n";
}

void handleBankerOption() {
    std::cout << "\n1. use default banker demo\n2. custom banker input\n";
    const int mode = readInt("choice: ");

    if (mode == 2) {
        const auto result = inputCustomBankerScenario();
        std::cout << "\ncustom banker request result\n";
        printBankerEvaluation(result);
        return;
    }

    const auto result = runBankerDemo();
    std::cout << "\ninitial safe state\n";
    printBankerEvaluation(result.safe_state);
    std::cout << "\nsafe request result\n";
    printBankerEvaluation(result.safe_request);
    std::cout << "\nunsafe request result\n";
    printBankerEvaluation(result.denied_request);
    writeBankerFile(result, std::filesystem::current_path() / "artifacts");
    std::cout << "\nbanker log saved in artifacts/\n";
}

void handleIpcOption(const std::string& executable_path) {
    std::cout << "\n1. use default ipc demo\n2. enter custom ipc requests\n";
    const int mode = readInt("choice: ");
    const auto requests = (mode == 2) ? inputIpcRequests() : demoRequests();
    const auto result = runIpcScenario(executable_path, requests);
    printIpcResult(result);
    writeIpcFile(result, std::filesystem::current_path() / "artifacts");
    std::cout << "\nipc log saved in artifacts/\n";
}

void handleMemoryOption() {
    std::cout << "\n1. use default memory demo\n2. enter custom memory input\n";
    const int mode = readInt("choice: ");
    const auto result = (mode == 2) ? inputMemoryScenario() : runMemoryDemo();
    printMemoryResult(result.fifo);
    printMemoryResult(result.lru);
    writeMemoryFiles(result, std::filesystem::current_path() / "artifacts");
    std::cout << "\nmemory files saved in artifacts/\n";
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void runBuiltInValidation(const std::string& executable_path) {
    const auto tasks = buildDemoTasks();
    const auto fcfs = runFcfs(tasks);
    require(fcfs.slices.size() == 5, "fcfs slice count mismatch");
    require(fcfs.metrics.at("P1").response_time == 3, "fcfs response mismatch");

    const auto priority = runPriority(tasks);
    require(priority.slices.size() >= 4, "priority slices missing");
    require(priority.slices[2].task_id == "V1", "priority vip mismatch");

    const auto round_robin = runRoundRobin(tasks, 2);
    require(round_robin.total_time == 16, "round robin total time mismatch");

    const auto shared = runSharedAccountDemo();
    require(shared.final_balance == 950, "shared account balance mismatch");

    const auto payroll = runCorporatePayrollScenario(10, 150, 2);
    require(payroll.total_paid == 1500, "payroll total mismatch");
    require(payroll.max_concurrency_seen <= 2, "semaphore limit violated");

    auto banker = buildDemoBanker();
    require(banker.isSafeState().safe, "banker state should be safe");
    require(banker.requestResources("Loan-B", {1, 0, 2}).granted, "safe banker request failed");

    banker = buildDemoBanker();
    require(!banker.requestResources("Loan-A", {0, 0, 2}).granted, "unsafe banker request granted");

    const auto ipc = runIpcDemo(executable_path);
    require(ipc.processed_requests == 4, "ipc processed count mismatch");
    require(ipc.final_accounts.at("ACC-100") == 1300, "ipc acc-100 mismatch");
    require(ipc.final_accounts.at("ACC-200") == 750, "ipc acc-200 mismatch");

    const auto memory = runMemoryDemo();
    require(memory.fifo.page_faults == 10, "fifo fault mismatch");
    require(memory.lru.page_faults == 9, "lru fault mismatch");

    std::cout << "\nall built-in checks passed.\n";
}

void runFullProjectOption(const std::string& executable_path) {
    const auto summary = runCompleteProject(executable_path);
    generateCompleteOutput(summary);
    std::cout << "\ncomplete project executed.\n";
    std::cout << "artifacts generated in: " << (std::filesystem::current_path() / "artifacts").string() << "\n";
    std::cout << "report generated at: " << (std::filesystem::current_path() / "REPORT.md").string() << "\n";
    std::cout << "fcfs average waiting time: " << formatDouble(summary.fcfs.average_waiting_time) << "\n";
    std::cout << "priority average waiting time: " << formatDouble(summary.priority.average_waiting_time) << "\n";
    std::cout << "round robin average waiting time: " << formatDouble(summary.round_robin.average_waiting_time) << "\n";
    std::cout << "fifo page faults: " << summary.memory.fifo.page_faults << "\n";
    std::cout << "lru page faults: " << summary.memory.lru.page_faults << "\n";
}

void printMenu() {
    std::cout << "\n========================================\n";
    std::cout << "Concurrent Banking System Menu\n";
    std::cout << "1. Run CPU Scheduling\n";
    std::cout << "2. Run Synchronization\n";
    std::cout << "3. Run Banker's Algorithm\n";
    std::cout << "4. Run IPC / Message Passing\n";
    std::cout << "5. Run Memory Management\n";
    std::cout << "6. Run Complete Project + Generate Report/Artifacts\n";
    std::cout << "7. Run Built-in Validation\n";
    std::cout << "0. Exit\n";
    std::cout << "========================================\n";
}

int main(int argc, char** argv) {
    if (handleIpcChildMode(argc, argv)) {
        return 0;
    }

    const std::string executable_path =
        std::filesystem::absolute(std::filesystem::path(argv[0])).string();

    while (true) {
        try {
            printMenu();
            const int choice = readInt("enter choice: ");

            if (choice == 0) {
                std::cout << "program closed.\n";
                break;
            }

            if (choice == 1) {
                handleSchedulingOption();
            } else if (choice == 2) {
                handleSynchronizationOption();
            } else if (choice == 3) {
                handleBankerOption();
            } else if (choice == 4) {
                handleIpcOption(executable_path);
            } else if (choice == 5) {
                handleMemoryOption();
            } else if (choice == 6) {
                runFullProjectOption(executable_path);
            } else if (choice == 7) {
                runBuiltInValidation(executable_path);
            } else {
                std::cout << "invalid menu choice.\n";
            }

            pressEnterToContinue();
        } catch (const std::exception& error) {
            std::cout << "\nerror: " << error.what() << "\n";
            pressEnterToContinue();
        }
    }

    return 0;
}
