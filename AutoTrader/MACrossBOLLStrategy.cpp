#include "stdafx.h"
#include "MACrossBOLLStrategy.h"
#include "MACrossBOLLTech.h"
#include "Order.h"
#include <assert.h>
#include "TickWrapper.h"
#include "BOLLTech.h"
#include "TechUtils.h"
#include <math.h>

MACrossBOLLStrategy::MACrossBOLLStrategy(size_t short_ma, size_t long_ma, size_t boll_period)
: m_curOrder(new Order())
, m_shortMA(short_ma)
, m_longMA(long_ma)
, m_bollperiod(boll_period)
{
}


MACrossBOLLStrategy::~MACrossBOLLStrategy()
{
}


bool MACrossBOLLStrategy::tryInvoke(const std::list<TickWrapper>& data, TickWrapper& info){
	TickType direction = TickType::Commom;
	const size_t breakthrough_confirm_duration = 100; //50ms
	MACrossBOLLTech* curPtr = new MACrossBOLLTech(CrossStratgyType::MA, m_shortMA, m_longMA, info.UUID(), info.InstrumentId(), info.Time(), info.LastPrice());
	bool orderSingal = false;
	double short_ma = calculateK(data, info, m_shortMA * 60);
	double long_ma = calculateK(data, info, m_longMA * 60);
	BOLLTech bolltech = calculateBoll(data, info, m_bollperiod * 60);
	curPtr->setShortMA(short_ma);
	curPtr->setLongMA(long_ma);
	curPtr->setBollTech(bolltech);


	if (!data.empty()){
		if (curPtr->IsTriggerPoint()){ // up
			if (!data.empty() && data.size() > 500){
				std::list<TickWrapper>::const_iterator stoper = data.begin();
				std::advance(stoper, breakthrough_confirm_duration);
				for (auto it = data.begin(); it != stoper; it++){
					StrategyTech* prePtr = it->GetTechVec();
					// if prePtr == NULL, mean it's recovered from db, so that md is not continuous. so it's should not be singal point.
					if (prePtr == NULL || !prePtr->IsTriggerPoint())
					{
						// not special point
						orderSingal = false;
						break;
					}
					orderSingal = true;
				}
				//special point
				if (orderSingal){
					//update m_curOrder
					m_curOrder->SetInstrumentId(info.InstrumentId());
					m_curOrder->SetRefExchangePrice(info.LastPrice());
					m_curOrder->SetExchangeDirection(ExchangeDirection::Buy);
					m_curOrder->SetCombOffsetFlagType(Order::FAK);
					curPtr->SetTickType(TickType::BuyPoint);
				}
			}
		}
		else{ // down
			if (!data.empty() && data.size() > 500){
				std::list<TickWrapper>::const_iterator stoper = data.begin();
				std::advance(stoper, breakthrough_confirm_duration);
				for (auto it = data.begin(); it != stoper; it++){
					StrategyTech* prePtr = it->GetTechVec();
					if (prePtr == NULL || prePtr->IsTriggerPoint())
					{
						// not special point
						orderSingal = false;
						break;
					}
					orderSingal = true;
				}
				if (orderSingal){
					//special point
					m_curOrder->SetInstrumentId(info.InstrumentId());
					m_curOrder->SetRefExchangePrice(info.LastPrice());
					m_curOrder->SetExchangeDirection(ExchangeDirection::Sell);
					m_curOrder->SetCombOffsetFlagType(Order::FAK);
					curPtr->SetTickType(TickType::SellPoint);
				}
			}
		}
	}

	//info.SetTechVec((StrategyTech*)curPtr);
	info.m_techvec = curPtr;
	return orderSingal;
}

Order MACrossBOLLStrategy::generateOrder(){
	assert(m_curOrder);
	return *m_curOrder;
}

BOLLTech MACrossBOLLStrategy::calculateBoll(const std::list<TickWrapper>& data, const TickWrapper& current, int seconds) const{
	double ma = TechUtils::CalulateMA(data, current, seconds);
	int tickCount = 2 * seconds;

	double total = 0;
	long long count = 0;

	long long leftedge = current.toTimeStamp() - tickCount;
	for (auto it = data.begin(); it != data.end(); it++)
	{
		if (it->toTimeStamp() > leftedge){
			double delta = it->LastPrice() - ma;
			total += (delta * delta);
			++count;
		}
		else{
			break;
		}
	}
	double var = sqrt(total / count);

	return BOLLTech(ma, var, current.LastPrice());
}


double MACrossBOLLStrategy::calculateK(const std::list<TickWrapper>& data, const TickWrapper& current, int seconds) const{
	return TechUtils::CalulateMA(data, current, seconds);
}