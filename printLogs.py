import argparse
from os.path import join
from os import getcwd
import re

cadmiumTimeRegex = re.compile("\d\d:\d\d:\d\d:\d\d\d\n")

class MyReader(object):
    def __init__(self, logfile):
        self.logfile = logfile
        self.curr = None
        self.next = None
        self.done = False

    def __enter__(self):
        self.fd = open(join(getcwd(),"simulation_results",self.logfile),'r')

    def __exit__(self, *_):
        self.fd.close()

    def nextTime(self, ):
        self.lines = []
        if self.done:
            return None
        if self.curr is None:
            self.curr = self.fd.readline()[:-1]
        else:
            self.curr = self.next
        while True:
            l = self.fd.readline()
            if l == None:
                self.next = None
                self.done = True
                break
            if cadmiumTimeRegex.fullmatch(l):
                self.next = l[:-1]
                break
            else:
                self.lines.append(l[:-1])
        return self.curr
            

parser = argparse.ArgumentParser(description='Interleave Cadmium Outputs.')
parser.add_argument('test', type=str, help='name of test to read')
args = parser.parse_args()

msgs = MyReader(args.test+"_test_output_messages.txt")
states = MyReader(args.test+"_test_output_state.txt")

with msgs, states:
    msgs.nextTime() # Burn the first entry...
    ts_m = msgs.nextTime()
    ts_s = states.nextTime()
    assert(ts_m == ts_s,"Different timestamps!")
    
    print(ts_m)
    msgIter = iter(msgs.lines)
    stateIter = iter(states.lines)
    while True:
        m = next(msgIter)
        print(m)
        temp = next(msgIter)
        comp = temp.split()[:-1]
        while True:
            s = next(stateIter)
            print(s)
            if comp in s.split()[3]:
                break