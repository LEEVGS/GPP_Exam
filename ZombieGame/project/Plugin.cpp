#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"

#include <iostream>

using namespace std;

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "MinionExam";
	info.Student_FirstName = "Lee";
	info.Student_LastName = "Vangraefschepe";
	info.Student_Class = "2DAE15N";

	//Create grid structure
	
	m_WorldInfo = m_pInterface->World_GetInfo();
	m_AmountCellsWidth = ceil(m_WorldInfo.Dimensions.x / m_CellSize);
	m_AmountCellsHeight = ceil(m_WorldInfo.Dimensions.y / m_CellSize);
	int amountOffCells = m_AmountCellsWidth * m_AmountCellsHeight;
	m_pArrTiles = new TileInfo[amountOffCells];
	m_TilesSize = amountOffCells;

	TileInfo::cellSize = m_CellSize;
	TileInfo::amountCellsWidth = m_AmountCellsWidth;
	TileInfo::correction = m_WorldInfo.Dimensions.x / 2;

	for (int i = 0; i < amountOffCells; ++i)
	{
		m_pArrTiles[i].id = i;
		m_pArrTiles[i].pos.y = (i / m_AmountCellsWidth);
		m_pArrTiles[i].pos.x = (i * m_CellSize) - (m_pArrTiles[i].pos.y * m_AmountCellsWidth * m_CellSize);
		m_pArrTiles[i].pos.y *= 5.f;
		m_pArrTiles[i].pos.x -= m_WorldInfo.Dimensions.x / 2;
		m_pArrTiles[i].pos.y -= m_WorldInfo.Dimensions.y / 2;
	}

	//for (int i = 0; i < amountOffCells; ++i)
	//{
	//	std::cout << m_pArrTiles[i].pos.x << "," << m_pArrTiles[i].pos.y << "\n";
	//}

	std::cout << m_WorldInfo.Dimensions.x << "," << m_WorldInfo.Dimensions.y << "\n";
	std::cout << amountOffCells << "\n";
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
	std::cout << "Loaded dll\n";
	CreateFSM();
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
	for (auto element : m_Conditions)
	{
		delete element;
	}
	for (auto element : m_States)
	{
		delete element;
	}
	delete m_pBlackboard;
	delete m_pDecisionMaking;
	delete m_pArrTiles;
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be useful to inspect certain behaviors (Default = false)
	params.LevelFile = "GameLevel.gppl";
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.StartingDifficultyStage = 1;
	params.InfiniteStamina = false;
	params.SpawnDebugPistol = true;
	params.SpawnDebugShotgun = true;
	params.SpawnPurgeZonesOnMiddleClick = true;
	params.PrintDebugMessages = true;
	params.ShowDebugItemNames = true;
	params.Seed = 36;
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Delete))
	{
		m_pInterface->RequestShutdown();
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Minus))
	{
		if (m_InventorySlot > 0)
			--m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Plus))
	{
		if (m_InventorySlot < 4)
			++m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Q))
	{
		ItemInfo info = {};
		m_pInterface->Inventory_GetItem(m_InventorySlot, info);
		std::cout << (int)info.Type << std::endl;
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	auto steering = SteeringPlugin_Output();
	
	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	m_AgentInfo = m_pInterface->Agent_GetInfo();


	//Use the navmesh to calculate the next navmesh point
	//auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(checkpointLocation);

	//OR, Use the mouse target

	m_pDecisionMaking->Update(dt);

	//std::cout << "Target: (" << m_Target.x << "," << m_Target.y << ")\n";

	auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(m_Target); //Uncomment this to use mouse position as guidance

	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)

	for (auto housesInFov: vHousesInFOV)
	{
		bool alreadyVisited{ false };
		bool alreadyAdded{ false };
		for (auto visitedHouse : m_VisitedHouses)
		{
			if (visitedHouse.Center == housesInFov.Center)
			{
				alreadyVisited = true;
				break;
			}
		}
		if (alreadyVisited == false)
		{
			for (auto newHouse : m_NewHouses)
			{
				if (newHouse.Center == housesInFov.Center)
				{
					alreadyAdded = true;
					break;
				}
			}
			if (alreadyAdded == false)
			{
				//Add house to list and tiles
				m_NewHouses.push_back(housesInFov);
				const int minX = housesInFov.Center.x - housesInFov.Size.x / 2 + m_CellSize;
				const int maxX = housesInFov.Center.x + housesInFov.Size.x / 2 - m_CellSize;
				const int minY = housesInFov.Center.y - housesInFov.Size.y / 2 + m_CellSize;
				const int maxY = housesInFov.Center.y + housesInFov.Size.y / 2 - m_CellSize;

				for (int y = minY; y < maxY; ++y)
				{
					for (int x = minX; x < maxX; ++x)
					{
						//int tileId = TileInfo::GetTileId(Elite::Vector2{ (float)x,(float)y }, m_WorldInfo.Dimensions.x/2.f, m_CellSize, m_AmountCellsWidth);
						//m_pArrTiles[tileId].type = TileInfo::House;
					}
				}
			}
		}
	}

	if (m_pInterface->Input_IsKeyboardKeyDown(Elite::InputScancode::eScancode_PageDown))
	{
		m_ShowDebug = !m_ShowDebug;
		std::cout << "Show debug toggled!";
	}
	if (m_ShowDebug)
	{
		std::cout << "Houses: " << m_NewHouses.size() << " Visited houses: " << m_VisitedHouses.size() << "\n";
	}

	//std::cout << "Houses: " << m_NewHouses.size() << " Visited houses: " << m_VisitedHouses.size() << "\n";

	for (auto& e : vEntitiesInFOV)
	{
		if (e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			//std::cout << "Purge Zone in FOV:" << e.Location.x << ", "<< e.Location.y << "---Radius: "<< zoneInfo.Radius << std::endl;
		}
	}

	//INVENTORY USAGE DEMO
	//********************

	if (m_GrabItem)
	{
		ItemInfo item;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		if (m_pInterface->Item_Grab({}, item))
		{
			//Once grabbed, you can add it to a specific inventory slot
			//Slot must be empty
			m_pInterface->Inventory_AddItem(m_InventorySlot, item);
		}
	}

	if (m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(m_InventorySlot);
	}

	if (m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(m_InventorySlot);
	}

	//Simple Seek Behaviour (towards Target)
	steering.LinearVelocity = nextTargetPos - m_AgentInfo.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= m_AgentInfo.MaxLinearSpeed; //Rescale to Max Speed

	if (Distance(nextTargetPos, m_AgentInfo.Position) < 2.f)
	{
		steering.LinearVelocity = Elite::ZeroVector2;
	}

	//steering.AngularVelocity = m_AngSpeed; //Rotate your character to inspect the world while walking
	steering.AutoOrient = true; //Setting AutoOrient to TRue overrides the AngularVelocity

	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

	//SteeringPlugin_Output is works the exact same way a SteeringBehaviour output

