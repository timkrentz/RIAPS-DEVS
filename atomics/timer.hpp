#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <NDTime.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>

#include "../data_structures/message.hpp"
#include "../utils/const.h"

using namespace cadmium;
using namespace std;

//Timer definition
struct Timer_defs{
    struct in : public in_port<int> {};
    struct out : public out_port<RIAPSMsg_t> {};
};

template<typename TIME> class Timer{
    public:
        Timer() {
            state.count = 0;
            nextInternal = std::numeric_limits<TIME>::infinity();
        }
        Timer(PortDescription_t _portDescription) {
            action = _portDescription.action;
            period = _portDescription.duty;
            state.count = 0;
            nextInternal = TIME({0,0,period/1000,period%1000});
        }
        string action;
        int period;
        int counter = 0;
        TIME nextInternal;
        
        // state definition
        struct state_type{
            int count;
        }; 
        state_type state;

        using input_ports = std::tuple<typename Timer_defs::in>;
        using output_ports = std::tuple<typename Timer_defs::out>;

        // internal transition
        void internal_transition() {
            state.count += 1;
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) { 
        }

        // confluence transition
        void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            internal_transition();
            external_transition(TIME(), std::move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output() const {
            typename make_message_bags<output_ports>::type bags;
            RIAPSMsg_t out;
            out = {action,counter};
            get_messages<typename Timer_defs::out>(bags).push_back(out);
            return bags;
        }

        // time_advance function
        TIME time_advance() const {  
             return nextInternal;
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Timer<TIME>::state_type& i) {
            os << "Msg Count: " << i.count; 
        return os;
        }
};     
#endif // __TIMER_HPP__