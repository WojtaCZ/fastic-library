#include "aurora.hpp"
#include <bitset>
#include <algorithm>
#include <cstdint>


uint64_t scrambler = 0, output = 0;

namespace aurora{

    /// @brief Construct the receiver with the buffer specified
    /// @param rxBuffer Pointer to the raw data buffer with 32bit MSB unscrambled data chunks
    /// @param rxBufferSize Size of the raw buffer
    /// @param syncErrorTresholdPercentage The treshold (in percent) of the ratio of correct to error packets for the stream to be considered in sync
    /// @param syncErrorSampleSize The number of data packets which will be checked to determine if they comply with the error treshold
    rx::rx(std::uint32_t * rxBuffer, std::size_t rxBufferSize, std::uint8_t syncErrorTresholdPercentage, int syncErrorSampleSize) : 
        rxBuffer_(rxBuffer), 
        rxBufferSize_(rxBufferSize)
    {
        if(syncErrorTresholdPercentage > 100) syncErrorTresholdPercentage = 100;

        if(rxBufferSize <= syncErrorSampleSize){
            syncErrorSampleSize_ = rxBufferSize;
        }else {
            syncErrorSampleSize_ = syncErrorSampleSize;
        }

        syncErrorTreshold_ = (int)((syncErrorSampleSize_ * syncErrorTresholdPercentage) / 100);
    }

    void rx::forceBitSlip(std::uint8_t bitSlip) {
        this->bitSlip_ = bitSlip;
    }

    // Calculate the index in the data array at which the sync bits of the specified packetIndex are located
    constexpr int rx::getSyncIndex(int packetIndex){
        return (this->bitSlip_ + (66 * packetIndex)) / 32;
    }

    // Calculate the bitshift of the sync bits of the specified packetIndex
    constexpr std::uint8_t rx::getSyncShift(int packetIndex){
        return (this->bitSlip_ + (66 * packetIndex)) % 32;
    }

    std::uint8_t rx::getSyncBits(int packetIndex){
        int syncIndex = getSyncIndex(packetIndex);
        std::uint8_t syncShift = getSyncShift(packetIndex);

        std::uint8_t syncBits = 0;
        
        if(syncShift != 31){
            // This is the special case where the sync bits are split between the two 32bit data words -> join them
            syncBits = ((rxBuffer_[syncIndex] & 0x00000001) << 1) | ((rxBuffer_[syncIndex+1] & 0x80000000) >> 31);
        }else{
             // This is the most usual case where the bits are in one data word -> just mask them
            syncBits = (rxBuffer_[syncIndex] & (0xC0000000 >> syncShift)) >> (30 - syncShift);
        }

        return syncBits;
    }

    // Check if the sync bits at the specified position are valid
    bool rx::hasValidSync(int packetIndex){
        // Get the sync bits
        std::uint8_t syncBits = getSyncBits(packetIndex);

        // If the sync bits are incorrect, return false
        if(syncBits == 0b00) return false;
        if(syncBits == 0b11) return false;
        
        // Otherwise, return true
        return true;
    }


    bool rx::synchronize(){ 
        // Counter for valid packets
        int validPacketCounter;
        
        // Go through all the possible bitslips
        for(std::uint8_t bitSlip = 0; bitSlip < 64; bitSlip++){
            // Force the selected bit slip
            forceBitSlip(bitSlip);
            // Check the first 5 packets for validity (this usually gets rid of most of the fake syncs)
            if(hasValidSync(0) && hasValidSync(1) && hasValidSync(2) && hasValidSync(3) && hasValidSync(4)){
                // If sync candidate was acquired, check if it suffices the sync error treshold
                validPacketCounter = 5;
                // Go through all the packets with the selected slip
                for(int packetIndex = 5; packetIndex < syncErrorSampleSize_; packetIndex++){
                    //Count how many of them were valid
                    if(hasValidSync(packetIndex)){
                        validPacketCounter++;
                        //If the ratio of good to error packets suffices, return the bit slip
                        if(validPacketCounter >= syncErrorTreshold_){
                            this->bitSlip_ = bitSlip;
                            this->synced_ = true;
                            return true;
                        }
                    }
                }
            }
        }
        
        // Otherwise return false
        return false;
    }

