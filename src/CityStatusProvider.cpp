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

#include "CityStatusProvider.h"
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cISC4App.h"
#include "cISC4AuraSimulator.h"
#include "cISC4BudgetSimulator.h"
#include "cISC4City.h"
#include "cISC4Demand.h"
#include "cISC4DemandSimulator.h"
#include "cISC4Region.h"
#include "cISC4ResidentialSimulator.h"
#include "cISC4Simulator.h"
#include "GZCLSIDDefs.h"
#include "GZServPtrs.h"
#include <array>
#include <vector>

static constexpr uint32_t kSC4MessageFundsChanged = 0x772FAD4;
static constexpr uint32_t kSC4MessageMayorNameChanged = 0xAB99381;
static constexpr uint32_t kSC4MessageSimNewMonth = 0x66956816;
static constexpr uint32_t kSC4MessageSimNewYear = 0x66956817;
static constexpr uint32_t kSC4MessageHistoryWarehouseRecordChanged = 0x89EFA536; // Same as kSC4CLSID_cSC4HistoryWarehouse

constexpr int32_t kSC4StartYear = 2000; // SC4 starts in the year 2000.

static const std::array<uint32_t, 5> MessageIds =
{
	kSC4MessageFundsChanged,
	kSC4MessageMayorNameChanged,
	kSC4MessageSimNewMonth,
	kSC4MessageSimNewYear,
	kSC4MessageHistoryWarehouseRecordChanged,
};

static const std::vector<uint32_t> CommercialDemandIds =
{
	0x3111,
	0x3121,
	0x3131,
	0x3321,
	0x3331,
};

static const std::vector<uint32_t> IndustrialDemandIds =
{
	0x4101,
	0x4201,
	0x4301,
	0x4401,
};

namespace
{
	int32_t GetTotalJobsBySensus(cISC4DemandSimulator& demandSim, std::vector<uint32_t> demandIds)
	{
		int32_t total = 0;

		for (const auto& demandId : demandIds)
		{
			total += static_cast<int32_t>(demandSim.GetJobsBySensus(demandId));
		}

		return total;
	}
}

CityStatusProvider::CityStatusProvider()
	: refCount(0),
	  mayorName(),
	  residentialPopulation(0),
	  commercialPopulation(0),
	  industrialPopulation(0),
	  mayorRating(0),
	  cityAgeInYears(0),
	  monthlyNetIncome(0),
	  totalFunds(0)
{
}

bool CityStatusProvider::Init()
{
	bool result = true;

	cIGZMessageServer2Ptr pMS2;

	if (pMS2)
	{
		for (const auto& id : MessageIds)
		{
			pMS2->AddNotification(this, id);
		}
	}
	else
	{
		result = false;
	}

	return true;
}

bool CityStatusProvider::Shutdown()
{
	bool result = true;

	cIGZMessageServer2Ptr pMsgServ;

	if (pMsgServ)
	{
		for (const auto& id : MessageIds)
		{
			pMsgServ->RemoveNotification(this, id);
		}
	}
	else
	{
		result = false;
	}

	return true;
}

const cRZBaseString& CityStatusProvider::GetMayorName() const
{
	return mayorName;
}

int32_t CityStatusProvider::GetResidentalPopulation() const
{
	return residentialPopulation;
}

int32_t CityStatusProvider::GetCommercialPopulation() const
{
	return commercialPopulation;
}

int32_t CityStatusProvider::GetIndustrialPopulation() const
{
	return industrialPopulation;
}

int32_t CityStatusProvider::GetMayorRating() const
{
	return mayorRating;
}

int32_t CityStatusProvider::GetCityAgeInYears() const
{
	return cityAgeInYears;
}

int32_t CityStatusProvider::GetMonthlyNetIncome() const
{
	return monthlyNetIncome;
}

int64_t CityStatusProvider::GetTotalFunds() const
{
	return totalFunds;
}

