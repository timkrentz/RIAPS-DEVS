#ifndef __TIMER_HPP__
#define __TIMER_HPP__

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

//Timer definition
struct Timer_defs{
    struct in : public in_port<int> {};
    struct out : public out_port<RIAPSMsg_t> {};
};

template<typename TIME> class Timer{
    public:
        Timer() noexcept{
            state.state = IDLE;
            state.nextInternal = std::numeric_limits<TIME>::infinity();
        }
        Timer(string _topic, int _period_ms) {
            topic = _topic;
            period = _period_ms;
            state.state = RUN;
            state.nextInternal = TIME({0,0,period/1000,period%1000});
        }
        string topic;
        int period;
        int counter = 0;
        
        // state definition
        struct state_type{
            int state;
            TIME nextInternal;
        }; 
        state_type state;
        using input_ports = std::tuple<typename Timer_defs::in>;
        using output_ports = std::tuple<typename Timer_defs::out>;

        // internal transition
        void internal_transition() {
            switch (state.state) {
            case RUN:
                state.state = FIRE;
                state.nextInternal = TIME();
                counter += period;
                break;
            case FIRE:
                state.state = RUN;
                state.nextInternal = TIME({0,0,period/1000,period%1000});
                break;
            default:
                break;
            }
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
            switch (state.state) {
            case FIRE:
            {
                RIAPSMsg_t out;
                out = {topic,counter};
                get_messages<typename Timer_defs::out>(bags).push_back(out);
                break;
            }
            default:
                break;
            }
            return bags;
        }

        // time_advance function
        TIME time_advance() const {  
             return state.nextInternal;
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Timer<TIME>::state_type& i) {
            os << "state: " << i.state; 
        return os;
        }
};     
#endif // __TIMER_HPP__