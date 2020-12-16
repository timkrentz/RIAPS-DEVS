#ifndef __PORT_HPP__
#define __PORT_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>

#include "../data_structures/message.hpp"
#include "../utils/const.h"

using namespace cadmium;
using namespace std;

//Port definition
struct Port_defs{
    // struct packetSentOut : public out_port<int> { };
    // struct ackReceivedOut : public out_port<int> {};
    // struct dataOut : public out_port<Message_t> { };
    // struct controlIn : public in_port<PortCMD_t> {};
    // struct controlOut: public out_port<PortCMD_t> {};
    // struct ackIn : public in_port<Message_t> { };
    struct in : public in_port<PortCMD_t> {};
    struct out : public out_port<PortCMD_t> {};
};

template<typename TIME> class Port{
    public:
        Port() noexcept{
            state.state = IDLE;
            state.nextInternal = std::numeric_limits<TIME>::infinity();
        }
        Port(PortDescription_t _portDescription) {
            name = _portDescription.name;
            handlerTime = _portDescription.duty;
            state.state = IDLE;
            state.nextInternal = std::numeric_limits<TIME>::infinity();
        }
        string name;
        TIME handlerTime;
        TIME remainingTime;
        // state definition
        struct state_type{
            int state;
            TIME nextInternal;
        }; 
        state_type state;
        using input_ports = std::tuple<typename Port_defs::in>;
        using output_ports = std::tuple<typename Port_defs::out>;

        // internal transition
        void internal_transition() {
            if (state.state == RUN) {
                state.state = FIRE;
                state.nextInternal = TIME();
                return;
            } else if (state.state == FIRE) {
                state.state = IDLE;
                state.nextInternal = std::numeric_limits<TIME>::infinity();
                return;
            }
            assert(false && ("Unexpected internal event in state "+std::to_string(state.state)).c_str());
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) { 
            if(get_messages<typename Port_defs::in>(mbs).size() > 1) 
                    assert(false && "one message per time unit");
            for(const auto &msg : get_messages<typename Port_defs::in>(mbs)){
                if (msg.name != this->name) continue;
                if (msg.cmd == RUN) {
                    if (state.state != IDLE && state.state != WAIT) assert(false && (name+" got RUN while in "+std::to_string(state.state)).c_str());
                    if (state.state == IDLE) {
                        state.state = RUN;
                        state.nextInternal = handlerTime;
                        remainingTime = handlerTime;
                    } else if (state.state == WAIT) {
                        state.state = RUN;
                        state.nextInternal = remainingTime;
                    }
                } else if (msg.cmd == WAIT) {
                    if (state.state != RUN) assert(false && (name+" got WAIT while in"+std::to_string(state.state)).c_str());
                    state.state = WAIT;
                    remainingTime = remainingTime - e;
                    state.nextInternal = std::numeric_limits<TIME>::infinity();
                }
            }         
        }

        // confluence transition
        void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            internal_transition();
            external_transition(TIME(), std::move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output() const {
            typename make_message_bags<output_ports>::type bags;
            switch (state.state) {
            case FIRE:
                get_messages<typename Port_defs::out>(bags).push_back(PortCMD_t(name,FIRE));
                break;
            case RUN:
            default:
                break;
            }
            return bags;
        }

        // time_advance function
        TIME time_advance() const {  
             return state.nextInternal;
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Port<TIME>::state_type& i) {
            os << "state: " << i.state; 
        return os;
        }
};     
#endif // __PORT_HPP__