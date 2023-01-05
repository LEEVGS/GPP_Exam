#include "FSM.h"
#include "Exam_HelperStructs.h"

//States
namespace states
{
	class Wanderstate : public FSMState
	{
	public:
		Wanderstate() : FSMState() {};
		virtual void OnEnter(Blackboard* pBlackboard) override;
		virtual void Update(Blackboard* pBlackboard, float deltaTime) override;
	private:
		float m_WanderAngle = 0.f;
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
}

//Conditions
namespace conditions
{
	class DefaultWanderingCondition : public FSMCondition
	{
	public:
		DefaultWanderingCondition() : FSMCondition() {};

		// Inherited via FSMCondition
		virtual bool Evaluate(Blackboard* pBlackboard) const override;
	};
	class NewHouseNearby : public FSMCondition
	{
	public:
		NewHouseNearby() : FSMCondition() {};
		virtual bool Evaluate(Blackboard* pBlackboard) const override;
	};
}
