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

//Port definition
struct Component_defs{
    struct fromPort : public in_port<RIAPSMsg_t> {};
    struct toPort : public out_port<RIAPSMsg_t> {};
    struct compOut : public out_port<RIAPSMsg_t> {};
    struct compIn : public in_port<RIAPSMsg_t> {};
};

struct PortDescription_t {
    PortDescription_t(string _name, string _topic, string _action, int _type, int _duty) : name(_name), topic(_topic), action(_action), type(_type), duty(_duty){}
    string name;
    string topic; // Topic that the port is TRIGGERED by
    string action;
    int type;
    int duty;
};

struct SchedulerEntry_t : public PortDescription_t {
    using PortDescription_t::PortDescription_t;
    SchedulerEntry_t(string _name, string _topic, string _action, int _type, int _duty, int _state) : PortDescription_t(_name, _topic, _action, _type, _duty), state(_state){}
    int state;
};

typedef unordered_map<string, queue<RIAPSMsg_t>> StringToMsgQueue_t;
typedef unordered_map<string, vector<string>> Topic2Names_t;

template<typename TIME> class Component{
    public:
        Component() {
            nextInternal = std::numeric_limits<TIME>::infinity();
            state.state = IDLE;
            state.current=nullptr;
        }
        Component(vector<PortDescription_t> _portList) {
            nextInternal = std::numeric_limits<TIME>::infinity();
            state.state = IDLE;
            state.current = nullptr;
            for (const auto& port : _portList) {
                state.schedule.push_back(SchedulerEntry_t(port.name,port.topic,port.action,port.type,port.duty,IDLE));
                auto search = topic2NamesMap.find(port.topic);
                if (search == topic2NamesMap.end()) {
                    topic2NamesMap.insert(std::pair<string, vector<string>>(port.topic,vector<string>({port.name})));
                } else {
                    topic2NamesMap[port.topic].push_back(port.name);
                }
                switch (port.type) {
                case TIMER:
                case SUB:
                    state.inMsgQ.insert(std::pair<string, queue<RIAPSMsg_t>>(port.name, queue<RIAPSMsg_t>()));
                case PUB:
                    outMsgQ.insert(std::pair<string, queue<RIAPSMsg_t>>(port.name, queue<RIAPSMsg_t>()));
                    break;
                default:
                    break;
                }
            }
        }
        // List of Ports strictly ordered by priority
        // vector<SchedulerEntry_t> schedule;
        // Map of topics to queues
        Topic2Names_t topic2NamesMap;
        // StringToMsgQueue_t inMsgQ;
        StringToMsgQueue_t outMsgQ; 
        TIME nextInternal;


        // state definition
        struct state_type{
            int state;
            SchedulerEntry_t* current;
            StringToMsgQueue_t inMsgQ;
            vector<SchedulerEntry_t> schedule;  
        }; 
        state_type state;
        // ports definition
        using input_ports = std::tuple<typename Component_defs::fromPort, typename Component_defs::compIn>;
        using output_ports = std::tuple<typename Component_defs::toPort, typename Component_defs::compOut>;

        // internal transition
        void internal_transition() {
            switch (state.state) {
                case RUN: {
                    state.state = FIRE;
                    auto insearch = state.inMsgQ.find(state.current->name);
                    if (insearch != state.inMsgQ.end()) {
                        assert(state.inMsgQ[state.current->name].size() > 0 && "popping from empty queue!");
                        state.inMsgQ[state.current->name].pop();
                        if (state.inMsgQ[state.current->name].size() > 0) {
                            state.current->state = READY;
                        } else {
                            state.current->state = IDLE;
                        }
                    } else {assert(false && "Fired event not found");}
                    nextInternal = TIME();
                    break;
                }
                case FIRE: {
                    //Just fired, check for active port
                    for (auto& entry : state.schedule) {
                        if (entry.state == READY) {
                            state.state = RUN;
                            entry.state = RUN;
                            state.current = &entry;
                            nextInternal = TIME({0,0,0,state.current->duty});
                            return;
                        }
                    }
                    //If no active ports, just wait...
                    state.state = IDLE;
                    state.current = nullptr;
                    nextInternal = std::numeric_limits<TIME>::infinity();
                }
                default:
                    break;
            }
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            // if(get_messages<typename Component_defs::fromPort>(mbs).size()>1) 
            //     assert(false && "one message per time unit");
            // bool added= false;
            // for(const auto &msg : get_messages<typename Component_defs::fromPort>(mbs)){
            //     for (auto& port : schedule) {
            //         if (port.topic != msg.topic) continue;
            //         // put into port's in queue
            //         auto insearch = inMsgQ.find(port.topic);
            //         if (insearch != inMsgQ.end()) inMsgQ[port.topic].push(msg);
            //         auto outsearch = outMsgQ.find(port.topic);
            //         if (outsearch != outMsgQ.end()) outMsgQ[port.topic].push(msg);
            //         port.state = READY;
            //     }
            // }
            // if (added && state.state == IDLE) {
            //     for (auto it = schedule.begin(); it != schedule.end(); ++it){
            //         if (it->state == READY) {
            //             state.state = RUN;
            //             state.current = &*it;
            //         }
            //     }
            // }
            // if(get_messages<typename Component_defs::compIn>(mbs).size()>1)
                // assert(false && "one message per time unit");
            for(const auto &msg : get_messages<typename Component_defs::compIn>(mbs)){
                //Make sure each port on topic gets the message
                vector<string> names = topic2NamesMap[msg.topic];
                for (const auto& name : names) {
                    for (auto& port : state.schedule) {
                        if (port.name == name) {
                            // put into port's in queue
                            auto insearch = state.inMsgQ.find(port.name);
                            if (insearch != state.inMsgQ.end()) state.inMsgQ[port.name].push(msg);
                            port.state = READY;
                            break;
                        }
                    }
                }
            }
            if (state.state == IDLE) {
                for (auto& entry : state.schedule) {
                    if (entry.state == READY) {
                        state.state = RUN;
                        entry.state = RUN;
                        state.current = &entry;
                        nextInternal = TIME({0,0,0,state.current->duty});
                        break;
                    }
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
            if (state.state == FIRE) {
                if (! state.current->action.empty()) {
                    RIAPSMsg_t out = {state.current->action,0};
                    get_messages<typename Component_defs::compOut>(bags).push_back(out);
                }
            }
            return bags;
        }

        // time_advance function
        TIME time_advance() const {  
             return nextInternal;
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename Component<TIME>::state_type& i) {
            os << ": " << i.state;
            for (auto& pr : i.inMsgQ){
                os << "\n" << pr.first << "state: ";
                for (auto& entry : i.schedule) {
                    if (pr.first == entry.name) {
                        os << entry.state;
                    }
                }
                os << "\t Len: " << i.inMsgQ.at(pr.first).size();
            }
            os << "\n";
        return os;
        }

    private:
};    

#endif // __COMPONENT_HPP__