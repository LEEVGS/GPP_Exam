#include "FSM.h"
#include "Exam_HelperStructs.h"
#include "IExamInterface.h"
#include "Inventory.h"

//States
namespace states
{
	class ExploreState : public FSMState
	{
	public:
		ExploreState() : FSMState() {};
		virtual void OnEnter(Blackboard* pBlackboard) override;
		virtual void Update(Blackboard* pBlackboard, float deltaTime) override;
	private:
		Elite::Vector2 m_Target;
	};
	class WanderState : public FSMState
	{
	public:
		WanderState() : FSMState() {};
		virtual void OnEnter(Blackboard* pBlackboard) override;
		virtual void Update(Blackboard* pBlackboard, float deltaTime) override;
	private:
		Elite::Vector2 m_Target;
	};
	class HouseSeek : public FSMState
	{
	public:
		HouseSeek() : FSMState() {};
		virtual void OnEnter(Blackboard* pBlackboard) override;
		virtual void Update(Blackboard* pBlackboard, float deltaTime) override;
	private:
		std::vector<Elite::Vector2> m_cornors;
	};
	class ItemSeek : public FSMState
	{
	public:
		ItemSeek() : FSMState() {};
		virtual void OnEnter(Blackboard* pBlackboard) override;
		virtual void Update(Blackboard* pBlackboard, float deltaTime) override;
		void PickupItem(IExamInterface** pInterface, Inventory** pInventory, std::vector<EntityInfo>* items);
		void DestroyItem(IExamInterface** pInterface, std::vector<EntityInfo>* items);
	private:
	};
}

//Conditions
namespace conditions
{
	class NoHouseNearby : public FSMCondition
	{
	public:
		NoHouseNearby() : FSMCondition() {};
		virtual bool Evaluate(Blackboard* pBlackboard) const override;
	};
	class NewHouseNearby : public FSMCondition
	{
	public:
		NewHouseNearby() : FSMCondition() {};
		virtual bool Evaluate(Blackboard* pBlackboard) const override;
	};
	class NoItemsNearby : public FSMCondition
	{
	public:
		NoItemsNearby() : FSMCondition() {};
		virtual bool Evaluate(Blackboard* pBlackboard) const override;
	};
	class ItemsNearby : public FSMCondition
	{
	public:
		ItemsNearby() : FSMCondition() {};
		virtual bool Evaluate(Blackboard* pBlackboard) const override;
	};
	class OutterRange : public FSMCondition
	{
	public:
		OutterRange() : FSMCondition() {};
		virtual bool Evaluate(Blackboard* pBlackboard) const override;
	};
}
