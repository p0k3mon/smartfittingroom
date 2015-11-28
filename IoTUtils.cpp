#include "IotUtils.h"
#include "hmac.h"
#include "sha256.h"

#include <arduino.h>

std::string IotUtils::IntToString(int num)
{
    // Format an integer into a string.
    
    return std::string(String(num).c_str());
}

std::string IotUtils::HexStringToBinary(std::string input)
{
    // Convert a hex string into an array of bytes stored in std::string.
    
    std::string out;
  
    while(input.length() > 0)
    {
        std::string tmp(input.c_str(),2);
        input.erase(0, 2);
        
        if (tmp.length() != 2) continue;

        unsigned char byte = 0;
        unsigned char nibble = tmp[0];
        
        if (nibble >= '0' && nibble <= '9')
        {
            byte = (nibble - '0') & 0x0F;
        }
        else if (nibble >= 'A' && nibble <= 'F')
        {
            byte = (nibble - 'A' + 10) & 0x0F;
        }
        else if (nibble >= 'a' && nibble <= 'f')
        {
            byte = (nibble - 'a' + 10) & 0x0F;
        }
        
        byte = byte << 4;
        
        nibble = tmp[1];
        
        if (nibble >= '0' && nibble <= '9')
        {
            byte |= (nibble - '0') & 0x0F;
        }
        else if (nibble >= 'A' && nibble <= 'F')
        {
            byte |= (nibble - 'A' + 10) & 0x0F;
        }
        else if (nibble >= 'a' && nibble <= 'f')
        {
            byte |= (nibble - 'a' + 10) & 0x0F;
        }
        
        out += byte;
    }
    
    return out;
}

std::string IotUtils::Base64Encode(std::string input)
{
    const std::string table_encode("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

    std::string output;

    while (input.length() > 0)
    {
        // Process 3 bytes at a time.
        std::string input_tmp(input.substr(0, 3));

        // Erase the number of bytes processed.
        input.erase(0, input_tmp.length());

        unsigned int num = 0;

        for (unsigned int ix = 0; ix < 3; ix++)
        {
            num = num * 256;
            if (ix < input_tmp.length()) num += input_tmp[ix] & 0xff;
        }

        // "num" is a 24 bit variable.

        // 4 characters are used to represent "num".
        // 6 bits for each character.

        // Since input_tmp is at least 1 byte in length.
        // This means at least 2 characters are needed.

        output += table_encode[((num >> 18) & 0x3f)];
        output += table_encode[((num >> 12) & 0x3f)];

        if (input_tmp.length() == 3)
        {
            output += table_encode[((num >> 6) & 0x3f)];
            output += table_encode[((num)& 0x3f)];
        }
        else if (input_tmp.length() == 2)
        {
            output += table_encode[((num >> 6) & 0x3f)];
            output += "=";
        }
        else if (input_tmp.length() == 1)
        {
            output += "==";
        }
    }

    return output;
}

std::string IotUtils::UrlEncode(std::string input)
{
    // Encode a string with URL encode format.
    std::string result;

    for (unsigned int ix = 0; ix < input.length(); ix++)
    {
        if (isalnum(input[ix]) != 0 || input[ix] == '-' || input[ix] == '_' || input[ix] == '.' || input[ix] == '~')
        {
            result += input[ix];
        }
        else
        {
            // Note that the %% is to print a % character.
            char buffer[1024];
            sprintf(buffer,"%%%02x",input[ix]);
            
            result += buffer;
        }
    }

    return result;
}
