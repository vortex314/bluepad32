
#ifndef MSG_H
#define MSG_H
#include <codec.h>

typedef int8_t PropertyId;

typedef enum MetaPropertyId : PropertyId {
    RET_CODE = -2,
} PredefinedPropertyId;

typedef enum InfoPropertyId : PropertyId {
    PROP_ID = 0,
    NAME ,
    DESCRIPTION ,
    TYPE ,
    MODE ,
} InfoPropertyId;

typedef enum MsgType {
    Pub = 1,   // from = PubSUb, to = Set
    Info = 2,  // contain name, description, type, etc
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
    uint32_t msg_type;
    Option<uint32_t> msg_id;

   public:
    Result<Void> encode(FrameEncoder& encoder) {
        if (dst.is_some()) {
            RET_ERR(encoder.encode_uint32(dst.unwrap()));
        } else {
            RET_ERR(encoder.encode_null());
        }
        if (src.is_some()) {
            RET_ERR(encoder.encode_uint32(src.unwrap()));
        } else {
            RET_ERR(encoder.encode_null());
        }
        RET_ERR(encoder.encode_uint32(msg_type));
        if (msg_id.is_some()) {
            RET_ERR(encoder.encode_uint32(msg_id.unwrap()));
        } else {
            RET_ERR(encoder.encode_null());
        }
        return Result<Void>::Ok(Void());
    }
};
#endif