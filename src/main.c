#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core.h"
#include "compiler.h"
#include "memory.h"
#include "hash_table.h"

char* getInput()
{
	size_t length = 128;
	size_t curr = 0;
	char* line = GROW_ARRAY(char, NULL, 0, length), *pOffset = line;
	for(;;)
	{
		char c = fgetc(stdin);
		if(c == EOF) break;
		
		if(++curr == length)
		{
			curr = length;
			char* extend = GROW_ARRAY(char, pOffset, length, GROW_CAPACITY(length));
			length = GROW_CAPACITY(length);
			line = extend + (line - pOffset);
			pOffset = extend;
		}
		*line++ = c;
	}
	*line = '\0';
	return pOffset;
}

static void repl()
{
	printf("To execute your code (assuming windows) type CTRL + Z and start a newline.\n");
	for(;;)
	{
		printf("> ");

		//test string: class Monad m { m a unit(a val); m b bind(a val, a -> b func) { return ret(func(val)); } }
		char* line = getInput();

		bool result = compile(line);

		free(line);
		printf("%s\n", result ? "COMPILE_SUCCESS" : "COMPILE_FAILURE");
	}
}

static char* readFile(const char* path)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(fileSize + 1);
	if (buffer == NULL)
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