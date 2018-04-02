#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_METHOD_LENGTH 7
#define MAX_VERSION 5
#define MAX_HOST_LENGTH 1000
#define MAX_REQUEST_LENGTH 1000
#define SPACE 32
#define CR 13
#define CHAR_LENGTH (sizeof(char))

char *get_port(void);
char *get_method(void);
char *get_host(void);
char *get_request_target(void);
char *get_spaced_info(int l, int delimeter);
char *get_version(void);
int state_http(int expected);
char clean(void);

int main(int argc, char **argv[])
{

	char *method = get_method();			 //ok
	char *req_target = get_request_target(); //ok
	char *version = get_version();
	//char * host = get_host();
	//char * port = get_port();

	printf("%s\t%s\t%s\t", method, req_target, version);
}

char *get_port(void)
{
}

char *get_version(void)
{
	char c = clean();
	if (c == 'H')
	{
		int state = (int)state_http('T');
		if (state != 0)
			return NULL;
		return get_spaced_info(MAX_VERSION, CR);
	}
	return NULL;
}

char *get_method(void)
{
	return get_spaced_info(MAX_METHOD_LENGTH, SPACE);
}

char *get_request_target(void)
{
	char c = clean();
	// Busco la primera aparición de '/'. Si aparecen
	// 2 consecutivas puede tratarse de un 'http://'.
	while (c != '/')
	{
		c = getchar();
	}
	c = getchar();
	if (c == '/')
		c = getchar();
	while (c != '/')
	{
		c = getchar();
	}
	return get_spaced_info(MAX_REQUEST_LENGTH, SPACE);
}

// "HTTP/" PARSER
int state_http(int expected)
{
	char c = getchar();
	if (c == expected)
	{
		switch (c)
		{
		case 'T':
			c = getchar();
			if (c == 'T')
			{
				return state_http('P');
			}
			else
			{
				return -1;
			}

		case 'P':
			return state_http('/');

		case '/':
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

char clean(void)
{
	char c = getchar();
	while (c == SPACE)
	{
		c = getchar();
	}
	return c;
}

char *get_spaced_info(int l, int delimeter)
{
	char ans[l];
	char c = clean();
	int i = 0;
	while (c != delimeter)
	{
		if (i <= l - 1)
		{
			ans[i] = c;
			c = getchar();
			i++;
		}
		else
		{
			return NULL;
		}
	}
	char *info = malloc(i * CHAR_LENGTH);
	memcpy(info, ans, i * CHAR_LENGTH);
	return info;
}
