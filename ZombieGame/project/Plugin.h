#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "Inventory.h"

#include "StatesAndTransitions.h"
#include <vector>

class IBaseInterface;
class IExamInterface;

class Plugin :public IExamPlugin
{
public:
	Plugin() = default;
	~Plugin() override = default;

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	WorldInfo m_WorldInfo;
	std::vector<HouseInfo> GetHousesInFOV() const;
	std::vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 m_Target = {};
	bool m_ShowDebug = false;
	bool m_CanRun = true; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose

	UINT m_InventorySlot = 0;

	void EntityHandling();
	void HouseHandling();
	void GridHandle();

	void CreateGrid();
	int m_AmountCellsWidth{};
	int m_AmountCellsHeight{};
	const float m_CellSize = 5.f;
	std::vector<TileInfo*> m_pTiles;

	void CreateFSM();
	Blackboard* CreateBlackboard();
	Blackboard* m_pBlackboard {nullptr};
	FiniteStateMachine* m_pDecisionMaking{ nullptr };

	Inventory* m_pInventory{ nullptr };

	AgentInfo m_AgentInfo;
	std::vector<HouseInfo> m_VisitedHouses;
	std::vector<HouseInfo> m_NewHouses;
	std::vector<EntityInfo> m_Items;
	std::vector<EntityInfo> m_Enemies;
	std::vector<FSMCondition*> m_Conditions;
	std::vector<FSMState*> m_States;
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}