/* JVMTIAgent.c
 *
 * The agent file defining the Java(tm) Virtual Machine Tool Interface callbacks
 * for handling Exception events.
 *
 * Author: <anup.k@directi.com>
 *
 * Copyright (C) 2008 Directi
 *
 * This file is part of ExceptionLoggingAgent.
 *
 * ExceptionLoggingAgent is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * ExceptionLoggingAgent is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "header.h"
#include "globalHeader.h"

static jrawMonitorID GlobalAgentLock;
static int NO_OF_STACKFRAMES;
static int TEXT_LOGGING;
static int DB_LOGGING;

void logEvent(EXCEPTION_LOG_STRUCTURE * excp_p)
{
	if (TEXT_LOGGING)
		logMessage(excp_p);
	if (DB_LOGGING)
		logMessageInDB(excp_p);
}

/* Every JVMTI interface returns an error code, which should be checked
 *   to avoid any cascading errors down the line.
 *   The interface GetErrorName() returns the actual enumeration constant
 *   name, making the error messages much easier to understand.
 */
static void checkJvmtiError(jvmtiEnv *jvmti, jvmtiError errNum, const char *str)
{
	char tempBuf[500];
	memset(tempBuf, 0, sizeof(tempBuf));
	if (errNum != JVMTI_ERROR_NONE)
	{
		char *errStr = NULL;
		(void) (*jvmti)->GetErrorName(jvmti, errNum, &errStr);
		if (errStr != NULL)
		{
			sprintf(tempBuf, "%s: ERROR: JVMTI: %d(%s)%c", (str == NULL ? ""
					: str), errNum, (errStr == NULL ? "Unknown" : errStr), '\0');
			logError(tempBuf);
			(*jvmti)->Deallocate(jvmti, (unsigned char *) errStr);
		}
	}
}

static void enterCriticalSection(jvmtiEnv *jvmti)
{
	jvmtiError error;
	error = (*jvmti)->RawMonitorEnter(jvmti, GlobalAgentLock);
	checkJvmtiError(jvmti, error, "Cannot enter with raw monitor: ");
}

static void exitCriticalSection(jvmtiEnv *jvmti)
{
	jvmtiError error;
	error = (*jvmti)->RawMonitorExit(jvmti, GlobalAgentLock);
	checkJvmtiError(jvmti, error, "Cannot exit with raw monitor: ");
}

static int getLineNumberForMethod(jvmtiEnv *jvmti, jvmtiFrameInfo methodFrame)
{
	jvmtiError error;
	jvmtiLineNumberEntry* table = NULL;
	jint entry_count = 0;
	int lineNumberCount;
	int lineNumber = -1;
	jlocation prevLocationId = -1;
	if (methodFrame.location == -1)
		return lineNumber;
	error = (*jvmti)->GetLineNumberTable(jvmti, methodFrame.method,
			&entry_count, &table);

	checkJvmtiError(jvmti, error, "GetLineNumberTable: ");
	if (error == JVMTI_ERROR_NONE)
	{
		prevLocationId = table[0].start_location;
		for (lineNumberCount = 1; lineNumberCount < entry_count; lineNumberCount++)
		{
			if ((table[lineNumberCount].start_location > methodFrame.location)
					&& (methodFrame.location >= prevLocationId))
			{
				lineNumber = table[lineNumberCount - 1].line_number;
				break;
			}
			prevLocationId = table[lineNumberCount].start_location;
		}
		if (lineNumberCount == entry_count)
		{
			if (entry_count == 1)
				lineNumber = table[lineNumberCount - 1].line_number;
			else
				lineNumber = -1;
		}
	}
	return lineNumber;
}

