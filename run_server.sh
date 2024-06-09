#!/bin/bash

# Compile source files into object files
g++ -std=c++17 -Wall -Wextra -pedantic -pthread -c Server.cpp -o Server.o
g++ -std=c++17 -Wall -Wextra -pedantic -pthread -c exchange.cpp -o exchange.o
g++ -std=c++17 -o client client.cpp -lboost_system -lboost_thread -pthread

# Check if compilation of source files was successful
if [ $? -eq 0 ]; then
    echo "Compilation of source files successful."
    
    # Link object files into executable
    g++ -std=c++17 -Wall -Wextra -pedantic -pthread -o Server_executable Server.o exchange.o -lboost_system -lboost_thread

    # Check if linking was successful
    if [ $? -eq 0 ]; then
        echo "Linking successful. Starting server..."
        
        # Run the server executable
        ./Server_executable
    else
        echo "Linking failed. Please check the source code and object files for errors."
    fi
else
    echo "Compilation of source files failed. Please check the source code for errors."
fi
