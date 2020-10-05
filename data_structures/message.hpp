#ifndef BOOST_SIMULATION_MESSAGE_HPP
#define BOOST_SIMULATION_MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>

using namespace std;

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

struct RIAPSMsg_t{
    RIAPSMsg_t(){}
    RIAPSMsg_t(string _topic, int _val) : topic(_topic), val(_val){}
    string topic;
    int val;
};

istream& operator>> (istream& is, RIAPSMsg_t& msg);

ostream& operator<<(ostream& os, const RIAPSMsg_t& msg);


#endif // BOOST_SIMULATION_MESSAGE_HPP