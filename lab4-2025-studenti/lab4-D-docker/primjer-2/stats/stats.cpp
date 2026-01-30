#include "../common/common.h"
#include "../common/rabbitmq_client.h"
#include <thread>
#include <atomic>
#include <csignal>
#include <sstream>
#include <map>
#include <vector>
#include <climits>
#include <unistd.h>

using namespace std;

atomic<bool> running(true);
Statistics global_stats;

// Track service instances for recovery time calculation
struct ServiceInstance {
    string instance_id;
    long long last_crash_ms;
    long long last_startup_ms;
    int crash_count;
    vector<long long> recovery_times_ms;

    ServiceInstance() : instance_id(""), last_crash_ms(0),
                       last_startup_ms(0), crash_count(0) {}
};

map<string, ServiceInstance> instances;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        log_event("STATS", "SHUTDOWN", "Signal received, shutting down gracefully");
        running = false;
    }
}

void parse_completion_message(const string& message, CompletionMessage& comp) {
    // Parse format: event_id|event_timestamp|started_at|completed_at|reaction_time|success
    stringstream ss(message);
    string token;

    if (getline(ss, token, '|')) comp.event_id = token;
    if (getline(ss, token, '|')) comp.event_timestamp_ms = stoll(token);
    if (getline(ss, token, '|')) comp.started_at_ms = stoll(token);
    if (getline(ss, token, '|')) comp.completed_at_ms = stoll(token);
    if (getline(ss, token, '|')) comp.reaction_time_ms = stoll(token);
    if (getline(ss, token, '|')) comp.success = (token == "1");
}

void parse_lifecycle_message(const string& message, LifecycleMessage& lifecycle) {
    // Parse format: service|instance_id|event|timestamp|details
    stringstream ss(message);
    string token;

    if (getline(ss, token, '|')) lifecycle.service = token;
    if (getline(ss, token, '|')) lifecycle.instance_id = token;
    if (getline(ss, token, '|')) lifecycle.event = token;
    if (getline(ss, token, '|')) lifecycle.timestamp_ms = stoll(token);
    if (getline(ss, token, '|')) lifecycle.details = token;
}

void handle_lifecycle_message(const LifecycleMessage& msg) {
    string key = msg.service + ":" + msg.instance_id;

    if (msg.event == "crash" || msg.event == "shutdown") {
        instances[key].instance_id = msg.instance_id;
        instances[key].last_crash_ms = msg.timestamp_ms;

        if (msg.event == "crash") {
            instances[key].crash_count++;
            log_event("STATS", "LIFECYCLE", msg.service + " instance " + msg.instance_id +
                     " CRASHED at " + to_string(msg.timestamp_ms) + "ms (" + msg.details + ")");
        } else {
            log_event("STATS", "LIFECYCLE", msg.service + " instance " + msg.instance_id +
                     " SHUTDOWN at " + to_string(msg.timestamp_ms) + "ms");
        }
    }
    else if (msg.event == "startup") {
        instances[key].instance_id = msg.instance_id;
        instances[key].last_startup_ms = msg.timestamp_ms;

        log_event("STATS", "LIFECYCLE", msg.service + " instance " + msg.instance_id +
                 " STARTUP at " + to_string(msg.timestamp_ms) + "ms");

        // Calculate recovery time if there was a previous crash
        if (instances[key].last_crash_ms > 0) {
            long long recovery_time = msg.timestamp_ms - instances[key].last_crash_ms;
            instances[key].recovery_times_ms.push_back(recovery_time);

            log_event("STATS", "RECOVERY", "Instance " + msg.instance_id +
                     " recovered in " + to_string(recovery_time) + "ms" +
                     " (crash_count=" + to_string(instances[key].crash_count) + ")");

            global_stats.add_recovery_time(recovery_time);
        }
    }
}

void results_consumer_thread(RabbitMQClient& client) {
    log_event("STATS", "INFO", "Starting results consumer thread");

    if (!client.consume_start("results")) {
        log_event("STATS", "ERROR", "Failed to start consuming from results queue");
        return;
    }

    while (running) {
        string message;
        if (client.consume_message(message, 1)) {
            CompletionMessage comp;
            parse_completion_message(message, comp);

            global_stats.add_reaction_time(comp.reaction_time_ms);

            log_event("STATS", "RESULT", "Event " + comp.event_id +
                     " completed (reaction=" + to_string(comp.reaction_time_ms) + "ms)");
        }
    }

    log_event("STATS", "INFO", "Results consumer thread stopped");
}

