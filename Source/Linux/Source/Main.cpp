#include <iostream>
#include <CLOC.hpp>

int main( int p_Argc, char **p_ppArgv )
{
	std::cout << "Project: Cipher" << std::endl;
	std::cout << "I hope you enjoy this game which was made in " <<
		CLOC_LINECOUNT << " lines of original code" << std::endl;
	std::cout << "Unfortunately, you will need a Vulkan-capable GPU to run "
		"this game." << std::endl;

	return 0;
}

