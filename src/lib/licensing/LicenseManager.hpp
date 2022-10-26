#pragma once

#include <chrono>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <streambuf>
#include <string>
#include <vector>

#include "AES.hpp"

namespace dc
{

class LicenseManager
{
  public:
    unsigned char *GenerateMacAddress(unsigned char *UUID, bool keyType);
    std::string GenerateDeviceToken();
    bool RegisterLicense(std::string license);
    std::string GenerateLicenseKey(std::string token, int years, int months, int days); // THIS SHOULD BE PRIVATE
    bool LicenseCheck();

  private:
    std::string DecryptKey(std::string token);
};

} // namespace dc
