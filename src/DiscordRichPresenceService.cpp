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
#include "cRZCOMDllDirector.h"
#include <Windows.h>
#include "wil/resource.h"
#include "wil/win32_helpers.h"

static constexpr uint32_t kDiscordRichPresenceServiceID = 0xFE95AAEA;

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
	  activityNeedsUpdate(false)
{
}

bool DiscordRichPresenceService::Init()
{
	bool result = true;

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

	return result;
}

bool DiscordRichPresenceService::Shutdown()
{
	if (discord)
	{
		discord->ActivityManager().ClearActivity(DiscordAPICallback);
		discord->RunCallbacks();
	}

	return true;
}

std::string DiscordRichPresenceService::GetDetails() const
{
	return std::string(activity.GetDetails());
}

void DiscordRichPresenceService::UpdatePresence(
	const char* const details,
	bool startElapsedTimer)
{
	UpdatePresence(details, nullptr, startElapsedTimer);
}

void DiscordRichPresenceService::UpdatePresence(
	const char* const details,
	const char* const state,
	bool startElapsedTimer)
{
	activity.SetDetails(details);

	if (state)
	{
		activity.SetState(state);
	}
	else
	{
		activity.SetState("");
	}

	if (startElapsedTimer)
	{
		activity.GetTimestamps().SetStart(time(nullptr));
	}
	else
	{
		activity.GetTimestamps().SetStart(0);
	}

	activityNeedsUpdate = true;
}

void DiscordRichPresenceService::UpdateState(const char* const state)
{
	if (state)
	{
		activity.SetState(state);
	}
	else
	{
		activity.SetState("");
	}
	activityNeedsUpdate = true;
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
		}
	}

	return true;
}
