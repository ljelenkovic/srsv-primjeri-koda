#include "../common/common.h"
#include "../common/rabbitmq_client.h"
#include <thread>
#include <atomic>
#include <csignal>
#include <sstream>
#include <fstream>
#include <unistd.h>

using namespace std;

atomic<bool> running(true);
Statistics stats;
int events_processed = 0;
int crash_after_n_events = 20;  // Default: crash after 20 events

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        log_event("PROCESSOR", "SHUTDOWN", "Signal received, shutting down gracefully");
        running = false;
    }
}

void update_health_marker() {
    ofstream health("/tmp/healthy");
    if (health.is_open()) {
        health << get_current_time_ms() << endl;
        health.close();
    }
}

void parse_event(const string& message, Event& evt) {
    // Parse format: id|timestamp|period|proc_time|seq
    stringstream ss(message);
    string token;

    if (getline(ss, token, '|')) evt.event_id = token;
    if (getline(ss, token, '|')) evt.timestamp_ms = stoll(token);
    if (getline(ss, token, '|')) evt.period_ms = stoi(token);
    if (getline(ss, token, '|')) evt.processing_time_ms = stoi(token);
    if (getline(ss, token, '|')) evt.sequence = stoi(token);
}

void process_event(RabbitMQClient& client, const Event& evt, const string& instance_id) {
    long long start_time = get_current_time_ms();
    long long reaction_time = start_time - evt.timestamp_ms;

    // Log processing start
    stringstream lifecycle_msg;
    lifecycle_msg << "processor|" << instance_id << "|processing_started|"
                  << start_time << "|" << evt.event_id;
    client.publish("lifecycle", lifecycle_msg.str());

    log_event("PROCESSOR", "PROCESSING", "Event " + evt.event_id +
             " (reaction=" + to_string(reaction_time) + "ms, " +
             "proc_time=" + to_string(evt.processing_time_ms) + "ms)");

    // Simulate processing work
    this_thread::sleep_for(milliseconds(evt.processing_time_ms));

    long long end_time = get_current_time_ms();

    // Send completion message
    stringstream completion_msg;
    completion_msg << evt.event_id << "|"
                   << evt.timestamp_ms << "|"
                   << start_time << "|"
                   << end_time << "|"
                   << reaction_time << "|1";  // success=1
    client.publish("results", completion_msg.str());

    // Log processing completion
    lifecycle_msg.str("");
    lifecycle_msg << "processor|" << instance_id << "|processing_completed|"
                  << end_time << "|" << evt.event_id;
    client.publish("lifecycle", lifecycle_msg.str());

    log_event("PROCESSOR", "COMPLETED", "Event " + evt.event_id +
             " (total_time=" + to_string(end_time - start_time) + "ms)");

    stats.add_reaction_time(reaction_time);
    events_processed++;

    if (events_processed >= crash_after_n_events) {
        log_event("PROCESSOR", "CRASH", "Intentional crash after " +
                 to_string(events_processed) + " events");

        // Send crash lifecycle message
        lifecycle_msg.str("");
        lifecycle_msg << "processor|" << instance_id << "|crash|"
                      << get_current_time_ms() << "|After " << events_processed << " events";
        client.publish("lifecycle", lifecycle_msg.str());

        this_thread::sleep_for(milliseconds(100));

        // Exit with failure code (Docker will restart)
        exit(1);
    }
}

int main() {
    log_event("PROCESSOR", "STARTUP", "Processor service starting");

    string instance_id = string(getenv("HOSTNAME") ? getenv("HOSTNAME") : "processor_1");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Get configuration from environment
    string rabbitmq_host = get_env_or_default("RABBITMQ_HOST", "localhost");
    string rabbitmq_user = get_env_or_default("RABBITMQ_USER", "guest");
    string rabbitmq_pass = get_env_or_default("RABBITMQ_PASS", "guest");
    crash_after_n_events = get_env_int_or_default("CRASH_AFTER_N_EVENTS", 20);

    log_event("PROCESSOR", "CONFIG", "Connecting to RabbitMQ at " + rabbitmq_host);
    log_event("PROCESSOR", "CONFIG", "Will crash after " + to_string(crash_after_n_events) + " events");

    // Connect to RabbitMQ with retry
    RabbitMQClient client(rabbitmq_host, 5672, rabbitmq_user, rabbitmq_pass);

    int retry_count = 0;
    while (!client.connect() && retry_count < 10) {
        log_event("PROCESSOR", "WARNING", "Failed to connect to RabbitMQ, retrying in 2s...");
        this_thread::sleep_for(seconds(2));
        retry_count++;
    }

    if (!client.is_connected()) {
        log_event("PROCESSOR", "ERROR", "Could not connect to RabbitMQ after 10 retries");
        return 1;
    }

    log_event("PROCESSOR", "INFO", "Connected to RabbitMQ");

    // Declare queues
    client.declare_queue("events", true);
    client.declare_queue("results", true);
    client.declare_queue("lifecycle", true);

    // Send startup lifecycle message
    stringstream lifecycle_msg;
    lifecycle_msg << "processor|" << instance_id << "|startup|"
                  << get_current_time_ms() << "|";
    client.publish("lifecycle", lifecycle_msg.str());

    // Start consuming from events queue
    if (!client.consume_start("events")) {
        log_event("PROCESSOR", "ERROR", "Failed to start consuming from events queue");
        return 1;
    }

    log_event("PROCESSOR", "INFO", "Listening for events...");

    // Create health marker file
    update_health_marker();

    // Main processing loop
    while (running) {
        string message;

        if (client.consume_message(message, 1)) {  // 1 second timeout
            Event evt;
            parse_event(message, evt);
            process_event(client, evt, instance_id);

            // Update health marker after processing
            update_health_marker();
        }
    }

    // Send shutdown lifecycle message
    lifecycle_msg.str("");
    lifecycle_msg << "processor|" << instance_id << "|shutdown|"
                  << get_current_time_ms() << "|Processed " << events_processed << " events";
    client.publish("lifecycle", lifecycle_msg.str());

    // Print statistics
    stats.total_events_processed = events_processed;
    stats.print_summary("PROCESSOR");

    client.disconnect();
    log_event("PROCESSOR", "SHUTDOWN", "Processor service stopped");

    return 0;
}
