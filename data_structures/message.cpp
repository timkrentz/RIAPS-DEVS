#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

#include "message.hpp"

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