static char * deallocateJVMMemoryAndAllocateMemoryFromHeap(jvmtiError error,
		jvmtiEnv *jvmti, char * str)
{
	char * resultStr = NULL;
	int size = 0;
	if ((error == JVMTI_ERROR_NONE) && (str != NULL))
	{
		size = strlen(str);
		resultStr = malloc(size + 1);
		if (resultStr == NULL)
		{
			logError(
					"DJVMMemoryAndAllocateMemoryFromHeap: Could not allocate memory");
		}
		else
		{
			strcpy(resultStr, str);
			resultStr[size] = '\0';
		}
		(*jvmti)->Deallocate(jvmti, (unsigned char *) str);
	}
	return resultStr;
}

static void getMethodInfo(jvmtiEnv *jvmti, jvmtiFrameInfo frameInfo,
		char ** methodName_p, char ** methodSig_p, char ** className_p,
		int * lineNumber_p)
{
	jvmtiError error;
	jclass declaring_class;
	char * tempMethodName = NULL;
	char * tempMethodSig = NULL;
	char * tempClassName = NULL;

	*lineNumber_p = getLineNumberForMethod(jvmti, frameInfo);

	*methodName_p = NULL;
	*methodSig_p = NULL;
	error = (*jvmti)->GetMethodName(jvmti, frameInfo.method, &tempMethodName,
			&tempMethodSig, NULL);
	checkJvmtiError(jvmti, error, "GetMethodName: ");

	*methodName_p = deallocateJVMMemoryAndAllocateMemoryFromHeap(error, jvmti,
			tempMethodName);
	*methodSig_p = deallocateJVMMemoryAndAllocateMemoryFromHeap(error, jvmti,
			tempMethodSig);

	*className_p = NULL;
	if (error == JVMTI_ERROR_NONE)
	{
		error = (*jvmti)->GetMethodDeclaringClass(jvmti, frameInfo.method,
				&declaring_class);
		checkJvmtiError(jvmti, error, "GetMethodDeclaringClass: ");

		if (error == JVMTI_ERROR_NONE)
		{
			error = (*jvmti)->GetClassSignature(jvmti, declaring_class,
					&tempClassName, NULL);
			checkJvmtiError(jvmti, error, "GetClassSignature: ");

			*className_p = deallocateJVMMemoryAndAllocateMemoryFromHeap(error,
					jvmti, tempClassName);
		}
	}
}

static void getThreadName(jvmtiEnv *jvmti, jthread thread, char ** threadName_p)
{
	jvmtiError error;
	jvmtiThreadInfo info;
	info.name = NULL;

	*threadName_p = NULL;
	error = (*jvmti)->GetThreadInfo(jvmti, thread, &info);
	checkJvmtiError(jvmti, error, "GetThreadInfo: ");
	*threadName_p = deallocateJVMMemoryAndAllocateMemoryFromHeap(error, jvmti,
			info.name);
}

static void captureStackTrace(jvmtiEnv *jvmti, jthread thread,
		EXCEPTION_LOG_STRUCTURE * excp_p)
{
	jvmtiError error;
	jvmtiFrameInfo frames[NO_OF_STACKFRAMES];
	const jint start_depth = 0;
	int i = 0;

	getThreadName(jvmti, thread, &(excp_p->threadName));

	error = (*jvmti)->GetStackTrace(jvmti, thread, start_depth,
			NO_OF_STACKFRAMES, frames, (jint *) &(excp_p->frameCount));
	checkJvmtiError(jvmti, error, "GetStackTrace: ");

	if (error == JVMTI_ERROR_NONE && excp_p->frameCount >= 1)
	{
		excp_p->stackTrace = malloc(sizeof(STACKTRACE_STRUCTURE)
				* excp_p->frameCount);
		if (excp_p->stackTrace == NULL)
		{
			logError(
					"captureStackTrace: Could not allocate memory for getting stackTrace");
			return;
		}

		for (i = 0; i < excp_p->frameCount; i++)
		{
			getMethodInfo(jvmti, frames[i],
					&(excp_p->stackTrace[i].methodName),
					&(excp_p->stackTrace[i].methodSig),
					&(excp_p->stackTrace[i].className),
					&(excp_p->stackTrace[i].lineNumber));
		}
	}
}