void lifecycle_consumer_thread(RabbitMQClient& client) {
    log_event("STATS", "INFO", "Starting lifecycle consumer thread");

    if (!client.consume_start("lifecycle")) {
        log_event("STATS", "ERROR", "Failed to start consuming from lifecycle queue");
        return;
    }

    while (running) {
        string message;
        if (client.consume_message(message, 1)) {
            LifecycleMessage lifecycle;
            parse_lifecycle_message(message, lifecycle);
            handle_lifecycle_message(lifecycle);
        }
    }

    log_event("STATS", "INFO", "Lifecycle consumer thread stopped");
}

void print_recovery_statistics() {
    cout << "\n========== Recovery Statistics ==========" << endl;

    for (const auto& pair : instances) {
        const ServiceInstance& inst = pair.second;

        if (inst.crash_count > 0) {
            cout << "\nService: " << pair.first << endl;
            cout << "  Total crashes: " << inst.crash_count << endl;
            cout << "  Recovery times: ";

            if (!inst.recovery_times_ms.empty()) {
                long long total = 0;
                long long max_rt = 0;
                long long min_rt = LLONG_MAX;

                for (long long rt : inst.recovery_times_ms) {
                    total += rt;
                    if (rt > max_rt) max_rt = rt;
                    if (rt < min_rt) min_rt = rt;
                    cout << rt << "ms ";
                }

                cout << endl;
                cout << "  Average recovery time: "
                     << (double)total / inst.recovery_times_ms.size() << "ms" << endl;
                cout << "  Min recovery time: " << min_rt << "ms" << endl;
                cout << "  Max recovery time: " << max_rt << "ms" << endl;
            } else {
                cout << "(no recovery data)" << endl;
            }
        }
    }

    cout << "==========================================" << endl;
}

int main() {
    log_event("STATS", "STARTUP", "Stats service starting");

    string instance_id = string(getenv("HOSTNAME") ? getenv("HOSTNAME") : "stats_1");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Get configuration from environment
    string rabbitmq_host = get_env_or_default("RABBITMQ_HOST", "localhost");
    string rabbitmq_user = get_env_or_default("RABBITMQ_USER", "guest");
    string rabbitmq_pass = get_env_or_default("RABBITMQ_PASS", "guest");

    log_event("STATS", "CONFIG", "Connecting to RabbitMQ at " + rabbitmq_host);

    // Create two separate connections for two consumer threads
    RabbitMQClient results_client(rabbitmq_host, 5672, rabbitmq_user, rabbitmq_pass);
    RabbitMQClient lifecycle_client(rabbitmq_host, 5672, rabbitmq_user, rabbitmq_pass);

    // Connect both clients with retry
    int retry_count = 0;
    while ((!results_client.connect() || !lifecycle_client.connect()) && retry_count < 10) {
        log_event("STATS", "WARNING", "Failed to connect to RabbitMQ, retrying in 2s...");
        this_thread::sleep_for(seconds(2));
        retry_count++;
    }

    if (!results_client.is_connected() || !lifecycle_client.is_connected()) {
        log_event("STATS", "ERROR", "Could not connect to RabbitMQ after 10 retries");
        return 1;
    }

    log_event("STATS", "INFO", "Connected to RabbitMQ");

    // Declare queues
    results_client.declare_queue("results", true);
    lifecycle_client.declare_queue("lifecycle", true);

    // Send startup lifecycle message
    stringstream lifecycle_msg;
    lifecycle_msg << "stats|" << instance_id << "|startup|"
                  << get_current_time_ms() << "|";
    lifecycle_client.publish("lifecycle", lifecycle_msg.str());

    // Start consumer threads
    thread results_thread(results_consumer_thread, ref(results_client));
    thread lifecycle_thread(lifecycle_consumer_thread, ref(lifecycle_client));

    log_event("STATS", "INFO", "All consumer threads started, running...");

    // Wait for threads to finish
    if (results_thread.joinable()) results_thread.join();
    if (lifecycle_thread.joinable()) lifecycle_thread.join();

    // Send shutdown lifecycle message
    lifecycle_msg.str("");
    lifecycle_msg << "stats|" << instance_id << "|shutdown|"
                  << get_current_time_ms() << "|";
    lifecycle_client.publish("lifecycle", lifecycle_msg.str());

    // Print comprehensive statistics
    global_stats.print_summary("STATS");
    print_recovery_statistics();

    results_client.disconnect();
    lifecycle_client.disconnect();

    log_event("STATS", "SHUTDOWN", "Stats service stopped");

    return 0;
}
