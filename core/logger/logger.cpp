/**
 * @file logger.cpp
 * @brief Asynchronous logger implementation for YDLidar SDK.
 */
#include "logger.hpp"

#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <deque>
#include <exception>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#if defined(__linux__)
#include <pthread.h>
#endif

namespace ydlidar::core::Log {
namespace {
struct LogMessage {
    std::chrono::milliseconds timestamp;
    Facility facility;
    std::string identifier;
    std::string message;
};
std::mutex loggerCountMutex;
int loggerCount = 0;
std::atomic<bool> runTasks{false};
std::mutex queueMutex;
std::condition_variable queueCondition;
std::deque<LogMessage> queue;
std::thread outputThread;
constexpr std::size_t maxQueuedMessages = 4096;

std::string colorFor(Facility facility) {
    switch (facility) {
        case EMERG: return "1;91m";
        case ALERT: return "0;91m";
        case CRIT:
        case ERR: return "0;31m";
        case WARNING: return "0;93m";
        case NOTICE: return "0;35m";
        case INFO: return "0;32m";
        case DEBUG: return "0;34m";
        default: return "0m";
    }
}
std::string nameFor(Facility facility) {
    switch (facility) {
        case EMERG: return "EMERG";
        case ALERT: return "ALERT";
        case CRIT: return "CRIT";
        case ERR: return "ERROR";
        case WARNING: return "WARN";
        case NOTICE: return "NOTICE";
        case INFO: return "INFO";
        case DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
void pushMessage(std::chrono::milliseconds timestamp, Facility facility,
                 const std::string &identifier, const std::string &message) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (queue.size() >= maxQueuedMessages) queue.pop_front();
        queue.push_back({timestamp, facility, identifier, message});
    }
    queueCondition.notify_one();
}
void outputTask() {
    try {
        for (;;) {
            std::deque<LogMessage> messages;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [] {
                    return !queue.empty() || !runTasks.load(std::memory_order_acquire);
                });
                // Drain pending messages before stopping the output thread.
                if (queue.empty() && !runTasks.load(std::memory_order_acquire)) break;
                messages.swap(queue);
            }
            for (const auto &message : messages) {
                const auto timePoint = std::chrono::system_clock::time_point(message.timestamp);
                const auto seconds = std::chrono::system_clock::to_time_t(timePoint);
                const auto milliseconds = message.timestamp.count() % 1000;
                std::tm timeInfo{};
#if defined(_WIN32)
                localtime_s(&timeInfo, &seconds);
#else
                localtime_r(&seconds, &timeInfo);
#endif
                std::ostringstream output;
                output << "\033[0;37m[" << std::put_time(&timeInfo, "%F %T") << '.'
                       << std::setfill('0') << std::setw(3) << milliseconds << "] \033[0m"
                       << "\033[" << colorFor(message.facility) << nameFor(message.facility)
                       << "\033[0m [" << message.identifier << "] " << message.message << '\n';
                // High-severity diagnostics belong on stderr.
                if (message.facility <= WARNING) std::cerr << output.str();
                else std::cout << output.str();
            }
        }
    } catch (const std::exception &exception) {
        std::fprintf(stderr, "YDLidar logger failed: %s\n", exception.what());
        std::fflush(stderr);
    } catch (...) {
        std::fputs("YDLidar logger failed: unknown exception\n", stderr);
        std::fflush(stderr);
    }
}
} // namespace

Logger::Logger(const std::string &ident, Facility maxFacility)
    : _ident(ident), _maxFacility(maxFacility) {
    std::lock_guard<std::mutex> lock(loggerCountMutex);
    ++loggerCount;
    if (loggerCount == 1) {
        runTasks.store(true, std::memory_order_release);
        if (!outputThread.joinable()) {
            outputThread = std::thread(outputTask);
#if defined(__linux__)
            pthread_setname_np(outputThread.native_handle(), "YDLidarLogger");
#endif
        }
    }
}
Logger::~Logger() {
    std::lock_guard<std::mutex> lock(loggerCountMutex);
    if (loggerCount > 0 && --loggerCount == 0) {
        runTasks.store(false, std::memory_order_release);
        queueCondition.notify_all();
        if (outputThread.joinable()) outputThread.join();
    }
}
void Logger::msg(Facility facility, const std::string &message) {
    if (facility > _maxFacility.load(std::memory_order_relaxed)) return;
    const auto now = std::chrono::system_clock::now();
    pushMessage(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()),
                facility, _ident, message);
}
void Logger::emerg(const std::string &message) { msg(EMERG, message); }
void Logger::alert(const std::string &message) { msg(ALERT, message); }
void Logger::critical(const std::string &message) { msg(CRIT, message); }
void Logger::error(const std::string &message) { msg(ERR, message); }
void Logger::warning(const std::string &message) { msg(WARNING, message); }
void Logger::notice(const std::string &message) { msg(NOTICE, message); }
void Logger::info(const std::string &message) { msg(INFO, message); }
void Logger::debug(const std::string &message) { msg(DEBUG, message); }
void Logger::setFacility(Facility facility) { _maxFacility.store(facility, std::memory_order_relaxed); }
const std::string Logger::facilityToString(Facility facility) { return nameFor(facility); }
const std::string Logger::facilityToColor(Facility facility) { return colorFor(facility); }
} // namespace ydlidar::core::Log