//@End (Demo Purposes)
	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;

	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	if (!m_ShowDebug)
	{
		return;
	}
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });

	Elite::Vector2 p0{}, p1{}, p2{}, p3{};

	for (int i = 0; i < m_TilesSize; ++i)
	{
		p0 = m_pArrTiles[i].pos;
		if (p0.DistanceSquared(m_AgentInfo.Position) > 1000.f)
		{
			continue;
		}

		Elite::Vector3 color{ 1.f,1.f,1.f };

		p1 = p0 + Elite::Vector2{m_CellSize, 0.f};
		p2 = p0 + Elite::Vector2{m_CellSize, m_CellSize};
		p3 = p0 + Elite::Vector2{0.f, m_CellSize};
	
		if (m_pArrTiles[i].type == TileInfo::House)
		{
			color = { 1.f,0.f,0.f };
		}
	
		m_pInterface->Draw_Segment(p0, p1, color);
		m_pInterface->Draw_Segment(p1, p2, color);
		m_pInterface->Draw_Segment(p2, p3, color);
		m_pInterface->Draw_Segment(p3, p0, color);
	}
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

void Plugin::CreateFSM()
{
	//Create states
	states::Wanderstate* pWanderstate = new states::Wanderstate{};
	m_States.push_back(pWanderstate);
	states::HouseSeek* pHouseSeek = new states::HouseSeek{};
	m_States.push_back(pHouseSeek);

	//Create conditions
	conditions::DefaultWanderingCondition* pWander = new conditions::DefaultWanderingCondition{};
	m_Conditions.push_back(pWander);
	conditions::NewHouseNearby* pHouseNearby = new conditions::NewHouseNearby{};
	m_Conditions.push_back(pHouseNearby);

	m_pBlackboard = CreateBlackboard();

	m_pDecisionMaking = new FiniteStateMachine{ pWanderstate, m_pBlackboard };
	m_pDecisionMaking->AddTransition(pWanderstate, pHouseSeek, pHouseNearby);
	m_pDecisionMaking->AddTransition(pHouseSeek, pWanderstate, pWander);
}

Blackboard* Plugin::CreateBlackboard()
{
	Blackboard* pBlackboard = new Blackboard();
	pBlackboard->AddData("Agent", &m_AgentInfo);
	pBlackboard->AddData("Target", &m_Target);
	pBlackboard->AddData("VisitedHouses", &m_VisitedHouses);
	pBlackboard->AddData("NewHouses", &m_NewHouses);
	pBlackboard->AddData("Tiles", m_pArrTiles);
	pBlackboard->AddData("WorldInfo", &m_WorldInfo);
	return pBlackboard;
}