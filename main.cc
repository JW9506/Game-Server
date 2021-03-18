#include <stdio.h>
#include <string>
#include <vector>
#include <cryptopp/base64.h>
#include <cryptopp/sha1_armv4.h>
#include <cryptopp/hex.h>
#include "base64_encoder.h"
#include "sha1.h"
#include "mysql.h"
#include "mysql_wrapper.h"
#include "hiredis/hiredis.h"
#include "redis_wrapper.h"
#include "time_list.h"
#include <uv.h>
#include "lua_wrapper.h"
using namespace CryptoPP;
using namespace std;

static void res(const char* err, MYSQL_RES* result) {
    if (err) {
        printf("%s\n", err);
        return;
    }
    for (auto&& row : result) {
        for (auto&& item : row) {
            std::cout << item << ' ';
        }
        std::cout << std::endl;
    }
}
static void task01(void* context) {
    mysql_wrapper::close(context);
}
static void open_cb(const char* err, void* context, void* udata) {
    if (err) {
        printf("%s\n", err);
        return;
    }
    mysql_wrapper::query(context, "select * from foo", res);
    mysql_wrapper::close(context);
    // schedule_once(task01, context, 0);
}

static void redis_cb02(const char* err, redisReply* result, void* udata) {
    if (err) {
        printf("%s\n", err);
        return;
    }
    printf("%s\n", result->str);
}

static void redis_cb(const char* err, void* context, void* udata) {
    redis_wrapper::query(context, "ping", redis_cb02);
    redis_wrapper::close(context);
}

int main(int argc, char** argv) {
// Base64Encoder
// cryptogams_sha1_block_data_order
// CryptoPP::Base64Encoder();
// CryptoPP::BufferedTransformation btf;

// byte decoded[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
//                    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 };
#ifdef a
    byte decoded[] = "123";
    // MTIz
    std::string encoded;

    Base64Encoder encoder;
    encoder.Put(decoded, sizeof(decoded));
    encoder.MessageEnd();

    word64 size = encoder.MaxRetrievable();
    if (size) {
        encoded.resize(size);
        encoder.Get((byte*)&encoded[0], encoded.size());
    }

    std::cout << encoded << std::endl;
#endif

#ifdef a
    using CryptoPP::Name::InsertLineBreaks;
    using CryptoPP::Name::Pad;

    CryptoPP::byte raw[] = "abc";

    std::string encoded, hexed;
    CryptoPP::Base64Encoder encoder;

    CryptoPP::AlgorithmParameters params =
        CryptoPP::MakeParameters(Pad(), false)(InsertLineBreaks(), false);
    encoder.IsolatedInitialize(params);
    encoder.Attach(new StringSink(encoded));

    CryptoPP::ArraySource as(raw, sizeof(raw), true, new Redirector(encoder));
    encoded.pop_back();
    encoded.pop_back();
    std::cout << encoded << std::endl;

    CryptoPP::StringSource ss(
        encoded, true,
        new Base64Decoder(new HexEncoder(new StringSink(hexed))));
    std::cout << hexed << std::endl;
#endif
    // using byte = CryptoPP::byte;
    // Base64Encoder encoder;
    // const CryptoPP::byte ALPHABET[] =
    //     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    // AlgorithmParameters params = MakeParameters(
    //     Name::EncodingLookupArray(), (const CryptoPP::byte*)ALPHABET);
    // encoder.IsolatedInitialize(params);

    // // Decoder
    // int lookup[256];
    // const CryptoPP::byte ALPHABET[] =
    //     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    // Base64Decoder::InitializeDecodingLookupArray(lookup, ALPHABET, 64, false);

    // Base64Decoder decoder;
    // AlgorithmParameters params =
    //     MakeParameters(Name::DecodingLookupArray(), (const int*)lookup);
    // decoder.IsolatedInitialize(params);

#ifdef a
    char raw[] = "Yoda said"; // LSsJbPn8hjD8gusSWzIRhF/f0RM=
    // unsigned char sha_out[512] = {0};
    char sha_out[512] = {0};
    sha1(raw, sizeof(raw), sha_out, sizeof(sha_out));
    char out[512] = {0};
    size_t out_size;
    base64_encode(sha_out, out, &out_size);
    printf("%s %lld\n", out, out_size);
#endif
#ifdef a
    mysql_wrapper::connect("127.0.0.1", 3306, "class_sql", "root", "root", open_cb);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
#endif
#ifdef a
    redis_wrapper::connect("127.0.0.1", 6379, redis_cb);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
#endif
    lua_wrapper::init();
    if (!lua_wrapper::exe_lua_file("./main.lua")) {
        std::cout << "lua exec err" << std::endl;
    }
    lua_wrapper::exit();
    return 0;
}
