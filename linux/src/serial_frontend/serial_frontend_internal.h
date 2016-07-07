/**
 * @brief Internal structs, definitions and helper functions for the serial frontend
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SERIAL_FRONTEND_INTERNAL_H
#define SERIAL_FRONTEND_INTERNAL_H

#include <string>
#include <cstring>
#include <cmath>

#include "../../../common/sensei_serial_protocol.h"

namespace sensei {
namespace serial_frontend {

const unsigned int READ_WRITE_TIMEOUT_MS = 1000;
/*
 * To avoid singularities near 90 degrees, this should be set below 0,5
 * 0,499 clamps at around 86 degrees, se
 * http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/jack.htm
 */
const float QUATERNION_SINGULARITY_LIMIT = 0.4995;


/*
 * Convenience function for comparing header signatures, same pattern as memcmp, strcmp
 */
int inline compare_packet_header(const PACKET_HEADER &lhv, const PACKET_HEADER &rhv)
{
    int diff = 0;
    for (unsigned int i = 0; i < sizeof(lhv.vByte); ++i)
    {
        diff += lhv.vByte[i] - rhv.vByte[i];
    }
    return diff;
}

/*
 * Calculate the checksum of a teensy packet
 */
uint16_t inline calculate_crc(const sSenseiDataPacket *packet)
{
    uint16_t sum = packet->cmd + packet->sub_cmd;
    for (unsigned int i = 0; i < SENSEI_PAYLOAD_LENGTH
                                 + sizeof(packet->continuation)
                                 + sizeof(packet->timestamp); ++i)
    {
        sum += *reinterpret_cast<const uint8_t*>(packet->payload + i);
    }
    return sum;
}

/*
 * Convenience function for getting the uid of an ack packet
 */
uint64_t inline extract_uuid(const sSenseiACKPacket* ack)
{
    uint64_t uuid = ack->timestamp;
    uuid += (static_cast<uint64_t>(ack->cmd) << 32);
    uuid += (static_cast<uint64_t>(ack->sub_cmd) << 48);
    return uuid;
}

/*
 * Convenience function for getting the uid of a data packet
 */
uint64_t inline extract_uuid(const sSenseiDataPacket* packet)
{
    uint64_t uuid = packet->timestamp;
    uuid += (static_cast<uint64_t>(packet->cmd) << 32);
    uuid += (static_cast<uint64_t>(packet->sub_cmd) << 48);
    return uuid;
}


/*
 * Translate a teensy status code to a string for debugging and logging.
 */
inline std::string& translate_teensy_status_code(int code)
{
    static std::string str;
    switch (code)
    {
        case SENSEI_ERROR_CODE::NO_EXTERNAL_PROCESSING_NECESSARY:
            return str = "NO_EXTERNAL_PROCESSING_NECESSARY";
        case SENSEI_ERROR_CODE::OK:
            return str ="OK";
        case SENSEI_ERROR_CODE::START_HEADER_NOT_PRESENT:
            return str ="START_HEADER_NOT_PRESENT";
        case SENSEI_ERROR_CODE::STOP_HEADER_NOT_PRESENT:
            return str ="STOP_HEADER_NOT_PRESENT";
        case SENSEI_ERROR_CODE::CRC_NOT_CORRECT:
            return str ="CRC_NOT_CORRECT";
        case SENSEI_ERROR_CODE::CMD_NOT_VALID:
            return str ="CMD_NOT_VALID";
        case SENSEI_ERROR_CODE::SUB_CMD_NOT_VALID:
            return str ="SUB_CMD_NOT_VALID";
        case SENSEI_ERROR_CODE::CMD_NOT_PROCESSED:
            return str ="CMD_NOT_PROCESSED";
        case SENSEI_ERROR_CODE::DIGITAL_OUTPUT_IDX_BANK_NOT_VALID:
            return str ="DIGITAL_OUTPUT_IDX_BANK_NOT_VALID";
        case SENSEI_ERROR_CODE::DIGITAL_OUTPUT_IDX_PIN_NOT_VALID:
            return str ="DIGITAL_OUTPUT_IDX_PIN_NOT_VALID";
        case SENSEI_ERROR_CODE::IDX_PIN_NOT_VALID:
            return str ="IDX_PIN_NOT_VALID";
        case SENSEI_ERROR_CODE::PIN_TYPE_NOT_VALID:
            return str ="PIN_TYPE_NOT_VALID";
        case SENSEI_ERROR_CODE::TIMEOUT_ON_RESPONSE:
            return str ="TIMEOUT_ON_RESPONSE";
        case SENSEI_ERROR_CODE::INCORRECT_PAYLOAD_SIZE:
            return str ="INCORRECT_PAYLOAD_SIZE";
        case SENSEI_ERROR_CODE::NO_AFFINITY_WITH_RESPONSE_PACKET:
            return str ="NO_AFFINITY_WITH_RESPONSE_PACKET";
        case SENSEI_ERROR_CODE::CMD_NOT_EXPECTED:
            return str ="CMD_NOT_EXPECTED";
        case SENSEI_ERROR_CODE::INCORRECT_PARAMETERS_NUMBER:
            return str ="INCORRECT_PARAMETERS_NUMBER";
        case SENSEI_ERROR_CODE::INCORRECT_PARAMETER_TYPE:
            return str ="INCORRECT_PARAMETER_TYPE";
        case SENSEI_ERROR_CODE::INCOMPLETE_PARAMETERS:
            return str ="INCOMPLETE_PARAMETERS";
        case SENSEI_ERROR_CODE::WRONG_NUMBER_EXPECTED_RESPONSE_PACKETS:
            return str ="WRONG_NUMBER_EXPECTED_RESPONSE_PACKETS";
        case SENSEI_ERROR_CODE::IMU_GENERIC_ERROR:
            return str ="IMU_GENERIC_ERROR";
        case SENSEI_ERROR_CODE::IMU_COMMUNICATION_ERROR:
            return str ="IMU_COMMUNICATION_ERROR";
        case SENSEI_ERROR_CODE::IMU_NOT_CONNECTED:
            return str ="IMU_NOT_CONNECTED";
        case SENSEI_ERROR_CODE::IMU_CMD_NOT_EXECUTED:
            return str ="IMU_CMD_NOT_EXECUTED";
        case SENSEI_ERROR_CODE::IMU_DISABLED:
            return str ="IMU_DISABLED";
        case SENSEI_ERROR_CODE::SERIAL_DEVICE_GENERIC_ERROR:
            return str ="SERIAL_DEVICE_GENERIC_ERROR";
        case SENSEI_ERROR_CODE::SERIAL_DEVICE_PORT_NOT_OPEN:
            return str ="SERIAL_DEVICE_PORT_NOT_OPEN";
        case SENSEI_ERROR_CODE::GENERIC_ERROR:
            return str ="GENERIC_ERROR";
        default:
            return str ="Unknown error code: " + code;
    }

}
struct EulerAngles
{
    float yaw;
    float pitch;
    float roll;
};

