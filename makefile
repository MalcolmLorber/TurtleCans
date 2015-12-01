C = g++

all:
	$C -std=c++11 bank.cpp -lboost_system -lboost_thread -lboost_regex -o bank.out
	$C -std=c++11 atm.cpp -lboost_system -lboost_thread -lboost_regex -lpthread -o atm.out
	$C -std=c++11 proxy.cpp -lboost_system -lboost_thread -lpthread -o proxy.out

clean: 
	@touch atm.out
	-@rm atm.out 
	@touch bank.out
	-@rm bank.out 
	@touch proxy.out
	-@rm proxy.out
