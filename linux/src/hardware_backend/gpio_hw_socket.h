#ifndef SENSEI_GPIO_HW_SOCKET_H
#define SENSEI_GPIO_HW_SOCKET_H

#include <string>
#include <atomic>

#include "base_hw_backend.h"

namespace sensei {
namespace hw_backend {

/**
 * @brief Class to provide an abstract interface to transfer gpio packets over unix sockets.
 *        This class creates a sensei socket and connects to the socket of the gpio hw process.
 *        It also provides member function to help synchronise and maintian connection to the 
 *        gpio hw socket.
 */
class GpioHwSocket : public BaseHwBackend
{
public:
    /**
     * @brief Construct a new Gpio Hw Socket backend.
     *
     * @param gpio_hw_socket_name The socket name to which it should connect to
     */
    GpioHwSocket(std::string gpio_hw_socket_name) : _in_socket(-1),
                                                    _out_socket(-1),
                                                    _connected(false),
                                                    _gpio_hw_socket_name(gpio_hw_socket_name)
    {}

    ~GpioHwSocket()
    {}

    /**
     * @brief Initialize the socket.
     */
    void init() override;

    /**
     * @brief Close and unlink the socket.
     */
    void deinit() override;

    /**
     * @brief Send a gpio packet through to socket. If it is unable to send, it will
     *        assume that the connection has been lost and set the flag _connected as
     *        false
     *
     * @param tx_gpio_packet The gpio packet to be sent.0
     * @return true The packet was sent successfully
     * @return false The packet was not sent. _connected flag is set to false.
     */
    bool send_gpio_packet(const gpio::GpioPacket& tx_gpio_packet) override;

    /**
     * @brief Receive a gpio packet through the socket. Unlike the sending thread, this
     *        does not assume that the sockets are not connected if unable to receive.
     *
     * @param rx_gpio_packet The destination to store the incoming packet
     * @return true If successful rx
     * @return false if unsuccessful
     */
    bool receive_gpio_packet(gpio::GpioPacket& rx_gpio_packet) override;

    /**
     * @brief Get status of the socket connection. Returns _connected
     *
     * @return true if connected
     * @return false if not connected
     */
    bool get_status() override;

    /**
     * @brief Reconnect to the gpio hw socket.
     *
     */
    void reconnect_to_gpio_hw() override;

private:
    /**
     * @brief Helper member to connect to the gpio hw socket.
     *
     * @return true for a succesful connection
     * @return false false if not
     */
    bool _connect_to_gpio_hw_socket();

    int _in_socket;
    int _out_socket;

    bool _connected;

    std::string _gpio_hw_socket_name;
};

} // hw_backend
} // sensei

#endif // SENSEI_GPIO_HW_SOCKET_H