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
#include "discord-game-sdk/discord.h"
#include <atomic>
#include <chrono>
#include <memory>

class DiscordRichPresenceService final : public ServiceBase
{
public:
	DiscordRichPresenceService();

	bool Init() override;
	bool Shutdown() override;

	/**
	 * @brief Updates the Discord rich presence text.
	 * @param details A short line shown in the user profile, Region: <name>, Building <city name>.
	 * @param startElapsedTimer true to start Discord's elapsed timer; otherwise, false.
	 */
	void UpdatePresence(
		const char* const details,
		bool startElapsedTimer = true);

	/**
	 * @brief Updates the Discord rich presence text.
	 * @param details A short line shown in the user profile, Region: <name>, Building <city name>.
	 * @param state Additional context for the details line, region size, mayor rating, etc.
	 * @param startElapsedTimer true to start Discord's elapsed timer; otherwise, false.
	 */
	void UpdatePresence(
		const char* const details,
		const char* const state,
		bool startElapsedTimer = true);

	/**
	 * @brief Updates the Discord rich presence status text.
	 * @param state This line is used for items like region size, mayor rating, etc.
	 */
	void UpdateState(const char* const state);

private:
	bool OnIdle(uint32_t unknown1) override;

	std::unique_ptr<discord::Core> discord;
	discord::Activity activity;

	std::chrono::time_point<std::chrono::system_clock> activityLastUpdateTime;
	std::atomic_bool activityNeedsUpdate;
};

