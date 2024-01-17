# Yeah yeah yeah whatever "we don't need this";
# can't a man make Makefiles (pun intended) for 
# fun these days? Toujours Pur. yeah sure,
# I'm just going to use VSCode's builtin 
# debugger/compiler, but the therapuetic 
# nature of a Makefile is undisputed 

CPPC = g++
CFLAGS = -Wall -ansi -g -Werror -std=c++11
LD = g++

HEADERS = packet_config.hpp
all: uart
uart: uart.o
	${LD} -o uart ${HEADERS} uart.o

uart.o: uart_gen.cpp packet_config.hpp code_gen
	${CPPC} ${CFLAGS} -c -o uart.o uart_gen.cpp

code_gen: sensors.def
	python code_gen.py

clean:
	-del uart.o
	-del uart
	-del *.exe
	-del packet_config.hpp
	-del packet_config.java
