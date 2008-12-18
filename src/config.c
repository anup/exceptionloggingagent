/*
 * config.c
 *
 *  Created on: Nov 19, 2008
 *      Author: anup.k
 */

#include "TextFileHeader.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

#define KEY_VALUE_SEPARATOR '='
#define MAX_KEY_LENGTH 128

static FILE * File_p;

static char readBuffer[MAX_READ_BUFFER_LENGTH];

static int TEXT_LOGGING;
static int DB_LOGGING;
static int NO_OF_STACKFRAMES;

static char *TEXT_FILE_NAME = NULL;
static char *ERROR_FILE_NAME = NULL;
static char *DB_SERVER_NAME = NULL;
static char *DB_USER_NAME = NULL;
static char *DB_PASSWORD = NULL;
static char *DB_NAME = NULL;

#define DEFAULT_NO_OF_STACKFRAMES 5

static int openFileForReading(const char * fileName)
{
	return (NULL == (File_p = fopen(fileName, READ_MODE)) ? FAILURE : SUCCESS);
}

static void closeFile(void)
{
	fclose(File_p);
	printf("Closed Properties File\n");
}

void shutdownConfig(void)
{
	if (TEXT_FILE_NAME != NULL)
		free(TEXT_FILE_NAME);
	if (ERROR_FILE_NAME != NULL)
		free(ERROR_FILE_NAME);
	if (DB_SERVER_NAME != NULL)
		free(DB_SERVER_NAME);
	if (DB_USER_NAME != NULL)
		free(DB_USER_NAME);
	if (DB_PASSWORD != NULL)
		free(DB_PASSWORD);
	if (DB_NAME != NULL)
		free(DB_NAME);
}

char * getTextLogFileName(void)
{
	return TEXT_FILE_NAME;
}

char * getErrorFileName(void)
{
	return ERROR_FILE_NAME;
}

char * getDBServerName(void)
{
	return DB_SERVER_NAME;
}

char * getDBUserName(void)
{
	return DB_USER_NAME;
}

char * getDBPassword(void)
{
	return DB_PASSWORD;
}

char * getDBName(void)
{
	return DB_NAME;
}

int getTextLogging(void)
{
	return TEXT_LOGGING;
}

int getDBLogging(void)
{
	return DB_LOGGING;
}

int getStackFrameCount(void)
{
	return NO_OF_STACKFRAMES;
}

static void setDefaultValues(void)
{
	NO_OF_STACKFRAMES = DEFAULT_NO_OF_STACKFRAMES;
	TEXT_LOGGING = TRUE;
	DB_LOGGING = TRUE;

	TEXT_FILE_NAME = "LogFile.txt";
	ERROR_FILE_NAME = "Error.txt";
	DB_SERVER_NAME = "localhost";
	DB_USER_NAME = "root";
	DB_PASSWORD = "qwedsa";
	DB_NAME = "exception";
}

static int getKeyFromBuffer(const char * searchKey)
{
	int result = (strlen(searchKey) < strlen(readBuffer)) ? SUCCESS : FAILURE;

	result
			= result
					&& (strncmp(searchKey, readBuffer, strlen(searchKey)) == 0) ? SUCCESS
					: FAILURE;

	result
			= result && (readBuffer[strlen(searchKey)] == KEY_VALUE_SEPARATOR) ? SUCCESS
					: FAILURE;

	return result;
}

static int getValueFromBuffer(const char * key, char * value)
{
	char * valueStart = strchr(readBuffer, KEY_VALUE_SEPARATOR);

	if (valueStart != NULL)
	{
		valueStart++;
		strncpy(value, valueStart, strlen(valueStart) - 1);
		value[strlen(valueStart) - 1] = '\0';
		return SUCCESS;
	}
	else
	{
		printf("Could not find any value for key: %s\n", key);
		return FAILURE;
	}
}

static int getValueForString(const char * key, char * value)
{
	if (fseek(File_p, 0, 0))
	{
		printf("Cannot seek to the beginning of the File\n");
		return FAILURE;
	}

	memset(readBuffer, 0, sizeof(readBuffer));
	while (fgets(readBuffer, MAX_READ_BUFFER_LENGTH, File_p) != NULL)
	{
		if (getKeyFromBuffer(key) == SUCCESS)
		{
			return getValueFromBuffer(key, value);
		}
	}
	return FAILURE;
}

static void readValuesFromFile(void)
{
	char tempValueBuf[MAX_READ_BUFFER_LENGTH];

	if (getValueForString("TEXT_FILE_NAME", tempValueBuf) == SUCCESS)
	{
		if ((TEXT_FILE_NAME = calloc(sizeof(char), strlen(tempValueBuf) + 1))
				!= NULL)
			strcpy(TEXT_FILE_NAME, tempValueBuf);
	}

	if (getValueForString("ERROR_FILE_NAME", tempValueBuf) == SUCCESS)
	{
		if ((ERROR_FILE_NAME = calloc(sizeof(char), strlen(tempValueBuf) + 1))
				!= NULL)
			strcpy(ERROR_FILE_NAME, tempValueBuf);
	}

	if (getValueForString("DB_SERVER_NAME", tempValueBuf) == SUCCESS)
	{
		if ((DB_SERVER_NAME = calloc(sizeof(char), strlen(tempValueBuf) + 1))
				!= NULL)
			strcpy(DB_SERVER_NAME, tempValueBuf);
	}

	if (getValueForString("DB_USER_NAME", tempValueBuf) == SUCCESS)
	{
		if ((DB_USER_NAME = calloc(sizeof(char), strlen(tempValueBuf) + 1))
				!= NULL)
			strcpy(DB_USER_NAME, tempValueBuf);
	}

	if (getValueForString("DB_PASSWORD", tempValueBuf) == SUCCESS)
	{
		if ((DB_PASSWORD = calloc(sizeof(char), strlen(tempValueBuf) + 1))
				!= NULL)
			strcpy(DB_PASSWORD, tempValueBuf);
	}

	if (getValueForString("DB_NAME", tempValueBuf) == SUCCESS)
	{
		if ((DB_NAME = calloc(sizeof(char), strlen(tempValueBuf) + 1)) != NULL)
			strcpy(DB_NAME, tempValueBuf);
	}

	if (getValueForString("TEXT_LOGGING", tempValueBuf) == SUCCESS)
	{
		sscanf(tempValueBuf, "%d", &TEXT_LOGGING);
	}

	if (getValueForString("DB_LOGGING", tempValueBuf) == SUCCESS)
	{
		sscanf(tempValueBuf, "%d", &DB_LOGGING);
	}

	if (getValueForString("NO_OF_STACKFRAMES", tempValueBuf) == SUCCESS)
	{
		sscanf(tempValueBuf, "%d", &NO_OF_STACKFRAMES);
	}
}

int initialize(const char * propFileName)
{
	int result = SUCCESS;
	setDefaultValues();
	printf(
			"%s\n",
			(openFileForReading(propFileName) == SUCCESS ? "Properties File opened"
					: "Failed to Open File"));
	if (File_p == NULL)
	{
		printf("Cannot open Properties File for reading: %s\n", propFileName);
		return FAILURE;
	}
	readValuesFromFile();
	closeFile();
	return result;
}

