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
#include "../atomics/port.hpp"

//C++ libraries
#include <iostream>
#include <string>

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;

struct top_out : public cadmium::out_port<PortCMD_t>{};
struct top_out_read : public cadmium::out_port<PortCMD_t>{};
struct top_out_send : public cadmium::out_port<RIAPSMsg_t>{};

template<typename T>
class InputReader_Int : public iestream_input<PortCMD_t,T> {
    public:
        InputReader_Int () = default;
        InputReader_Int (const char* file_path) : iestream_input<PortCMD_t,T>(file_path) {}
};

template<typename T>
class InputReader_RIAPSMsg_t : public iestream_input<RIAPSMsg_t,T> {
    public:
        InputReader_RIAPSMsg_t () = default;
        InputReader_RIAPSMsg_t (const char* file_path) : iestream_input<RIAPSMsg_t,T>(file_path) {}
};

int main(){


    /****** Input Reader atomic models instantiation *******************/
    const char * i_input_data_control = "input_data/port_test_input.txt";
    shared_ptr<dynamic::modeling::model> input_reader_con = dynamic::translate::make_dynamic_atomic_model<InputReader_Int, TIME, const char* >("input_reader_con" , move(i_input_data_control));
    const char * i_input_data_msgs = "input_data/port_test_msg_input.txt";
    shared_ptr<dynamic::modeling::model> input_reader_msgs = dynamic::translate::make_dynamic_atomic_model<InputReader_RIAPSMsg_t, TIME, const char* >("input_reader_msgs" , move(i_input_data_msgs));

    /****** Sender atomic model instantiation *******************/
    PortDescription_t port1Desc("port1","MyPort","Topic2",SUB,100);
    shared_ptr<dynamic::modeling::model> port1 = dynamic::translate::make_dynamic_atomic_model<Port, TIME>("port1",port1Desc);

    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(top_out), typeid(top_out_read), typeid(top_out_send)};
    dynamic::modeling::Models submodels_TOP = {input_reader_con, input_reader_msgs, port1};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        dynamic::translate::make_EOC<Port_defs::out,top_out>("port1"),
        dynamic::translate::make_EOC<Port_defs::zmqRead,top_out_read>("port1"),
        dynamic::translate::make_EOC<Port_defs::zmqSend,top_out_send>("port1"),
    };
    dynamic::modeling::ICs ics_TOP = {
        dynamic::translate::make_IC<iestream_input_defs<PortCMD_t>::out,Port_defs::in>("input_reader_con","port1"),
        dynamic::translate::make_IC<iestream_input_defs<RIAPSMsg_t>::out,Port_defs::zmqRecv>("input_reader_msgs","port1"),
    };
    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*************** Loggers *******************/
    // static ofstream out_messages("../simulation_results/sender_test_output_messages.txt");
    static ofstream out_messages("simulation_results/port_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    // static ofstream out_state("../simulation_results/sender_test_output_state.txt");
    static ofstream out_state("simulation_results/port_test_output_state.txt");
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
    r.run_until(NDTime("00:01:00:000"));
    return 0;
}
