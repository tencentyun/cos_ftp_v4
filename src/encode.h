#ifndef ENCODE_H_
#define ENCODE_H_

#ifdef __cplusplus
extern "C"
{
#endif
	bool IsGBKChin(unsigned char *pszStr);

	bool IsUtf8(unsigned char *pszStr) ;

	int CodeConvert(const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen, char *outbuf, size_t outlen);
#ifdef __cplusplus
}
#endif

#endif
