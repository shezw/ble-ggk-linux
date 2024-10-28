// Copyright 2017-2019 Paul Nettle
//
// This file is part of Gobbledegook.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file in the root of the source tree.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// >>
// >>>  INSIDE THIS FILE
// >>
//
// This file represents the complete interface to Gobbledegook from a stand-alone application.
//
// >>
// >>>  DISCUSSION
// >>
//
// The interface to Gobbledegook is rether simple. It consists of the following categories of functionality:
//
//     * Logging
//
//       The server defers all logging to the application. The application registers a set of logging delegates (one for each
//       log level) so it can manage the logs however it wants (syslog, console, file, an external logging service, etc.)
//
//     * Managing updates to server data
//
//       The application is required to implement two delegates (`GGKServerDataGetter` and `GGKServerDataSetter`) for sharing data
//       with the server. See standalone.cpp for an example of how this is done.
//
//       In addition, the server provides a thread-safe queue for notifications of data updates to the server. Generally, the only
//       methods an application will need to call are `ggkNofifyUpdatedCharacteristic` and `ggkNofifyUpdatedDescriptor`. The other
//       methods are provided in case an application requies extended functionality.
//
//     * Server control
//
//       A small set of methods for starting and stopping the server.
//
//     * Server state
//
//       These routines allow the application to query the server's current state. The server runs through these states during its
//       lifecycle:
//
//           EUninitialized -> EInitializing -> ERunning -> EStopping -> EStopped
//
//     * Server health
//
//       The server maintains its own health information. The states are:
//
//           EOk         - the server is A-OK
//           EFailedInit - the server had a failure prior to the ERunning state
//           EFailedRun  - the server had a failure during the ERunning state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

    typedef struct RawAdvertisingData_s
    {
        uint8_t advDataLen;
        uint8_t rspDataLen;
        uint8_t *advData;
        uint8_t *rspData;
    } RawAdvertisingData;

	// -----------------------------------------------------------------------------------------------------------------------------
	// LOGGING
	// -----------------------------------------------------------------------------------------------------------------------------

	// Type definition for callback delegates that receive log messages
	typedef void (*GGKLogReceiver)(const char *pMessage);
    typedef void (*GGKMessageReceived) ( const char * msg, int size );

	// Each of these methods registers a log receiver method. Receivers are set when registered. To unregister a log receiver,
	// simply register with `nullptr`.
	void ggkLogRegisterDebug(GGKLogReceiver receiver);
	void ggkLogRegisterInfo(GGKLogReceiver receiver);
	void ggkLogRegisterStatus(GGKLogReceiver receiver);
	void ggkLogRegisterWarn(GGKLogReceiver receiver);
	void ggkLogRegisterError(GGKLogReceiver receiver);
	void ggkLogRegisterFatal(GGKLogReceiver receiver);
	void ggkLogRegisterAlways(GGKLogReceiver receiver);
	void ggkLogRegisterTrace(GGKLogReceiver receiver);

    void ggkServerRegisterBrand( const char * brand );
    void ggkServerRegisterDeviceModel( const char * model );
    void ggkServerRegisterSenderChar( const char * ch );
    void ggkServerRegisterReceiverCB( const char * ch, GGKMessageReceived receivedCB );
    void ggkServerSendMessage( const char * message, int size );

	typedef const void *(*GGKServerDataGetter)(const char *pName);

	typedef int (*GGKServerDataSetter)(const char *pName, const void *pData);

	// -----------------------------------------------------------------------------------------------------------------------------
	// SERVER DATA UPDATE MANAGEMENT
	// -----------------------------------------------------------------------------------------------------------------------------

	// Adds an update to the front of the queue for a characteristic at the given object path
	//
	// Returns non-zero value on success or 0 on failure.
	int ggkNofifyUpdatedCharacteristic(const char *pObjectPath);

	// Adds an update to the front of the queue for a descriptor at the given object path
	//
	// Returns non-zero value on success or 0 on failure.
	int ggkNofifyUpdatedDescriptor(const char *pObjectPath);

	// Adds a named update to the front of the queue. Generally, this routine should not be used directly. Instead, use the
	// `ggkNofifyUpdatedCharacteristic()` instead.
	//
	// Returns non-zero value on success or 0 on failure.
	int ggkPushUpdateQueue(const char *pObjectPath, const char *pInterfaceName);

	// Returns 1 on success, 0 if the queue is empty, -1 on error (such as the length too small to store the element)
	int ggkPopUpdateQueue(char *pElement, int elementLen, int keep);

	// Returns 1 if the queue is empty, otherwise 0
	int ggkUpdateQueueIsEmpty();

	// Returns the number of entries waiting in the queue
	int ggkUpdateQueueSize();

	// Removes all entries from the queue
	void ggkUpdateQueueClear();

	int ggkStart(const char *pServiceName, const char *pAdvertisingName, const char *pAdvertisingShortName, 
		GGKServerDataGetter getter, GGKServerDataSetter setter, int maxAsyncInitTimeoutMS, const RawAdvertisingData &advData);

	int ggkWait();

	void ggkTriggerShutdown();

	int ggkShutdownAndWait();

	enum GGKServerRunState
	{
		EUninitialized,
		EInitializing,
		ERunning,
		EStopping,
		EStopped
	};

	// Retrieve the current running state of the server
	//
	// See `GGKServerRunState` (enumeration) for more information.
	enum GGKServerRunState ggkGetServerRunState();

	// Convert a `GGKServerRunState` into a human-readable string
	const char *ggkGetServerRunStateString(enum GGKServerRunState state);

	// Convenience method to check ServerRunState for a running server
	int ggkIsServerRunning();

	// -----------------------------------------------------------------------------------------------------------------------------
	// SERVER HEALTH
	// -----------------------------------------------------------------------------------------------------------------------------

	// The current health of the server
	//
	// A running server's health will always be EOk, therefore it is only necessary to check the health status after the server
	// has shut down to determine if it was shut down due to an unhealthy condition.
	//
	// Use `ggkGetServerHealth` to retrieve the health and `ggkGetServerHealthString` to convert a `GGKServerHealth` into a
	// human-readable string.
	enum GGKServerHealth
	{
		EOk,
		EFailedInit,
		EFailedRun
	};

	// Retrieve the current health of the server
	//
	// See `GGKServerHealth` (enumeration) for more information.
	enum GGKServerHealth ggkGetServerHealth();

	// Convert a `GGKServerHealth` into a human-readable string
	const char *ggkGetServerHealthString(enum GGKServerHealth state);

#ifdef __cplusplus
}
#endif //__cplusplus
