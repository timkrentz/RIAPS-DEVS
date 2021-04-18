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
#include <functional>
#include <list>

#include "../data_structures/message.hpp"
#include "../utils/const.h"

using namespace cadmium;
using namespace std;

// struct SchedulerEntry_t : public PortDescription_t {
//     using PortDescription_t::PortDescriptioun_t;
//     SchedulerEntry_t(string _name, string _topic, string _action, int _type, int _duty, int _state) : PortDescription_t(_name, _topic, _action, _type, _duty), state(_state){}
//     int state;
// };

const int BATCH = 0;
const int PRIO = 1;
const int RR = 2;


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
            state.poller = FIRE;
            state.port = IDLE;
            // state.current=nullptr;
            schedPolicy = BATCH;
        }
        Component(vector<PortDescription_t> _portList, int _policy) {
            nextInternal = std::numeric_limits<TIME>::infinity();
            state.poller = FIRE;
            state.port = IDLE;
            // state.current = nullptr;
            schedPolicy = _policy;
            portList = _portList;

            // int count = 0;
            // for (auto& port : portList) _pM.insert(std::pair<string, int>({++count,port.name}));
        } 

        // List of Ports strictly ordered by priority
        vector<PortDescription_t> portList;
        // unordered_map<string, int> _pM; //priority map
        list<ScheduleEntry_t> schedule;

        TIME nextInternal;
        int schedPolicy;

        // state definition
        struct state_type{
            int port;
            int poller;
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
            if (state.port == FIRE) { // Just started a handler
                schedule.pop_front();
                state.port = RUN;
            }
            if (state.poller == FIRE) {
                state.poller = WAIT;
            }
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) { 
            cout<<"COMPONENT EXTERNAL"<< endl;

            // Handle ZMQ Poll Response
            for(const auto &msg : get_messages<typename Component_defs::zmqIn>(mbs)){
                assert(msg.size() > 0 && "Poll response is empty!");
                assert(state.poller == WAIT && "Poll response recvd without polling");
                assert(state.port != RUN && "Poll response recvd while running port");
                for (auto& entry : msg) scheduleMsg(entry);
                state.poller = IDLE;
                state.port = RUN;
            }

            // Handle events from the ports
            for(const auto &msg : get_messages<typename Component_defs::fromPort>(mbs)){
                assert(get_messages<typename Component_defs::fromPort>(mbs).size() == 1 
                    && "More than one fromPort");
                assert(state.port == RUN && "Message from port despite not in RUN!");
                assert(state.poller == IDLE && "Message from port despite idle poller");
                if (schedule.size() > 0) {
                    assert(state.port != FIRE && "Reading schedule while firing port");
                    state.port = FIRE;
                } else {
                    state.port = IDLE;
                    state.poller = FIRE;
                }
                // CHECK ACTION!
            }
            cout << "Schedule Length: " << schedule.size() << endl;
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
            if (state.port == FIRE) {
                // PortCMD_t out = {schedule.front().portName,RUN};
                assert(schedule.size() > 0 && "TRIED FIRING FROM EMPTY SCHEDULE"); 
                auto msg = PortCMD_t(schedule.front().portName, RUN);
                get_messages<typename Component_defs::toPort>(bags).push_back(msg);
            }
            if (state.poller == FIRE) {
                assert(state.port != RUN && "TRIED POLLING WHILE RUNNING PORT");
                get_messages<typename Component_defs::poll>(bags).push_back(1);
            }
            return bags;
        }

        // time_advance function
        TIME time_advance() const {
            cout<<"COMPONENT TIME ADVANCE: STATE "<< state.poller << " " << state.port << endl;
            if ((state.poller == FIRE) != (state.port == FIRE)) {
                return TIME();
            }
            assert(state.poller != FIRE && state.port != FIRE &&
                "FIRING POLLER AND PORT AT SAME TIME");
            assert(!(state.poller == WAIT && state.port == RUN) && 
                "POLLER IS WAITING DESPITE PORT RUNNING");
            
            return std::numeric_limits<TIME>::infinity();
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Component<TIME>::state_type& i) {
            os << ": PORT_";
            switch (i.port){
                case IDLE: os << "IDLE";
                break;
                case RUN: os << "RUN";
                break;
                case FIRE: os << "FIRE";
                break;
            }
            os << " POLLER_";
            switch(i.poller){
                case IDLE: os << "IDLE";
                break;
                case WAIT: os << "WAIT";
                break;
                case FIRE: os << "FIRE";
                break;
            }
            return os;
        }

    private:

        void scheduleMsg(ScheduleEntry_t entry){
            switch (this->schedPolicy){
                case BATCH:
                case PRIO: {
                    auto schedIter = this->schedule.begin();
                    auto prioMark = this->portList.begin();
                    auto prioEnd = this->portList.end();
                    while(prioMark != prioEnd){
                        if (prioMark->name == entry.portName) break;
                        ++prioMark;
                    }
                    assert(prioMark != prioEnd && "prioMark pointing out of list!");
                    for ( ; ; ){
                        // If we're at the end, just stick it in
                        if (schedIter == this->schedule.end()) break;

                        bool found = false;

                        // Am I higher than the current?
                        auto m = prioMark;
                        m++;
                        for ( ; m != prioEnd; m++){
                            if (m->name == schedIter->portName){  //YES
                                found = true;
                                break;
                            }
                        }

                        if (found) break;

                        schedIter++;
                    }
                    this->schedule.insert(schedIter,entry);
                    break;
                }
                case RR:
                default:
                    break;
            }
        }
};
#endif // __COMPONENT_HPP__