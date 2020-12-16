#ifndef __COMPONENT_HPP__
#define __COMPONENT_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>
#include <queue>
#include <unordered_map>

#include "../data_structures/message.hpp"
#include "../utils/const.h"

using namespace cadmium;
using namespace std;

// struct SchedulerEntry_t : public PortDescription_t {
//     using PortDescription_t::PortDescription_t;
//     SchedulerEntry_t(string _name, string _topic, string _action, int _type, int _duty, int _state) : PortDescription_t(_name, _topic, _action, _type, _duty), state(_state){}
//     int state;
// };

const int BATCH = 0;
const int PRIO = 1;
const int RR = 2;

const int WAIT = 0;
const int RUN_READY = 1;
const int RUN_ACTIVE = 2;

//Port definition
struct Component_defs{
    struct fromPort : public in_port<PortCMD_t> {};
    struct toPort : public out_port<PortCMD_t> {};
    struct zmqIn : public in_port<PollResult_t> {};
    struct poll : public out_port<int> {};
};

typedef unordered_map<string, queue<RIAPSMsg_t>> StringToMsgQueue_t;
typedef unordered_map<string, vector<string>> Topic2Names_t;

template<typename TIME> class Component{
    public:
        Component() {
            nextInternal = std::numeric_limits<TIME>::infinity();
            state.state = WAIT;
            state.pollActive = false;
            // state.current=nullptr;
            schedPolicy = BATCH;
        }
        Component(vector<PortDescription_t> _portList, int _policy) {
            nextInternal = std::numeric_limits<TIME>::infinity();
            state.state = WAIT;
            state.pollActive = false;
            // state.current = nullptr;
            schedPolicy = _policy;
            portList = _portList;
        } 

        // List of Ports strictly ordered by priority
        vector<PortDescription_t> portList;
        list<ScheduleEntry_t> schedule;
        TIME nextInternal;
        int schedPolicy;
        // vector<SchedulerEntry_t> schedule;


        // state definition
        struct state_type{
            int state;
            bool pollActive;
            // SchedulerEntry_t* current;
        }; 
        state_type state;

        // ports definition
        using input_ports = std::tuple<typename Component_defs::fromPort,
                                       typename Component_defs::zmqIn>;
        using output_ports = std::tuple<typename Component_defs::toPort,
                                        typename Component_defs::poll>;

        // internal transition
        void internal_transition() {
            cout<<"COMPONENT INTERNAL"<<endl;
            if (state.state == WAIT) {
                if (state.pollActive == false) {
                    state.pollActive = true;
                }
            } else if (state.state == RUN_READY) {
                state.state = RUN_ACTIVE;
            }
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) { 
            cout<<"COMPONENT EXTERNAL"<<endl;

            // Handle poll response (should never be empty)
            assert(get_messages<typename Component_defs::zmqIn>(mbs).size() <= 1 && "Two vectors in poll response!");
            for(const auto &msg : get_messages<typename Component_defs::zmqIn>(mbs)){
                assert(msg.size() > 0 && "Poll response is empty!");
                assert(state.state == WAIT && "Poll response came during running port!");
                for (auto& entry : msg) scheduleMsg(entry);
                state.state = RUN_READY;
                state.pollActive = false;
            }

            // Handle events from the ports
            for(const auto &msg : get_messages<typename Component_defs::fromPort>(mbs)){
                if(get_messages<typename Component_defs::fromPort>(mbs).size()>1) assert(false && "more than one fromPort");
                assert(state.state == RUN_ACTIVE && "Message from port despite not running one!")
                assert(msg.name == schedule.front().portName && "Message from inactive handler!");
                schedule.pop_front();
                state.state = WAIT;
            }
        }

        // confluence transition
        void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout<<"COMPONENT CONFLUENCE"<<endl;
            internal_transition();
            external_transition(TIME(), std::move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output() const {
            cout<<"COMPONENT OUTPUT"<<endl;
            typename make_message_bags<output_ports>::type bags;
            if (state.state == RUN_READY) {
                PortCMD_t out = {schedule.front().portName,RUN};
                get_messages<typename Component_defs::toPort>(bags).push_back(out);
            } else if (state.state == WAIT) {
                if (state.pollActive == false){
                    get_messages<typename Component_defs::poll>(bags).push_back(1);
                }
            }
            return bags;
        }

        // time_advance function
        TIME time_advance() const {
            cout<<"COMPONENT TIME ADVANCE"<<endl;
            switch (state.state) {
                case WAIT:
                    if (!state.pollActive) return TIME();
                    return std::numeric_limits<TIME>::infinity();
                case RUN_READY:
                    return TIME();
                case RUN_ACTIVE:
                    return std::numeric_limits<TIME>::infinity();
                default:
                    assert(false && "Unexpected state value!");
                    break;
            }
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Component<TIME>::state_type& i) {
            os << ": " << i.state;
            return os;
        }

    private:

        void scheduleMsg(ScheduleEntry_t& entry){
            switch (this->schedPolicy){
                case BATCH:
                case PRIO: {
                    auto schedIter = this->schedule.begin();
                    auto currentHighest = this->portList.begin();
                    for ( ; ; ){
                        // If we're at the end, just stick it in
                        if (schedIter == this->schedule.end()) break;

                        // Is entry priority the highest?
                        if (entry.portName == *currentHighest.name) { //if it is...
                            while (*schedIter.portName == entry.portName && schedIter != this->schedule.end()){
                                schedIter++;
                            }
                            break;
                        }
                        
                        // Check next highest priority
                        currentHighest++;
                        if (currentHighest == this->portList.end()) {
                            schedIter = this->schedule.end();
                            break;
                        }
                    }
                    this->schedule.insert(schedIter,entry);
                    break;
                }
                case RR:
                default:
                    break;
            }
        }

#endif // __COMPONENT_HPP__