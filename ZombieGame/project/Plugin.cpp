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
	m_pInterface = dynamic_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "NatteKanker";
	info.Student_FirstName = "Lee";
	info.Student_LastName = "Vangraefschepe";
	info.Student_Class = "2DAE15N";

	CreateGrid();

	m_pInventory = new Inventory{ m_pInterface };
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
	for (const auto element : m_Conditions)
	{
		delete element;
	}
	for (const auto element : m_States)
	{
		delete element;
	}
	for (const auto element : m_pTiles)
	{
		delete element;
	}
	delete m_pBlackboard;
	delete m_pDecisionMaking;
	delete m_pInventory;
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
	params.SpawnZombieOnRightClick = true;
	params.PrintDebugMessages = false;
	params.Seed = 36;
	//params.Seed = 0;
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
		const Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
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
		std::cout << static_cast<int>(info.Type) << std::endl;
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	auto steering = SteeringPlugin_Output();
	
	//Update agent info
	m_AgentInfo = m_pInterface->Agent_GetInfo();

	UpdatePurge(dt);

	//Item handling
	EntityHandling();

	//Grid handling
	GridHandle();

	//House handling
	HouseHandling();

	//Update target
	m_pDecisionMaking->Update(dt);

	//Handle inventory
	m_pInventory->Update(&m_AgentInfo);
	
	//Handle enemy target
	HandleEnemies(steering);

	const auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(m_Target);

	//Toggle debug
	if (m_pInterface->Input_IsKeyboardKeyDown(Elite::InputScancode::eScancode_PageDown))
	{
		m_ShowDebug = !m_ShowDebug;
		std::cout << "Show debug toggled!";
	}

	//Print debug
	if (m_ShowDebug)
	{
		std::cout << "Items: " << m_Items.size() << "\n";
		std::cout << "Enemies: " << m_Enemies.size() << "\n";
		std::cout << "Zones: " << m_Purges.size() << "\n";
		std::cout << "Houses: " << m_NewHouses.size() << " Visited houses: " << m_VisitedHouses.size() << "\n";
		std::cout << "Target: " << m_Target.x << "," << m_Target.y << "\n";
	}

	steering.LinearVelocity = nextTargetPos - m_AgentInfo.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= m_AgentInfo.MaxLinearSpeed; //Rescale to Max Speed

	if (Distance(nextTargetPos, m_AgentInfo.Position) < 1.f)
	{
		steering.LinearVelocity = Elite::ZeroVector2;
	}

	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

	m_GrabItem = false;
	m_UseItem = false;
	m_RemoveItem = false;

	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{

	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });

	Elite::Vector2 p0{}, p1{}, p2{}, p3{};

	for (int i = 0; i < static_cast<int>(m_pTiles.size()); ++i)
	{
		p0 = m_pTiles[i]->pos;
		if (p0.DistanceSquared(m_AgentInfo.Position) > 1000.f)
		{
			continue;
		}

		Elite::Vector3 color{ 1.f,1.f,1.f };

		p1 = p0 + Elite::Vector2{m_CellSize, 0.f};
		p2 = p0 + Elite::Vector2{m_CellSize, m_CellSize};
		p3 = p0 + Elite::Vector2{0.f, m_CellSize};
	
		if (m_pTiles[i]->type == TileInfo::House)
		{
			color = { 1.f,0.f,0.f };
		}
		else if (m_pTiles[i]->type == TileInfo::Void)
		{
			color = { 0.f,1.f,0.f };
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

void Plugin::EntityHandling()
{
	const auto vEntitiesInFOV = GetEntitiesInFOV();
	for (const auto& value : vEntitiesInFOV)
	{
		if (value.Type == eEntityType::ITEM)
		{
			bool alreadyAdded{};
			for (const auto& alreadyAddedItem : m_Items)
			{
				if (value.Location == alreadyAddedItem.Location)
				{
					alreadyAdded = true;
				}
			}
			if (alreadyAdded == false)
			{
				m_Items.push_back(value);
				for (auto & house : m_VisitedHouses)
				{
					house.ItemsRemaining--;
				}
			}
		}
		else if (value.Type == eEntityType::ENEMY)
		{
			bool alreadyAdded{};
			int id{};
			for (int i = 0; i < static_cast<int>(m_Enemies.size()); ++i)
			{
				if (value.EntityHash == m_Enemies[i].EntityHash)
				{
					alreadyAdded = true;
					id = i;
					break;
				}
			}
			if (alreadyAdded)
			{
				m_Enemies.at(id).Location = value.Location;
			}
			else
			{
				m_Enemies.push_back(value);
			}
		}
		else if (value.Type == eEntityType::PURGEZONE)
		{
			bool alreadyAdded{};
			PurgeZoneInfo zoneInfo{};
			m_pInterface->PurgeZone_GetInfo(value, zoneInfo);
			for (const auto & zone : vEntitiesInFOV)
			{
				for (const auto& knownZone : m_Purges)
				{
					if (knownZone.Center == zone.Location)
					{
						alreadyAdded = true;
						break;
					}
				}
				if (alreadyAdded == false)
				{
					m_Purges.push_back(zoneInfo);
				}
			}
		}
	}
}
void Plugin::UpdatePurge(float dt)
{
	for (int i = 0; i < static_cast<int>(m_Purges.size()); ++i)
	{
		m_Purges[i].TimeRemaining -= dt;
		if (m_Purges[i].TimeRemaining <= 0.f)
		{
			m_Purges.clear();
			break;
		}
	}
	m_CanRun = !m_Purges.empty();
}
void Plugin::HouseHandling()
{
	for (int i = 0; i < static_cast<int>(m_VisitedHouses.size()); ++i)
	{
		if (m_VisitedHouses[i].ItemsRemaining <= 0)
		{
			m_VisitedHouses[i].ItemsRemaining = 10;
			m_NewHouses.push_back(m_VisitedHouses[i]);
			m_VisitedHouses.erase(m_VisitedHouses.begin() + i);
		}
	}


	const auto vHousesInFOV = GetHousesInFOV();
	for (const auto& housesInFov : vHousesInFOV)
	{
		bool alreadyVisited{ false };
		bool alreadyAdded{ false };
		for (const auto& [Center, Size, ItemsRemaining] : m_VisitedHouses)
		{
			if (Center == housesInFov.Center)
			{
				alreadyVisited = true;
				break;
			}
		}
		if (alreadyVisited == false)
		{

			for (const auto& newHouse : m_NewHouses)
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
				const int minX = static_cast<int>(housesInFov.Center.x - housesInFov.Size.x / 2 /*+ m_CellSize*/);
				const int maxX = static_cast<int>(housesInFov.Center.x + housesInFov.Size.x / 2 /*- m_CellSize*/);
				const int minY = static_cast<int>(housesInFov.Center.y - housesInFov.Size.y / 2 /*+ m_CellSize*/);
				const int maxY = static_cast<int>(housesInFov.Center.y + housesInFov.Size.y / 2 /*- m_CellSize*/);

				for (int y = minY; y < maxY; ++y)
				{
					for (int x = minX; x < maxX; ++x)
					{
						const int tileId = TileInfo::GetTileId(Elite::Vector2{ static_cast<float>(x),static_cast<float>(y) });
						m_pTiles[tileId]->type = TileInfo::House;
					}
				}
			}
		}
	}
}

void Plugin::GridHandle()
{
	const Elite::Vector2 position = m_AgentInfo.Position;
	if (m_pTiles[TileInfo::GetTileId(position)]->type == TileInfo::Type::Unknown)
	{
		m_pTiles[TileInfo::GetTileId(position)]->type = TileInfo::Type::Void;

		const Elite::Vector2 directionN = m_AgentInfo.LinearVelocity.GetNormalized();
		for (int i = 1; i < 4; ++i)
		{
			Elite::Vector2 addedDirection = directionN * 5.f * static_cast<float>(i);
			if (m_pTiles[TileInfo::GetTileId(position + addedDirection)]->type == TileInfo::Type::Unknown)
			{
				m_pTiles[TileInfo::GetTileId(position + addedDirection)]->type = TileInfo::Type::Void;
			}
			m_pInterface->Draw_Point(position + addedDirection, 2.f, { 0,1,0 });
		}
		m_pInterface->Draw_Direction(position, m_AgentInfo.LinearVelocity, 15.f, { 1,0,0 });
	}
}

void Plugin::HandleEnemies(SteeringPlugin_Output& steering)
{
	if (!m_Enemies.empty() && m_pInventory->HasWeapon() && m_Purges.empty())
	{
		m_Target = m_Enemies.at(m_Enemies.size() - 1).Location;
		Elite::Vector2 directionToTarget = m_Target - m_AgentInfo.Position;

		directionToTarget.Normalize();

		steering.AngularVelocity = Elite::VectorToOrientation(directionToTarget) - m_AgentInfo.Orientation;

		if (abs(steering.AngularVelocity) < 0.1f)
		{
			std::cout << steering.AngularVelocity << "\n";
			m_pInventory->Shoot();
			m_Enemies.pop_back();
		}

		if (steering.AngularVelocity > m_AgentInfo.MaxAngularSpeed)
		{
			steering.AngularVelocity = m_AgentInfo.MaxAngularSpeed;
		}
		else if (steering.AngularVelocity < -m_AgentInfo.MaxAngularSpeed)
		{
			steering.AngularVelocity = -m_AgentInfo.MaxAngularSpeed;
		}
		steering.AutoOrient = false;
		m_Target = m_AgentInfo.Position;
	}
	else
	{
		steering.AngularVelocity = m_AgentInfo.MaxAngularSpeed;
		steering.AutoOrient = false;
	}
}

void Plugin::CreateGrid()
{
	m_WorldInfo = m_pInterface->World_GetInfo();
	m_AmountCellsWidth = static_cast<int>(ceil(m_WorldInfo.Dimensions.x / m_CellSize));
	m_AmountCellsHeight = static_cast<int>(ceil(m_WorldInfo.Dimensions.y / m_CellSize));
	m_pTiles.reserve(m_AmountCellsWidth * m_AmountCellsHeight);

	for (int i = 0; i < static_cast<int>(m_pTiles.capacity()); ++i)
	{
		m_pTiles.push_back(new TileInfo{});
		m_pTiles[i]->id = i;
		m_pTiles[i]->pos.y = static_cast<float>(i / m_AmountCellsWidth);
		m_pTiles[i]->pos.x = i * m_CellSize - m_pTiles[i]->pos.y * m_AmountCellsWidth * m_CellSize;
		m_pTiles[i]->pos.y *= 5.f;
		m_pTiles[i]->pos.x -= m_WorldInfo.Dimensions.x / 2;
		m_pTiles[i]->pos.y -= m_WorldInfo.Dimensions.y / 2;
		m_pTiles[i]->middlePos = m_pTiles[i]->pos + Elite::Vector2{ m_CellSize / 2.f, m_CellSize / 2.f };

		constexpr float voidRange = 250.f;

		if (m_pTiles[i]->pos.x >= voidRange || m_pTiles[i]->pos.x <= -voidRange ||
			m_pTiles[i]->pos.y >= voidRange || m_pTiles[i]->pos.y <= -voidRange)
		{
			m_pTiles[i]->type = TileInfo::Void;
		}
	}

	TileInfo::cellSize = m_CellSize;
	TileInfo::amountCellsWidth = m_AmountCellsWidth;
	TileInfo::correction = m_WorldInfo.Dimensions.x / 2;
}

void Plugin::CreateFSM()
{
	//Create states
	auto* pExploreState = new states::ExploreState{};
	m_States.push_back(pExploreState);
	auto* pWanderState = new states::WanderState{};
	m_States.push_back(pWanderState);
	auto* pHouseSeek = new states::HouseSeek{};
	m_States.push_back(pHouseSeek);
	auto* pItemSeek = new states::ItemSeek{};
	m_States.push_back(pItemSeek);
	auto* pEscapePurge = new states::EscapePurge{};
	m_States.push_back(pEscapePurge);

	//Create conditions
	auto* pNoHouseNearby = new conditions::NoHouseNearby{};
	m_Conditions.push_back(pNoHouseNearby);
	auto* pHouseNearby = new conditions::NewHouseNearby{};
	m_Conditions.push_back(pHouseNearby);
	auto* pNoItemsNearby = new conditions::NoItemsNearby{};
	m_Conditions.push_back(pNoItemsNearby);
	auto* pItemsNearby = new conditions::ItemsNearby{};
	m_Conditions.push_back(pItemsNearby);
	auto* pOuterRange = new conditions::OutterRange{};
	m_Conditions.push_back(pOuterRange);
	auto* pPurgeNearby = new conditions::PurgeNearby{};
	m_Conditions.push_back(pPurgeNearby);
	auto* pNoPurgeNearby = new conditions::NoPurgeNearby{};
	m_Conditions.push_back(pNoPurgeNearby);

	m_pBlackboard = CreateBlackboard();

	m_pDecisionMaking = new FiniteStateMachine{ pExploreState, m_pBlackboard };
	m_pDecisionMaking->AddTransition(pExploreState, pHouseSeek, pHouseNearby);
	m_pDecisionMaking->AddTransition(pWanderState, pHouseSeek, pHouseNearby);
	m_pDecisionMaking->AddTransition(pHouseSeek, pItemSeek, pItemsNearby);

	m_pDecisionMaking->AddTransition(pHouseSeek, pWanderState, pNoHouseNearby);
	m_pDecisionMaking->AddTransition(pItemSeek, pHouseSeek, pNoItemsNearby);

	m_pDecisionMaking->AddTransition(pWanderState, pExploreState, pOuterRange);
	m_pDecisionMaking->AddTransition(pExploreState, pHouseSeek, pHouseNearby);
	m_pDecisionMaking->AddTransition(pExploreState, pItemSeek, pItemsNearby);

	m_pDecisionMaking->AddTransition(pWanderState, pEscapePurge, pPurgeNearby);
	m_pDecisionMaking->AddTransition(pExploreState, pEscapePurge, pPurgeNearby);
	m_pDecisionMaking->AddTransition(pHouseSeek, pEscapePurge, pPurgeNearby);
	m_pDecisionMaking->AddTransition(pItemSeek, pEscapePurge, pPurgeNearby);
	m_pDecisionMaking->AddTransition(pEscapePurge, pExploreState, pNoPurgeNearby);
}

Blackboard* Plugin::CreateBlackboard()
{
	auto* pBlackboard = new Blackboard();
	pBlackboard->AddData("Agent", &m_AgentInfo);
	pBlackboard->AddData("Interface", &m_pInterface);
	pBlackboard->AddData("Target", &m_Target);
	pBlackboard->AddData("VisitedHouses", &m_VisitedHouses);
	pBlackboard->AddData("NewHouses", &m_NewHouses);
	pBlackboard->AddData("Tiles", &m_pTiles);
	pBlackboard->AddData("WorldInfo", &m_WorldInfo);
	pBlackboard->AddData("Items", &m_Items);
	pBlackboard->AddData("Enemies", &m_Enemies);
	pBlackboard->AddData("Purges", &m_Purges);
	pBlackboard->AddData("Inventory", &m_pInventory);
	return pBlackboard;
}