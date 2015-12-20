#include <iostream>
#include <CLOC.hpp>
#include <GitVersion.hpp>

int main( int p_Argc, char **p_ppArgv )
{
	std::cout << "Project: Cipher" << std::endl;
	std::cout << "I hope you enjoy this game which was made in " <<
		CLOC_LINECOUNT << " lines of original code" << std::endl;
	std::cout << "Unfortunately, you will need a Vulkan-capable GPU to run "
		"this game." << std::endl;
	std::cout << "Version information" << std::endl;
	std::cout << "\tVersion:     " << GIT_BUILD_VERSION << std::endl;
	std::cout << "\tHash:        " << GIT_COMMITHASH << std::endl;
	std::cout << "\tCommit date: " << GIT_COMMITTERDATE << std::endl;
	std::cout << "\tBranch:      " << GIT_BRANCH << std::endl;
	std::cout << "\tTag:         " << GIT_TAG_NAME << std::endl;

	return 0;
}

