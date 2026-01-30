#ifndef RABBITMQ_CLIENT_H
#define RABBITMQ_CLIENT_H

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace std;

class RabbitMQClient {
private:
    amqp_connection_state_t conn;
    amqp_channel_t channel;
    string hostname;
    int port;
    string username;
    string password;
    bool connected;

    void handle_amqp_error(amqp_rpc_reply_t reply, const string& context) {
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            cerr << "RabbitMQ error in " << context << ": ";
            if (reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
                cerr << "Library exception: " << amqp_error_string2(reply.library_error) << endl;
            } else if (reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
                if (reply.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
                    cerr << "Connection closed by server" << endl;
                } else if (reply.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
                    cerr << "Channel closed by server" << endl;
                }
            }
        }
    }

public:
    RabbitMQClient(const string& host = "localhost", int p = 5672,
                   const string& user = "guest", const string& pass = "guest")
        : channel(1), hostname(host), port(p), username(user), password(pass),
          connected(false) {
        conn = amqp_new_connection();
    }

    ~RabbitMQClient() {
        disconnect();
    }

    bool connect() {
        amqp_socket_t* socket = amqp_tcp_socket_new(conn);
        if (!socket) {
            cerr << "Failed to create TCP socket" << endl;
            return false;
        }

        int status = amqp_socket_open(socket, hostname.c_str(), port);
        if (status != AMQP_STATUS_OK) {
            cerr << "Failed to open socket: " << amqp_error_string2(status) << endl;
            return false;
        }

        amqp_rpc_reply_t reply = amqp_login(conn, "/", 0, 131072, 0,
                                             AMQP_SASL_METHOD_PLAIN,
                                             username.c_str(), password.c_str());
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            handle_amqp_error(reply, "login");
            return false;
        }

        amqp_channel_open(conn, channel);
        reply = amqp_get_rpc_reply(conn);
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            handle_amqp_error(reply, "channel open");
            return false;
        }

        connected = true;
        return true;
    }

    void disconnect() {
        if (connected) {
            amqp_channel_close(conn, channel, AMQP_REPLY_SUCCESS);
            amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
            amqp_destroy_connection(conn);
            connected = false;
        }
    }

    bool declare_queue(const string& queue_name, bool durable = true) {
        if (!connected) return false;

        amqp_queue_declare(conn, channel,
                          amqp_cstring_bytes(queue_name.c_str()),
                          0, durable, 0, 0, amqp_empty_table);

        amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn);
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            handle_amqp_error(reply, "queue declare");
            return false;
        }

        return true;
    }

    bool publish(const string& queue_name, const string& message) {
        if (!connected) return false;

        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.delivery_mode = 2; // persistent delivery mode

        int status = amqp_basic_publish(
            conn, channel,
            amqp_cstring_bytes(""),  // default exchange
            amqp_cstring_bytes(queue_name.c_str()),
            0, 0, &props,
            amqp_cstring_bytes(message.c_str())
        );

        if (status != AMQP_STATUS_OK) {
            cerr << "Failed to publish: " << amqp_error_string2(status) << endl;
            return false;
        }

        return true;
    }

    bool consume_start(const string& queue_name) {
        if (!connected) return false;

        amqp_basic_consume(conn, channel,
                          amqp_cstring_bytes(queue_name.c_str()),
                          amqp_empty_bytes, 0, 1, 0, amqp_empty_table);

        amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn);
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            handle_amqp_error(reply, "consume start");
            return false;
        }

        return true;
    }

    bool consume_message(string& message_out, int timeout_sec = 1) {
        if (!connected) return false;

        amqp_envelope_t envelope;
        struct timeval timeout;
        timeout.tv_sec = timeout_sec;
        timeout.tv_usec = 0;

        amqp_rpc_reply_t reply = amqp_consume_message(conn, &envelope, &timeout, 0);

        if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
            message_out = string(
                static_cast<char*>(envelope.message.body.bytes),
                envelope.message.body.len
            );
            amqp_destroy_envelope(&envelope);
            return true;
        } else if (reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
            if (reply.library_error == AMQP_STATUS_TIMEOUT) {
                return false;  // Timeout is not an error, just no message
            }
            handle_amqp_error(reply, "consume message");
        }

        return false;
    }

    bool is_connected() const {
        return connected;
    }
};

// Helper function to get environment variable with default value
inline string get_env_or_default(const char* env_var, const string& default_value) {
    const char* val = getenv(env_var);
    return val ? string(val) : default_value;
}

// Helper to get integer environment variable
inline int get_env_int_or_default(const char* env_var, int default_value) {
    const char* val = getenv(env_var);
    return val ? atoi(val) : default_value;
}

#endif // RABBITMQ_CLIENT_H
