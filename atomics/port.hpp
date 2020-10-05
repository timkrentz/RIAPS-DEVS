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
    // struct controlIn : public in_port<int> { };
    // struct ackIn : public in_port<Message_t> { };
    struct in : public in_port<int> {};
    struct out : public out_port<RIAPSMsg_t> {};
};

template<typename TIME> class Port{
    public:
        //Parameters to be overwriten when instantiating the atomic model
        // TIME   preparationTime;
        // TIME   timeout;
        // default constructor
        // Sender() noexcept{
        //   preparationTime  = TIME("00:00:10");
        //   timeout          = TIME("00:00:20");
        //   state.alt_bit    = 0;
        //   state.next_internal    = std::numeric_limits<TIME>::infinity();
        //   state.model_active     = false;
        // }
        Port() noexcept{
            state.state = IDLE;
            state.nextInternal = std::numeric_limits<TIME>::infinity();
        }
        Port(string _name) {
            name = _name;
            state.state = IDLE;
            state.nextInternal = std::numeric_limits<TIME>::infinity();
        }
        string name;
        
        // state definition
        struct state_type{
            int state;
            TIME nextInternal;
        }; 
        state_type state;
        // ports definition
        // using input_ports=std::tuple<typename Sender_defs::controlIn, typename Sender_defs::ackIn>;
        // using output_ports=std::tuple<typename Sender_defs::packetSentOut, typename Sender_defs::ackReceivedOut, typename Sender_defs::dataOut>;
        using input_ports = std::tuple<typename Port_defs::in>;
        using output_ports = std::tuple<typename Port_defs::out>;

        // internal transition
        void internal_transition() {
            // if (state.ack){
            //     if (state.packetNum < state.totalPacketNum){
            //         state.packetNum ++;
            //     state.ack = false;
            //     state.alt_bit = (state.alt_bit + 1) % 2;
            //     state.sending = true;
            //     state.model_active = true; 
            //     state.next_internal = preparationTime;   
            //     } else {
            //         state.model_active = false;
            //         state.next_internal = std::numeric_limits<TIME>::infinity();
            //     }
            // } else{
            //     if (state.sending){
            //         state.sending = false;
            //         state.model_active = true;
            //         state.next_internal = timeout;
            //     } else {
            //         state.sending = true;
            //         state.model_active = true;
            //         state.next_internal = preparationTime;    
            //     } 
            // 
            if (state.state == RUN) {
                state.state = IDLE;
                state.nextInternal = std::numeric_limits<TIME>::infinity();
            }
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) { 
            // if((get_messages<typename Sender_defs::controlIn>(mbs).size()+get_messages<typename Sender_defs::ackIn>(mbs).size())>1) 
            //         assert(false && "one message per time uniti");
            // for(const auto &x : get_messages<typename Sender_defs::controlIn>(mbs)){
            //     if(state.model_active == false){
            //         state.totalPacketNum = x;
            //         if (state.totalPacketNum > 0){
            //             state.packetNum = 1;
            //             state.ack = false;
            //             state.sending = true;
            //             state.alt_bit = 0;  //set initial alt_bit
            //             state.model_active = true;
            //             state.next_internal = preparationTime;
            //         }else{
            //             if(state.next_internal != std::numeric_limits<TIME>::infinity()){
            //                 state.next_internal = state.next_internal - e;
            //             }
            //         }
            //     }
            // }
            // for(const auto &x : get_messages<typename Sender_defs::ackIn>(mbs)){
            //     if(state.model_active == true) { 
            //         if (state.alt_bit == x.bit) {
            //             state.ack = true;
            //             state.sending = false;
            //             state.next_internal = TIME("00:00:00");
            //         }else{
            //             if(state.next_internal != std::numeric_limits<TIME>::infinity()){
            //                 state.next_internal = state.next_internal - e;
            //             }
            //         }
            //     }
            // }
            if(get_messages<typename Port_defs::in>(mbs).size()>1) 
                assert(false && "one message per time unit");
            for(const auto &msg : get_messages<typename Port_defs::in>(mbs)){
                switch (state.state){
                case IDLE:
                    if (msg == RUN) {
                        state.state = RUN;
                        state.nextInternal = TIME({0,0,5});
                    }
                    break;
                default:
                    break;
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
            // Message_t out;
            // if (state.sending){
            //     out.packet = state.packetNum;
            //     out.bit = state.alt_bit;
            //     get_messages<typename Sender_defs::dataOut>(bags).push_back(out);
            //     get_messages<typename Sender_defs::packetSentOut>(bags).push_back(state.packetNum);
            // }else{
            //     if (state.ack){
            //         get_messages<typename Sender_defs::ackReceivedOut>(bags).push_back(state.alt_bit);
            //     }
            // }
            RIAPSMsg_t out = {"NONE",-1};
            switch (state.state) {
            case RUN:
                out = {"TestTopic", 0};
                break;
            default:
                break;
            }
            get_messages<typename Port_defs::out>(bags).push_back(out);
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