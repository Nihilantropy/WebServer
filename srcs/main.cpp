#include "config/parser/ConfParser.hpp"
#include "tests/ConfigTests.hpp"
#include "tests/WebServerTests.hpp"
#include "server/Server.hpp"
#include <iostream>
#include <string>
#include <csignal>

// Global server pointer for signal handling
Server* g_server = NULL;

// Signal handler for CTRL+C
void signalHandler(int signal) {
    if (signal == SIGINT && g_server != NULL) {
        std::cout << "\nReceived SIGINT. Shutting down server..." << std::endl;
        g_server->shutdown();
        exit(0);
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] [config_file]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --test, -t              Run configuration tests" << std::endl;
    std::cout << "  --fulltest, -f          Run comprehensive test suite" << std::endl;
    std::cout << "  --help, -h              Show this help message" << std::endl;
    std::cout << "  --config, -c <file>     Specify configuration file (default: config/webserv.conf)" << std::endl;
}

int main(int argc, char **argv) {
    // Default configuration file
    std::string confFile = "config/webserv.conf";
    bool runConfigTests = false;
    bool runFullTests = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        
        if (arg == "--test" || arg == "-t") {
            runConfigTests = true;
        } else if (arg == "--fulltest" || arg == "-f") {
            runFullTests = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                confFile = argv[++i];
            } else {
                std::cerr << "Error: Missing configuration file path" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        } else {
            // Assume it's a configuration file
            confFile = arg;
        }
    }
    
    // Run configuration tests if requested
    if (runConfigTests) {
        std::cout << "Running configuration tests..." << std::endl;
        ConfigTests::runAllTests();
        return 0;
    }
    
    // Run full test suite if requested
    if (runFullTests) {
        std::cout << "Running comprehensive test suite..." << std::endl;
        WebServerTests::runAllTests();
        return 0;
    }
    
    // Regular server operation mode
    try {
        // Parse the configuration file
        ConfParser parser(confFile);
        
        std::cout << "Configuration loaded successfully." << std::endl;
        std::cout << "Starting server with " << parser.getServers().size() << " virtual host(s)." << std::endl;
        
        // Initialize server with the parsed configuration
        Server server(parser.getServers());
        g_server = &server;
        
        // Set up signal handler for clean shutdown
        signal(SIGINT, signalHandler);
        
        // Initialize and run the server
        server.initialize();
        std::cout << "Server initialization complete. Starting main loop." << std::endl;
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}