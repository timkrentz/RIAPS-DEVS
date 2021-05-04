//Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>

//Time class header
#include <NDTime.hpp>

//Messages structures
#include "../data_structures/message.hpp"
#include "../utils/const.h"

//Atomic model headers
#include <cadmium/basic_model/pdevs/iestream.hpp> //Atomic model for inputs
#include "../atomics/zmq_context.hpp"
#include "../atomics/component.hpp"
#include "../atomics/port.hpp"
#include "../atomics/timer.hpp"
#include "../atomics/ethernet.hpp"

//C++ libraries
#include <iostream>
#include <string>

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;

/***** Define input port for coupled models *****/

/***** Define output ports for coupled model *****/
struct top_out : public cadmium::out_port<RIAPSMsg_t>{};

int main(){

    // Load model
    //Right now, ZMQ needs both the TIMER and SUB descriptors, so it knows to only create 1 msgQ.
    //The TIMER description just sets up the period
    //The SUB description models the handler execution time and triggers following topics
    PortDescription_t timer1Desc({"timer1","timerTopic1","timerAction1", TIMER, 100});
    PortDescription_t timer1Activity({"timer1Handler","timerTopic1","timerAction1",SUB,25});
    PortDescription_t pubPortDesc({"pub1","timerAction1","MainTopic", PUB, 0});


    /****** Input Reader atomic models instantiation *******************/
    /*
    Timer and ZMQ Models
    */
    shared_ptr<dynamic::modeling::model> timer1 = dynamic::translate::make_dynamic_atomic_model<Timer, TIME>(timer1Desc.name,timer1Desc);
    shared_ptr<dynamic::modeling::model> timer1Handler = dynamic::translate::make_dynamic_atomic_model<Port, TIME>(timer1Activity.name,timer1Activity);

    vector<PortDescription_t> portList;
    portList.push_back(timer1Desc);
    portList.push_back(timer1Activity);
    portList.push_back(pubPortDesc);
    shared_ptr<dynamic::modeling::model> zmq = dynamic::translate::make_dynamic_atomic_model<ZMQContext, TIME>("zmq",portList);

    shared_ptr<dynamic::modeling::model> component = dynamic::translate::make_dynamic_atomic_model<Component, TIME>("component",portList, PRIO);

    shared_ptr<dynamic::modeling::model> ethernet = dynamic::translate::make_dynamic_atomic_model<Ethernet, TIME>("ethernet",10,20);


    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(top_out)};
    dynamic::modeling::Models submodels_TOP = {timer1, timer1Handler, zmq, component, ethernet};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        // dynamic::translate::make_EOC<ZMQContext_defs::toNet,top_out>("zmq")
        dynamic::translate::make_EOC<Ethernet_defs::out,top_out>("ethernet")
    };
    dynamic::modeling::ICs ics_TOP = {
        dynamic::translate::make_IC<Timer_defs::out, ZMQContext_defs::fromTimer>(timer1Desc.name,"zmq"),

        dynamic::translate::make_IC<Component_defs::toPort, Port_defs::in>("component",timer1Activity.name),
        dynamic::translate::make_IC<Port_defs::out, Component_defs::fromPort>(timer1Activity.name, "component"),
        dynamic::translate::make_IC<Port_defs::zmqRead, ZMQContext_defs::portRead>(timer1Activity.name, "zmq"),
        dynamic::translate::make_IC<ZMQContext_defs::toPort, Port_defs::zmqRecv>("zmq",timer1Activity.name),
        dynamic::translate::make_IC<Port_defs::zmqSend, ZMQContext_defs::fromPort>(timer1Activity.name, "zmq"),

        dynamic::translate::make_IC<Component_defs::poll, ZMQContext_defs::poll>("component", "zmq"),
        dynamic::translate::make_IC<ZMQContext_defs::toComp, Component_defs::zmqIn>("zmq","component"),
        dynamic::translate::make_IC<ZMQContext_defs::toNet, Ethernet_defs::in>("zmq","ethernet"),
        

    };

    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*************** Loggers *******************/
    static ofstream out_messages("simulation_results/integration_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    static ofstream out_state("simulation_results/integration_test_output_state.txt");
    struct oss_sink_state{
        static ostream& sink(){          
            return out_state;
        }
    };
    
    using state=logger::logger<logger::logger_state, dynamic::logger::formatter<TIME>, oss_sink_state>;
    using log_messages=logger::logger<logger::logger_messages, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_mes=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_sta=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_state>;

    using logger_top=logger::multilogger<state, log_messages, global_time_mes, global_time_sta>;
    // using logger_top=logger::multilogger<state, log_messages, global_time_mes>;

    /************** Runner call ************************/ 
    dynamic::engine::runner<NDTime, logger_top> r(TOP, {0});
    r.run_until(NDTime("00:00:01:000"));
    return 0;
}
