#ifndef _CODEC_H_
#define _CODEC_H_
#include <Log.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

typedef bool Void;

template <typename T>
class Option {
   private:
    bool _none = false;
    T _value;

   public:
    static Option<T> None() {
        Option<T> option(true);
        option._none = true;
        return option;
    }
    static Option<T> Some(T value) {
        Option<T> option(false);
        option._value = value;
        return option;
    }
    Option() : _none(true) {}
    Option(bool none) : _none(none) {}
    bool is_some() { return !_none; }
    bool is_none() { return _none; }
    T unwrap() { return _value; }
    void inspect(std::function<void(T)> ff) {
        if (_none == false) {
            ff(_value);
        }
    }
    template <typename U>
    Option<U> map(std::function<Option<U>(T)> ff) {
        if (_none == false) {
            return ff(_value);
        } else {
            return Option<U>::None();
        }
    }
};

template <typename T>
class Result {
    T _value;
    int _err;
    const char* _msg;

   public:
    Result() : _value(), _err(0) {}
    bool is_ok() { return _err == 0; }
    bool is_err() { return _err != 0; }
    const char* get_err_msg() { return (_err == 0) ? "No error" : _msg; }
    Result& on_error(std::function<void(const char*)> ff) {
        if (_err != 0) {
            ff(_msg);
        }
        return *this;
    }
    Result& on_ok(std::function<void(T)> ff) {
        if (_err == 0) {
            ff(_value);
        } else {
            ERROR("Error: %d\n", _err);
        }
        return *this;
    }
    T unwrap() {
        if (_err != 0) {
            ERROR("Error: %d\n", _err);
        }
        return _value;
    }
    static Result<T> Ok(T value) {
        Result<T> result;
        result._value = value;
        result._err = 0;
        return result;
    }
    static Result<T> Err(int err) {
        Result<T> result;
        result._err = err;
        result._msg = strerror(err);
        return result;
    }
    static Result<T> Err(int err, const char* msg) {
        Result<T> result;
        result._err = err;
        result._msg = msg;
        return result;
    }
    void inspect(std::function<void(T)> ff) {
        if (_err == 0) {
            ff(_value);
        }
    }
    template <typename U>
    Result<U> map(std::function<Result<U>(T)> ff) {
        if (_none == false) {
            return ff(_value);
        }
        return Result<U>::Err(this->_err, this->_msg);
    }
};

#define RET_ERR(x)      \
    if ((x).is_err()) { \
        return x;       \
    }
#define RET_ER(x)       \
    if ((x).is_err()) { \
        return;         \
    }

class FrameEncoder {
   private:
    std::vector<uint8_t> _buffer;
    uint32_t _max;
    Result<Void> write_byte(uint8_t byte);

   public:
    FrameEncoder(uint32_t max);
    Result<Void> begin_array();
    Result<Void> begin_map();
    Result<Void> end_array();
    Result<Void> end_map();
    Result<Void> add_map(int8_t key, int32_t value);
    Result<Void> encode_uint32(uint32_t input_value);
    Result<Void> encode_str(const char* str);
    Result<Void> encode_bstr(std::vector<uint8_t>& buffer);
    Result<Void> encode_float(float value);
    Result<Void> encode_int32(int32_t value);
    Result<Void> encode_bool(bool b);
    Result<Void> encode_null();
    Result<Void> add_crc();
    Result<Void> add_cobs();
    Result<Void> read_buffer(uint8_t* buffer, size_t len);
    Result<Void> read_buffer(std::vector<unsigned char>& buffer);
    Result<Void> clear();
    Result<Void> rewind();
    Result<std::string> to_string();
    uint8_t* data() { return _buffer.data(); }
    uint32_t size() { return _buffer.size(); }
};

enum CborType {
    CBOR_UINT32 = 0,
    CBOR_STR = 1,
    CBOR_FLOAT = 2,
    CBOR_DOUBLE = 3,
    CBOR_INT32 = 4,
    CBOR_BSTR = 5,
};

class FrameDecoder {
   private:
    std::vector<uint8_t> _buffer;
    uint32_t _index;
    uint32_t _max;

   public:
    FrameDecoder(uint32_t max);
    Result<uint8_t> read_next();
    Result<uint8_t> peek_next();
    Result<CborType> peek_type();
    Result<Void> begin_array();
    Result<Void> begin_map();
    Result<Void> end_map();
    Result<Void> end_array();
    Result<uint32_t> decode_uint32();
    Result<std::string> decode_str();
    Result<float> decode_float();
    Result<int32_t> decode_int32();
    Result<std::vector<uint8_t>> decode_bstr();
    Result<bool> check_crc();
    Result<Void> decode_cobs();
    Result<bool> add_byte(uint8_t byte);
    Result<bool> fill_buffer(std::vector<uint8_t> buffer);
    Result<bool> fill_buffer(uint8_t* buffer, uint32_t size);
    Result<Void> read_buffer(uint8_t* buffer, size_t len);
    Result<Void> read_buffer(std::vector<unsigned char>& buffer);
    Result<Void> clear();
    Result<Void> rewind();
    Result<std::string> to_string();
};

// FNV-1a hash function for 32-bit hash value
constexpr uint32_t fnv1a_32_1(const char* str, uint32_t hash = 2166136261U) {
    return *str == '\0' ? hash : fnv1a_32_1(str + 1, (hash ^ static_cast<uint32_t>(*str)) * 16777619U);
}

// Helper to compute the hash at compile time for a string literal
template <std::size_t N>
constexpr uint32_t FNV(const char (&str)[N]) {
    return fnv1a_32_1(str);
}
#endif