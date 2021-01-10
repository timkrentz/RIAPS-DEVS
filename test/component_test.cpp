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
// #include "../atomics/port.hpp"
// #include "../atomics/timer.hpp"
#include "../atomics/component.hpp"
// #include "../atomics/zmq_context.hpp"

//C++ libraries
#include <iostream>
#include <string>

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;

/***** Define input port for coupled models *****/

/***** Define output ports for coupled model *****/
struct toPort : public cadmium::out_port<PortCMD_t>{};
struct poll : public cadmium::out_port<int>{};

template<typename T>
class InputReader_PollResult_t : public iestream_input<PollResult_t,T> {
    public:
        InputReader_PollResult_t () = default;
        InputReader_PollResult_t (const char* file_path) : iestream_input<PollResult_t,T>(file_path) {}
};

int main(){


    /****** Input Reader atomic models instantiation *******************/
    const char * i_input_data_control = "input_data/component_input_test.txt";
    shared_ptr<dynamic::modeling::model> input_reader = dynamic::translate::make_dynamic_atomic_model<InputReader_PollResult_t, TIME, const char* >("input_reader" , move(i_input_data_control));
    /*
    Calibrate
    */

    // Load model
    PortDescription_t timer1Desc({"timer1","timerTopic1","timerAction1", TIMER, 100});
    // PortDescription_t timer2Desc({"timer2","timerTopic2","timerAction2", TIMER, 300});
    PortDescription_t subDesc1({"sub1","subTopic1","pub1", SUB, 50});
    PortDescription_t subDesc2({"sub2","subTopic2","pub2", SUB, 60});

    vector<PortDescription_t> portList;
    portList.push_back(timer1Desc);
    // portList.push_back(timer2Desc);
    portList.push_back(subDesc1);
    portList.push_back(subDesc2);


    shared_ptr<dynamic::modeling::model> component = dynamic::translate::make_dynamic_atomic_model<Component, TIME>("component",portList, PRIO);

    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(toPort),typeid(poll)};
    dynamic::modeling::Models submodels_TOP = {input_reader, component};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        dynamic::translate::make_EOC<Component_defs::toPort,toPort>("component"),
        dynamic::translate::make_EOC<Component_defs::poll,poll>("component"),
    };
    dynamic::modeling::ICs ics_TOP = {
        dynamic::translate::make_IC<iestream_input_defs<PollResult_t>::out,Component_defs::zmqIn>("input_reader","component"),
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
    // using global_time_sta=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_messages>;

    using logger_top=logger::multilogger<state, log_messages, global_time_mes, global_time_sta>;

    /************** Runner call ************************/ 
    dynamic::engine::runner<NDTime, logger_top> r(TOP, {0});
    r.run_until(NDTime("00:00:01:000"));
    return 0;
}
