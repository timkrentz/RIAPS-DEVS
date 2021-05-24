import riaps_devs as RD

portDesc = RD.PortDescription("Timer1","timerTopic1","timerAction1", 0, 100)

timer1 = RD.makeTimer("Timer1",portDesc)

top = RD.makeCoupled("TOP",
                     RD.Model_list([timer1]),
                     RD.Port_list(),
                     RD.Port_list(),
                     RD.EIC_list(),
                     RD.EOC_list(),
                     RD.IC_list())

runner = RD.Runner(top,0)

print("GGGGGG")
