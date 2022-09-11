#include "LicenseManager.hpp"

namespace dc {

#define keylength 128

unsigned char key[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x08, 0x06, 0x07, 0x05, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

AES aes(AESKeyLength::AES_128);

/* aaaack but it's fast and const should make it shared text page. */
static const unsigned char pr2six[256] =
{
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 50, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

int Base64decode_len(const char *bufcoded)
{
    int nbytesdecoded;
    const unsigned char *bufin;
    int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);

    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}

int Base64decode(char *bufplain, const char *bufcoded)
{
    int nbytesdecoded;
    const unsigned char *bufin;
    unsigned char *bufout;
    int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);
    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (unsigned char *) bufplain;
    bufin = (const unsigned char *) bufcoded;

    while (nprbytes > 4) {
    *(bufout++) =
        (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    *(bufout++) =
        (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    *(bufout++) =
        (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    bufin += 4;
    nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1) {
    *(bufout++) =
        (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    }
    if (nprbytes > 2) {
    *(bufout++) =
        (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    }
    if (nprbytes > 3) {
    *(bufout++) =
        (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    }

    *(bufout++) = '\0';
    nbytesdecoded -= (4 - nprbytes) & 3;
    return nbytesdecoded;
}

static const char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int Base64encode_len(int len)
{
    return ((len + 2) / 3 * 4) + 1;
}

int Base64encode(char *encoded, const char *string, int len)
{
    int i;
    char *p;

    p = encoded;

    for (i = 0; i < len - 2; i += 3) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    *p++ = basis_64[((string[i] & 0x3) << 4) |
                    ((int) (string[i + 1] & 0xF0) >> 4)];
    *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                    ((int) (string[i + 2] & 0xC0) >> 6)];
    *p++ = basis_64[string[i + 2] & 0x3F];
    }
    if (i < len) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    if (i == (len - 1)) {
        *p++ = basis_64[((string[i] & 0x3) << 4)];
        *p++ = '=';
    }
    else {
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int) (string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
    }
    *p++ = '=';
    }

    *p++ = '\0';
    return p - encoded;
}

unsigned char *LicenseManager::GenerateMacAddress(unsigned char* UUID, bool keyType) // 0 for token / 1 for license
{
  char *buffer = new char[keylength];

  std::string sourceType = "";
  std::string result;
  std::string source_id;
  if (keyType == 0) {
    result = "KEY---->";
  }
  else {
    result = "LIC---->";
  }
  FILE* pipe;
  FILE* file;
  
  if (sourceType == "") {
    if ((file = fopen("/sys/class/dmi/id/product_uuid", "r")) != NULL) {
        pipe = popen("cat /sys/class/dmi/id/product_uuid", "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        try {
            result += "sourcetype=PID,"; // Product ID
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                source_id += std::string(buffer);
            }
            result += "sourceid=" + source_id;
            sourceType = "PID"; // Product ID
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
    }
  }

  if (sourceType == "") {
    if ((file = fopen("/sys/class/dmi/id/board_serial", "r")) != NULL) {
        pipe = popen("cat /sys/class/dmi/id/board_serial", "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        try {
            result += "sourcetype=BSN,"; // Board Serial Number
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                source_id += std::string(buffer);
            }
            result += "sourceid=" + source_id;
            sourceType = "BSN"; // Board Serial Number
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
    }
  }

  if (sourceType == "") {
    if ((file = fopen("/etc/machine-id", "r")) != NULL) {
        pipe = popen("cat /etc/machine-id", "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        try {
            result += "sourcetype=MID,"; // Machine ID
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                source_id += std::string(buffer);
            }
            result += "sourceid=" + source_id;
            sourceType = "MID"; // Machine ID
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
    }
  }

  if (sourceType == "") {
    pipe = popen("ifconfig en0 | awk '/ether/{print $2}'", "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        result += "sourcetype=MAC,"; // Mac Address
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            source_id += std::string(buffer);
        }
        result += "sourceid=" + source_id;
        sourceType = "MAC"; // Mac Address
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
  }

  strcpy( (char*)UUID, result.c_str() );

  delete[] buffer;
  return UUID;
}

std::string LicenseManager::GenerateDeviceToken() {
    unsigned char *generatedKey = new unsigned char[keylength];
    unsigned char *encryptedKey = new unsigned char[keylength];
    unsigned char *UUID = new unsigned char[keylength];
    char *key64 = new char[keylength];

    generatedKey = GenerateMacAddress(UUID, 0);
    encryptedKey = aes.EncryptECB(generatedKey, keylength, key);
    Base64encode(key64, (char*)encryptedKey, keylength);
    std::string deviceToken = std::string(key64);


    generatedKey = NULL;
    encryptedKey = NULL;
    key64 = NULL;
    delete[] generatedKey;
    delete[] encryptedKey;
    delete[] key64;

    return deviceToken;
}

std::string LicenseManager::GenerateLicenseKey(std::string deviceToken, int years, int months, int days) {

    char *token = new char[keylength];
    unsigned char *decryptedKey = new unsigned char[keylength];
    unsigned char *encryptedKey = new unsigned char[keylength];
    char *key64 = new char[keylength];

    Base64decode(token, deviceToken.c_str());

    decryptedKey = aes.DecryptECB((unsigned char*)token, keylength, key);

    time_t t = time(NULL);   // get time now

    int expiryDate_s = (int)t + (years * 365 * 24 * 60 * 60) + (months * 30 * 24 * 60 * 60) + (days * 24 * 60 * 60);

    int i = 0;
    std::string newKeyType = "LIC---->";
    while (decryptedKey[i] != '>'){
        decryptedKey[i] = newKeyType[i];
        i++;
    }

    i = 0;
    int delimeterCount = 0;
    while (delimeterCount < 2){
        if (decryptedKey[i] == ',' || decryptedKey[i] == '\0' || decryptedKey[i] == '\n') {
            delimeterCount++;
        }
        i++;
    }

    std::string expiryDate = ",expirydate=" + std::to_string(expiryDate_s);

    for (int j = 0; j < expiryDate.length(); j++) {
        decryptedKey[i+j-1] = (int)expiryDate[j];
    }

    encryptedKey = aes.EncryptECB(decryptedKey, keylength, key);
    Base64encode(key64, (char*)encryptedKey, keylength);
    std::string licenseKey = std::string(key64);

    token = NULL;
    key64 = NULL;
    decryptedKey = NULL;
    encryptedKey = NULL;
    delete[] token;
    delete[] key64;
    delete[] decryptedKey;
    delete[] encryptedKey;

    return licenseKey;
}

void LicenseManager::RegisterLicense(std::string license) {
    std::ofstream outfile ("digitalconductor.lic");
    outfile << license << std::endl;
    outfile.close();
    return;
}

std::string LicenseManager::DecryptKey(std::string deviceToken) {
    char *token = new char[keylength];
    unsigned char *decryptedKey = new unsigned char[keylength];

    Base64decode(token, deviceToken.c_str());
    decryptedKey = aes.DecryptECB((unsigned char*)(token), keylength, key);
    std::string decryptedLicenseKey = std::string((char*)decryptedKey);

    token = NULL;
    decryptedKey = NULL;
    delete[] token;
    delete[] decryptedKey;
    return decryptedLicenseKey;
}

bool LicenseManager::LicenseCheck() {

    std::ifstream file("digitalconductor.lic");
    std::string deviceToken;

    file.seekg(0, std::ios::end);   
    deviceToken.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    deviceToken.assign((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

    char *token = new char[keylength];
    unsigned char *decryptedKey = new unsigned char[keylength];
    unsigned char *macAddress = new unsigned char[keylength];
    unsigned char *UUID = new unsigned char[keylength];

    Base64decode(token, deviceToken.c_str());

    decryptedKey = aes.DecryptECB((unsigned char*)(token), keylength, key);

    time_t t = time(NULL);   // get time now

    int expiryDate_s = (int)t;

    std::string currentDate = std::to_string(expiryDate_s);

    macAddress = GenerateMacAddress(UUID, 1);

    // Check that source type / source id matches
    int i = 0;
    int delimeterCount = 0;
    while (delimeterCount < 2){
        if (decryptedKey[i] == ',') {
            delimeterCount++;
        }
        if ((decryptedKey[i] != macAddress[i]) && (delimeterCount != 2)) {
            return false;
        }
        i++;
    }

    // Check that expiry date has not passed
    std::string expiryDate;
    bool date = false;
    while (decryptedKey[i] != '\0'){
        if (date) {
            expiryDate += decryptedKey[i];
        }
        if (decryptedKey[i] == '=') {
            date = true;
        }
        i++;
    }

    if (atoi(expiryDate.c_str()) < atoi(currentDate.c_str())) {
        return false;
    }

    token = NULL;
    decryptedKey = NULL;
    macAddress = NULL;
    UUID = NULL;
    delete[] token;
    delete[] decryptedKey;
    delete[] macAddress;
    delete[] UUID;

    return true;
}

} // namespace dc