/*
 * Convert quaternions (from the IMU) to euler angles (pitch, roll, yaw)
 */
inline EulerAngles quat_to_euler(float qw, float qx, float qy, float qz)
{
    EulerAngles angles;
    float singularity_limit = qw * qx + qy * qz;
    if (singularity_limit > QUATERNION_SINGULARITY_LIMIT)
    {
        angles.yaw   = 2 * atan2(qx, qw);
        angles.pitch = M_PI / 2;
        angles.roll  = 0;
    }
    else if (singularity_limit < -QUATERNION_SINGULARITY_LIMIT)
    {
        angles.yaw   = -2 * atan2(qx, qw);
        angles.pitch = -M_PI / 2;
        angles.roll  = 0;
    }
    else
    {
        angles.yaw   = atan2(2 * qy * qw - 2 * qx * qz, 1 - 2 * qy * qy - 2 * qz *qz);
        angles.pitch = asin(2 * qx * qy + 2 * qz * qw);
        angles.roll  = atan2(2 * qx * qw -2 * qy * qz, 1 - 2 * qx * qx - 2 * qz * qz);
    }
    return angles;
}

/*
 * Simple convenience class for assembling serial packets sent as several parts.
 * Returns a pointer to a complete assembled payload if possible.
 * Incomplete messages will return a nullptr.
 */
class MessageConcatenator
{
public:
    MessageConcatenator() : _waiting{false} {}
    ~MessageConcatenator() {}
    const char* add(const sSenseiDataPacket *packet)
    {
        if (!_waiting && !packet->continuation)
        {
            return packet->payload;
        }
        if (packet->continuation)
        {
            memcpy(_storage, packet->payload, SENSEI_PAYLOAD_LENGTH);
            _waiting = true;
            return nullptr;
        }
        if (_waiting && !packet->continuation)
        {
            memcpy(_storage + SENSEI_PAYLOAD_LENGTH, packet->payload, SENSEI_PAYLOAD_LENGTH);
            _waiting = false;
            return _storage;
        }
        return nullptr;
    }

private:
    bool _waiting;
    char _storage[SENSEI_PAYLOAD_LENGTH * 2];
};

// set 1 byte boundary for matching
#pragma pack(push, 1)

struct teensy_value_msg
{
    uint16_t pin_id;
    uint16_t value;
    uint8_t pin_type;
};

struct teensy_set_value_cmd
{
    uint16_t    pin_idx;
    uint8_t     value;
};

struct teensy_set_samplerate_cmd
{
    uint8_t     sample_rate_divisor;
};

#pragma pack(pop)


}; // end namespace sensei
}; // end namespace serial_frontend

#endif //SERIAL_FRONTEND_INTERNAL_H
