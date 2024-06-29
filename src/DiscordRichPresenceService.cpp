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

#include "DiscordRichPresenceService.h"
#include "DebugUtil.h"
#include "Logger.h"
#include "cIGZFrameWork.h"
#include "cIGZLanguageManager.h"
#include "cIGZLanguageUtility.h"
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4Region.h"
#include "cRZCOMDllDirector.h"
#include "GZCLSIDDefs.h"
#include "GZServPtrs.h"
#include <array>

static constexpr uint32_t kDiscordRichPresenceServiceID = 0xFE95AAEA;

static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessageCityEstablished = 0x26D31EC4;
static constexpr uint32_t kSC4MessageCityNameChanged = 0x0AB99380;
static constexpr uint32_t kSC4MessagePostRegionInit = 0xCBB5BB45;
static constexpr uint32_t kSC4MessagePreRegionShutdown = 0x8BB5BB46;

static constexpr std::array<uint32_t, 5> MessageIds =
{
	kSC4MessagePostCityInit,
	kSC4MessageCityEstablished,
	kSC4MessageCityNameChanged,
	kSC4MessagePostRegionInit,
	kSC4MessagePreRegionShutdown,
};

namespace
{
	void DebugLogHook(discord::LogLevel level, const char* message)
	{
		DebugUtil::PrintLineToDebugOutputFormatted("Discord:%d %s", static_cast<int>(level), message);
	}

	void DiscordAPICallback(discord::Result result)
	{
#ifdef _DEBUG
		DebugUtil::PrintLineToDebugOutputFormatted("Discord result: %d", static_cast<int>(result));
#endif // _DEBUG
	}
}

DiscordRichPresenceService::DiscordRichPresenceService()
	: ServiceBase(kDiscordRichPresenceServiceID, 2000010),
	  discord(),
	  activity{},
	  activityLastUpdateTime(),
	  statusLastUpdateTime(),
	  activityNeedsUpdate(false),
	  currentCityStatus(CityStatusType::MayorName),
	  currentRegionStatus(RegionStatusType::TotalResidentialPopulation),
	  view(DiscordView::Unknown),
	  pLanguageUtility(nullptr)
{
}

bool DiscordRichPresenceService::QueryInterface(uint32_t riid, void** ppvObj)
{
	if (riid == GZCLSID::kcIGZMessageTarget2)
	{
		*ppvObj = static_cast<cIGZMessageTarget2*>(this);
		AddRef();

		return true;
	}

	return ServiceBase::QueryInterface(riid, ppvObj);
}

uint32_t DiscordRichPresenceService::AddRef()
{
	return ServiceBase::AddRef();
}

uint32_t DiscordRichPresenceService::Release()
{
	return ServiceBase::Release();
}

