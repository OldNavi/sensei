/**
 * @brief Main class for remapping sensors data into output format
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#include <memory>
#include <cassert>

#include "mapping_processor.h"
#include "logging.h"

using namespace sensei;
using namespace sensei::mapping;

SENSEI_GET_LOGGER;

MappingProcessor::MappingProcessor(const int max_n_input_pins) :
    _max_n_pins(max_n_input_pins)
{
    _mappers.resize(_max_n_pins);
    std::fill(_mappers.begin(), _mappers.end(), nullptr);
}

CommandErrorCode MappingProcessor::apply_command(const Command *cmd)
{
    // TODO: find a better way to manage changes of PinType,
    //       possibly without requiring SetPinType to be the 1st command received

    int sensor_index = cmd->index();
    if ( (sensor_index < 0) || (sensor_index > (_max_n_pins-1)) )
    {
        return CommandErrorCode::INVALID_PIN_INDEX;
    }

    if (cmd->type() == CommandType::SET_PIN_TYPE)
    {
        CommandErrorCode status = CommandErrorCode::OK;
        const auto typed_cmd = static_cast<const SetPinTypeCommand*>(cmd);
        auto pin_type = typed_cmd->data();
        SENSEI_LOG_INFO("Got a set pin type command, pintype {}", (int)pin_type);
        switch(pin_type)
        {
        case PinType::DIGITAL_INPUT:
            _mappers[sensor_index].reset(new DigitalSensorMapper(sensor_index));
            break;

        case PinType::ANALOG_INPUT:
            _mappers[sensor_index].reset(new AnalogSensorMapper(sensor_index));
            break;

        case PinType::IMU_INPUT:
            _mappers[sensor_index].reset(new ImuMapper(sensor_index));
            break;

        default:
            status = CommandErrorCode::INVALID_VALUE;

        }
        return status;
    }
    else
    {
        // Apply command only to already initialized pins
        if (_mappers[sensor_index] == nullptr)
        {
            return CommandErrorCode::UNINITIALIZED_PIN;
        }
        return _mappers[sensor_index]->apply_command(cmd);
    }

}

void MappingProcessor::put_config_commands_into(CommandIterator out_iterator)
{
    for (auto& mapper : _mappers)
    {
        if (mapper != nullptr)
        {
            mapper->put_config_commands_into(out_iterator);
        }
    }
}

void MappingProcessor::process(Value *value, output_backend::OutputBackend *backend)
{
    int sensor_index = value->index();
    SENSEI_LOG_INFO("Mappingprocessor: processing a value for pin {}", sensor_index);
    if (_mappers[sensor_index] != nullptr)
    {
        _mappers[sensor_index]->process(value, backend);
    }
    else
    {
        SENSEI_LOG_ERROR("Got value message for initialized pin {}", value->index());
    }
}

