#ifndef FASTIC_H
#define FASTIC_H

#include <cstdint>
#include <vector>
#include <array>
#include "aurora.hpp"

namespace fastic{

    /*
        EVENT PACKET STRUCTURE
        bit 0 = MSB (sent first onto the bus)

        BITS    NAME          DESCRIPTION
        -------------------------------------------------------------------------------
        [0:3]   CHANNEL     - ID of the channel that the event was detected on
        [4:5]   TYPE        - A type of the packet (mode of the channel)
        [6:27]  TIMESTAMP   - ToA of the event
        [28:41] PULSE WIDTH - ToT of the event
        [42]    DBG         - Debug flag
        [43]    CHP         - Even parity bit for CHANNEL field
        [44]    TYP         - Even parity bit for TYPE field
        [45]    TSP         - Even parity bit for TIMESTAMP field
        [46]    PWP         - Even parity bit for PULSE WIDTH field
        [47]    PAR         - Combined even parity for all the fields
    */


    /*
        STATISTICS PACKET STRUCTURE
        bit 0 = MSB (sent first onto the bus)

        BITS    NAME           DESCRIPTION
        -------------------------------------------------------------------------------
        [0:19]  FIFO DROP    - Number of dropped packets from the FIFO
        [20:39] PWIDTH DROP  - Number of packets dropped due to out of range pulse width
        [40:59] DCOUNT DROP  - Number of dark count drops (only in High-Energy res mode)
        [60:79] TRIGGER DROP - Number of packets dropped due to malformed trigger pulse
        [80:95] PULSE ERROR  - Malformed pulse counter (malformed = too many edges per clock period)
    */

    /*
        EXTENSION PACKET STRUCTURE
        bit 0 = MSB (sent first onto the bus)

        BITS    NAME             DESCRIPTION
        -------------------------------------------------------------------------------
        [0:22]  PACKET COUNT   - Number of packets transmitted since last reset
        [23:46] COARSE COUNTER - Timestamp from the coarse counter
        [47]    RST            - Field indicating that the coarse counter was reset after last packet)
    */

    class eventPacket
    {
        public:
            //The FastIC+ has 8 detection channel and one trigger chanel
            enum class channel{
                CH0     = 0b0000,
                CH1     = 0b0001, 
                CH2     = 0b0010,
                CH3     = 0b0011,
                CH4     = 0b0100,
                CH5     = 0b0101,
                CH6     = 0b0110,
                CH7     = 0b0111,
                TRIGGER = 0b1000
            };

            enum class type{
                ToA_and_nonlinear_ToT = 0b00,   //ToA + non-linear ToT = high speed
                ToA_only              = 0b01,   //ToA only = not useful
                linear_ToT_only       = 0b10,   //Linear ToT only = hybrid
                ToA_and_linear_ToT    = 0b11    //ToA + linear ToT = high energy resolution
            };

            enum class field{
                type,
                channel,
                timestamp,
                pulsewidth
            };

        private:
            
            std::uint64_t rawData_;

            //Packet fields
            channel eventChannel_;
            bool eventChannelParity_;
            type eventType_;
            bool eventTypeParity_;
            std::uint32_t eventTimestamp_;
            bool eventTimestampParity_;
            std::uint16_t eventPulseWidth_;
            bool eventPulseWidthParity_;

            //Generic function to calculate parity of integers
            template<typename T>
            bool calculateParity(T data);

        public:
            eventPacket(aurora::dataPacket packet);
            eventPacket(type type, channel channel, std::uint32_t timestamp, std::uint16_t pulseWidth);
            
            channel getChannel(bool & isValid);
            type getType(bool & isValid);
            std::uint32_t getTimestamp(bool & isValid);
            std::uint16_t getPulseWidth(bool & isValid);
            bool getDebugBit();

            bool hasValidParity();

    };
    
    //Statistics packet declaration
    class statisticsPacket
    {
        private:
            //Packet fields
            std::uint32_t statFifoDrop_;
            std::uint32_t statPwidthDrop_;
            std::uint32_t statDcountDrop_;
            std::uint32_t statTriggerDrop_;
            std::uint16_t statPulseError_;

        public:
            statisticsPacket(std::array<aurora::controlPacket, 2> packets);
            statisticsPacket(std::uint32_t fifo, std::uint32_t pulsewidth, std::uint32_t darkcount, std::uint32_t trigger, std::uint16_t pulseError);
            std::uint32_t getTimestamp();
            std::uint16_t getPulseWidth();

    };

    //Extension counter packet declaration
    class extensionPacket
    {
        private:
            //Packet fields
            std::uint32_t extPacketCount_;
            std::uint32_t extCoarseCounter;
            bool statReset_;

        public:
            extensionPacket(aurora::controlPacket packet);
            extensionPacket(std::uint32_t packetCount, std::uint32_t coarseCounter, bool reset = false);
            std::uint32_t getPacketCount();
            std::uint16_t getCoarseCounter();
            bool getReset();
    };


}

#endif
