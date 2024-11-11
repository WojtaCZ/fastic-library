#include "fastic.hpp"
#include <type_traits>

namespace fastic{

    //Generic function template to calculate parity for integers
    template<typename T>
    bool eventPacket::calculateParity(T data){
        static_assert(std::is_integral_v<T>, "The data needs to be of integral type");
        
        T out = data ^ (data >> 1);

        for(std::uint16_t shift = 2; shift < sizeof(T)*4; shift *= 2){
            out = out ^ (out >> shift);
        }

        //Return 0 if the parity is even 
        return out & 1;
    }

    eventPacket::eventPacket(aurora::dataPacket packet){
        this->rawData_ = packet.getData();

        //Parse channel data and calculate parity
        this->eventChannel_ = static_cast<eventPacket::channel>(this->rawData_ >> 60);
        this->eventChannelParity_ = calculateParity(this->eventChannel_);

        //Parse type data and calculate parity
        this->eventType_ = static_cast<eventPacket::type>((this->rawData_ >> 58) & 0x03);
        this->eventTypeParity_ = calculateParity(this->eventType_);

        //Parse timestamp data and calculate parity
        this->eventTimestamp_ = (this->rawData_ >> 20) & 0x3FFFFF;
        this->eventTimestampParity_ = calculateParity(this->eventTimestamp_);

        //Parse pulse width data and calculate parity
        this->eventPulseWidth_ = (this->rawData_ >> 6) & 0x3FFF; 
        this->eventPulseWidthParity_ = calculateParity(this->eventPulseWidth_);
    }

    eventPacket::channel eventPacket::getChannel(bool & isValid){
        isValid = (this->eventChannelParity_ == static_cast<bool>((this->rawData_ >> 4) & 1));
        return this->eventChannel_; 
    }

    eventPacket::type eventPacket::getType(bool & isValid){
        isValid = (this->eventTypeParity_ == static_cast<bool>((this->rawData_ >> 3) & 1));
        return this->eventType_; 
    }

    std::uint32_t eventPacket::getTimestamp(bool & isValid){
        isValid = (this->eventTimestampParity_ == static_cast<bool>((this->rawData_ >> 2) & 1));
        return this->eventTimestamp_; 
    }

    std::uint16_t eventPacket::getPulseWidth(bool & isValid){
        isValid = (this->eventPulseWidthParity_ == static_cast<bool>((this->rawData_ >> 1) & 1));
        return this->eventPulseWidth_; 
    }

    bool eventPacket::getDebugBit(){
        return static_cast<bool>((this->rawData_ >> 5) & 1);
    }

    bool eventPacket::hasValidParity(){
        return ((this->eventChannelParity_ ^ this->eventTypeParity_ ^ this->eventTimestampParity_ ^ this->eventPulseWidthParity_) == static_cast<bool>(this->rawData_ & 1));
    }



}