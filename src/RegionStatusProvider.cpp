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

#include "RegionStatusProvider.h"
#include "cISC4Region.h"
#include "cISC4RegionalCity.h"

RegionStatusProvider::RegionStatusProvider()
	: totalResidentialPopulation(0),
	  totalCommercialJobs(0),
	  totalIndustrialJobs(0),
	  totalFunds(0),
	  totalCities(0),
	  developedCityCount(0),
	  undevelopedCityCount(0)
{
}

int64_t RegionStatusProvider::GetTotalResidentialPopulation() const
{
	return totalResidentialPopulation;
}

int64_t RegionStatusProvider::GetTotalCommercialJobs() const
{
	return totalCommercialJobs;
}

int64_t RegionStatusProvider::GetTotalIndustrialJobs() const
{
	return totalIndustrialJobs;
}

int64_t RegionStatusProvider::GetTotalFunds() const
{
	return totalFunds;
}

uint32_t RegionStatusProvider::GetTotalCities() const
{
	return totalCities;
}

uint32_t RegionStatusProvider::GetDevelopedCityCount() const
{
	return developedCityCount;
}

uint32_t RegionStatusProvider::GetUndevelopedCityCount() const
{
	return undevelopedCityCount;
}

void RegionStatusProvider::SetupRegionStatusData(cISC4Region* pRegion)
{
	totalResidentialPopulation = 0;
	totalCommercialJobs = 0;
	totalIndustrialJobs = 0;
	totalFunds = 0;
	totalCities = 0;
	developedCityCount = 0;
	undevelopedCityCount = 0;

	if (pRegion)
	{
		eastl::vector<cISC4Region::cLocation> cityLocations;

		pRegion->GetCityLocations(cityLocations);

		uint32_t count = cityLocations.size();

		totalCities = count;

		for (uint32_t i = 0; i < count; i++)
		{
			const cISC4Region::cLocation& cityLocation = cityLocations[i];

			cISC4RegionalCity** ppRegionalCity = pRegion->GetCity(cityLocation.x, cityLocation.y);

			if (ppRegionalCity && *ppRegionalCity)
			{
				cISC4RegionalCity* pRegionalCity = *ppRegionalCity;

				if (pRegionalCity->GetEstablished())
				{
					totalResidentialPopulation += pRegionalCity->GetPopulation();
					totalCommercialJobs += pRegionalCity->GetCommercialJobs();
					totalIndustrialJobs += pRegionalCity->GetIndustrialJobs();
					totalFunds += static_cast<int64_t>(pRegionalCity->GetBudget());
					developedCityCount++;
				}
				else
				{
					undevelopedCityCount++;
				}
			}
		}
	}
}

