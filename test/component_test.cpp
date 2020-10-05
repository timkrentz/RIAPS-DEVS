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
#include "../atomics/port.hpp"
#include "../atomics/timer.hpp"
#include "../atomics/component.hpp"

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

template<typename T>
class InputReader_RIAPSMsg : public iestream_input<int,T> {
    public:
        InputReader_RIAPSMsg () = default;
        InputReader_RIAPSMsg (const char* file_path) : iestream_input<RIAPSMsg_t,T>(file_path) {}
};

int main(){


    /****** Input Reader atomic models instantiation *******************/
    // const char * i_input_data_control = "../input_data/component_test_input.txt";
    // shared_ptr<dynamic::modeling::model> input_reader_con = dynamic::translate::make_dynamic_atomic_model<InputReader_RIAPSMsg, TIME, const char* >("input_reader_con" , move(i_input_data_control));

    shared_ptr<dynamic::modeling::model> timerPort1 = dynamic::translate::make_dynamic_atomic_model<Timer, TIME>("TimerPort1","intTimerTopic",1000);
    vector<PortDescription_t> component1Ports;
    component1Ports.push_back(PortDescription_t{"TimerHandler1","intTimerTopic","MyTopic",TIMER, 100});
    shared_ptr<dynamic::modeling::model> component1 = dynamic::translate::make_dynamic_atomic_model<Component, TIME>("component1",component1Ports);

    vector<PortDescription_t> component2Ports;
    component2Ports.push_back(PortDescription_t{"SubHandler","MyTopic","DONE",SUB,100});
    shared_ptr<dynamic::modeling::model> component2 = dynamic::translate::make_dynamic_atomic_model<Component, TIME>("component2",component2Ports);

    vector<PortDescription_t> component3Ports;
    component3Ports.push_back(PortDescription_t{"SubHandler","MyTopic","DONE",SUB,200});
    shared_ptr<dynamic::modeling::model> component3 = dynamic::translate::make_dynamic_atomic_model<Component, TIME>("component3",component3Ports);

    shared_ptr<dynamic::modeling::model> timerPort2 = dynamic::translate::make_dynamic_atomic_model<Timer, TIME>("TimerPort2","intTimerTopic",1000);
    vector<PortDescription_t> component4Ports;
    component4Ports.push_back(PortDescription_t{"TimerHandler2","intTimerTopic","MyTopic",TIMER, 100});
    shared_ptr<dynamic::modeling::model> component4 = dynamic::translate::make_dynamic_atomic_model<Component, TIME>("component4",component4Ports);

    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(top_out)};
    dynamic::modeling::Models submodels_TOP = {component1,timerPort1,timerPort2,component4,component2,component3};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        dynamic::translate::make_EOC<Component_defs::compOut,top_out>("component2")
    };
    dynamic::modeling::ICs ics_TOP = {
        dynamic::translate::make_IC<Timer_defs::out,Component_defs::compIn>("TimerPort1","component1"),
        dynamic::translate::make_IC<Timer_defs::out,Component_defs::compIn>("TimerPort2","component4"),
        dynamic::translate::make_IC<Component_defs::compOut,Component_defs::compIn>("component1","component2"),
        dynamic::translate::make_IC<Component_defs::compOut,Component_defs::compIn>("component1","component3"),
        dynamic::translate::make_IC<Component_defs::compOut,Component_defs::compIn>("component4","component2"),
        dynamic::translate::make_IC<Component_defs::compOut,Component_defs::compIn>("component4","component3")

    };
    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*************** Loggers *******************/
    static ofstream out_messages("simulation_results/component_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    static ofstream out_state("simulation_results/component_test_output_state.txt");
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

    /************** Runner call ************************/ 
    dynamic::engine::runner<NDTime, logger_top> r(TOP, {0});
    r.run_until(NDTime("00:00:05:000"));
    return 0;
}