void CityStatusProvider::SetupCityStatusData(cISC4City* pCity)
{
	mayorName.FromChar("");
	residentialPopulation = 0;
	commercialPopulation = 0;
	industrialPopulation = 0;
	mayorRating = 0;
	cityAgeInYears = 0;
	monthlyNetIncome = 0;
	totalFunds = 0;

	if (pCity)
	{
		pCity->GetMayorName(mayorName);

		cISC4DemandSimulator* pDemandSim = pCity->GetDemandSimulator();
		cISC4ResidentialSimulator* pResidentialSim = pCity->GetResidentialSimulator();

		if (pDemandSim && pResidentialSim)
		{
			residentialPopulation = pResidentialSim->GetPopulation();
			commercialPopulation = GetTotalJobsBySensus(*pDemandSim, CommercialDemandIds);
			industrialPopulation = GetTotalJobsBySensus(*pDemandSim, IndustrialDemandIds);
		}

		cISC4AuraSimulator* pAuraSim = pCity->GetAuraSimulator();

		if (pAuraSim)
		{
			mayorRating = pAuraSim->GetMayorRating();
		}

		cISC4Simulator* pSim = pCity->GetSimulator();

		if (pSim)
		{
			int32_t currentYear;
			pSim->GetSimDate(&currentYear, nullptr, nullptr, nullptr, nullptr);

			cityAgeInYears = currentYear - kSC4StartYear;
		}

		cISC4BudgetSimulator* pBudgetSim = pCity->GetBudgetSimulator();

		if (pBudgetSim)
		{
			totalFunds = pBudgetSim->GetTotalFunds();
			monthlyNetIncome = pBudgetSim->GetTotalMonthlyIncome() - pBudgetSim->GetTotalMonthlyExpense();
		}
	}
}

bool CityStatusProvider::QueryInterface(uint32_t riid, void** ppvObj)
{
	if (riid == GZCLSID::kcIGZMessageTarget2)
	{
		*ppvObj = static_cast<cIGZMessageTarget2*>(this);
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

uint32_t CityStatusProvider::AddRef()
{
	return ++refCount;
}

uint32_t CityStatusProvider::Release()
{
	if (refCount > 0)
	{
		--refCount;
	}

	return refCount;
}

bool CityStatusProvider::DoMessage(cIGZMessage2* pMsg)
{
	cIGZMessage2Standard* pStandardMsg = static_cast<cIGZMessage2Standard*>(pMsg);

	switch (pMsg->GetType())
	{
	case kSC4MessageFundsChanged:
		UpdateCityFunds(pStandardMsg);
		break;
	case kSC4MessageHistoryWarehouseRecordChanged:
		HistoryWarehouseRecordChanged(pStandardMsg);
		break;
	case kSC4MessageMayorNameChanged:
		UpdateMayorName(pStandardMsg);
		break;
	case kSC4MessageSimNewMonth:
		SimNewMonth();
		break;
	case kSC4MessageSimNewYear:
		SimNewYear(pStandardMsg);
		break;
	}

	return true;
}

void CityStatusProvider::HistoryWarehouseRecordChanged(cIGZMessage2Standard* pStandardMsg)
{
	constexpr int32_t kCommercialJobs = 0x0A4E2056;
	constexpr int32_t kIndustrialJobs = 0x4A4E206B;
	constexpr uint32_t kMayorRating = 0x0A5CBF37;
	constexpr uint32_t kResidentialPopulation = 0xAA1A2CCA;

	const uint32_t id = static_cast<uint32_t>(pStandardMsg->GetData1());

	switch (id)
	{
	case kCommercialJobs:
		commercialPopulation = static_cast<int32_t>(pStandardMsg->GetData2());
		break;
	case kIndustrialJobs:
		industrialPopulation = static_cast<int32_t>(pStandardMsg->GetData2());
		break;
	case kMayorRating:
		mayorRating = static_cast<int32_t>(pStandardMsg->GetData2());
		break;
	case kResidentialPopulation:
		residentialPopulation = static_cast<int32_t>(pStandardMsg->GetData2());
		break;
	}
}

void CityStatusProvider::SimNewMonth()
{
	cISC4AppPtr pSC4App;

	if (pSC4App)
	{
		cISC4City* pCity = pSC4App->GetCity();

		if (pCity)
		{
			cISC4BudgetSimulator* pBudgetSim = pCity->GetBudgetSimulator();

			if (pBudgetSim)
			{
				monthlyNetIncome = pBudgetSim->GetTotalMonthlyIncome() - pBudgetSim->GetTotalMonthlyExpense();
			}
		}
	}
}

void CityStatusProvider::SimNewYear(cIGZMessage2Standard* pStandardMsg)
{
	int32_t currentYear = static_cast<int32_t>(pStandardMsg->GetData3());

	cityAgeInYears = currentYear - kSC4StartYear;
}

void CityStatusProvider::UpdateCityFunds(cIGZMessage2Standard* pStandardMsg)
{
	cISC4BudgetSimulator* pBudgetSim = static_cast<cISC4BudgetSimulator*>(pStandardMsg->GetVoid1());

	if (pBudgetSim)
	{
		totalFunds = pBudgetSim->GetTotalFunds();
	}
}

void CityStatusProvider::UpdateMayorName(cIGZMessage2Standard* pStandardMsg)
{
	cISC4City* pCity = static_cast<cISC4City*>(pStandardMsg->GetVoid1());

	if (pCity)
	{
		pCity->GetMayorName(mayorName);
	}
}
