////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-discord-rich-presence, a DLL Plugin for
// SimCity 4 that implements Discord rich presence support.
//
// Copyright (c) 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////

#pragma once
#include "cIGZSystemService.h"

class ServiceBase : public cIGZSystemService
{
public:

	bool QueryInterface(uint32_t riid, void** ppvObj) override;

	uint32_t AddRef() override;

	uint32_t Release() override;

	bool Init() override;

	bool Shutdown() override;

	uint32_t GetServiceID() override;

	cIGZSystemService* SetServiceID(uint32_t id) final override;

	int32_t GetServicePriority() override;

	bool IsServiceRunning() override;

	cIGZSystemService* SetServiceRunning(bool running) override;

	bool OnTick(uint32_t unknown1) override;

	bool OnIdle(uint32_t unknown1) override;

	int32_t GetServiceTickPriority() override;

protected:

	ServiceBase(uint32_t serviceID, int32_t servicePriority);

	uint32_t refCount;
	uint32_t serviceID;
	int32_t servicePriority;
	int32_t serviceTickPriority;
	bool serviceRunning;
};
