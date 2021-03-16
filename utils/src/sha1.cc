#include "sha1.h"
#include <cstdlib>
#include <cryptopp/cryptlib.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

char hex_num(char a) {
    if (a <= 0x39) {
        return a - '0';
    } else {
        return a - 'A' + 10;
    }
}

void sha1(const char* in, size_t in_size, char* out, size_t out_size) {
    CryptoPP::HexEncoder encoder(
        new CryptoPP::ArraySink((CryptoPP::byte*)out, out_size));

    std::string msg{ in };
    std::string digest;

    CryptoPP::SHA1 hash;
    hash.Update((const CryptoPP::byte*)msg.data(), msg.size());
    digest.resize(hash.DigestSize());
    hash.Final((CryptoPP::byte*)&digest[0]);
    CryptoPP::StringSource(digest, true, new CryptoPP::Redirector(encoder));
    size_t end = strlen(out);
    for (int i = 0, j = 0; i < end; i += 2, ++j) {
        out[j] = hex_num(out[i]) * 16 + hex_num(out[i + 1]);
    }
    out[end / 2] = 0;
}
