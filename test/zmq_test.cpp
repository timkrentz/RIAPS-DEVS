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
#include "../atomics/timer.hpp"

//C++ libraries
#include <iostream>
#include <string>

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;

/***** Define input port for coupled models *****/

/***** Define output ports for coupled model *****/
struct top_out : public cadmium::out_port<PollResult_t>{};

template<typename T>
class InputReader_Int_t : public iestream_input<int,T> {
    public:
        InputReader_Int_t () = default;
        InputReader_Int_t (const char* file_path) : iestream_input<int,T>(file_path) {}
};

template<typename T>
class InputReader_RIAPSMsg_t : public iestream_input<RIAPSMsg_t,T> {
    public:
        InputReader_RIAPSMsg_t () = default;
        InputReader_RIAPSMsg_t (const char* file_path) : iestream_input<RIAPSMsg_t,T>(file_path) {}
};

template<typename T>
class InputReader_PortCMD_t : public iestream_input<PortCMD_t,T> {
    public:
        InputReader_PortCMD_t () = default;
        InputReader_PortCMD_t (const char* file_path) : iestream_input<PortCMD_t,T>(file_path) {}
};

int main(){

    // Load model
    PortDescription_t timer1Desc({"timer1","timerTopic1","timerAction1", TIMER, 100});
    PortDescription_t timer2Desc({"timer2","timerTopic2","timerAction2", TIMER, 300});
    PortDescription_t pubPortDesc({"pub1","timerAction1","MainTopic", PUB, 0});


    /****** Input Reader atomic models instantiation *******************/
    const char * i_input_data_control = "input_data/zmq_input_test.txt";
    shared_ptr<dynamic::modeling::model> input_reader = dynamic::translate::make_dynamic_atomic_model<InputReader_Int_t, TIME, const char* >("input_reader" , move(i_input_data_control));
    const char * i_input_data_port_read = "input_data/zmq_input_test_port.txt";
    shared_ptr<dynamic::modeling::model> input_reader_port = dynamic::translate::make_dynamic_atomic_model<InputReader_PortCMD_t, TIME, const char* >("input_reader_port" , move(i_input_data_port_read));
    const char * i_input_port_send = "input_data/zmq_test_local_send.txt";
    shared_ptr<dynamic::modeling::model> local_port_reader = dynamic::translate::make_dynamic_atomic_model<InputReader_RIAPSMsg_t, TIME, const char* >("local_port_reader" , move(i_input_port_send));

    /*
    Timer and ZMQ Models
    */
    shared_ptr<dynamic::modeling::model> timer1 = dynamic::translate::make_dynamic_atomic_model<Timer, TIME>(timer1Desc.name,timer1Desc);
    shared_ptr<dynamic::modeling::model> timer2 = dynamic::translate::make_dynamic_atomic_model<Timer, TIME>(timer2Desc.name,timer2Desc);


    vector<PortDescription_t> portList;
    portList.push_back(timer1Desc);
    portList.push_back(timer2Desc);
    portList.push_back(pubPortDesc);
    shared_ptr<dynamic::modeling::model> zmq = dynamic::translate::make_dynamic_atomic_model<ZMQContext, TIME>("zmq",portList);

    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(top_out)};
    dynamic::modeling::Models submodels_TOP = {input_reader,input_reader_port,local_port_reader,zmq,timer1,timer2};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        dynamic::translate::make_EOC<ZMQContext_defs::toComp,top_out>("zmq")
    };
    dynamic::modeling::ICs ics_TOP = {
        dynamic::translate::make_IC<iestream_input_defs<int>::out,ZMQContext_defs::poll>("input_reader","zmq"),
        dynamic::translate::make_IC<iestream_input_defs<PortCMD_t>::out,ZMQContext_defs::portRead>("input_reader_port","zmq"),
        dynamic::translate::make_IC<iestream_input_defs<RIAPSMsg_t>::out,ZMQContext_defs::fromPort>("local_port_reader","zmq"),
        dynamic::translate::make_IC<Timer_defs::out, ZMQContext_defs::fromTimer>(timer1Desc.name,"zmq"),
        dynamic::translate::make_IC<Timer_defs::out, ZMQContext_defs::fromTimer>(timer2Desc.name,"zmq"),
    };

    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*************** Loggers *******************/
    static ofstream out_messages("simulation_results/zmq_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    static ofstream out_state("simulation_results/zmq_test_output_state.txt");
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
