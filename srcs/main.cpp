#include "config/parser/ConfParser.hpp"
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;

	std::string	confFile(argv[1]);
	
	try
	{
		ConfParser parser(confFile);

		std::cout << parser << std::endl;

	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}


	return 0;
}