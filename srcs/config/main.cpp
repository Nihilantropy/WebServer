#include "ConfParser.hpp"
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	(void)argv;
	
	try
	{
		ConfParser parser("config/webserv.conf");

		parser.parseConfigFile();
		std::cout << parser << std::endl;

	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}


	return 0;
}