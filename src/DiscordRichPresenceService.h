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
	 * @param details A short line shown in the user profile, In Region View, In City View, etc.
	 * @param startElapsedTimer true to start Discord's elapsed timer; otherwise, false.
	 */
	void UpdatePresence(
		const char* const details,
		bool startElapsedTimer = true);

	/**
	 * @brief Updates the Discord rich presence text.
	 * @param details A short line shown in the user profile, In Region View, In City View, etc.
	 * @param largeImageToolTip Additional context for the details line, region name, city name, etc.
	 * @param startElapsedTimer true to start Discord's elapsed timer; otherwise, false.
	 */
	void UpdatePresence(
		const char* const details,
		const char* const largeImageToolTip,
		bool startElapsedTimer = true);

private:
	bool OnIdle(uint32_t unknown1) override;

	std::unique_ptr<discord::Core> discord;
	discord::Activity activity;

	std::chrono::time_point<std::chrono::system_clock> activityLastUpdateTime;
	std::atomic_bool activityNeedsUpdate;
};

