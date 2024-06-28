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

#include "version.h"
#include "DiscordRichPresenceService.h"
#include "FileSystem.h"
#include "Logger.h"
#include "cIGZApp.h"
#include "cIGZCOM.h"
#include "cIGZFrameWork.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cISC4App.h"
#include "cISC4AuraSimulator.h"
#include "cISC4City.h"
#include "cISC4Region.h"
#include "GZServPtrs.h"
#include "cRZAutoRefCount.h"
#include "cRZBaseString.h"
#include "cRZCOMDllDirector.h"
#include "StringResourceKey.h"
#include "StringResourceManager.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

static constexpr uint32_t kDiscordRichPresenceDirectorID = 0x7A559E00;

using namespace std::string_view_literals;

static constexpr std::string_view PluginLogFileName = "SC4DiscordRichPresence.log"sv;

class DiscordRichPresenceDllDirector final : public cRZCOMDllDirector
{
public:
	DiscordRichPresenceDllDirector()
		: service(),
		  serviceAddedToFramework(false),
		  serviceAddedToOnIdle(false)
	{
		std::filesystem::path dllFolderPath = FileSystem::GetDllFolderPath();

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
		logger.Init(logFilePath, LogLevel::Info, false);
		logger.WriteLogFileHeader("SC4DiscordRichPresence v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kDiscordRichPresenceDirectorID;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		cIGZFrameWork* const pFramework = pCOM->FrameWork();

		if (pFramework->GetState() < cIGZFrameWork::kStatePreAppInit)
		{
			pFramework->AddHook(this);
		}
		else
		{
			PreAppInit();
		}

		return true;
	}

	bool PreAppInit()
	{
		if (service.Init())
		{
			cIGZFrameWork* const pFramework = RZGetFramework();

			serviceAddedToFramework = pFramework->AddSystemService(&service);

			if (serviceAddedToFramework)
			{
				serviceAddedToOnIdle = pFramework->AddToOnIdle(&service);
			}
		}

		if (!serviceAddedToOnIdle)
		{
			Logger::GetInstance().WriteLine(LogLevel::Error, "Failed to initialize the Discord Rich Presence service.");
		}

		return true;
	}

	bool PreAppShutdown()
	{
		cIGZFrameWork* const pFramework = RZGetFramework();

		if (serviceAddedToOnIdle)
		{
			pFramework->RemoveFromOnIdle(&service);
		}

		if (serviceAddedToFramework)
		{
			pFramework->RemoveSystemService(&service);
		}

		service.Shutdown();

		return true;
	}

private:

	DiscordRichPresenceService service;
	bool serviceAddedToFramework;
	bool serviceAddedToOnIdle;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static DiscordRichPresenceDllDirector sDirector;
	return &sDirector;
}