static void initializeExceptionPointer(EXCEPTION_LOG_STRUCTURE * excp_p)
{
	excp_p->timeStamp = getTimeStamp();
	excp_p->catchClassName = NULL;
	excp_p->catchMethodSig = NULL;
	excp_p->catchMethodLineNumber = -1;
	excp_p->catchMethodname = NULL;
	excp_p->class = NULL;
	excp_p->frameCount = 0;
	excp_p->msg = NULL;
	excp_p->stackTrace = NULL;
	excp_p->threadName = NULL;
	excp_p->sourceFileName = NULL;
}

static void deallocateExceptionPointer(void * msg)
{
	EXCEPTION_LOG_STRUCTURE * excp_p = (EXCEPTION_LOG_STRUCTURE *) msg;
	int count;

	if (excp_p->class != NULL)
		free(excp_p->class);
	if (excp_p->msg != NULL)
		free(excp_p->msg);
	if (excp_p->threadName != NULL)
		free(excp_p->threadName);
	if (excp_p->sourceFileName != NULL)
		free(excp_p->sourceFileName);
	if (excp_p->catchClassName != NULL)
		free(excp_p->catchClassName);
	if (excp_p->catchMethodSig != NULL)
		free(excp_p->catchMethodSig);
	if (excp_p->catchMethodname != NULL)
		free(excp_p->catchMethodname);

	if (excp_p->stackTrace != NULL)
	{
		for (count = 0; count < excp_p->frameCount; count++)
		{
			if (excp_p->stackTrace[count].className != NULL)
			{
				if (excp_p->stackTrace[count].className != NULL)
					free(excp_p->stackTrace[count].className);
			}
			if (excp_p->stackTrace[count].methodName != NULL)
			{
				if (excp_p->stackTrace[count].methodName != NULL)
					free(excp_p->stackTrace[count].methodName);
			}
			if (excp_p->stackTrace[count].methodSig != NULL)
			{
				if (excp_p->stackTrace[count].methodSig != NULL)
					free(excp_p->stackTrace[count].methodSig);
			}
		}
		free(excp_p->stackTrace);
	}
	free(excp_p);
}

static char * getExceptionMessage(JNIEnv* env, jobject exception)
{
	jmethodID getMessageMethodId;
	jstring jExceptionMessage;
	int msgLength;
	char * msgArray = NULL;

	getMessageMethodId = (*env)->GetMethodID(env, ((*env)->FindClass(env,
			"java/lang/Throwable")), "getMessage", "()Ljava/lang/String;");
	if (getMessageMethodId != NULL)
	{
		jExceptionMessage = (*env)->CallObjectMethod(env, exception,
				getMessageMethodId);
		if (jExceptionMessage != NULL)
		{
			msgLength = (*env)->GetStringLength(env, jExceptionMessage);
			if (msgLength > 0)
			{
				msgArray = malloc(msgLength + 1); /* To be freed explicitly by the caller of this function */
				if (msgArray == NULL)
					return NULL;
				(*env)->GetStringUTFRegion(env, jExceptionMessage, 0,
						msgLength, msgArray);
				msgArray[msgLength] = '\0';
			}
		}
	}
	return msgArray;
}

static void getSourceFileNameForMethod(jvmtiEnv *jvmti, jmethodID method,
		char ** sourceFileName_p)
{
	jvmtiError error;
	jclass sourceClass;
	char * tempSourceFileName = NULL;

	error = (*jvmti)->GetMethodDeclaringClass(jvmti, method, &sourceClass);
	checkJvmtiError(jvmti, error,
			"getSourceFileNameForMethod: GetMethodDeclaringClass: ");

	*sourceFileName_p = NULL;
	if (error == JVMTI_ERROR_NONE)
	{
		error = (*jvmti)->GetSourceFileName(jvmti, sourceClass,
				&tempSourceFileName);
		checkJvmtiError(jvmti, error, "GetSourceFileName: ");
		*sourceFileName_p = deallocateJVMMemoryAndAllocateMemoryFromHeap(error,
				jvmti, tempSourceFileName);
	}
}

