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
// This file contains various functions for interacting with Bluetooth Management interface, which provides adapter configuration.
//
// >>
// >>>  DISCUSSION
// >>
//
// We only cover the basics here. If there are configuration features you need that aren't supported (such as configuring BR/EDR),
// then this would be a good place for them.
//
// Note that this class relies on the `HciAdapter`, which is a very primitive implementation. Use with caution.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <string.h>

#include "Mgmt.h"
#include "Logger.h"
#include "Utils.h"

namespace ggk {

// Construct the Mgmt device
//
// Set `controllerIndex` to the zero-based index of the device as recognized by the OS. If this parameter is omitted, the index
// of the first device (0) will be used.
Mgmt::Mgmt(uint16_t controllerIndex)
: controllerIndex(controllerIndex)
{
	HciAdapter::getInstance().sync(controllerIndex);
}

// Set the adapter name and short name
//
// The inputs `name` and `shortName` may be truncated prior to setting them on the adapter. To ensure that `name` and
// `shortName` conform to length specifications prior to calling this method, see the constants `kMaxAdvertisingNameLength` and
// `kMaxAdvertisingShortNameLength`. In addition, the static methods `truncateName()` and `truncateShortName()` may be helpful.
//
// Returns true on success, otherwise false
bool Mgmt::setName(std::string name, std::string shortName)
{
	// Ensure their lengths are okay
	name = truncateName(name);
	shortName = truncateShortName(shortName);

	struct SRequest : HciAdapter::HciHeader
	{
		char name[249];
		char shortName[11];
	} __attribute__((packed));

	SRequest request;
	request.code = Mgmt::ESetLocalNameCommand;
	request.controllerId = controllerIndex;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::HciHeader);

	memset(request.name, 0, sizeof(request.name));
	snprintf(request.name, sizeof(request.name), "%s", name.c_str());

	memset(request.shortName, 0, sizeof(request.shortName));
	snprintf(request.shortName, sizeof(request.shortName), "%s", shortName.c_str());

	if (!HciAdapter::getInstance().sendCommand(request))
	{
		Logger::warn(SSTR << "  + Failed to set name");
		return false;
	}

	return true;
}

bool Mgmt::setRawAdvertisingData( const RawAdvertisingData &adv )
{
    setPowered(0);
    usleep(200*1000);

    size_t cp_len;
    struct SRequest : HciAdapter::HciHeader
    {
        uint8_t Instance;
        uint32_t Flags;
        uint16_t Duration;
        uint16_t Timeout;
        uint8_t Adv_Data_Len;
        uint8_t Scan_Rsp_Len;
        uint8_t data[0];
    } __attribute__((packed));

    SRequest *request;
    cp_len = sizeof(*request) + adv.advDataLen + adv.rspDataLen ;
    uint8_t buf[cp_len];
    request = (SRequest *)(buf);

    request->code = Mgmt::EAddAdvertisingCommand;
    request->controllerId = controllerIndex;
    request->dataSize = cp_len - sizeof(HciAdapter::HciHeader);
    request->Instance = 1;

    request->Flags = 0b0000000000000000;

    request->Timeout = 0;
    request->Duration = 0 ;
    request->Adv_Data_Len = adv.advDataLen;
    request->Scan_Rsp_Len = adv.rspDataLen;

    if (adv.advDataLen>0)
        memcpy(request->data, adv.advData, adv.advDataLen);

    /**
        0	Switch into Connectable mode
        1	Advertise as Discoverable
        2	Advertise as Limited Discoverable
        3	Add Flags field to Adv_Data
        4	Add TX Power field to Adv_Data
        5	Add Appearance field to Scan_Rsp
        6	Add Local Name in Scan_Rsp
        7	Secondary Channel with LE 1M
        8	Secondary Channel with LE 2M
        9	Secondary Channel with LE Coded
    **/
    if (adv.rspDataLen>0)
        memcpy(request->data + adv.advDataLen, adv.rspData, adv.rspDataLen);

    if (!HciAdapter::getInstance().sendCommand(*request))
    {
        Logger::warn(SSTR << "  + Failed to set discoverable");
        return false;
    }

    return true;
}
// Sets discoverable mode
// 0x00 disables discoverable
// 0x01 enables general discoverable
// 0x02 enables limited discoverable
// Timeout is the time in seconds. For 0x02, the timeout value is required.
bool Mgmt::setDiscoverable(uint8_t disc, uint16_t timeout)
{
	struct SRequest : HciAdapter::HciHeader
	{
		uint8_t disc;
		uint16_t timeout;
	} __attribute__((packed));

	SRequest request;
	request.code = Mgmt::ESetDiscoverableCommand;
	request.controllerId = controllerIndex;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::HciHeader);
	request.disc = disc;
	request.timeout = timeout;

	if (!HciAdapter::getInstance().sendCommand(request))
	{
		Logger::warn(SSTR << "  + Failed to set discoverable");
		return false;
	}

	return true;
}

