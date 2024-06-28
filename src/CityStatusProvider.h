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
#include "cIGZMessageTarget2.h"
#include "cRZBaseString.h"
#include <cstdint>
#include <string>

class cIGZMessage2Standard;
class cISC4City;

class CityStatusProvider : private cIGZMessageTarget2
{
public:
	CityStatusProvider();

	bool Init();

	bool Shutdown();

	const cRZBaseString& GetMayorName() const;
	int32_t GetResidentalPopulation() const;
	int32_t GetCommercialPopulation() const;
	int32_t GetIndustrialPopulation() const;
	int32_t GetMayorRating() const;
	int32_t GetCityAgeInYears() const;
	int32_t GetMonthlyNetIncome() const;
	int64_t GetTotalFunds() const;

	void SetupCityStatusData(cISC4City*);

private:
	bool QueryInterface(uint32_t riid, void** ppvObj);
	uint32_t AddRef();
	uint32_t Release();

	bool DoMessage(cIGZMessage2*);

	void HistoryWarehouseRecordChanged(cIGZMessage2Standard*);
	void SimNewMonth();
	void SimNewYear(cIGZMessage2Standard*);
	void UpdateCityFunds(cIGZMessage2Standard*);
	void UpdateMayorName(cIGZMessage2Standard*);

	uint32_t refCount;
	cRZBaseString mayorName;
	int32_t residentialPopulation;
	int32_t commercialPopulation;
	int32_t industrialPopulation;
	int32_t mayorRating;
	int32_t cityAgeInYears;
	int32_t monthlyNetIncome;
	int64_t totalFunds;
};