static void captureExceptionInfo(jvmtiEnv *jvmti, JNIEnv* env,
		jobject exception, jmethodID method, jlocation location,
		jmethodID catchMethod, jlocation catchLocation,
		EXCEPTION_LOG_STRUCTURE * excp_p)
{
	jvmtiError error;
	jclass expClass;
	jvmtiFrameInfo catchFrameInfo;
	char * tempExceptionType = NULL;

	expClass = (*env)->GetObjectClass(env, exception);
	error = (*jvmti)->GetClassSignature(jvmti, expClass, &tempExceptionType,
			NULL);
	checkJvmtiError(jvmti, error, "GetClassSignature: ");
	excp_p->class = deallocateJVMMemoryAndAllocateMemoryFromHeap(error, jvmti,
			tempExceptionType);

	excp_p->msg = getExceptionMessage(env, exception);

	getSourceFileNameForMethod(jvmti, method, &(excp_p->sourceFileName));

	catchFrameInfo.location = catchLocation;
	catchFrameInfo.method = catchMethod;
	getMethodInfo(jvmti, catchFrameInfo, &(excp_p->catchMethodname),
			&(excp_p->catchMethodSig), &(excp_p->catchClassName),
			&(excp_p->catchMethodLineNumber));
}

static void JNICALL callbackException(jvmtiEnv *jvmti, JNIEnv* env,
		jthread thread, jmethodID method, jlocation location,
		jobject exception, jmethodID catchMethod, jlocation catchLocation)
{
	enterCriticalSection(jvmti);
	EXCEPTION_LOG_STRUCTURE * excp_p;
	excp_p = malloc(sizeof(EXCEPTION_LOG_STRUCTURE));
	if (excp_p == NULL)
	{
		logError("Cannot allocate Memory for Exception Structure.");
	}
	else
	{
		initializeExceptionPointer(excp_p);
		captureExceptionInfo(jvmti, env, exception, method, location,
				catchMethod, catchLocation, excp_p);
		captureStackTrace(jvmti, thread, excp_p);
		logEvent(excp_p);
		deallocateExceptionPointer(excp_p);
	}
	exitCriticalSection(jvmti);
}

static void JNICALL callbackVMDeath(jvmtiEnv *jvmti, JNIEnv* jniEnv)
{
	enterCriticalSection(jvmti);
	{
		logError("Got VM Death Event");
	}
	exitCriticalSection(jvmti);
}

static void JNICALL callbackVMInit(jvmtiEnv *jvmti, JNIEnv* jniEnv,
		jthread thread)
{
	enterCriticalSection(jvmti);
	{
		logError("Got VM Init Event");
		/*Set the notification for Exception Events*/
		jvmtiError error;
		error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
				JVMTI_EVENT_EXCEPTION, (jthread) NULL);
		checkJvmtiError(jvmti, error, "Cannot set event notification: ");
	}
	exitCriticalSection(jvmti);
}

static jint getEnvironment(JavaVM *jvm, jvmtiEnv **jvmti_p)
{
	char tempBuf[500];
	jint responseCode;
	responseCode = (*jvm)->GetEnv(jvm, (void **) jvmti_p, JVMTI_VERSION_1_0);
	if (responseCode != JNI_OK || (*jvmti_p) == NULL)
	{
		/* VM was unable to obtain this version of the JVMTI interface, this is a fatal error. */
		sprintf(
				tempBuf,
				"ERROR: Unable to access JVMTI Version 1 (0x%x), is your J2SE a 1.5 or newer version? JNIEnv's GetEnv() returned %d",
				JVMTI_VERSION_1, (int) responseCode);
		logError(tempBuf);
	}
	return responseCode;
}