bool DiscordRichPresenceService::Init()
{
	constexpr uint32_t USEnglishLanguageId = 1;

	bool result = true;

	cIGZLanguageManagerPtr pLM;
	cIGZMessageServer2Ptr pMS2;

	if (pLM && pMS2)
	{
		for (const auto& id : MessageIds)
		{
			pMS2->AddNotification(this, id);
		}

		pLanguageUtility = pLM->GetNewLanguageUtility(USEnglishLanguageId);

		if (pLanguageUtility)
		{
			result = cityStatusProvider.Init();

			if (result)
			{
				discord::Core* instance = nullptr;

				discord::Result discordStatus = discord::Core::Create(APPLICATION_ID, DiscordCreateFlags_NoRequireDiscord, &instance);

				if (discordStatus == discord::Result::Ok)
				{
					discord.reset(instance);

#ifdef _DEBUG
					discord->SetLogHook(discord::LogLevel::Debug, DebugLogHook);
#endif // _DEBUG

					activity.GetAssets().SetLargeImage("sc4_icon_1024");
					activity.SetType(discord::ActivityType::Playing);

					// Set the user's status to Playing.
					activityLastUpdateTime = std::chrono::system_clock::now();
					discord->ActivityManager().UpdateActivity(activity, DiscordAPICallback);
					result = discord->RunCallbacks() == discord::Result::Ok;
				}
				else
				{
					result = false;
				}
			}
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool DiscordRichPresenceService::Shutdown()
{
	cIGZMessageServer2Ptr pMS2;

	if (pMS2)
	{
		for (const auto& id : MessageIds)
		{
			pMS2->RemoveNotification(this, id);
		}
	}

	if (pLanguageUtility)
	{
		pLanguageUtility->Release();
		pLanguageUtility = nullptr;
	}

	if (discord)
	{
		discord->ActivityManager().ClearActivity(DiscordAPICallback);
		discord->RunCallbacks();
	}

	return cityStatusProvider.Shutdown();
}

cRZBaseString DiscordRichPresenceService::GetUSEnglishNumberString(int64_t value, NumberType type)
{
	cRZBaseString result;

	if (pLanguageUtility)
	{
		if (type == NumberType::Number)
		{
			pLanguageUtility->MakeNumberString(value, result);
		}
		else if (type == NumberType::Money)
		{
			// The currently symbol string is the hexadecimal-escaped UTF-8 encoding
			// of the section symbol (§).
			// The \xC2 value is the first byte in a two byte UTF-8 sequence, and the
			// \xA7 value is the Unicode value of the section symbol (U+00A7).
			// See the following page for more information on UTF-8 encoding:
			// https://www.fileformat.info/info/unicode/utf8.htm
			//
			// UTF-8 is SC4's native string encoding.

			static const cRZBaseString currencySymbol("\xC2\xA7");

			pLanguageUtility->MakeMoneyString(value, result, &currencySymbol);
		}
	}

	return result;
}

bool DiscordRichPresenceService::DoMessage(cIGZMessage2* pMsg)
{
	cIGZMessage2Standard* pStandardMsg = static_cast<cIGZMessage2Standard*>(pMsg);

	switch (pStandardMsg->GetType())
	{
	case kSC4MessageCityEstablished:
		CityEstablished(pStandardMsg);
		break;
	case kSC4MessageCityNameChanged:
		CityNameChanged(pStandardMsg);
		break;
	case kSC4MessagePostCityInit:
		PostCityInit(pStandardMsg);
		break;
	case kSC4MessagePostRegionInit:
		PostRegionInit();
		break;
	case kSC4MessagePreRegionShutdown:
		view = DiscordView::Unknown;
		break;
	}

	return true;
}

void DiscordRichPresenceService::CityEstablished(cIGZMessage2Standard* pStandardMsg)
{
	SetCityViewPresence(static_cast<cISC4City*>(pStandardMsg->GetVoid1()));
}

void DiscordRichPresenceService::CityNameChanged(cIGZMessage2Standard* pStandardMsg)
{
	UpdateCityName(static_cast<cISC4City*>(pStandardMsg->GetVoid1()));
	activityNeedsUpdate = true;
}

void DiscordRichPresenceService::PostCityInit(cIGZMessage2Standard* pStandardMsg)
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

			const std::string_view regionDetailsText = activity.GetDetails();

			if (regionDetailsText.starts_with("Region: "))
			{
				// Trim the Region: prefix and the space that follows it.
				const std::string_view regionName = regionDetailsText.substr(8);

				details.append(" in ").append(regionName);
			}

			activity.SetDetails(details.c_str());
			activity.SetState("");
			activity.GetTimestamps().SetStart(0);
			view = DiscordView::UnestablishedCity;
			activityNeedsUpdate = true;
		}
	}
}

void DiscordRichPresenceService::PostRegionInit()
{
	if (view != DiscordView::Region)
	{
		view = DiscordView::Region;

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

				activity.SetDetails(details.c_str());

				regionStatusProvider.SetupRegionStatusData(pRegion);
				currentRegionStatus = RegionStatusType::TotalResidentialPopulation;
				SetRegionStatusText();
				activity.GetTimestamps().SetStart(0);
				activityNeedsUpdate = true;
			}
		}
	}
}

void DiscordRichPresenceService::SetCityStatusText()
{
	char buffer[1024]{};

	switch (currentCityStatus)
	{
	case CityStatusType::MayorName:
	default:
		std::snprintf(buffer, sizeof(buffer), "Mayor: %s", cityStatusProvider.GetMayorName().ToChar());
		break;
	case CityStatusType::MayorRating:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Mayor Rating: %s",
			GetUSEnglishNumberString(cityStatusProvider.GetMayorRating()).ToChar());
		break;
	case CityStatusType::ResidentialPopulation:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Residential Pop. %s",
			GetUSEnglishNumberString(cityStatusProvider.GetResidentalPopulation()).ToChar());
		break;
	case CityStatusType::CommercialPopulation:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Commercial Pop. %s",
			GetUSEnglishNumberString(cityStatusProvider.GetCommercialPopulation()).ToChar());
		break;
	case CityStatusType::IndustrialPopulation:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Industrial Pop. %s",
			GetUSEnglishNumberString(cityStatusProvider.GetIndustrialPopulation()).ToChar());
		break;

	case CityStatusType::CityAgeInYears:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"City Age in Years: %s",
			GetUSEnglishNumberString(cityStatusProvider.GetCityAgeInYears()).ToChar());
		break;
	case CityStatusType::MonthlyNetIncome:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Monthly Net Income: %s",
			GetUSEnglishNumberString(cityStatusProvider.GetMonthlyNetIncome(), NumberType::Money).ToChar());
		break;
	case CityStatusType::TotalFunds:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Total Funds: %s",
			GetUSEnglishNumberString(cityStatusProvider.GetTotalFunds(), NumberType::Money).ToChar());
		break;
	}

	activity.SetState(buffer);
}

