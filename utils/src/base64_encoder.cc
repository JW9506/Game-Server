#include "base64_encoder.h"
#include <cstdlib>
#include <cryptopp/base64.h>

void base64_encode(const char* in, char* out, size_t* out_size) {
    using CryptoPP::Name::InsertLineBreaks;
    using CryptoPP::Name::Pad;

    size_t in_size = strlen(in);
    CryptoPP::byte* raw = (CryptoPP::byte*)in;
    std::string hexed;
    CryptoPP::Base64Encoder encoder;

    CryptoPP::AlgorithmParameters params =
        CryptoPP::MakeParameters(Pad(), true)(InsertLineBreaks(), false);
    encoder.IsolatedInitialize(params);
    encoder.Attach(new CryptoPP::StringSink(hexed));

    CryptoPP::ArraySource(raw, in_size, true,
                          new CryptoPP::Redirector(encoder));
    memcpy(out, hexed.c_str(), hexed.size() + 1);
    *out_size = hexed.size();
}
