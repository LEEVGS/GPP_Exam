#include "stdafx.h"
#include "StatesAndTransitions.h"
#include  <iostream>

void states::Wanderstate::OnEnter(Blackboard* pBlackboard){}

void states::HouseSeek::OnEnter(Blackboard* pBlackboard)
{
	std::vector<HouseInfo>* houses;
	if (pBlackboard->GetData("NewHouses", houses) == false)
	{
		return;
	}
	auto ptr = houses->end() - 1;
	Elite::Vector2 center = ptr->Center;
	Elite::Vector2 size = ptr->Size;
	size -= {10.f, 10.f};
	m_cornors.push_back({ center.x - size.x / 2,center.y - size.y / 2 });
	m_cornors.push_back({ center.x + size.x / 2,center.y + size.y / 2 });
}

void states::HouseSeek::Update(Blackboard* pBlackboard, float deltaTime)
{
	Elite::Vector2* pTarget;
	AgentInfo* pAgent;
	if (pBlackboard->GetData("Target", pTarget) == false || pTarget == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Agent", pAgent) == false || pAgent == nullptr)
	{
		return;
	}

	if (m_cornors.empty())
	{
		return;
	}

	if (pAgent->Position.DistanceSquared(m_cornors[m_cornors.size()-1]) < 10.f)
	{
		m_cornors.pop_back();
	}

	if (m_cornors.empty())
	{
		std::vector<HouseInfo>* houses;
		std::vector<HouseInfo>* visitedHouses;
		if (pBlackboard->GetData("NewHouses", houses) == false)
		{
			return;
		}
		if (pBlackboard->GetData("VisitedHouses", visitedHouses) == false)
		{
			return;
		}
		visitedHouses->push_back(*(houses->end() - 1));
		houses->pop_back();
		return;
	}

	pTarget->x = m_cornors[m_cornors.size()-1].x;
	pTarget->y = m_cornors[m_cornors.size() - 1].y;
}
void states::Wanderstate::Update(Blackboard* pBlackboard, float deltaTime)
{
	constexpr float m_OffSetDistance = 6.f;
	constexpr float m_Radius = 10.f;
	constexpr int m_MaxAngularChange = 45;
	

	Elite::Vector2* pTarget;
	AgentInfo* pAgent;
	if (pBlackboard->GetData("Target", pTarget) == false || pTarget == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Agent", pAgent) == false || pAgent == nullptr)
	{
		return;
	}

	const Elite::Vector2 pointCircle{ pAgent->Position + (pAgent->LinearVelocity.GetNormalized() * m_OffSetDistance) };
	const auto degreeAngelChange = static_cast<float>(rand() % (m_MaxAngularChange * 2) - m_MaxAngularChange);
	m_WanderAngle += Elite::ToRadians(degreeAngelChange);

	const Elite::Vector2 pointOnCircle = pointCircle + (Elite::Vector2(cosf(m_WanderAngle) * m_Radius, sinf(m_WanderAngle) * m_Radius));

	pTarget->x = pointOnCircle.x;
	pTarget->y = pointOnCircle.y;
}


bool conditions::DefaultWanderingCondition::Evaluate(Blackboard* pBlackboard) const
{
	std::vector<HouseInfo>* houses;
	if (pBlackboard->GetData("NewHouses", houses) == false)
	{
		return false;
	}
	if (!houses->empty())
	{
		return false;
	}
	return true;
}
bool conditions::NewHouseNearby::Evaluate(Blackboard* pBlackboard) const
{
	std::vector<HouseInfo>* houses;
	if (pBlackboard->GetData("NewHouses", houses) == false)
	{
		return false;
	}
	return !houses->empty();
}