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
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cISC4App.h"
#include "cISC4AuraSimulator.h"
#include "cISC4City.h"
#include "cISC4Region.h"
#include "GZServPtrs.h"
#include "cRZAutoRefCount.h"
#include "cRZBaseString.h"
#include "cRZMessage2COMDirector.h"
#include "StringResourceKey.h"
#include "StringResourceManager.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include <Windows.h>
#include "wil/result.h"

static constexpr uint32_t kDiscordRichPresenceDirectorID = 0x7A559E00;

static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessageCityEstablished = 0x26D31EC4;
static constexpr uint32_t kSC4MessagePostRegionInit = 0xCBB5BB45;
static constexpr uint32_t kSC4MessagePreRegionShutdown = 0x8BB5BB46;
static constexpr uint32_t kSC4MessageHistoryWarehouseRecordChanged = 0x89EFA536; // Same as kSC4CLSID_cSC4HistoryWarehouse

using namespace std::string_view_literals;

static constexpr std::string_view PluginLogFileName = "SC4DiscordRichPresence.log"sv;

class DiscordRichPresenceDllDirector final : public cRZMessage2COMDirector
{
public:
	DiscordRichPresenceDllDirector()
		: service(),
		  lastMayorRating(0),
		  serviceInitialized(false),
		  serviceAddedToFramework(false),
		  serviceAddedToOnIdle(false),
		  setRegionName(false)
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

	bool DoMessage(cIGZMessage2* pMsg)
	{
		cIGZMessage2Standard* pStandardMsg = static_cast<cIGZMessage2Standard*>(pMsg);

		switch (pStandardMsg->GetType())
		{
		case kSC4MessageCityEstablished:
			CityEstablished();
			break;
		case kSC4MessagePostCityInit:
			PostCityInit(pStandardMsg);
			break;
		case kSC4MessagePostRegionInit:
			PostRegionInit();
			break;
		case kSC4MessagePreRegionShutdown:
			PreRegionShutdown();
			break;
		case kSC4MessageHistoryWarehouseRecordChanged:
			HistoryWarehouseRecordChanged(pStandardMsg);
			break;
		}

		return true;
	}

	void SetCityViewPresence(cISC4City* pCity)
	{
		if (pCity)
		{
			cRZBaseString cityName;
			const cISC4AuraSimulator* pAuraSim = pCity->GetAuraSimulator();

			if (pCity->GetCityName(cityName) && pAuraSim)
			{
				std::string details("City: ");
				details.append(cityName.ToChar(), cityName.Strlen());

				char stateBuffer[1024]{};

				int8_t mayorRating = pAuraSim->GetMayorRating();

				std::snprintf(
					stateBuffer,
					sizeof(stateBuffer),
					"Mayor Rating: %d",
					mayorRating);
				lastMayorRating = mayorRating;
				mayorRatingLastUpdateTime = std::chrono::system_clock::now();

				service.UpdatePresence(details.c_str(), stateBuffer);
			}
		}
	}

	void CityEstablished()
	{
		cISC4AppPtr pSC4App;

		if (pSC4App)
		{
			SetCityViewPresence(pSC4App->GetCity());
		}
	}

	void HistoryWarehouseRecordChanged(cIGZMessage2Standard* pStandardMsg)
	{
		constexpr uint32_t kMayorRating = 0x0A5CBF37;

		const uint32_t id = static_cast<uint32_t>(pStandardMsg->GetData1());

		if (id == kMayorRating)
		{
			int32_t value = static_cast<int32_t>(pStandardMsg->GetData2());

			if (lastMayorRating != value)
			{
				lastMayorRating = value;

				// We only update the displayed mayor rating once every 10 seconds.
				if ((std::chrono::system_clock::now() - mayorRatingLastUpdateTime) > std::chrono::seconds(10))
				{
					char buffer[1024]{};

					std::snprintf(
						buffer,
						sizeof(buffer),
						"Mayor Rating: %d",
						value);

					service.UpdateState(buffer);
				}
			}
		}
	}

	void PostCityInit(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4City* pCity = static_cast<cISC4City*>(pStandardMsg->GetVoid1());

		if (pCity)
		{
			if (pCity->GetEstablished())
			{
				SetCityViewPresence(pCity);
			}
			else
			{
				std::string details("Establishing City");

				// Append the region name to the establishing city text.
				// The final string will use the form: Establishing City in <Region name>.

				const std::string regionDetailsText = service.GetDetails();

				if (regionDetailsText.starts_with("Region: "))
				{
					// Trim the Region: prefix and the space that follows it.
					const std::string_view regionName = std::string_view(regionDetailsText).substr(8);

					details.append(" in ").append(regionName);
				}

				service.UpdatePresence(details.c_str(), /*startElapsedTimer*/false);
			}
		}
	}

	void PostRegionInit()
	{
		if (!setRegionName)
		{
			setRegionName = true;

			cISC4AppPtr pSC4App;

			if (pSC4App)
			{
				cISC4Region* pRegion = pSC4App->GetRegion();

				if (pRegion)
				{
					// The following is an ugly hack to work around a problem with the cISC4Region class:
					//
					// The developers who wrote the class made the GetName and GetDirectoryName methods
					// return a pointer to the internal cRZString class that implements cIGZString instead
					// of a cIGZString pointer.
					//
					// To fix this we cast the char pointer returned by GetName to void**, the void** pointer
					// points to the start of the cRZSting vtable.
					// Then we cast that cRZString vtable pointer to a cIGZSting pointer.

					cIGZString* name = reinterpret_cast<cIGZString*>(reinterpret_cast<void**>(pRegion->GetName()));

					std::string details("Region: ");
					details.append(name->ToChar(), name->Strlen());

					service.UpdatePresence(details.c_str(), /*startElapsedTimer*/false);
				}
			}
		}
	}

	void PreRegionShutdown()
	{
		setRegionName = false;
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
		serviceInitialized = service.Init();

		if (serviceInitialized)
		{
			cIGZFrameWork* const pFramework = RZGetFramework();

			serviceAddedToFramework = pFramework->AddSystemService(&service);

			if (serviceAddedToFramework)
			{
				serviceAddedToOnIdle = pFramework->AddToOnIdle(&service);
			}
		}

		return true;
	}

	bool PostAppInit()
	{
		Logger& logger = Logger::GetInstance();

		if (serviceAddedToOnIdle)
		{
			cIGZMessageServer2Ptr pMsgServ;

			if (pMsgServ)
			{
				std::vector<uint32_t> requiredNotifications;
				requiredNotifications.push_back(kSC4MessageCityEstablished);
				requiredNotifications.push_back(kSC4MessagePostCityInit);
				requiredNotifications.push_back(kSC4MessagePostRegionInit);
				requiredNotifications.push_back(kSC4MessagePreRegionShutdown);
				requiredNotifications.push_back(kSC4MessageHistoryWarehouseRecordChanged);

				for (uint32_t messageID : requiredNotifications)
				{
					pMsgServ->AddNotification(this, messageID);
				}
			}
		}
		else
		{
			logger.WriteLine(LogLevel::Error, "Failed to initialize the Discord Rich Presence service.");
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

		if (serviceInitialized)
		{
			service.Shutdown();
		}

		return true;
	}

private:

	DiscordRichPresenceService service;
	std::chrono::time_point<std::chrono::system_clock> mayorRatingLastUpdateTime;
	int32_t lastMayorRating;
	bool serviceInitialized;
	bool serviceAddedToFramework;
	bool serviceAddedToOnIdle;
	bool setRegionName;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static DiscordRichPresenceDllDirector sDirector;
	return &sDirector;
}