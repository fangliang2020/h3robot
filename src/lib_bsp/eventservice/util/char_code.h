/*
Copyright(C) 1999 Aladdin Enterprises.All rights reserved.

This software is provided 'as-is', without any express or implied
warranty.In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

L.Peter Deutsch
ghost@aladdin.com

Independent implementation of MD5 (RFC 1321).

This code implements the MD5 Algorithm defined in RFC 1321.
It is derived directly from the text of the RFC and not from the
reference implementation.

The original and principal author of md5.h is L. Peter Deutsch
<ghost@aladdin.com>.  Other authors are noted in the change history
that follows (in reverse chronological order):

1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5);
added conditionalization for C++ compilation from Martin
Purschke <purschke@bnl.gov>.
1999-05-03 lpd Original version.
*/
#include <string>

namespace chml {

//检测编码是否符合UTF8规则，可以用于检测是否是UTF8字符串。纯英文字符串一定返回True。
//注意：GB2312/GBK字符串也有小概率符合UTF8编码规则，调此接口小概率返回True。
bool IsUtf8String(const std::string &str);
bool IsUtf8String(const char *str, int size);
std::string Gb2312ToUtf8(std::string src);
bool Gb2312ToUtf8(std::string &outStr, const std::string inputStr);
bool Utf8ToGb2312(std::string &outStr, const std::string inputStr);
void GBcode_Unicode(unsigned char *pout, unsigned char *pintput);
void Unicode_GBcode(unsigned char *pout, unsigned char *pintput);

std::string Gb2312ToUtf8(std::string src);
std::string Utf8ToGb2312(std::string src);

//GB2312 תΪ UTF-8
int RGN_Gb2312_To_UTF8(unsigned char *pOut, int pOutLen,
                       unsigned char *pText, int pLen);
//UTF-8 תΪ GB2312
int RGN_UTF8_To_Gb2312(unsigned char *pOut, int pOutLen,
                       unsigned char *pText, int pLen);

}