static jvmtiError setJVMTICapabilities(jvmtiEnv *jvmti)
{
	jvmtiCapabilities jvmtiCapabilities;
	jvmtiError errorCode;

	/*Set the Capabilities for getting Exception Events*/
	(void) memset(&jvmtiCapabilities, 0, sizeof(jvmtiCapabilities));
	jvmtiCapabilities.can_generate_exception_events = 1;
	jvmtiCapabilities.can_get_line_numbers = 1;
	jvmtiCapabilities.can_get_source_file_name = 1;

	errorCode = (*jvmti)->AddCapabilities(jvmti, &jvmtiCapabilities);
	checkJvmtiError(jvmti, errorCode,
			"Unable to get necessary JVMTI capabilities: ");
	return errorCode;
}

static jvmtiError setEventCallbacks(jvmtiEnv *jvmti)
{
	jvmtiEventCallbacks callbacks;
	jvmtiError errorCode;

	(void) memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
	callbacks.VMInit = &callbackVMInit;
	callbacks.VMDeath = &callbackVMDeath;
	callbacks.Exception = &callbackException;
	errorCode = (*jvmti)->SetEventCallbacks(jvmti, &callbacks,
			(jint) sizeof(callbacks));
	checkJvmtiError(jvmti, errorCode, "Cannot set jvmti callbacks: ");
	return errorCode;
}

static jvmtiError setEventNotification(jvmtiEnv *jvmti)
{
	jvmtiError errorCode;
	/*Set only the initial events - VM initialization, VM death. */
	errorCode = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
			JVMTI_EVENT_VM_INIT, (jthread) NULL);
	errorCode = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
			JVMTI_EVENT_VM_DEATH, (jthread) NULL);
	checkJvmtiError(jvmti, errorCode, "Cannot set event notification: ");
	return errorCode;
}

static jvmtiError createMonitor(jvmtiEnv *jvmti)
{
	jvmtiError errorCode;
	/* Create a raw monitor for our use in this agent to protect critical sections of code. */
	errorCode = (*jvmti)->CreateRawMonitor(jvmti, "agent data",
			&GlobalAgentLock);
	checkJvmtiError(jvmti, errorCode, "Cannot create raw monitor: ");
	return errorCode;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
	char * logFileName;
	char * errorFileName;
	char * dbServerName;
	char * dbUserName;
	char * dbPassword;
	char * dbName;

	jint responseCode;
	jvmtiEnv *jvmti;

	initialize(options);

	NO_OF_STACKFRAMES = getStackFrameCount();

	errorFileName = getErrorFileName();
	initializeErrorLogger(errorFileName);

	TEXT_LOGGING = getTextLogging();
	DB_LOGGING = getDBLogging();

	if (TEXT_LOGGING)
	{
		logFileName = getTextLogFileName();
		initializeLogger(logFileName, deallocateExceptionPointer);
	}
	if (DB_LOGGING)
	{
		dbServerName = getDBServerName();
		dbUserName = getDBUserName();
		dbName = getDBName();
		dbPassword = getDBPassword();
		initializeDBLogger(dbServerName, dbUserName, dbPassword, dbName,
				deallocateExceptionPointer);
	}

	responseCode = getEnvironment(jvm, &jvmti);
	if (responseCode == JNI_OK)
	{
		responseCode = setJVMTICapabilities(jvmti);
		if (responseCode == JNI_OK)
		{
			responseCode = setEventCallbacks(jvmti);
			if (responseCode == JNI_OK)
			{
				responseCode = setEventNotification(jvmti);
				if (responseCode == JNI_OK)
				{
					responseCode = createMonitor(jvmti);
				}
			}
		}
	}
	return responseCode;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm)
{
	if (DB_LOGGING)
	{
		shutdownLoggerInDB();
	}
	if (TEXT_LOGGING)
	{
		shutdownLogger();
	}
	shutdownErrorLogger();
	shutdownConfig();
}
