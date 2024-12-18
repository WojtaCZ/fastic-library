#ifndef AURORA_H
#define AURORA_H

#include <cstdint>
#include <vector>

namespace aurora{

    class packet;

    class rx
    {
        public:
            //Signal if the receiver has acquired sync
            bool synced_ = false;

            //What is the bitslip needed to get to sync
            std::uint8_t bitSlip_ = 0;

            //Index of the packet to retreive by the getPacket function
            std::uint16_t packetIdx_ = 0;

            //Pointer to bitstream with the raw data
            std::uint32_t * rxBuffer_;
            std::size_t rxBufferSize_;

            //Buffer of received packets
            std::vector<packet> packetBuffer_;
            std::uint32_t berCounter_;

            bool hasValidSync(int packetIndex);
            
            std::uint64_t getPacketData(int packetIndex);
            void descramblePacketData(std::uint64_t & currentData, std::uint64_t previousData);
            packet getPacket();
            packet getPacket(int index);

            // Calculate the index in the data array at which the sync bits of the specified packetIndex are located
            constexpr int getSyncIndex(int packetIndex);
            // Calculate the bitshift of the sync bits of the specified packetIndex
            constexpr std::uint8_t getSyncShift(int packetIndex);
            // Get the synchronization bits value for the specified packet 
            std::uint8_t getSyncBits(int packetIndex);

            // Force the selected value of bitslip, this value persist until a sync attempt is executed
            void forceBitSlip(std::uint8_t bitSlip);

            int syncErrorTreshold_;
            int syncErrorSampleSize_;


        //public:
            rx(std::uint32_t * rxBuffer, std::size_t rxBufferSize, std::uint8_t syncErrorTresholdPercentage = 90, int syncErrorSampleSize = 64);

            void setRxBuffer(std::uint32_t * rxBuffer, std::size_t rxBufferSize);
            void processRxBuffer(bool discardControl = false);

            bool synchronize();
            bool isSynchronized();

            
            const std::vector<packet> & getPacketBuffer() const;

            std::uint8_t getBitSlip();
    };

    class packet
    {
        public:
            enum class btf{
                idle = 0x78,    //Idle, not ready or clock compensation
                nfc = 0xaa,     //Native Flow Control
                ufc = 0x2d,     //User Flow Control
                sep = 0x1e,     //Separator
                sep7 = 0xe1,    //Separator 7
                k0 = 0xd2,      //User K-Block 0
                k1 = 0x99,      //User K-Block 1
                k2 = 0x55,      //User K-Block 2
                k3 = 0xb4,      //User K-Block 3
                k4 = 0xcc,      //User K-Block 4
                k5 = 0x66,      //User K-Block 5
                k6 = 0x33,      //User K-Block 6
                k7 = 0x4b,      //User K-Block 7
                k8 = 0x87,      //User K-Block 8
                res = 0xff      //Reserved
            };

            struct idle {
                std::uint64_t raw_ : 56;
                bool sa : 1;
                bool nr : 1;
                bool cb : 1;
                bool cc : 1;
                btf btf : 8;
            };

            struct nfc {
                std::uint64_t raw_ : 47;
                bool xoff : 1;
                std::uint8_t pause : 8;
                btf btf : 8;
            };

            struct ufc {
                std::uint64_t data : 48;
                std::uint8_t size : 8;
                btf btf : 8;
            };

            struct separator7 {
                std::uint64_t data : 56;
                btf btf : 8;
            };

            struct separator {
                std::uint64_t data : 48;
                std::uint8_t size : 8;
                btf btf : 8;
            };

            struct generic {
                std::uint64_t data : 56;
                btf btf : 8;
            };

            union fields {
                struct idle idle;
                struct ufc ufc;
                struct nfc nfc;
                struct separator separator;
                struct separator7 separator7;
                struct generic generic;
            };
            
            enum class type {
                data = 0b01,    // Data frame preamble
                control = 0b10,  // Control frame preamble
                error
            } type;

            union {
                std::uint64_t data;
                fields fields;
            };
    };
}

#endif
