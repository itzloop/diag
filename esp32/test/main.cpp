#include <iostream>
#define SUPPORTED_PID_01_TO_20 0x00188000
#define SUPPORTED_PID_21_TO_40 0x00000000
#define SUPPORTED_PID_41_TO_60 0x00000000
#define SUPPORTED_PID_61_TO_80 0x00000000
#define SUPPORTED_PID_81_TO_A0 0x00000000
#define SUPPORTED_PID_A1_TO_C0 0x00000000
#define SUPPORTED_PID_C1_TO_E0 0x00000000
#define SUPPORTED_SERVICE 0x01
int checksum(uint8_t *b, uint8_t len)
{
    uint8_t ret = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        ret += b[i];
    }
    return ret;
}

void printhex(uint8_t *b, uint8_t len)
{
    char hex[] = "0123456789ABCDEF";
    char result[len * 2];
    for (uint8_t i = 0, j = 0; i < len; i++, j += 2)
    {
        uint8_t ch = b[i];
        result[j] = hex[(ch >> 4) & 0xF];
        result[j + 1] = hex[(ch)&0xF];
    }

    std::cout << result;
}

bool supportService(uint8_t service)
{
    return service == SUPPORTED_SERVICE;
}

bool supportPid(uint8_t pid)
{
    if (pid % 32 == 0)
        return true;

    if (pid > 0xE0)
        return false;

    int supportedList[] = {
        SUPPORTED_PID_01_TO_20,
        SUPPORTED_PID_21_TO_40,
        SUPPORTED_PID_41_TO_60,
        SUPPORTED_PID_61_TO_80,
        SUPPORTED_PID_81_TO_A0,
        SUPPORTED_PID_A1_TO_C0,
        SUPPORTED_PID_C1_TO_E0};

    uint8_t idx = pid / 32;
    int num = supportedList[idx];

    return ((num >> (32 - (pid % 32))) & 0x1) == 1;
}

main(void)
{
    // // checksum = 0x8f
    // uint8_t arr[5] = {0xC1, 0x33, 0xF1, 0x81, 0};
    // // if ( == 0x66)
    // // std::cout << checksum(arr, 5);

    // uint8_t service = 0x1;
    // uint8_t pid = 0x1;

    // for (size_t i = 0; i < 256; i++)
    // {
    //     std::cout << i << " pid: " << supportPid(i) << std::endl;
    // }

    // 0b0000 0000 0001 1000 1000 0000 0000 0000
    // std::cout << ((0x00188000 >> (32 - (0x2E % 32))) & 0x1);

    // have to modify the first bytes.
    uint8_t rbuf[request_len];
    memcpy(rbuf, request, request_len);
    // now we modify the header, the payload is the request_len - 3 header bytes
    rbuf[0] = (0b11 << 6) | (request_len - 3);
    rbuf[1] = 0x33; // second byte should be 0x33
    return (this->requestKWP(&rbuf, request_len) == ret_len);
}
