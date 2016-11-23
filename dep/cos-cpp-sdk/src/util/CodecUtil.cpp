#include "util/CodecUtil.h"
#include "util/FileUtil.h"
#include "util/Sha1.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string.h>
#include <string>
#include <algorithm>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>

using std::string;
namespace qcloud_cos {

unsigned char CodecUtil::ToHex(const unsigned char &x) {
    return x > 9 ? (x - 10 + 'A') : x + '0';
}

string CodecUtil::UrlEncode(const string &str) {
    string encodedUrl = "";
    std::size_t length = str.length();
    for (size_t i = 0; i < length; ++i) {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~') ||
            (str[i] == '/')) {

            encodedUrl += str[i];
        } else {
            encodedUrl += '%';
            encodedUrl += ToHex((unsigned char)str[i] >> 4);
            encodedUrl += ToHex((unsigned char)str[i] % 16);
        }
    }
    return encodedUrl;
}

string CodecUtil::Base64Encode(const string &plainText) {
    static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const std::size_t plainTextLen = plainText.size();
    string retval((((plainTextLen + 2) / 3) * 4), '=');
    std::size_t outpos = 0;
    int bits_collected = 0;
    unsigned int accumulator = 0;
    const string::const_iterator plainTextEnd = plainText.end();

    for (string::const_iterator i = plainText.begin(); i != plainTextEnd; ++i) {
        accumulator = (accumulator << 8) | (*i & 0xffu);
        bits_collected += 8;
        while (bits_collected >= 6) {
            bits_collected -= 6;
            retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
        }
    }

    if (bits_collected > 0) {
        assert(bits_collected < 6);
        accumulator <<= 6 - bits_collected;
        retval[outpos++] = b64_table[accumulator & 0x3fu];
    }
    assert(outpos >= (retval.size() - 2));
    assert(outpos <= retval.size());
    return retval;
}

string CodecUtil::HmacSha1(const string &plainText, const string &key) {
    const EVP_MD *engine = EVP_sha1();
    unsigned char *output = (unsigned char *)malloc(EVP_MAX_MD_SIZE);
    unsigned int output_len = 0;
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, (char *)key.c_str(), key.length(), engine, NULL);
    HMAC_Update(&ctx, (unsigned char*)plainText.c_str(), plainText.length());
    HMAC_Final(&ctx, output, &output_len);
    HMAC_CTX_cleanup(&ctx);
    string hmac_sha1_ret((char *)output, output_len);
    free(output);
    return hmac_sha1_ret;
}

string CodecUtil::GetFileSha1(const string &localFilePath) {
    unsigned char buf[8192];
    SHA_CTX sc;
    SHA1_Init(&sc);
    size_t len;
    std::ifstream fileInput(localFilePath.c_str(), std::ios::in | std::ios::binary);
    while(!fileInput.eof()) {
        fileInput.read((char *)buf, sizeof(buf));
        len = fileInput.gcount();
        SHA1_Update(&sc, buf, len);
    }
    fileInput.close();

    unsigned char digestBuf[SHA_DIGEST_LENGTH];
    SHA1_Final(digestBuf, &sc);

    string digestStr = "";
    unsigned char temp_hex;
    for(int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        temp_hex = ToHex(digestBuf[i] / 16);
        digestStr.append(1, temp_hex);
        temp_hex = ToHex(digestBuf[i] % 16);
        digestStr.append(1, temp_hex);
    }

    std::transform(digestStr.begin(),digestStr.end(),digestStr.begin(),::tolower);
    return digestStr;
}

int CodecUtil::conv_file_to_upload_parts(const string &filepath, uint64_t slice_size, string &upload_parts, string& sha)
{
    Json::FastWriter writer;
    Json::Value data(Json::objectValue), parts(Json::arrayValue);
    Sha1 sha1;

    uint64_t fileSize = FileUtil::getFileLen(filepath);
    uint64_t offset = 0;
    unsigned char *pbuf = new unsigned char[slice_size + 1];
    memset(pbuf, 0, slice_size+1);
    ifstream fin(filepath.c_str(), std::ios::in | std::ios::binary );
    uint64_t readLen = 0;
    uint64_t totalLen = 0;
    while (offset < fileSize) {
        fin.read((char*)pbuf, slice_size);
        readLen = fin.gcount();
        totalLen += readLen;
        Json::Value part(Json::objectValue);
        if((readLen < slice_size) || (readLen == slice_size && totalLen == fileSize)) {
            sha1.append((char*)pbuf, readLen);
            string final_sha = sha1.final();
            sha = final_sha;
            part["datasha"] = final_sha;
        } else {
            sha1.append((char*)pbuf, readLen);
            part["datasha"] = sha1.hexdigest();
        }

        part["offset"] = (Json::Value::UInt64)offset;
        part["datalen"] = (Json::Value::UInt64)readLen;
        parts.append(part);

        offset += readLen;
    }

    upload_parts = writer.write(parts);

    delete [] pbuf;
    fin.close();
    return 0;
}

string CodecUtil::GetFileSha1(const char* buffer, size_t buff_len) {
    SHA_CTX sc;
    SHA1_Init(&sc);
    SHA1_Update(&sc, buffer, buff_len);

    unsigned char digestBuf[SHA_DIGEST_LENGTH];
    SHA1_Final(digestBuf, &sc);

    string digestStr;
    unsigned char temp_hex;
    for(int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        temp_hex = ToHex(digestBuf[i] / 16);
        digestStr.append(1, temp_hex);
        temp_hex = ToHex(digestBuf[i] % 16);
        digestStr.append(1, temp_hex);
    }

    std::transform(digestStr.begin(),digestStr.end(),digestStr.begin(),::tolower);
    return digestStr;
}
}
