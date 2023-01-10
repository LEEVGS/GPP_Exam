#include "stdafx.h"
#include "StatesAndTransitions.h"
#include  <iostream>

void states::ExploreState::OnEnter(Blackboard* pBlackboard)
{
	std::cout << "Explore mode\n";
	Elite::Vector2* pTarget;
	AgentInfo* pAgent;
	std::vector<TileInfo*>* pTiles;
	if (pBlackboard->GetData("Target", pTarget) == false || pTarget == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Agent", pAgent) == false || pAgent == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Tiles", pTiles) == false || pTiles == nullptr)
	{
		return;
	}

	//Agent not init
	if (pAgent->Health < 0.001f)
	{
		m_Target = Elite::Vector2{ 0.f,0.f };
		return;
	}

	bool foundValid{};
	int randomId{};
	while (foundValid == false)
	{
		randomId = Elite::randomInt(static_cast<int>(pTiles->size()));
		if (pTiles->at(randomId)->type == TileInfo::Unknown)
		{
			foundValid = true;
		}
	}
	m_Target = pTiles->at(randomId)->middlePos;

	//int idClosestTile{ 0 };
	//float farestDistance{ };
	//Elite::Vector2 agentPos = pAgent->Position;
	//
	//for (int i = 0; i < static_cast<int>(pTiles->size()); ++i)
	//{
	//	float distance = pTiles->at(i)->middlePos.DistanceSquared(agentPos);
	//	if (pTiles->at(i)->type == TileInfo::Unknown && distance > farestDistance)
	//	{
	//		idClosestTile = i;
	//		farestDistance = distance;
	//	}
	//}
	//m_Target = pTiles->at(idClosestTile)->middlePos;
}

void states::HouseSeek::OnEnter(Blackboard* pBlackboard)
{
	if (!m_cornors.empty())
	{
		return;
	}
	std::cout << "House mode\n";
	std::vector<HouseInfo>* houses;
	if (pBlackboard->GetData("NewHouses", houses) == false || houses == nullptr)
	{
		return;
	}
	auto ptr = houses->begin();
	Elite::Vector2 center = ptr->Center;
	Elite::Vector2 size = ptr->Size;
	size -= {10.f, 10.f};
	m_cornors.emplace_back(center.x + size.x / 2,center.y + size.y / 2);
	m_cornors.emplace_back(center.x - size.x / 2,center.y - size.y / 2);
	m_cornors.emplace_back(center.x - size.x / 2,center.y + size.y / 2);
	m_cornors.emplace_back(center.x + size.x / 2,center.y - size.y / 2);
}

void states::ItemSeek::OnEnter(Blackboard* pBlackboard)
{
	std::cout << "Item mode\n";
}

