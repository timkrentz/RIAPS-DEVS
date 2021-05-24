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

//Atomic model headers
#include <cadmium/basic_model/pdevs/iestream.hpp> //Atomic model for inputs
#include "../atomics/timer.hpp"
#include "../utils/const.h"

//C++ libraries
#include <iostream>
#include <string>
#include "../externals/nlohmann/json.hpp"

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;
using json = nlohmann::json;

using TIME = NDTime;

shared_ptr<dynamic::modeling::model>
makeTimer(std::string name, int duty) {
    return dynamic::translate::make_dynamic_atomic_model<Timer, TIME>(
        name, PortDescription_t(name, name+"Topic", name+"Action", TIMER, duty)});
}

shared_ptr<dynamic::modeling::model>
makeTimerHandler(std::string name, int duty) {
    return dynamic::translate::make_dynamic_atomic_model<Port, TIME>(
        name, PortDescription_t(name+"Handler", name+"Topic", name+"Action", SUB, duty)});
}


shared_ptr<dynamic::modeling::coupled<TIME>>
makeComponent(json main, std::string component){

    shared_ptr<dynamic::modeling::model> timer1;
    timer1 = dynamic::translate::make_dynamic_atomic_model<Timer, TIME>(myTimer.name, myTimer);



    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {};
    dynamic::modeling::Models submodels_TOP = {timer1};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        // dynamic::translate::make_EOC<Timer_defs::out,top_out>("Timer1")
    };
    dynamic::modeling::ICs ics_TOP = {
        // dynamic::translate::make_IC<iestream_input_defs<int>::out,Timer_defs::in>("input_reader_con","port1"),
    };

    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );
}

int main(){
    json p = {
        {"name", "Timer1"},
        {"topic", "timerTopic1"},
        {"action", "timerAction1"},
        {"type", 0},
        {"duty", 100}
    };

    PortDescription_t myTimer(p);

    /****** MAKE A COUPLED MODEL PER ACTOR **********/


    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {};
    dynamic::modeling::Models submodels_TOP = {timer1};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        // dynamic::translate::make_EOC<Timer_defs::out,top_out>("Timer1")
    };
    dynamic::modeling::ICs ics_TOP = {
        // dynamic::translate::make_IC<iestream_input_defs<int>::out,Timer_defs::in>("input_reader_con","port1"),
    };

    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*************** Loggers *******************/
    static ofstream out_messages("simulation_results/automated_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    static ofstream out_state("simulation_results/automated_test_output_state.txt");
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
    r.run_until(NDTime("00:00:01:000"));
    return 0;
}