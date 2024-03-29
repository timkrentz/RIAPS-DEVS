CC=g++
CFLAGS=-std=c++17 -g

INCLUDECADMIUM=-I ../Cadmium-Simulation-Environment/cadmium/include
INCLUDEDESTIMES=-I ../Cadmium-Simulation-Environment/DESTimes/include

#CREATE BIN AND BUILD FOLDERS TO SAVE THE COMPILED FILES DURING RUNTIME
bin_folder := $(shell mkdir -p bin)
build_folder := $(shell mkdir -p build)
results_folder := $(shell mkdir -p simulation_results)

#TARGET TO COMPILE ALL THE TESTS TOGETHER (NOT SIMULATOR)
message.o: data_structures/message.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) data_structures/message.cpp -o build/message.o

# port_test.o: test/port_test.cpp
	# $(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/port_test.cpp -o build/port_test.o

timer_test.o: test/timer_test.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/timer_test.cpp -o build/timer_test.o

# zmq_test.o: test/zmq_test.cpp
	# $(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/zmq_test.cpp -o build/zmq_test.o

# component_test.o: test/component_test.cpp
	# $(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/component_test.cpp -o build/component_test.o

# integration_test.o: test/integration_test.cpp
	# $(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/integration_test.cpp -o build/integration_test.o

# ethernet_test.o: test/ethernet_test.cpp
	# $(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/ethernet_test.cpp -o build/ethernet_test.o

# python_bindings: pybind/pycomponent.cpp
	# $(CC) -O3 -Wall -shared $(CFLAGS) -fPIC $(shell python3 -m pybind --includes) pybind/pycomponent.cpp -o exampleModule$(shell python3-config --extension-suffix)

# assemble_and_run.o:
	# $(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) automate/assemble_and_run.cpp -o build/assemble_and_run.o



# tests: main_subnet_test.o main_sender_test.o main_receiver_test.o port_test.o message.o
tests: timer_test.o message.o
	# $(CC) -g -o bin/PORT_TEST build/port_test.o build/message.o
	$(CC) -g -o bin/TIMER_TEST build/timer_test.o build/message.o
	# $(CC) -g -o bin/ZMQ_TEST build/zmq_test.o build/message.o
	# $(CC) -g -o bin/COMPONENT_TEST build/component_test.o build/message.o
	# $(CC) -g -o bin/INTEGRATION_TEST build/integration_test.o build/message.o
	# $(CC) -g -o bin/ETHERNET_TEST build/ethernet_test.o build/message.o
	# $(CC) -g -o bin/AUTOMATE build/assemble_and_run.o build/message.o
	



#TARGET TO COMPILE ONLY ABP SIMULATOR
# simulator: main_top.o message.o 
	# $(CC) -g -o bin/ABP build/main_top.o build/message.o 
	
#TARGET TO COMPILE EVERYTHING (ABP SIMULATOR + TESTS TOGETHER)
all: tests

#CLEAN COMMANDS
clean: 
	rm -f bin/* build/*