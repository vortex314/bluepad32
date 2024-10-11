#include <codec.h>
#include <stdint.h>
typedef enum Ps4 : int8_t {
    DPAD,

    BUTTON_SQUARE,
    BUTTON_CROSS,
    BUTTON_CIRCLE,
    BUTTON_TRIANGLE,

    BUTTON_LEFT_SHOULDER,
    BUTTON_RIGHT_SHOULDER,

    BUTTON_LEFT_TRIGGER,
    BUTTON_RIGHT_TRIGGER,

    BUTTON_LEFT_JOYSTICK,
    BUTTON_RIGHT_JOYSTICK,

    BUTTON_SHARE,
    STICK_LEFT_X,
    STICK_LEFT_Y,
    STICK_RIGHT_X,
    STICK_RIGHT_Y,

    GYRO_X,
    GYRO_Y,
    GYRO_Z,

    ACCEL_X,
    ACCEL_Y,
    ACCEL_Z,

    RUMBLE,
    LIGHTBAR_RGB,

    CONNECTED,

} Ps4;

typedef enum Ps4Event { Connected = 0, Disconnected, Data, OOB } Ps4Event;

class Ps4Map {
   public:
    Option<uint8_t> dpad;
    Option<int8_t> button_square;
    Option<int8_t> button_cross;
    Option<int8_t> button_circle;
    Option<int8_t> button_triangle;
    Option<int8_t> button_left_shoulder;
    Option<int8_t> button_right_shoulder;
    Option<int8_t> button_left_trigger;
    Option<int8_t> button_right_trigger;
    Option<int8_t> button_left_joystick;
    Option<int8_t> button_right_joystick;
    Option<int8_t> button_share;
    Option<int32_t> stick_left_x;
    Option<int32_t> stick_left_y;
    Option<int32_t> stick_right_x;
    Option<int32_t> stick_right_y;
    Option<int32_t> gyro_x;
    Option<int32_t> gyro_y;
    Option<int32_t> gyro_z;
    Option<int32_t> accel_x;
    Option<int32_t> accel_y;
    Option<int32_t> accel_z;
    Option<uint8_t> rumble;
    Option<uint32_t> lightbar_rgb;
    Option<bool> connected;

    Result<void> decode(FrameDecoder decoder) {
        RET_ERR(decoder.begin_map());
        while (true) {
            if (decoder.peek_next().unwrap() == 0xFF) {
                break;
            }
            uint32_t key = 0;
            decoder.decode_uint32().inspect([&](uint32_t value) { key = value; });
            switch (key) {
                case Ps4::DPAD: {
                    decoder.decode_uint8().inspect([&](uint8_t value) { dpad = Option<uint8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_SQUARE: {
                    decoder.decode_int8().inspect([&](int8_t value) { button_square = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_CROSS: {
                    decoder.decode_int8().inspect([&](int8_t value) { button_cross = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_CIRCLE: {
                    decoder.decode_int8().inspect([&](int8_t value) { button_circle = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_TRIANGLE: {
                    decoder.decode_int8().inspect([&](int8_t value) { button_triangle = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_LEFT_SHOULDER: {
                    decoder.decode_int8().inspect(
                        [&](int8_t value) { button_left_shoulder = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_RIGHT_SHOULDER: {
                    decoder.decode_int8().inspect(
                        [&](int8_t value) { button_right_shoulder = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_LEFT_TRIGGER: {
                    decoder.decode_int8().inspect(
                        [&](int8_t value) { button_left_trigger = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_RIGHT_TRIGGER: {
                    decoder.decode_int8().inspect(
                        [&](int8_t value) { button_right_trigger = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_LEFT_JOYSTICK: {
                    decoder.decode_int8().inspect(
                        [&](int8_t value) { button_left_joystick = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_RIGHT_JOYSTICK: {
                    decoder.decode_int8().inspect(
                        [&](int8_t value) { button_right_joystick = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::BUTTON_SHARE: {
                    decoder.decode_int8().inspect([&](int8_t value) { button_share = Option<int8_t>::Some(value); });
                    break;
                }
                case Ps4::STICK_LEFT_X: {
                    decoder.decode_int32().inspect([&](int32_t value) { stick_left_x = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::STICK_LEFT_Y: {
                    decoder.decode_int32().inspect([&](int32_t value) { stick_left_y = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::STICK_RIGHT_X: {
                    decoder.decode_int32().inspect(
                        [&](int32_t value) { stick_right_x = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::STICK_RIGHT_Y: {
                    decoder.decode_int32().inspect(
                        [&](int32_t value) { stick_right_y = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::GYRO_X: {
                    decoder.decode_int32().inspect([&](int32_t value) { gyro_x = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::GYRO_Y: {
                    decoder.decode_int32().inspect([&](int32_t value) { gyro_y = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::GYRO_Z: {
                    decoder.decode_int32().inspect([&](int32_t value) { gyro_z = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::ACCEL_X: {
                    decoder.decode_int32().inspect([&](int32_t value) { accel_x = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::ACCEL_Y: {
                    decoder.decode_int32().inspect([&](int32_t value) { accel_y = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::ACCEL_Z: {
                    decoder.decode_int32().inspect([&](int32_t value) { accel_z = Option<int32_t>::Some(value); });
                    break;
                }
                case Ps4::RUMBLE: {
                    decoder.decode_uint8().inspect([&](uint8_t value) { rumble = Option<uint8_t>::Some(value); });
                    break;
                }
                case Ps4::LIGHTBAR_RGB {
                    decoder.decode_uint32().inspect([&](uint32_t value) {
                        lightbar_rgb = Option<uint32_t>::Some(value);
                    });
                    break;
                } case Ps4::CONNECTED: {
                    decoder.decode_bool().inspect([&](bool value) { connected = Option<bool>::Some(value); });
                    break;
                }
                default: {
                    // Handle unknown key
                    break;
                }
            }
        }
        RET_ERR(decoder.end_map());
        return Result<void>::Ok(Void());
    }
};