
#ifndef MSG_H
#define MSG_H
#include <codec.h>

typedef int8_t PropertyId;

typedef enum HeaderId { DST = 0, SRC, MSG_TYPE, RET_CODE, MSG_ID, QOS } HeaderId;

typedef enum InfoPropertyId : PropertyId {
    PROP_ID = 0,
    NAME,
    DESCRIPTION,
    TYPE,
    MODE,
} InfoPropertyId;

typedef enum MsgType {
    Alive = 0,  // keep alive
    Pub = 1,    // publish data if dst=None => broadcast
                // if dst=Some  => send to one as Set
    Sub = 2,    // Subscribe to data , send to src as Endpoint
    Info = 3,   // contain name, description, type, etc
} MsgType;

typedef enum ValueType {
    UINT = 0,
    INT = 1,
    STR = 2,
    BYTES = 3,
    FLOAT = 4,
} ValueType;

typedef enum ValueMode {
    READ = 0,
    WRITE = 1,
} ValueMode;

class MsgHeader {
   public:
    Option<uint32_t> dst;
    Option<uint32_t> src;
    MsgType msg_type;
    Option<uint32_t> ret_code;
    Option<uint16_t> msg_id;
    Option<uint8_t> qos;

   public:
    /*MsgHeader()
        : dst(Option<uint32_t>::None()),
          src(Option<uint32_t>::None()),
          msg_type(MsgType::Alive),
          ret_code(Option<uint32_t>::None()),
          msg_id(Option<uint16_t>::None()),
          qos(Option<uint8_t>::None()) {};*/

    Result<Void> encode(FrameEncoder& encoder) {
        RET_ERR(encoder.begin_map());
        dst.inspect([&](uint32_t value) {
            RET_ER(encoder.encode_uint32(HeaderId::DST));
            RET_ER(encoder.encode_uint32(value));
        });
        src.inspect([&](uint32_t value) {
            RET_ER(encoder.encode_uint32(HeaderId::SRC));
            RET_ER(encoder.encode_uint32(value));
        });
        RET_ERR(encoder.encode_uint32(HeaderId::MSG_TYPE));
        RET_ERR(encoder.encode_uint32(msg_type));
        ret_code.inspect([&](uint32_t value) {
            RET_ER(encoder.encode_uint32(HeaderId::RET_CODE));
            RET_ER(encoder.encode_uint32(value));
        });
        msg_id.inspect([&](uint16_t value) {
            RET_ER(encoder.encode_uint32(HeaderId::MSG_ID));
            RET_ER(encoder.encode_uint32(value));
        });
        qos.inspect([&](uint8_t value) {
            RET_ER(encoder.encode_uint32(HeaderId::QOS));
            RET_ER(encoder.encode_uint32(value));
        });
        RET_ERR(encoder.end_map());
        return Result<Void>::Ok(Void());
    }

    Result<Void> decode(FrameDecoder& decoder) {
        bool found_mandatory_msg_type = false;
        MsgHeader header{
            .dst = Option<uint32_t>::None(),
            .src = Option<uint32_t>::None(),
            .msg_type = MsgType::Alive,
            .ret_code = Option<uint32_t>::None(),
            .msg_id = Option<uint16_t>::None(),
            .qos = Option<uint8_t>::None(),
        } ;
        RET_ERR(decoder.begin_map());
        while (true)  {
            if ( decoder.peek_next().unwrap() == 0xFF) {
                break;
            }
            uint32_t key = 0;
            decoder.decode_uint32().inspect([&](uint32_t value) {
                key = value;
            });
            switch (key) {
                case HeaderId::DST: {
                    decoder.decode_uint32().inspect([&](uint32_t value) {
                        header.dst = Option<uint32_t>::Some(value);
                    });
                    break;
                }
                case HeaderId::SRC: {
                    decoder.decode_uint32().inspect([&](uint32_t value) {
                        header.src = Option<uint32_t>::Some(value);
                    });
                    break;
                }
                case HeaderId::MSG_TYPE: {
                    decoder.decode_uint32().inspect([&](uint32_t value) {
                        header.msg_type = (MsgType)value;
                        found_mandatory_msg_type = true;
                    });
                    break;
                }
                case HeaderId::RET_CODE: {
                    decoder.decode_uint32().inspect([&](uint32_t value) {
                        header.ret_code = Option<uint32_t>::Some(value);
                    });
                    break;
                }
                case HeaderId::MSG_ID: {
                    decoder.decode_uint32().inspect([&](uint32_t value) {
                        header.msg_id = Option<uint16_t>::Some(value);
                    });
                    break;
                }
                case HeaderId::QOS: {
                    decoder.decode_uint32().inspect([&](uint32_t value) {
                        header.qos = Option<uint8_t>::Some(value);
                    });
                    break;
                }
                default:
                    return Result<Void>::Err(-1, "Unknown key");
            }
        }
        RET_ERR(decoder.end_map());
        if (!found_mandatory_msg_type) {
            return Result<Void>::Err(-1, "Mandatory field MSG_TYPE not found");
        }
        return Result<Void>::Ok(Void());
    }
};

struct InfoMsg {
    public :
    PropertyId prop_id;
    std::string name;
    Option<std::string> description;
    Option<ValueType> type;
    Option<ValueMode> mode;

    Result<Void> encode(FrameEncoder& encoder) {
        RET_ERR(encoder.begin_map());
        RET_ERR(encoder.encode_int32(InfoPropertyId::PROP_ID));
        RET_ERR(encoder.encode_uint32(prop_id));
        RET_ERR(encoder.encode_int32(InfoPropertyId::NAME));
        RET_ERR(encoder.encode_str(name));
        description.inspect([&](std::string value) {
            RET_ER(encoder.encode_int32(InfoPropertyId::DESCRIPTION));
            RET_ER(encoder.encode_str(value));
        });
        type.inspect([&](ValueType value) {
            RET_ER(encoder.encode_int32(InfoPropertyId::TYPE));
            RET_ER(encoder.encode_uint32(value));
        });
        mode.inspect([&](ValueMode value) {
            RET_ER(encoder.encode_int32(InfoPropertyId::MODE));
            RET_ER(encoder.encode_uint32(value));
        });
        RET_ERR(encoder.end_map());
        return Result<Void>::Ok(Void());
    }
}
#endif