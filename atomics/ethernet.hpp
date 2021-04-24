#ifndef __ETHERNET_HPP__
#define __ETHERNET_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <NDTime.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>
#include <queue>

#include "../data_structures/message.hpp"
#include "../utils/const.h"

using namespace cadmium;
using namespace std;

//Timer definition
struct Ethernet_defs{
    struct in : public in_port<RIAPSMsg_t> {};
    struct out : public out_port<RIAPSMsg_t> {};
};

typedef queue<RIAPSMsg_t> EthernetMsgQueue_t; 

template<typename TIME> class Ethernet{
    public:
        Ethernet() {
            myDistrib = ::std::uniform_int_distribution(10,20);
            state.empty = true;
            state.nextInternal = std::numeric_limits<TIME>::infinity();
        }
        Ethernet(int _low, int _high ) {
            myDistrib = uniform_int_distribution(_low, _high);
            state.empty = true;
            state.nextInternal = std::numeric_limits<TIME>::infinity();

        }


        EthernetMsgQueue_t myQueue;

        random_device rd;
        uniform_int_distribution<> myDistrib;
        
        // state definition
        struct state_type{
            bool empty;
            TIME nextInternal;
        }; 
        state_type state;

        using input_ports = std::tuple<typename Ethernet_defs::in>;
        using output_ports = std::tuple<typename Ethernet_defs::out>;

        // internal transition
        void internal_transition() {
            cout << "INTERNAL" << endl;
            myQueue.pop();
            state.empty = myQueue.empty();
            if (state.empty) {
                state.nextInternal = std::numeric_limits<TIME>::infinity();
            } else {
                state.nextInternal = TIME({0,0,0,myDistrib(rd)});
            }
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout << "EXTERNAL" << endl;
            for(const auto &msg : get_messages<typename Ethernet_defs::in>(mbs)){
                myQueue.push(msg);
            }
            assert(state.nextInternal > e && "Missed Internal Transition!");
            if (state.nextInternal == std::numeric_limits<TIME>::infinity()) {
                state.nextInternal = TIME({0,0,0,myDistrib(rd)});
            } else {
                state.nextInternal = state.nextInternal - e;
            }
            state.empty = false;
        }

        // confluence transition
        void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout << "CONFLUENCE" << endl;
            internal_transition();
            external_transition(TIME(), std::move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output() const {
            cout << "OUTPUT" << endl;
            typename make_message_bags<output_ports>::type bags;
            get_messages<typename Ethernet_defs::out>(bags).push_back(myQueue.front());
            return bags;
        }

        // time_advance function
        TIME time_advance() const {  
            cout << "TIME ADVANCE" << endl;
             return state.nextInternal;
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Ethernet<TIME>::state_type& i) {
            os << "EMPTY: " << i.empty; 
        return os;
        }
};     
#endif // __ETHERNET_HPP__