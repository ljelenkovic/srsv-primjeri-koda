#include "../common/common.h"
#include "../common/rabbitmq_client.h"
#include <thread>
#include <vector>
#include <atomic>
#include <csignal>
#include <sstream>
#include <unistd.h>

using namespace std;

atomic<bool> running(true);
Statistics stats;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        log_event("PRODUCER", "SHUTDOWN", "Signal received, shutting down gracefully");
        running = false;
    }
}

// Event generator thread
void event_generator(RabbitMQClient& client, int period_ms, int processing_time_ms, int id) {
    string instance_id = string(getenv("HOSTNAME") ? getenv("HOSTNAME") : "producer_1");
    int sequence = 0;

    // Wait for initial offset to stagger event generation
    this_thread::sleep_for(milliseconds(id * 100));

    while (running) {
        long long event_time = get_current_time_ms();

        // Create event
        Event evt(
            "evt_" + to_string(id) + "_" + to_string(sequence),
            event_time,
            period_ms,
            processing_time_ms,
            sequence
        );

        // Serialize event to simple format: id|timestamp|period|proc_time|seq
        stringstream ss;
        ss << evt.event_id << "|"
           << evt.timestamp_ms << "|"
           << evt.period_ms << "|"
           << evt.processing_time_ms << "|"
           << evt.sequence;

        string message = ss.str();

        // Publish to events queue
        if (client.publish("events", message)) {
            stats.total_events_generated++;
            log_event("PRODUCER", "EVENT_SENT", "Event " + evt.event_id +
                     " (period=" + to_string(period_ms) + "ms, " +
                     "proc_time=" + to_string(processing_time_ms) + "ms)");
        } else {
            log_event("PRODUCER", "ERROR", "Failed to publish event " + evt.event_id);
        }

        sequence++;

        // Sleep until next period
        this_thread::sleep_for(milliseconds(period_ms));
    }
}

int main() {
    log_event("PRODUCER", "STARTUP", "Producer service starting");

    // Send lifecycle message
    string instance_id = string(getenv("HOSTNAME") ? getenv("HOSTNAME") : "producer_1");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Get RabbitMQ connection parameters from environment
    string rabbitmq_host = get_env_or_default("RABBITMQ_HOST", "localhost");
    string rabbitmq_user = get_env_or_default("RABBITMQ_USER", "guest");
    string rabbitmq_pass = get_env_or_default("RABBITMQ_PASS", "guest");

    log_event("PRODUCER", "CONFIG", "Connecting to RabbitMQ at " + rabbitmq_host);

    // Connect to RabbitMQ
    RabbitMQClient client(rabbitmq_host, 5672, rabbitmq_user, rabbitmq_pass);

    // Retry connection with backoff
    int retry_count = 0;
    while (!client.connect() && retry_count < 10) {
        log_event("PRODUCER", "WARNING", "Failed to connect to RabbitMQ, retrying in 2s...");
        this_thread::sleep_for(seconds(2));
        retry_count++;
    }

    if (!client.is_connected()) {
        log_event("PRODUCER", "ERROR", "Could not connect to RabbitMQ after 10 retries");
        return 1;
    }

    log_event("PRODUCER", "INFO", "Connected to RabbitMQ");

    // Declare queues
    client.declare_queue("events", true);
    client.declare_queue("lifecycle", true);

    // Send startup lifecycle message
    stringstream lifecycle_msg;
    lifecycle_msg << "producer|" << instance_id << "|startup|" << get_current_time_ms() << "|";
    client.publish("lifecycle", lifecycle_msg.str());

    // Parse event configuration from environment
    vector<pair<int, int>> event_configs = {
        {1000, 150},   // Type A
        {5000, 250},   // Type B
        {10000, 400}   // Type C
    };

    log_event("PRODUCER", "INFO", "Starting " + to_string(event_configs.size()) +
             " event generator threads");

    // Start event generator threads
    vector<thread> generators;
    for (size_t i = 0; i < event_configs.size(); i++) {
        generators.emplace_back(
            event_generator,
            ref(client),
            event_configs[i].first,
            event_configs[i].second,
            i
        );
    }

    log_event("PRODUCER", "INFO", "All event generators started, running...");

    // Wait for signal
    for (auto& t : generators) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Send shutdown lifecycle message
    lifecycle_msg.str("");
    lifecycle_msg << "producer|" << instance_id << "|shutdown|" << get_current_time_ms() << "|";
    client.publish("lifecycle", lifecycle_msg.str());

    // Print statistics
    stats.print_summary("PRODUCER");

    client.disconnect();
    log_event("PRODUCER", "SHUTDOWN", "Producer service stopped");

    return 0;
}
