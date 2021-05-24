#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include "../data_structures/message.hpp"
#include "../atomics/timer.hpp"
#include <NDTime.hpp>
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>
#include <iostream>
#include <string>
#include <fstream>


namespace py = pybind11;

using TIME = NDTime;
using namespace cadmium;

struct top_out : public cadmium::out_port<RIAPSMsg_t>{};


// std::shared_ptr<cadmium::dynamic::modeling::atomic_abstract<TIME>>
// makeTimer(std::string _name, PortDescription_t _desc) {
//     return std::make_shared<cadmium::dynamic::modeling::atomic<Timer, TIME, PortDescription_t>>(_name, std::forward<PortDescription_t>(_desc));
// }


shared_ptr<dynamic::modeling::coupled<TIME>>
makeCoupled(std::string name,
            // dynamic::modeling::Models submodels_TOP,
            std::vector<std::shared_ptr<dynamic::modeling::model>> submodels_TOP,
            dynamic::modeling::Ports iports_TOP, 
            dynamic::modeling::Ports oports_TOP,
            dynamic::modeling::EICs eics_TOP,
            dynamic::modeling::EOCs eocs_TOP,
            dynamic::modeling::ICs ics_TOP) {
    dynaminc::modeling::Models m(submodels_TOP);
    return make_shared<dynamic::modeling::coupled<TIME>>(name, m, //submodels_TOPs, 
                                                        iports_TOP, oports_TOP,
                                                        eics_TOP, eocs_TOP, ics_TOP);
}


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
dynamic::engine::runner<NDTime, logger_top>
makeRunner(shared_ptr<dynamic::modeling::coupled<TIME>> _name){
    return dynamic::engine::runner<TIME, logger_top>(_name, {0});
}

// Make_IC: Internally map (type,type) -> IC types

// PYBIND11_MAKE_OPAQUE(dynamic::modeling::Models);

PYBIND11_MODULE(riaps_devs, m) {
    m.doc() = "Bindings to create RIAPS-DEVS models";
    
    py::class_<PortDescription_t> portDesc(m, "PortDescription");
    portDesc.def(py::init<std::string, std::string, std::string, int, int>());

    py::class_<RIAPSMsg_t>(m, "RIAPSMsg")
        .def(py::init<std::string, int>());
    
    // py::class_<std::vector<std::shared_ptr<cadmium::dynamic::modeling::model>>>(m,"Model_list")
        // .def(py::init<py::list>());
    py::bind_vector<std::shared_ptr<cadmium::dynamic::modeling::model>>(m,"Model_list");
    // py::bind_vector<dynamic::modeling::Models>(m,"Model_list");
    py::class_<dynamic::modeling::Ports>(m,"Port_list");
    py::class_<dynamic::modeling::EICs>(m,"EIC_list");
    py::class_<dynamic::modeling::EOCs>(m,"EOC_list");
    py::class_<dynamic::modeling::ICs>(m,"IC_list");

    py::class_<Timer_defs>(m,"Timer_defs");

    // py::class_<Timer<TIME>>(m, "Timer")
        // .def(py::init<PortDescription_t>());

    py::class_<dynamic::modeling::atomic_abstract<NDTime>>(m,"atomic_abstract");

    m.def("makeTimer", [](std::string _name, PortDescription_t _desc) {
        return dynamic::translate::make_dynamic_atomic_model<Timer, TIME>(_name, _desc);
    });


    // m.def("makeEOC", &makeEOC); 

    m.def("makeCoupled", &makeCoupled);

    py::class_<dynamic::engine::runner<NDTime, logger_top>>(m, "Runner")
        .def(py::init<shared_ptr<dynamic::modeling::coupled<TIME>>,TIME>());

}