#ifndef __CONST_HPP__
#define __CONST_HPP__

const int IDLE = 0;
const int RUN = 1;
const int FIRE = 2;
const int READY = 3;

const int TIMER = 0;
const int PUB = 1;
const int SUB = 2;

#endif //__CONST_HPP__

/*
00:00:00:000
State for model component1 is : 0
TimerHandlerstate: 0     Len: 0
State for model TimerPort is state: 1

00:00:01:000
State for model component1 is : 0
TimerHandlerstate: 0     Len: 0
State for model TimerPort is state: 2
[Timer_defs::out: {}] generated by model TimerPort                                                    

00:00:01:000
State for model component1 is : 1
TimerHandlerstate: 1     Len: 0
State for model TimerPort is state: 1
[Timer_defs::out: {On intTimerTopic: 1000}] generated by model TimerPort                              

00:00:01:100
State for model component1 is : 2
TimerHandlerstate: 3     Len: 18446744073709551615
State for model TimerPort is state: 1
[Component_defs::toPort: {}, Component_defs::compOut: {}] generated by model component1               

00:00:01:100
State for model component1 is : 1
TimerHandlerstate: 1     Len: 18446744073709551615
State for model TimerPort is state: 1
[Component_defs::toPort: {}, Component_defs::compOut: {On MyTopic: 0}] generated by model component1  

00:00:01:200
State for model component1 is : 2
TimerHandlerstate: 3     Len: 18446744073709551614
State for model TimerPort is state: 1
[Component_defs::toPort: {}, Component_defs::compOut: {}] generated by model component1               
[Component_defs::toPort: {}, Component_defs::compOut: {On MyTopic: 0}] generated by model component1  

00:00:01:200
State for model component1 is : 1
TimerHandlerstate: 1     Len: 18446744073709551614
State for model TimerPort is state: 1



00:00:00:000                                                                                          
00:00:01:000                                                                                          
00:00:01:000                                                                                          
00:00:01:100                                                                                          
00:00:01:100                                                                                          
00:00:01:200                                                                                          
00:00:01:200                                                                                          
00:00:01:300                                                                                          
[Component_defs::toPort: {}, Component_defs::compOut: {}] generated by model component1
*/