    // Return the current bitslip
    std::uint8_t rx::getBitSlip(){
        return this->bitSlip_;
    }

    std::uint64_t rx::getPacketData(int packetIndex){
        int syncIndex = getSyncIndex(packetIndex);
        std::uint8_t syncShift = getSyncShift(packetIndex);

        std::uint64_t data;

        // Reconstruct the packet data
        if(syncShift == 31){
            data = 0
                   | ((std::uint64_t)(rxBuffer_[syncIndex+1] & 0x7FFFFFFF) << 33)
                   | ((std::uint64_t) rxBuffer_[syncIndex+2]               << 1)
                   | ((std::uint64_t)(rxBuffer_[syncIndex+3] & 0x80000000) >> 31);
        }else{
            data = 0
                 | ((std::uint64_t)((rxBuffer_[syncIndex]     & (0x3FFFFFFF >> syncShift)))           << (32 + syncShift + 2))
                 | ((std::uint64_t)  rxBuffer_[syncIndex+1])                                          << (syncShift + 2)
                 | ((std::uint64_t)((rxBuffer_[syncIndex+2]   & (0xFFFFFFFF << (30 - syncShift))))    >> (30 - syncShift));
        }

        return data;
    }

    std::uint64_t rx::descramblePacketData(std::uint64_t currentData, std::uint64_t previousData){
        output = 0;

        for(int i = 63; i >= 0; i--){
            uint8_t newBit = ((currentData >> i) & 1);
            //Shift the latest bit to the scrambler buffer
            previousData <<= 1;
            previousData |= newBit;
            output |= ((previousData >> 0) & 1) ^ ((previousData >> 39) & 1) ^ ((previousData >> 58) & 1);
            
            
            if(i != 0) output <<= 1;

        }

        return output;

    }

   /* packet rx::getPacket(){
        // Get the sync bits of the packet
        std::uint8_t syncBits = getSyncBits(packetIndex);

        //Sort out the packet type
        packet::type packetType;

        if(syncBits == 0b01){
            packetType = packet::type::data;
        }else if(syncBits == 0b10){
            packetType = packet::type::control;
        }else{
            packetType = packet::type::error;
        }

        std::uint64_t data = getPacketData(packetIndex);

        output = 0;

        for(int i = 63; i >= 0; i--){
            uint8_t newBit = ((data >> i) & 1);
            //Shift the latest bit to the scrambler buffer
            scrambler |= newBit;
            output |= ((scrambler >> 0) & 1) ^ ((scrambler >> 39) & 1) ^ ((scrambler >> 58) & 1);
            
            scrambler <<= 1;
            if(i != 0) output <<= 1;

        }

        

         this->packetIdx_++;

        return packet(packetType, data);

    }*/
/*
    void rx::processBuffer(bool discardControl){
        //Clear the buffer
        packetBuffer_.clear();
        this->packetIdx_ = 0;

        std::uint16_t treshold = 0;

        //If there is no bitslip, one more packet can be read
        if(bitSlip_){
            treshold = (rxBufferSize_*32)/66 - 1;
        }else treshold = (rxBufferSize_*32)/66;
        
        //Go through the buffer and retreive all available packets
        for(int idx = 0; idx < treshold; idx++){
            packet p = getPacket(true);
            if(p.getType() == packet::type::error) berCounter_++;
            if(!(discardControl && p.getType() == packet::type::control)){
                packetBuffer_.push_back(p);
            } 
        }

        //Divide the error frames by the number of frames received
        berCounter_ /= (treshold + 1);
    }*/

    //Return the reference to the internal buffer
    const std::vector<packet> & rx::getPacketBuffer() const{
        return packetBuffer_;
    }

    //Packet constructor
    packet::packet(type packetType, std::uint64_t packetData) : 
        packetType_(packetType), 
        packetData_(packetData)
    { }

    //Return the packet type
    packet::type packet::getType(){
        return packetType_;
    }

    //Return the packet data
    std::uint64_t packet::getData(){
        return packetData_;
    }   

    
}