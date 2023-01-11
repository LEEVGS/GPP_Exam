#pragma once
#include "Exam_HelperStructs.h"

class IBaseInterface;
class IExamInterface;

class Inventory
{
public:
	explicit Inventory(IExamInterface* pInterface);
	void Update(const AgentInfo* pAgentInfo);
	void PullInventory();
	
	void GetInventorySlot(ItemInfo& item, int slot) const;
	int GetAmmoShotgun(int& slot);
	int GetAmmoPistol(int& slot);
	bool Shoot();
	bool HasFreeSlot();
	bool Heal();
	bool Eat();
private:
	IExamInterface* m_pInterface;

	
	void DropNoAmmo();

	static inline constexpr int m_AmountOfSlots = 5;
	ItemInfo m_Slots[m_AmountOfSlots]{};

	bool m_HasHeal{}, m_HasFood{}, m_HasWeapon{};
};

