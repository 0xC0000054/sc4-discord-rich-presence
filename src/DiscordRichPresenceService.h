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
#include "ServiceBase.h"
#include "CityStatusProvider.h"
#include "cIGZMessageTarget2.h"
#include "discord-game-sdk/discord.h"
#include <atomic>
#include <chrono>
#include <memory>

class cIGZLanguageUtility;
class cIGZMessage2Standard;
class cISC4City;
class cISC4Region;

class DiscordRichPresenceService final : public ServiceBase, private cIGZMessageTarget2
{
public:
	DiscordRichPresenceService();

	bool QueryInterface(uint32_t riid, void** ppvObj) override;
	uint32_t AddRef() override;
	uint32_t Release() override;

	bool Init() override;
	bool Shutdown() override;

private:
	enum class CityStatusType : int32_t
	{
		MayorName,
		MayorRating,
		ResidentialPopulation,
		CommercialPopulation,
		IndustrialPopulation,
		CityAgeInYears,
		MonthlyNetIncome,
		TotalFunds,
	};

	enum class DiscordView : int32_t
	{
		Unknown,
		Region,
		EstablishedCity,
		UnestablishedCity,
	};

	enum class NumberType
	{
		Number,
		Money
	};

	cRZBaseString GetUSEnglishNumberString(int64_t value, NumberType type = NumberType::Number);

	bool DoMessage(cIGZMessage2* pMsg);

	void CityEstablished(cIGZMessage2Standard*);

	void CityNameChanged(cIGZMessage2Standard*);

	void PostCityInit(cIGZMessage2Standard* pStandardMsg);

	void PostRegionInit();

	void SetCityStatusText();

	void SetCityViewPresence(cISC4City* pCity);

	void UpdateCityName(cISC4City*);

	bool OnIdle(uint32_t unknown1) override;

	std::unique_ptr<discord::Core> discord;
	discord::Activity activity;
	std::chrono::time_point<std::chrono::system_clock> activityLastUpdateTime;
	std::chrono::time_point<std::chrono::system_clock> statusLastUpdateTime;
	std::atomic_bool activityNeedsUpdate;
	CityStatusProvider cityStatusProvider;
	CityStatusType currentCityStatus;
	std::atomic<DiscordView> view;
	cIGZLanguageUtility* pLanguageUtility;
};