// Set a setting state to 'newState'
//
// Many settings are set the same way, this is just a convenience routine to handle them all
//
// Returns true on success, otherwise false
bool Mgmt::setState(uint16_t commandCode, uint16_t controllerId, uint8_t newState)
{
	struct SRequest : HciAdapter::HciHeader
	{
		uint8_t state;
	} __attribute__((packed));

	SRequest request;
	request.code = commandCode;
	request.controllerId = controllerId;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::HciHeader);
	request.state = newState;

	if (!HciAdapter::getInstance().sendCommand(request))
	{
		Logger::warn(SSTR << "  + Failed to set " << HciAdapter::kCommandCodeNames[commandCode] << " state to: " << static_cast<int>(newState));
		return false;
	}

	return true;
}

// Set the powered state to `newState` (true = powered on, false = powered off)
//
// Returns true on success, otherwise false
bool Mgmt::setPowered(bool newState)
{
	return setState(Mgmt::ESetPoweredCommand, controllerIndex, newState ? 1 : 0);
}

// Set the BR/EDR state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setBredr(bool newState)
{
	return setState(Mgmt::ESetBREDRCommand, controllerIndex, newState ? 1 : 0);
}

// Set the Secure Connection state (0 = disabled, 1 = enabled, 2 = secure connections only mode)
//
// Returns true on success, otherwise false
bool Mgmt::setSecureConnections(uint8_t newState)
{
	return setState(Mgmt::ESetSecureConnectionsCommand, controllerIndex, newState);
}

// Set the bondable state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setBondable(bool newState)
{
	return setState(Mgmt::ESetBondableCommand, controllerIndex, newState ? 1 : 0);
}

// Set the connectable state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setConnectable(bool newState)
{
	return setState(Mgmt::ESetConnectableCommand, controllerIndex, newState ? 1 : 0);
}

// Set the LE state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setLE(bool newState)
{
	return setState(Mgmt::ESetLowEnergyCommand, controllerIndex, newState ? 1 : 0);
}

// Set the advertising state to `newState` (0 = disabled, 1 = enabled (with consideration towards the connectable setting),
// 2 = enabled in connectable mode).
//
// Returns true on success, otherwise false
bool Mgmt::setAdvertising(uint8_t newState)
{
	return setState(Mgmt::ESetAdvertisingCommand, controllerIndex, newState);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Utilitarian
// ---------------------------------------------------------------------------------------------------------------------------------

// Truncates the string `name` to the maximum allowed length for an adapter name. If `name` needs no truncation, a copy of
// `name` is returned.
std::string Mgmt::truncateName(const std::string &name)
{
	if (name.length() <= kMaxAdvertisingNameLength)
	{
		return name;
	}

	return name.substr(0, kMaxAdvertisingNameLength);
}

// Truncates the string `name` to the maximum allowed length for an adapter short-name. If `name` needs no truncation, a copy
// of `name` is returned.
std::string Mgmt::truncateShortName(const std::string &name)
{
	if (name.length() <= kMaxAdvertisingShortNameLength)
	{
		return name;
	}

	return name.substr(0, kMaxAdvertisingShortNameLength);
}

}; // namespace ggk
