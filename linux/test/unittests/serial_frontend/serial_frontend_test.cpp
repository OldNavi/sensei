#include "gtest/gtest.h"
#define private public
#include "hardware_frontend/serial_frontend.cpp"

using namespace sensei;
using namespace hw_frontend;

static uint8_t test_msg[] = { 0x1, 0x2, 0x3, 0xff, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x8c, 0x3, 0x0, 0x0, 0x64, 0x1, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xe8,
                              0xe2, 0xf6, 0x10, 0xc3, 0x4, 0x4, 0x5, 0x6 };

static uint8_t imu_test[] = { 0x1, 0x2, 0x3, 0xfd, 0x02, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xe8,
                              0xe2, 0xf6, 0x10, 0xc3, 0x4, 0x4, 0x5, 0x6 };
// Test standalone functions

TEST (TestHelperFunctions, test_verify_message)
{
    EXPECT_TRUE(verify_message(reinterpret_cast<sensei::sSenseiDataPacket*>(test_msg)));
}

class SerialFrontendTest : public ::testing::Test
{
protected:
    SerialFrontendTest() :
            _module_under_test("/dev/ttyS011", &_in_queue, &_out_queue)
    {
    }
    void SetUp()
    {
    }

    void TearDown()
    {
    }
    SynchronizedQueue<std::unique_ptr<BaseMessage>>  _out_queue;
    SynchronizedQueue<std::unique_ptr<Command>>      _in_queue;
    SerialFrontend _module_under_test;
};

/*
 * Verify that serial packets are correctly created from Command messages
 */
TEST_F(SerialFrontendTest, test_create_serial_message)
{
    MessageFactory factory;
    auto command = factory.make_set_sending_delta_ticks_command(3, 5, 100u);
    const sSenseiDataPacket* packet = _module_under_test.handle_command(static_cast<Command*>(command.get()));
    EXPECT_EQ(SENSEI_CMD::CONFIGURE_PIN, packet->cmd);
    auto payload = reinterpret_cast<const sPinConfiguration*>(packet->payload);
    EXPECT_EQ(5, payload->deltaTicksContinuousMode);

    auto lp_command = factory.make_set_lowpass_cutoff_command(4, 1234.0, 100u);
    packet = _module_under_test.handle_command(static_cast<Command*>(lp_command.get()));
    EXPECT_EQ(SENSEI_CMD::CONFIGURE_PIN, packet->cmd);
    auto payload_cfg = reinterpret_cast<const sPinConfiguration*>(packet->payload);
    EXPECT_FLOAT_EQ(1234, payload_cfg->lowPassCutOffFilter);
}

/*
 * Test that an internal message is properly created from a teensy packet
 */
TEST_F(SerialFrontendTest, test_process_serial_packet)
{
    sSenseiDataPacket packet;
    teensy_value_msg* payload = reinterpret_cast<teensy_value_msg*>(packet.payload);
    packet.cmd = SENSEI_CMD::VALUE;
    packet.sub_cmd = 0;
    packet.timestamp = 1234;
    payload->pin_id = 12;
    payload->pin_type = PIN_ANALOG_INPUT;
    payload->value = 35;
    // Prep the pin to id table to unsure proper mapping
    _module_under_test._pin_to_id_table[12] = 10;
    _module_under_test.process_serial_packet(&packet);
    // The message is put in the queue so assert that it exists and retrieve it
    ASSERT_FALSE(_out_queue.empty());
    std::unique_ptr<BaseMessage> msg = _out_queue.pop();
    AnalogValue* valuemessage = static_cast<AnalogValue*>(msg.get());

    ASSERT_EQ(MessageType::VALUE, valuemessage->base_type());
    EXPECT_EQ(1234u, valuemessage->timestamp());
    EXPECT_EQ(10, valuemessage->index());
    EXPECT_EQ(35, valuemessage->value());
}

TEST_F(SerialFrontendTest, test_mute_function)
{
    EXPECT_FALSE(_module_under_test._muted);
    _module_under_test.mute(true);
    EXPECT_TRUE(_module_under_test._muted);
}

TEST_F(SerialFrontendTest, test_imu_packet)
{
    /* First test without setting up any virtual ports
     * This should not result in any messages */
    sSenseiDataPacket* packet = reinterpret_cast<sSenseiDataPacket*>(imu_test);
    _module_under_test.process_serial_packet(packet);
    ASSERT_TRUE(_out_queue.empty());

    /* Set up the virtual ports table, would be done from a config file otherwise */
    _module_under_test._imu_sensor_index[ImuIndex::YAW] = 0;
    _module_under_test._imu_sensor_index[ImuIndex::PITCH] = 1;
    _module_under_test._imu_sensor_index[ImuIndex::ROLL] = 2;

    _module_under_test.process_serial_packet(packet);
    /* This should result in 3 imu messages */
    ASSERT_FALSE(_out_queue.empty());
    auto msg = _out_queue.pop();
    auto typed_msg = static_cast<ContinuousValue*>(msg.get());
    EXPECT_EQ(ImuIndex::YAW, typed_msg->index());
    EXPECT_EQ(0, typed_msg->value());

    msg = _out_queue.pop();
    typed_msg = static_cast<ContinuousValue*>(msg.get());
    EXPECT_EQ(ImuIndex::PITCH, typed_msg->index());
    EXPECT_EQ(0, typed_msg->value());

    msg = _out_queue.pop();
    typed_msg = static_cast<ContinuousValue*>(msg.get());
    EXPECT_EQ(ImuIndex::ROLL, typed_msg->index());
    EXPECT_EQ(0, typed_msg->value());
}