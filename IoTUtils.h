#ifndef IOTUTILS_H
#define IOTUTILS_H

#include <string>

class IotUtils
{
public:
    static std::string HexStringToBinary(std::string);
    static std::string Base64Encode(std::string);
    static std::string IntToString(int);
    static std::string UrlEncode(std::string);
};

#endif
