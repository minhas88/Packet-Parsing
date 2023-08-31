#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

const uint32_t MAGIC_HEADER = 0xABBACFFC;
const uint8_t RESERVED_VALUE = 0xFF;
const uint8_t TELEM_TYPE_TEMPERATURE = 1;

enum SubsystemID {
    AOCS = 1,
    CDH = 3,
    COM = 5
};

int validCompID[3][6] = {
    {20, 21, 22, 23, 30, 31}, // AOCS
    {0},                      // CDH
    {1, 2, 10, 20}            // COM
};

struct indices
{
    int i;
    int j;
};

struct SecondaryHeader {
    uint8_t protocolVersion : 3;
    uint8_t subsystemID : 5;
    uint8_t componentID : 5;
    uint8_t telemetryType : 3;
};

struct Packet {
    uint32_t magicHeader;
    struct SecondaryHeader secondaryHeader;
    uint8_t reserved;
    float payload;
    uint8_t checksum;
};


struct indices lookupcompID(int targetVal){
    struct indices indi = {-1, -1};
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            if(validCompID[i][j] == targetVal){
                indi.i = i;
                indi.j = j;
                return indi;
            }
        }
    }
    return indi;  
}

int detectPacket(const uint32_t* data) {
    return data[0] == MAGIC_HEADER;
}

int calCheckSum(const uint32_t *data, const size_t *size){
    uint32_t checksum = 0;
    for (int i = 1; i < *size-1 ; i++)
    {
        checksum ^= data[i];
    }
    checksum &= 0xFF;
    //printf("Checksum = 0x%x\n", checksum);
    return checksum == data[*size-1];
}

void parseData(uint32_t inputData[]) {
    struct Packet packet;
    unsigned int container[32];
    if (!detectPacket(inputData)) {
        printf("Header does not match. Data is not relevant"); 
    }
    else {
        packet.magicHeader = inputData[0];
        uint8_t componentID = (inputData[1] >> 3) & 0x1F;

        struct indices compIndices = lookupcompID(componentID);

        if (compIndices.i != -1 && compIndices.j != -1) {
            packet.secondaryHeader.subsystemID = compIndices.i + 1; 
            packet.secondaryHeader.componentID = validCompID[compIndices.i][compIndices.j];
        } 
        
        packet.secondaryHeader.protocolVersion = (inputData[1] >> 13) & 0x07;
        packet.secondaryHeader.telemetryType = inputData[1] & 0x07;

        packet.reserved = inputData[2];

        uint32_t payloadValue = (inputData[3] << 24) | (inputData[4] << 16) | (inputData[5] << 8) | inputData[6];

        /*int sign = payloadValue >> 31 & 0x01;
        for (int i = 1, j=31; i >= 0, j > 30; i--, j--)
        {
            container[j] = (sign >> i) & 0x01;
        }

        int exponent = (payloadValue >> 23 & 0x0FF);
        for (int i = 7, j=30; i >= 1, j > 22; i--, j--)
        {
            container[j] = (exponent >> i) & 0x01;
        }

        int Significand = payloadValue & 0x4FFFFF;
        for (int i = 22, j=22; i >= 1, j >= 0 ; i--, j--)
        {
            container[j] = (Significand >> i) & 0x01;
        }*/

        memcpy(&packet.payload, &payloadValue, sizeof(float));

        packet.checksum = inputData[7];

        printf("Magic Header: 0x%x\n", packet.magicHeader);
        printf("Protocol Version: %u\n", packet.secondaryHeader.protocolVersion);
        printf("Subsystem ID: %u\n", packet.secondaryHeader.subsystemID);
        printf("Component ID: %u\n", packet.secondaryHeader.componentID);
        printf("Telemetry Type: %u\n", packet.secondaryHeader.telemetryType);
        printf("Reserved: 0x%x\n", packet.reserved);
        printf("Payload: %f\n", packet.payload);
        printf("Checksum: 0x%x\n", packet.checksum);
    }
}

int main() {
    uint32_t inputData[] = {0xABBACFFC, 0x41B9, 0xFF, 0x41, 0xC6, 0x9E, 0x3F, 0x60};
    uint32_t inputData2[] = {0xABBACFFC, 0x2551, 0xFF, 0xC1, 0xA6, 0x9E, 0x1E, 0x49};
    size_t inputSize = sizeof(inputData)/sizeof(inputData[0]);
    if (!calCheckSum(inputData, &inputSize))
    {
        printf("Checksum does not match. Data is corrupted..");
    }else{
        parseData(inputData);
    }
    return 0;
}

