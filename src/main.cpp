#include <iostream>

#include "compiler.h"

static void repl()
{
	std::cout << "To execute your code, type '-eval' on a new line after the end of your block.\n";
	std::string source;
	
	std::cout << "> ";
	for(;;)
	{

		/*test string: 
		class Monad m {
			m a unit(a val);
			m b bind(a val, a -> b func) { return ret(func(val)); }
		}
		*/
		std::string line;
		std::getline(std::cin, line);
		if(line.compare("-eval") == 0)
		{
		bool result = compile(source);
		std::cout << "\n" << (result ? "COMPILE_SUCCESS" : "COMPILE_FAILURE") << std::endl;
		std::cout << "> ";
		}
		else
		{
			source.append(line);
			source.append("\n");
		}
	}
}

static char* readFile(const char* path)
{
	FILE* file = fopen(path, "rb");
	if (file == nullptr)
	{
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(fileSize + 1);
	if (buffer == nullptr)
	{
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}

	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize)
	{
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}

	buffer[bytesRead] = '\0';

	fclose(file);
	return buffer;
}

static void runFile(const char* path)
{
	char* source = readFile(path);
	bool result = compile(source);
	printf("%s", result ? "COMPILE_SUCCESS" : "COMPILE_FAILURE");
	free(source);
}

int main(int argc, const char* argv[])
{
	if(argc == 1)
	{
		repl();
	}
	else if (argc == 2)
	{
		runFile(argv[1]);
	}
	else{
		fprintf(stderr, "Usage: ctc [path] \n");
		exit(64);
	}
	return (0);
}