#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iterator>

#include "message.hpp"
#include "../externals/nlohmann/json.hpp"


using json = nlohmann::json;

// ========== Message_t =========================
ostream& operator<<(ostream& os, const Message_t& msg) {
  os << msg.packet << " " << msg.bit;
  return os;
}

istream& operator>> (istream& is, Message_t& msg) {
  is >> msg.packet;
  is >> msg.bit;
  return is;
}

// ========== PortCMD_t =========================
istream& operator>> (istream& is, PortCMD_t& msg) {
    is >> msg.name;
    is >> msg.cmd;
    return is;
}

ostream& operator<<(ostream& os, const PortCMD_t& msg) {
    os << msg.name << "-" << msg.cmd;
    return os;
}


// ============= RIAPSMsg_t ==========================
istream& operator>> (istream& is, RIAPSMsg_t& msg) {
    is >> msg.topic;
    is >> msg.val;
    return is;
}

ostream& operator<<(ostream& os, const RIAPSMsg_t& msg) {
    os << "On " << msg.topic << ": " << msg.val;
    return os;
}

// ============= PollResult_t ==========================
ostream& operator<<(ostream& os, const PollResult_t& msg) {
    os << "Poll Result: {";
    for (auto entry : msg) {
        os << "PORT: " << entry.portName << " | ";
    }
    os << "}";
    return os;
}

istream& operator>>(istream& is, PollResult_t& msg) {
    std::string tmp;
    is >> tmp;
    assert(tmp == "|" && "Error parsing PollResult stream!");
    for (;;) {
        std::string name;
        is >> name;
        if (name == "|") break;
        RIAPSMsg_t temp;
        is >> temp;
        msg.push_back(ScheduleEntry_t({name,temp}));
    }
    return is;
}