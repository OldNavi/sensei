/**
 * @brief Class handling communication with XMOS based controller hardware over Raspa
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SENSEI_RASPA_FRONTEND_H
#define SENSEI_RASPA_FRONTEND_H

#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <cassert>
#include <xmos_control_protocol.h>

#include "hw_frontend.h"
#include "message_tracker.h"
#include "message/base_message.h"
#include "message/base_command.h"
#include "message/message_factory.h"
#include "xmos_command_creator.h"
#include "xmos_control_protocol.h"

namespace sensei {
namespace hw_frontend {

struct ControlPacket;

class RaspaFrontend : public HwFrontend
{
public:
    /**
    * @brief Class constructor
    *
    * @param [in] in_queue Output queue where decoded messages go
    * @param [in] out_queue Queue for messages to be sent to the board
    */
    RaspaFrontend(SynchronizedQueue<std::unique_ptr<Command>>*in_queue,
                  SynchronizedQueue<std::unique_ptr<BaseMessage>>*out_queue);

    ~RaspaFrontend();

    /**
    * @brief Returns true if the connection to Raspa is up and running
    * @return State of connection
    */
    bool connected() override;

    /**
    * @brief Start the frontend
    */
    void run() override;

    /**
    * @brief Stops the frontend if it is running
    */
    void stop() override;

    /**
     * @brief Stops the flow of messages. If enabled, incoming packets are silently dropped.
     *
     * @param [in] enabled Sets mute enabled/disabled
     */
    void mute(bool enabled) override;

    /**
     * @brief Enables tracking and verification of packets ent
     *
     * @param [in] enabled Sets ack verification enabled/disabled
     */
    void verify_acks(bool enabled) override;

private:
    enum class ThreadState : int
    {
        RUNNING,
        STOPPING,
        STOPPED,
    };

    void read_loop();
    void write_loop();

    //void _handle_timeouts();
    bool _connect_to_raspa();
    void _handle_raspa_packet(const XmosControlPacket& packet);
    void _handle_ack(const XmosControlPacket& ack);
    void _handle_value(const XmosControlPacket& packet);
    void _process_sensei_command(const Command*message);

    MessageFactory   _message_factory;
    XmosCommandCreator _packet_factory;
    std::deque<XmosControlPacket>  _send_list;

    std::atomic<ThreadState> _state;
    std::thread     _read_thread;
    std::thread     _write_thread;

    int             _in_socket;
    int             _out_socket;

    std::mutex      _send_mutex;
    std::condition_variable _ready_to_send_notifier;

    uint32_t        _pending_sequence_number;
    bool            _ready_to_send;
    bool            _connected;
    bool            _muted;
    bool            _verify_acks;
};


} // end namespace hw_frontend
} // end namespace sensei

#endif //SENSEI_RASPA_FRONTEND_H
