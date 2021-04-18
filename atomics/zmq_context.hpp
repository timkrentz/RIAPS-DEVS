#ifndef __ZMQ_HPP__
#define __ZMQ_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <NDTime.hpp>

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
using TIME = NDTime;

template<typename T> struct TimerListEntry_t {
    TimerListEntry_t(){}
    TimerListEntry_t(string _name, int _duty, string _topic){
        name = _name;
        remaining = T({0,0,0,_duty});
        duty = remaining;
        topic = _topic;
    }
    string name;
    T remaining;
    T duty;
    string topic;
};


//Port definition
struct ZMQContext_defs{
    struct fromNet : public in_port<RIAPSMsg_t> {};
    struct toNet : public out_port<RIAPSMsg_t> {};
    struct fromComp : public in_port<RIAPSMsg_t> {};
    struct toComp : public out_port<PollResult_t> {};
    struct fromTimer : public in_port<RIAPSMsg_t> {};
    struct poll : public in_port<int> {};
};

typedef unordered_map<string, queue<RIAPSMsg_t>> NameToMsgQueue_t;
typedef unordered_map<string, vector<string>> Topic2Names_t;
typedef vector<TimerListEntry_t<TIME>> TimerList_t;

template<typename TIME> class ZMQContext{
    public:
        ZMQContext() {
            nextTimerEvent = std::numeric_limits<TIME>::infinity();
            state.pollWaiting = false;
            state.pollAvailable = false;
        }
        ZMQContext(vector<PortDescription_t> _portList) {
            nextTimerEvent = std::numeric_limits<TIME>::infinity();
            state.pollWaiting = false;
            state.pollAvailable = false;
            portList = _portList;
            isPolling = true;

            for (const auto& port : portList) {
                switch (port.type) {
                case TIMER:
                    timerList.push_back(TimerListEntry_t<TIME>(port.name,port.duty,port.topic));
                case SUB:
                    inMsgQ.insert(std::pair<string, queue<RIAPSMsg_t>>(port.name, queue<RIAPSMsg_t>()));
                    break;
                case PUB:
                    outMsgQ.insert(std::pair<string, queue<RIAPSMsg_t>>(port.name, queue<RIAPSMsg_t>()));
                    break;
                default:
                    break;
                }
            }
            

            for (const auto& timer : timerList) {
                if (timer.remaining < nextTimerEvent) {
                    nextTimerEvent = timer.remaining;
                }
            }
        }
        // Map of topics to queues
        Topic2Names_t topic2Names;
        NameToMsgQueue_t inMsgQ;
        NameToMsgQueue_t outMsgQ;
        TIME nextTimerEvent;
        TIME lastExternal;
        vector<PortDescription_t> portList;
        TimerList_t timerList;
        int schedPolicy;
        bool isPolling;


        // state definition
        struct state_type{
            bool pollWaiting;
            bool pollAvailable;
        }; 
        state_type state;
        // ports definition
        using input_ports = std::tuple<typename ZMQContext_defs::fromNet,
                                       typename ZMQContext_defs::fromComp,
                                       typename ZMQContext_defs::fromTimer,
                                       typename ZMQContext_defs::poll>;

        using output_ports = std::tuple<typename ZMQContext_defs::toNet, 
                                        typename ZMQContext_defs::toComp>;

        // internal transition
        void internal_transition() {
            cout<<"ZMQ INTERNAL"<<endl;

            if (state.pollWaiting && state.pollAvailable) {
                state.pollWaiting = false;
            }
            state.pollAvailable = isMsgAvailable();
        }

        // external transition
        void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout<<"ZMQ EXTERNAL"<<endl;
            //These are messages being sent
            for(const auto &msg : get_messages<typename ZMQContext_defs::fromComp>(mbs)){
                //Check for local subscriptions
                toLocalPort(msg);
                //Check for remote subscriptions
                toRemotePort(msg);
            }
            //These are messages being received
            for(const auto &msg : get_messages<typename ZMQContext_defs::fromNet>(mbs)){
                //Check for local subscriptions
                toLocalPort(msg);
            }

            //These are messages from the local timers
            for(const auto &msg : get_messages<typename ZMQContext_defs::fromTimer>(mbs)){
                //Check for local subscriptions
                toLocalPort(msg);
            }

            // Check if any messages are available
            state.pollAvailable = isMsgAvailable();

            // Respond to poll requests
            assert(get_messages<typename ZMQContext_defs::poll>(mbs).size() <= 1 && "More than 1 poll event!");
            for(const auto &msg : get_messages<typename ZMQContext_defs::poll>(mbs)){
                state.pollWaiting = true;
            }

            
        }

        // confluence transition
        void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout<<"ZMQ CONFLUENCE"<<endl;
            internal_transition();
            external_transition(TIME(), std::move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output() const {
            cout<<"ZMQ OUTPUT"<<endl;
            typename make_message_bags<output_ports>::type bags;

            if (state.pollWaiting && state.pollAvailable) {
                // Send one from each Msg Queue
                PollResult_t res;
                for (auto pair : inMsgQ) {
                    if (! pair.second.empty()){
                        res.push_back(ScheduleEntry_t({pair.first,pair.second.front()}));
                        pair.second.pop();
                    }
                }
                get_messages<typename ZMQContext_defs::toComp>(bags).push_back(res);
                return bags;
            }
            return bags;
        }

        // time_advance function
        TIME time_advance() const {
            cout<<"ZMQ TIME ADVANCE"<<endl;
            if (state.pollWaiting && state.pollAvailable) return TIME();
            return std::numeric_limits<TIME>::infinity();
        }

        friend std::ostringstream& operator<<(std::ostringstream& os, const typename ZMQContext<TIME>::state_type& i) {
            os << "POLLING: " << (i.pollWaiting ? "TRUE" : "FALSE") << " AVAILABLE: " << (i.pollAvailable ? "TRUE" : "FALSE");
            return os;
        }

    private:
        bool toLocalPort(const RIAPSMsg_t& msg) {
            bool added = false;
            for (auto& port : this->portList) {
                if (port.topic == msg.topic) {
                    // put into port's in queue
                    auto insearch = this->inMsgQ.find(port.name);
                    if (insearch != this->inMsgQ.end()) {
                        this->inMsgQ[port.name].push(msg);
                        added = true;
                    }
                }
            }
            
            return added;
        }

        void toRemotePort(const RIAPSMsg_t& msg) {
            for (auto& port : this->portList) {
                if (port.topic == msg.topic) {
                    // put into port's out queue
                    auto outsearch = this->outMsgQ.find(port.name);
                    if (outsearch != this->outMsgQ.end()) {
                        this->outMsgQ[port.name].push(msg);
                    }
                }
            }
        }

        bool isMsgAvailable() {
            for (auto pair : this->inMsgQ) {
                if (! pair.second.empty()){
                    return true;
                }
            }
            return false;
        }
};    

#endif // __ZMQ_HPP__