#include "config/parser/ConfParser.hpp"
#include "tests/ConfigTests.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    // Check if we should run tests
    if (argc > 1 && (std::string(argv[1]) == "--test" || std::string(argv[1]) == "-t")) {
        std::cout << "Running configuration tests..." << std::endl;
        ConfigTests::runAllTests();
        return 0;
    }

    // Normal server operation
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::string confFile(argv[1]);
    
    try {
        // Parse the configuration file
        ConfParser parser(confFile);
        
        // Print out the parsed configuration
        std::cout << parser << std::endl;
        
        // TODO: Initialize server with the parsed configuration
        // This will be implemented in the next phase
        
        std::cout << "Configuration loaded successfully. Server initialization comes next." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}