void states::ItemSeek::Update(Blackboard* pBlackboard, float deltaTime)
{
	Elite::Vector2* pTarget;
	AgentInfo* pAgent;
	IExamInterface** pInterface;
	Inventory** pInventory;
	std::vector<EntityInfo>* items;
	if (pBlackboard->GetData("Target", pTarget) == false || pTarget == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Agent", pAgent) == false || pAgent == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Items", items) == false || items == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Interface", pInterface) == false || pInterface == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Inventory", pInventory) == false || pInventory == nullptr)
	{
		return;
	}

	if (items->empty())
	{
		return;
	}

	if (pAgent->Position.DistanceSquared((items->end() - 1)->Location) < 10.f)
	{
		ItemInfo itemInfo{};
		if ((**pInterface).Item_GetInfo(*(items->end() - 1), itemInfo) == false)
		{
			return;
		}


		switch (itemInfo.Type)
		{
		case eItemType::GARBAGE:
			{
			DestroyItem(pInterface, items);
			}
			break;
		case eItemType::PISTOL:
			{
			int slot{};
			const int ammo = (**pInventory).GetAmmoPistol(slot);
			if ((**pInterface).Weapon_GetAmmo(itemInfo) > ammo)
			{
				if (slot >= 0)
				{
					(**pInterface).Inventory_RemoveItem(slot);
				}
				PickupItem(pInterface, pInventory, items);
			}
			}
			break;
		case eItemType::SHOTGUN:
		{
			int slot{};
			const int ammo = (**pInventory).GetAmmoShotgun(slot);
			if ((**pInterface).Weapon_GetAmmo(itemInfo) > ammo)
			{
				if (slot >= 0)
				{
					(**pInterface).Inventory_RemoveItem(slot);
				}
				PickupItem(pInterface, pInventory, items);
			}
		}
		break;
		case eItemType::FOOD:
		case eItemType::MEDKIT:
			{
			PickupItem(pInterface, pInventory, items);
			}
			break;
		}
	}

	if (!items->empty())
	{
		pTarget->x = (items->end() - 1)->Location.x;
		pTarget->y = (items->end() - 1)->Location.y;
	}
}
void states::ItemSeek::PickupItem(IExamInterface** pInterface, Inventory** pInventory, std::vector<EntityInfo>* items)
{
	int slotId{ -1 };
	for (int i = 0; i < 5; ++i)
	{
		ItemInfo item{};
		const bool isEmpty = !(**pInterface).Inventory_GetItem(i, item);
		if (isEmpty)
		{
			slotId = i;
			break;
		}
	}

	ItemInfo temp{};
	(**pInterface).Item_Grab(*(items->end() - 1), temp);

	if (slotId != -1)
	{
		(**pInterface).Inventory_AddItem(slotId, temp);
		items->pop_back();
		(**pInventory).PullInventory();
	}
}
void states::ItemSeek::DestroyItem(IExamInterface** pInterface, std::vector<EntityInfo>* items)
{
	(**pInterface).Item_Destroy(*(items->end() - 1));
	items->pop_back();
}
void states::WanderState::OnEnter(Blackboard* pBlackboard)
{
	std::cout << "Wander mode\n";
}
void states::WanderState::Update(Blackboard* pBlackboard, float deltaTime)
{
	Elite::Vector2* pTarget;
	AgentInfo* pAgent;
	std::vector<TileInfo*>* pTiles;
	if (pBlackboard->GetData("Target", pTarget) == false || pTarget == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Agent", pAgent) == false || pAgent == nullptr)
	{
		return;
	}
	if (pBlackboard->GetData("Tiles", pTiles) == false || pTiles == nullptr)
	{
		return;
	}

	int idClosestTile{ 0 };
	float closestDistance{ FLT_MAX };
	Elite::Vector2 agentPos = pTiles->at(TileInfo::GetTileId(pAgent->Position))->middlePos;

	for (int i = 0; i < static_cast<int>(pTiles->size()); ++i)
	{
		float distance = pTiles->at(i)->middlePos.DistanceSquared(agentPos);
		if (distance > 1000.f)
		{
			continue;
		}
		if (pTiles->at(i)->type == TileInfo::Unknown && distance < closestDistance)
		{
			idClosestTile = i;
			closestDistance = distance;
		}
	}

	pTarget->x = pTiles->at(idClosestTile)->middlePos.x;
	pTarget->y = pTiles->at(idClosestTile)->middlePos.y;
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

	if (pAgent->Position.DistanceSquared(m_cornors[m_cornors.size()-1]) < 20.f)
	{

		m_cornors.pop_back();
	}

	if (m_cornors.empty())
	{
		std::vector<HouseInfo>* houses;
		std::vector<HouseInfo>* visitedHouses;
		if (pBlackboard->GetData("NewHouses", houses) == false || houses == nullptr)
		{
			return;
		}
		if (pBlackboard->GetData("VisitedHouses", visitedHouses) == false || visitedHouses == nullptr)
		{
			return;
		}
		visitedHouses->push_back(*(houses->begin()));
		houses->erase(houses->begin());

		std::vector<EntityInfo>* items;
		if (pBlackboard->GetData("Items", items) == false || items == nullptr)
		{
			return;
		}

		if (!houses->empty())
		{
			OnEnter(pBlackboard);
		}
		return;
	}

	pTarget->x = m_cornors[m_cornors.size()-1].x;
	pTarget->y = m_cornors[m_cornors.size() - 1].y;
}
void states::ExploreState::Update(Blackboard* pBlackboard, float deltaTime)
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

	if (pAgent->Position.DistanceSquared(m_Target) < 10.f)
	{
		OnEnter(pBlackboard);
	}

	pTarget->x = m_Target.x;
	pTarget->y = m_Target.y;
}


bool conditions::NoHouseNearby::Evaluate(Blackboard* pBlackboard) const
{
	std::vector<HouseInfo>* houses;
	if (pBlackboard->GetData("NewHouses", houses) == false || houses == nullptr)
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
	if (pBlackboard->GetData("NewHouses", houses) == false || houses == nullptr)
	{
		return false;
	}
	std::vector<EntityInfo>* items;
	if (pBlackboard->GetData("Items", items) == false || items == nullptr)
	{
		return false;
	}
	return !houses->empty() && items->empty();
}

bool conditions::ItemsNearby::Evaluate(Blackboard* pBlackboard) const
{
	std::vector<EntityInfo>* items;
	if (pBlackboard->GetData("Items", items) == false || items == nullptr)
	{
		return false;
	}
	return !items->empty();
}

bool conditions::NoItemsNearby::Evaluate(Blackboard* pBlackboard) const
{
	std::vector<EntityInfo>* items;
	if (pBlackboard->GetData("Items", items) == false || items == nullptr)
	{
		return false;
	}
	return items->empty();
}

bool conditions::OutterRange::Evaluate(Blackboard* pBlackboard) const
{
	AgentInfo* pAgent;
	if (pBlackboard->GetData("Agent", pAgent) == false || pAgent == nullptr)
	{
		return false;
	}
	const float outerRange = TileInfo::correction * 0.5f;

	if (pAgent->Position.x >= outerRange || pAgent->Position.x <= -outerRange ||
		pAgent->Position.y >= outerRange || pAgent->Position.y <= -outerRange)
	{
		return true;
	}
	return false;
}
