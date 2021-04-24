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
    struct in : public in_port<PortCMD_t> {};
    struct out : public out_port<PortCMD_t> {};
    struct zmqRead : public out_port<PortCMD_t> {};
    struct zmqRecv : public in_port<RIAPSMsg_t> {};
    struct zmqSend : public out_port<RIAPSMsg_t> {};
};

template<typename TIME> class Port{
    public:
        Port() noexcept{
            state.state = IDLE;
            state.receiving = false;
        }
        Port(PortDescription_t _portDescription) {
            name = _portDescription.name;
            handlerTime = TIME({0,0,_portDescription.duty/1000,_portDescription.duty%1000});
            action = _portDescription.action;
            state.state = IDLE;
            state.receiving = false;

            myDistrib = uniform_int_distribution(_portDescription.duty-5,_portDescription.duty+5);
        }
        string name;
        string action;
        TIME handlerTime;
        TIME remainingTime;

        
        random_device rd;
        uniform_int_distribution<> myDistrib;

        // state definition
        struct state_type{
            int state;
            bool receiving;
        }; 
        state_type state;

        using input_ports = std::tuple<typename Port_defs::in,
                                        typename Port_defs::zmqRecv>;
        using output_ports = std::tuple<typename Port_defs::out,
                                        typename Port_defs::zmqRead,
                                        typename Port_defs::zmqSend>;

        // internal transition
        void internal_transition() {
            // cout<<"PORT INTERNAL"<<endl;
            if (state.state == IDLE && state.receiving == true) {
                state.state = WAIT;
                return;
            } else if (state.state == RUN) {
                assert((state.receiving == false) && "Internal transition from RUN where expecting to receive");
                state.state = IDLE;
                return;
            }
            assert(false && "Unexpected internal event");
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) { 
            // cout<<"PORT EXTERNAL"<<endl;
            assert(get_messages<typename Port_defs::in>(mbs).size() <= 1  && "one message per time unit");
            for(const auto &msg : get_messages<typename Port_defs::in>(mbs)){
                if (msg.name != this->name) continue;
                if (msg.cmd == RUN) {
                    assert((state.state == IDLE) && "Got RUN while in IDLE");
                    assert((state.receiving == false) && "Got RUN while in receiving");
                    state.receiving = true;
                    // if (state.state == IDLE) {
                    //     state.state = RUN;
                    //     state.nextInternal = handlerTime;
                    //     remainingTime = handlerTime;
                    // } else if (state.state == WAIT) {
                    //     state.state = RUN;
                    //     state.nextInternal = remainingTime;
                    // }
                // } else if (msg.cmd == WAIT) {
                //     assert(state.state == RUN && (name+" got WAIT while in"+std::to_string(state.state)).c_str());
                
                //     remainingTime = remainingTime - e;
                //     state.nextInternal = std::numeric_limits<TIME>::infinity();
                }
            
            }         

            assert(get_messages<typename Port_defs::zmqRecv>(mbs).size() <= 1 && "one recv per time unit");
            for(const auto &msg : get_messages<typename Port_defs::zmqRecv>(mbs)){
                state.receiving = false;
                state.state = RUN;
                remainingTime = TIME({0,0,0,myDistrib(rd)});
            }
        }

        // confluence transition
        void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            // cout<<"PORT CONFLUENCE"<<endl;
            internal_transition();
            external_transition(TIME(), std::move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output() const {
            // cout<<"PORT OUTPUT"<<endl;
            typename make_message_bags<output_ports>::type bags;
            if (state.receiving) get_messages<typename Port_defs::zmqRead>(bags).push_back(PortCMD_t(name,RECV));
            if (state.state == RUN) {
                get_messages<typename Port_defs::out>(bags).push_back(PortCMD_t(name,FIRE));
                if (action != "") get_messages<typename Port_defs::zmqSend>(bags).push_back(RIAPSMsg_t(action,1));
            }
            return bags;
        }

        // time_advance function
        TIME time_advance() const {  
            // cout<<"PORT TIME ADVANCE"<<endl;
             if (state.state == RUN){
                    return remainingTime;
             } else if (state.state == IDLE && state.receiving == true) {
                    return TIME();
            }
            return std::numeric_limits<TIME>::infinity();
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Port<TIME>::state_type& i) {
            os << ": PORT_";
            switch (i.state){
                case IDLE: os << "IDLE";
                break;
                case RUN: os << "RUN";
                break;
                case WAIT: os << "WAIT";
                break;
            }
            os << " ";
            if (i.receiving) {
                os << "READING";
            } else {
                os << "NOT_READING";
            }
            return os;
        }
};     
#endif // __PORT_HPP__