void DiscordRichPresenceService::SetRegionStatusText()
{
	char buffer[1024]{};

	switch (currentRegionStatus)
	{
	case RegionStatusType::TotalResidentialPopulation:
	default:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Population: %s",
			GetUSEnglishNumberString(regionStatusProvider.GetTotalResidentialPopulation()).ToChar());
		break;
	case RegionStatusType::TotalCommercialJobs:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Commercial Jobs: %s",
			GetUSEnglishNumberString(regionStatusProvider.GetTotalCommercialJobs()).ToChar());
		break;
	case RegionStatusType::TotalIndustrialJobs:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Industrial Jobs: %s",
			GetUSEnglishNumberString(regionStatusProvider.GetTotalIndustrialJobs()).ToChar());
		break;
	case RegionStatusType::TotalFunds:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Total Funds: %s",
			GetUSEnglishNumberString(regionStatusProvider.GetTotalFunds(), NumberType::Money).ToChar());
		break;
	case RegionStatusType::TotalCities:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Total Cities: %s",
			GetUSEnglishNumberString(regionStatusProvider.GetTotalCities()).ToChar());
		break;
	case RegionStatusType::DevelopedCityCount:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Developed Cities: %s",
			GetUSEnglishNumberString(regionStatusProvider.GetDevelopedCityCount()).ToChar());
		break;
	case RegionStatusType::UndevelopedCityCount:
		std::snprintf(
			buffer,
			sizeof(buffer),
			"Undeveloped Cities: %s",
			GetUSEnglishNumberString(regionStatusProvider.GetUndevelopedCityCount()).ToChar());
		break;
	}

	activity.SetState(buffer);
}

void DiscordRichPresenceService::SetCityViewPresence(cISC4City* pCity)
{
	if (pCity)
	{
		UpdateCityName(pCity);
		cityStatusProvider.SetupCityStatusData(pCity);
		currentCityStatus = CityStatusType::MayorName;
		SetCityStatusText();

		activity.GetTimestamps().SetStart(time(nullptr));
		view = DiscordView::EstablishedCity;
		activityNeedsUpdate = true;
	}
}

void DiscordRichPresenceService::UpdateCityName(cISC4City* pCity)
{
	if (pCity)
	{
		cRZBaseString cityName;

		pCity->GetCityName(cityName);

		std::string details("City: ");
		details.append(cityName.ToChar(), cityName.Strlen());

		activity.SetDetails(details.c_str());
	}
}

bool DiscordRichPresenceService::OnIdle(uint32_t unknown1)
{
	if (discord)
	{
		if (discord->RunCallbacks() == discord::Result::Ok)
		{
			// The Discord API requires a minimum of 5 seconds between activity updates.
			if (activityNeedsUpdate
				&& (std::chrono::system_clock::now() - activityLastUpdateTime) > std::chrono::seconds(5))
			{
				activityNeedsUpdate = false;
				activityLastUpdateTime = std::chrono::system_clock::now();

				discord->ActivityManager().UpdateActivity(activity, DiscordAPICallback);
			}
			else
			{
				if ((std::chrono::system_clock::now() - statusLastUpdateTime) > std::chrono::seconds(30))
				{
					statusLastUpdateTime = std::chrono::system_clock::now();

					if (view == DiscordView::EstablishedCity)
					{
						switch (currentCityStatus)
						{
						case CityStatusType::MayorName:
							currentCityStatus = CityStatusType::MayorRating;
							break;
						case CityStatusType::MayorRating:
							currentCityStatus = CityStatusType::ResidentialPopulation;
							break;
						case CityStatusType::ResidentialPopulation:
							currentCityStatus = CityStatusType::CommercialPopulation;
							break;
						case CityStatusType::CommercialPopulation:
							currentCityStatus = CityStatusType::IndustrialPopulation;
							break;
						case CityStatusType::IndustrialPopulation:
							currentCityStatus = CityStatusType::CityAgeInYears;
							break;
						case CityStatusType::CityAgeInYears:
							currentCityStatus = CityStatusType::MonthlyNetIncome;
							break;
						case CityStatusType::MonthlyNetIncome:
							currentCityStatus = CityStatusType::TotalFunds;
							break;
						case CityStatusType::TotalFunds:
						default:
							currentCityStatus = CityStatusType::MayorName;
							break;
						}
						SetCityStatusText();

						activityNeedsUpdate = true;
					}
					else if (view == DiscordView::Region)
					{
						switch (currentRegionStatus)
						{
						case RegionStatusType::TotalResidentialPopulation:
							currentRegionStatus = RegionStatusType::TotalCommercialJobs;
							break;
						case RegionStatusType::TotalCommercialJobs:
							currentRegionStatus = RegionStatusType::TotalIndustrialJobs;
							break;
						case RegionStatusType::TotalIndustrialJobs:
							currentRegionStatus = RegionStatusType::TotalFunds;
							break;
						case RegionStatusType::TotalFunds:
							currentRegionStatus = RegionStatusType::TotalCities;
							break;
						case RegionStatusType::TotalCities:
							currentRegionStatus = RegionStatusType::DevelopedCityCount;
							break;
						case RegionStatusType::DevelopedCityCount:
							currentRegionStatus = RegionStatusType::UndevelopedCityCount;
							break;
						case RegionStatusType::UndevelopedCityCount:
						default:
							currentRegionStatus = RegionStatusType::TotalResidentialPopulation;
							break;
						}
						SetRegionStatusText();

						activityNeedsUpdate = true;
					}
				}
			}
		}
	}

	return true;
}
