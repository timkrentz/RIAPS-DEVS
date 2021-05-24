#ifndef BOOST_SIMULATION_MESSAGE_HPP
#define BOOST_SIMULATION_MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include "../externals/nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

/*******************************************/
/**************** Message_t ****************/
/*******************************************/
struct Message_t{
  Message_t(){}
  Message_t(int i_packet, int i_bit) : packet(i_packet), bit(i_bit){}
  	int   packet;
  	int   bit;
};

istream& operator>> (istream& is, Message_t& msg);

ostream& operator<<(ostream& os, const Message_t& msg);


/*******************************************/
/**************** PortCMD_t ****************/
/*******************************************/

struct PortCMD_t{
    PortCMD_t(){}
    PortCMD_t(string _name, int _cmd) : name(_name), cmd(_cmd){}
    string name;
    int cmd;
};

istream& operator>> (istream& is, PortCMD_t& msg);

ostream& operator<<(ostream& os, const PortCMD_t& msg);


/*******************************************/
/*********** PortDescription_t *************/
/*******************************************/
struct PortDescription_t {
    PortDescription_t(json obj) : name(obj["name"]), topic(obj["topic"]), action(obj["action"]), type(obj["type"]), duty(obj["duty"]){}
    PortDescription_t(string _name, string _topic, string _action, int _type, int _duty) : name(_name), topic(_topic), action(_action), type(_type), duty(_duty){}
    string name;
    string topic; // Topic that the port IS TRIGGERED BY
    string action; // topic that the port TRIGGERS
    int type; // ZMQ Type: TIMER, PUB, SUB, etc...
    int duty; // Port Handler's "consumed" time, or timer period, in ms
};

/*******************************************/
/************** RIAPSMsg_t *****************/
/*******************************************/
struct RIAPSMsg_t{
    RIAPSMsg_t(){}
    RIAPSMsg_t(string _topic, int _val) : topic(_topic), val(_val){}
    string topic;
    int val;
};

istream& operator>>(istream& is, RIAPSMsg_t& msg);

ostream& operator<<(ostream& os, const RIAPSMsg_t& msg);


/*******************************************/
/*************** PollResult_t **************/
/*************** ScheduleEntry_t ***********/
/*******************************************/
struct ScheduleEntry_t{
    ScheduleEntry_t(){}
    ScheduleEntry_t(string _portName, RIAPSMsg_t _msg) : portName(_portName), msg(_msg){} 
    string portName;
    RIAPSMsg_t msg;
};

typedef vector<ScheduleEntry_t> PollResult_t;

ostream& operator<<(ostream& os, const PollResult_t& msg);

istream& operator>>(istream& is, PollResult_t& msg);

#endif // BOOST_SIMULATION_MESSAGE_HPP