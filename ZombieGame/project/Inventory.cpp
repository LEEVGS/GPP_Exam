#include "stdafx.h"
#include "Inventory.h"
#include <IExamInterface.h>

Inventory::Inventory(IExamInterface* pInterface)
	: m_pInterface{pInterface}
{
	PullInventory();
}

void Inventory::Update(const AgentInfo* pAgentInfo)
{
	if (pAgentInfo->Energy < 1.f && m_HasFood)
	{
		m_HasFood = Eat();
	}
	if (pAgentInfo->Health <= 8.f)
	{
		m_HasHeal = Heal();
	}
}

void Inventory::GetInventorySlot(ItemInfo& item, int slot) const
{
	item = m_Slots[slot];
}

int Inventory::GetAmmoShotgun(int& slot)
{
	slot = -1;
	for (int i = 0; i < m_AmountOfSlots; ++i)
	{
		if (m_Slots[i].Type == eItemType::SHOTGUN)
		{
			slot = i;
			return m_pInterface->Weapon_GetAmmo(m_Slots[i]);
		}
	}
	return 0;
}

int Inventory::GetAmmoPistol(int& slot)
{
	slot = -1;
	for (int i = 0; i < m_AmountOfSlots; ++i)
	{
		if (m_Slots[i].Type == eItemType::PISTOL)
		{
			slot = i;
			return m_pInterface->Weapon_GetAmmo(m_Slots[i]);
		}
	}
	return 0;
}

void Inventory::PullInventory()
{
	m_HasFood = m_HasHeal = m_HasWeapon = true;
	for (auto& m_Slot : m_Slots)
	{
		m_Slot = ItemInfo{};
	}
	for (int i = 0; i < m_AmountOfSlots; ++i)
	{
		m_pInterface->Inventory_GetItem(i, m_Slots[i]);
	}
}

void Inventory::DropNoAmmo()
{
	for (int i = 0; i < m_AmountOfSlots; ++i)
	{
		if (m_Slots[i].Type == eItemType::PISTOL || m_Slots[i].Type == eItemType::SHOTGUN)
		{
			const int ammo = m_pInterface->Weapon_GetAmmo(m_Slots[i]);
			if (ammo == 0)
			{
				m_pInterface->Inventory_RemoveItem(i);
			}
		}
	}
	PullInventory();
}

bool Inventory::Eat()
{
	for (int i = 0; i < m_AmountOfSlots; ++i)
	{
		if (m_Slots[i].Type == eItemType::FOOD)
		{
			m_pInterface->Inventory_UseItem(i);
			m_pInterface->Inventory_RemoveItem(i);
			PullInventory();
			return true;
		}
	}
	return false;
}

bool Inventory::Shoot()
{
	for (int i = 0; i < m_AmountOfSlots; ++i)
	{
		if ((m_Slots[i].Type == eItemType::PISTOL && m_Slots[i].ItemHash != 0))
		{
			m_pInterface->Inventory_UseItem(i);
			DropNoAmmo();
			return true;
		}
		else if (m_Slots[i].Type == eItemType::SHOTGUN)
		{
			m_pInterface->Inventory_UseItem(i);
			DropNoAmmo();
			return true;
		}
	}
	return false;
}

bool Inventory::HasFreeSlot()
{
	for (const auto& m_Slot : m_Slots)
	{
		if (m_Slot.ItemHash == 0)
		{
			return true;
		}
	}
	return false;
}

bool Inventory::HasWeapon()
{
	int slot{};
	return GetAmmoPistol(slot) + GetAmmoShotgun(slot) > 0;
}

bool Inventory::Heal()
{
	for (int i = 0; i < m_AmountOfSlots; ++i)
	{
		if (m_Slots[i].Type == eItemType::MEDKIT)
		{
			m_pInterface->Inventory_UseItem(i);
			m_pInterface->Inventory_RemoveItem(i);
			PullInventory();
			return true;
		}
	}
	return false;
}
