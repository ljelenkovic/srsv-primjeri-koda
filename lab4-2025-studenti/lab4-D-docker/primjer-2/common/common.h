#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <ctime>
#include <climits>

using namespace std;
using namespace std::chrono;

// Get current time in milliseconds since epoch
inline long long get_current_time_ms() {
    auto now = steady_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return ms;
}

// Get wall clock time for human-readable logging
inline string get_wall_clock_time() {
    auto now = system_clock::now();
    auto now_c = system_clock::to_time_t(now);
    auto now_ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;

    stringstream ss;
    ss << put_time(localtime(&now_c), "%H:%M:%S");
    ss << "." << setfill('0') << setw(3) << now_ms;
    return ss.str();
}

// Formatted logging with timestamp
inline void log_event(const string& service_name, const string& event_type, const string& message = "") {
    long long timestamp_ms = get_current_time_ms();
    string wall_time = get_wall_clock_time();

    cout << "[" << wall_time << "] [" << timestamp_ms << "ms] "
         << "[" << service_name << "] [" << event_type << "] "
         << message << endl;
    cout.flush();
}

// Event structure (similar to Input from Lab1-3)
struct Event {
    string event_id;
    long long timestamp_ms;
    int period_ms;
    int processing_time_ms;
    int sequence;

    Event() : event_id(""), timestamp_ms(0), period_ms(0),
              processing_time_ms(0), sequence(0) {}

    Event(const string& id, long long ts, int period, int proc_time, int seq)
        : event_id(id), timestamp_ms(ts), period_ms(period),
          processing_time_ms(proc_time), sequence(seq) {}
};

// Completion message structure
struct CompletionMessage {
    string event_id;
    long long event_timestamp_ms;
    long long started_at_ms;
    long long completed_at_ms;
    long long reaction_time_ms;
    bool success;

    CompletionMessage() : event_id(""), event_timestamp_ms(0),
                          started_at_ms(0), completed_at_ms(0),
                          reaction_time_ms(0), success(false) {}
};

// Lifecycle message structure
struct LifecycleMessage {
    string service;
    string instance_id;
    string event;  // "startup", "crash", "shutdown", "processing_started", "processing_completed"
    long long timestamp_ms;
    string details;

    LifecycleMessage() : service(""), instance_id(""), event(""),
                         timestamp_ms(0), details("") {}

    LifecycleMessage(const string& svc, const string& inst, const string& evt,
                     long long ts, const string& det = "")
        : service(svc), instance_id(inst), event(evt),
          timestamp_ms(ts), details(det) {}
};

// Statistics structure
struct Statistics {
    int total_events_generated = 0;
    int total_events_processed = 0;
    int total_events_unprocessed = 0;
    long long total_reaction_time_ms = 0;
    long long max_reaction_time_ms = 0;
    long long min_reaction_time_ms = LLONG_MAX;

    int total_crashes = 0;
    long long total_recovery_time_ms = 0;
    long long max_recovery_time_ms = 0;
    long long min_recovery_time_ms = LLONG_MAX;

    void add_reaction_time(long long rt_ms) {
        total_events_processed++;
        total_reaction_time_ms += rt_ms;
        if (rt_ms > max_reaction_time_ms) max_reaction_time_ms = rt_ms;
        if (rt_ms < min_reaction_time_ms) min_reaction_time_ms = rt_ms;
    }

    void add_recovery_time(long long rec_ms) {
        total_crashes++;
        total_recovery_time_ms += rec_ms;
        if (rec_ms > max_recovery_time_ms) max_recovery_time_ms = rec_ms;
        if (rec_ms < min_recovery_time_ms) min_recovery_time_ms = rec_ms;
    }

    void print_summary(const string& service_name) {
        cout << "\n========== " << service_name << " Statistics ==========" << endl;
        cout << "Total events generated: " << total_events_generated << endl;
        cout << "Total events processed: " << total_events_processed << endl;
        cout << "Total events unprocessed: " << total_events_unprocessed << endl;

        if (total_events_processed > 0) {
            cout << "Average reaction time: "
                 << (double)total_reaction_time_ms / total_events_processed
                 << " ms" << endl;
            cout << "Max reaction time: " << max_reaction_time_ms << " ms" << endl;
            cout << "Min reaction time: " << min_reaction_time_ms << " ms" << endl;
        }

        if (total_crashes > 0) {
            cout << "\nTotal crashes: " << total_crashes << endl;
            cout << "Average recovery time: "
                 << (double)total_recovery_time_ms / total_crashes
                 << " ms" << endl;
            cout << "Max recovery time: " << max_recovery_time_ms << " ms" << endl;
            cout << "Min recovery time: " << min_recovery_time_ms << " ms" << endl;
        }

        cout << "==========================================" << endl;
    }
};

#endif // COMMON_H
