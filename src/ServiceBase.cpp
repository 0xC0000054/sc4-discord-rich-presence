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

#include "ServiceBase.h"

static constexpr uint32_t GZIID_cIGZSystemService = 0x287fb697;

ServiceBase::ServiceBase(uint32_t serviceID, int32_t servicePriority)
	: serviceID(serviceID),
	  servicePriority(servicePriority),
	  serviceTickPriority(servicePriority),
	  serviceRunning(false),
	  refCount(0)
{
}

bool ServiceBase::QueryInterface(uint32_t riid, void** ppvObj)
{
	if (riid == GZIID_cIGZSystemService)
	{
		*ppvObj = static_cast<cIGZSystemService*>(this);
		AddRef();

		return true;
	}
	else if (riid == GZIID_cIGZUnknown)
	{
		*ppvObj = static_cast<cIGZUnknown*>(this);
		AddRef();

		return true;
	}

	return false;
}

uint32_t ServiceBase::AddRef()
{
	return ++refCount;
}

uint32_t ServiceBase::Release()
{
	if (refCount > 0)
	{
		--refCount;
	}
	return refCount;
}

bool ServiceBase::Init()
{
	return true;
}

bool ServiceBase::Shutdown()
{
	return true;
}

uint32_t ServiceBase::GetServiceID()
{
	return serviceID;
}

cIGZSystemService* ServiceBase::SetServiceID(uint32_t id)
{
	serviceID = id;
	return this;
}

int32_t ServiceBase::GetServicePriority()
{
	return servicePriority;
}

bool ServiceBase::IsServiceRunning()
{
	return serviceRunning;
}

cIGZSystemService* ServiceBase::SetServiceRunning(bool running)
{
	serviceRunning = running;
	return this;
}

bool ServiceBase::OnTick(uint32_t unknown1)
{
	return true;
}

bool ServiceBase::OnIdle(uint32_t unknown1)
{
	return true;
}

int32_t ServiceBase::GetServiceTickPriority()
{
	return serviceTickPriority;
}
