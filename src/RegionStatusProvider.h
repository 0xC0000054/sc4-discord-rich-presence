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
#include <cstdint>

class cISC4Region;

class RegionStatusProvider
{
public:
	RegionStatusProvider();

	int64_t GetTotalResidentialPopulation() const;
	int64_t GetTotalCommercialJobs() const;
	int64_t GetTotalIndustrialJobs() const;
	int64_t GetTotalFunds() const;

	uint32_t GetTotalCities() const;
	uint32_t GetDevelopedCityCount() const;
	uint32_t GetUndevelopedCityCount() const;

	void SetupRegionStatusData(cISC4Region*);

private:
	int64_t totalResidentialPopulation;
	int64_t totalCommercialJobs;
	int64_t totalIndustrialJobs;
	int64_t totalFunds;
	size_t totalCities;
	size_t developedCityCount;
	size_t undevelopedCityCount;
};

