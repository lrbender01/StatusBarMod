#include "PostureBarMod.hpp"
#include "Ini/ini.h"
#include "Main/Logger.hpp"
#include "Main/D3DRenderer.hpp"
#include "Main/Hooking.hpp"
#include "Main/PostureBarUI.hpp"

using namespace ER;

bool loadIni()
{
    try
    {
        using namespace mINI;
        INIFile config("mods\\PostureBarModConfig.ini");
        INIStructure ini;

        if (!config.read(ini))
            throw std::exception("Failed to read PostureModConfig.ini in mods folder");

        ScreenParams::inGameCoordSizeX = std::stof(ini["General"].get("InGameScreenCoordWidth"));
        ScreenParams::inGameCoordSizeY = std::stof(ini["General"].get("InGameScreenCoordHeight"));
        ScreenParams::posX = std::stof(ini["General"].get("ScreenPositionX"));
        ScreenParams::posY = std::stof(ini["General"].get("ScreenPositionY"));
        ScreenParams::gameToViewportScaling = std::stof(ini["General"].get("GameToScreenScaling"));
        ScreenParams::autoPositionSetup = ini["General"].get("AutoPositionSetup") == "true";
        ScreenParams::autoGameToViewportScaling = ini["General"].get("AutoGameToScreenScaling") == "true";

        BossPostureBarData::useStaminaForNPC = ini["Boss Posture Bar"].get("UseStaminaForNPC") == "true";
        BossPostureBarData::barWidth = std::stof(ini["Boss Posture Bar"].get("BarWidth"));
        BossPostureBarData::barHeight = std::stof(ini["Boss Posture Bar"].get("BarHeight"));
        BossPostureBarData::resetStaggerTotalTime = std::stof(ini["Boss Posture Bar"].get("ResetStaggerTotalTime"));
        BossPostureBarData::firstBossScreenX = std::stof(ini["Boss Posture Bar"].get("FirstBossScreenX"));
        BossPostureBarData::firstBossScreenY = std::stof(ini["Boss Posture Bar"].get("FirstBossScreenY"));
        BossPostureBarData::nextBossBarDiffScreenY = std::stof(ini["Boss Posture Bar"].get("NextBossBarDiffScreenY"));

        PostureBarData::useStaminaForNPC = ini["Entity Posture Bar"].get("UseStaminaForNPC") == "true";
        PostureBarData::barWidth = std::stof(ini["Entity Posture Bar"].get("BarWidth"));
        PostureBarData::barHeight = std::stof(ini["Entity Posture Bar"].get("BarHeight"));
        PostureBarData::resetStaggerTotalTime = std::stof(ini["Entity Posture Bar"].get("ResetStaggerTotalTime"));
        PostureBarData::offsetScreenX = std::stof(ini["Entity Posture Bar"].get("OffsetScreenX"));
        PostureBarData::offsetScreenY = std::stof(ini["Entity Posture Bar"].get("OffsetScreenY"));
        PostureBarData::leftScreenThreshold = std::stof(ini["Entity Posture Bar"].get("LeftScreenThreshold"));
        PostureBarData::rightScreenThreshold = std::stof(ini["Entity Posture Bar"].get("RightScreenThreshold"));
        PostureBarData::topScreenThreshold = std::stof(ini["Entity Posture Bar"].get("TopScreenThreshold"));
        PostureBarData::bottomScreenThreshold = std::stof(ini["Entity Posture Bar"].get("BottomScreenThreshold"));
        PostureBarData::usePositionFixing = ini["Entity Posture Bar"].get("UsePositionFixing") == "true";
        PostureBarData::positionFixingMultiplierX = std::stof(ini["Entity Posture Bar"].get("PositionFixingMultiplierX"));
        PostureBarData::positionFixingMultiplierY = std::stof(ini["Entity Posture Bar"].get("PositionFixingMultiplierY"));

        Logger::useLogger = ini["Debug"].get("Log") == "true";
        offsetTesting = ini["Debug"].get("OffsetTest") == "true";
    }
    catch(const std::exception& e)
    {
        Logger::useLogger = true;
        Logger::log(e.what());
        return false;
    }
    catch (...) 
    {
        Logger::useLogger = true;
        Logger::log("Unknown exception during loading of PostureBarModConfig.ini");
        return false;
    }

    return true;
}

bool saveTestOffsetToIni()
{
    try
    {
        using namespace mINI;
        INIFile config("mods\\PostureBarModConfig.ini");
        INIStructure ini;

        if (!config.read(ini))
            throw std::exception("Failed to read PostureModConfig.ini in mods folder");

        switch (debugState)
        {
            using enum EDebugTestState;

            case BossBarOffset:
                ini["Boss Posture Bar"]["FirstBossScreenX"] = std::to_string(BossPostureBarData::firstBossScreenX);
                ini["Boss Posture Bar"]["FirstBossScreenY"] = std::to_string(BossPostureBarData::firstBossScreenY);
                break;
            case EntityBarOffset:
                ini["Entity Posture Bar"]["OffsetScreenX"] = std::to_string(PostureBarData::offsetScreenX);
                ini["Entity Posture Bar"]["OffsetScreenY"] = std::to_string(PostureBarData::offsetScreenY);
                break;
            case GameScreenOffset:
                ini["General"]["ScreenPositionX"] = std::to_string(ScreenParams::posX);
                ini["General"]["ScreenPositionY"] = std::to_string(ScreenParams::posY);
                break;
            case PosFixingMultiplier:
                ini["Entity Posture Bar"]["PositionFixingMultiplierX"] = std::to_string((float)PostureBarData::positionFixingMultiplierX);
                ini["Entity Posture Bar"]["PositionFixingMultiplierY"] = std::to_string((float)PostureBarData::positionFixingMultiplierY);
                break;
        }

        config.write(ini, true);
    }
    catch (const std::exception& e)
    {
        Logger::useLogger = true;
        Logger::log(e.what());
        return false;
    }
    catch (...)
    {
        Logger::useLogger = true;
        Logger::log("Unknown exception during saving of PostureBarModConfig.ini");
        return false;
    }

    return true;
}

void MainThread()
{
    if (!loadIni())
        return;

    Logger::log("Starting Main Thread");
    g_D3DRenderer = std::make_unique<D3DRenderer>();
    g_postureUI = std::make_unique<PostureBarUI>();
    g_Hooking = std::make_unique<Hooking>();
    g_Hooking->Hook();

    std::pair<float, float> previousMoveVec{1.f, 1.f};
    int counter = 0;
    while (g_Running)
    {
        if (offsetTesting)
        {
            std::pair<float, float> moveVec{ float((GetAsyncKeyState(VK_RIGHT) & 1) - (GetAsyncKeyState(VK_LEFT) & 1)), float((GetAsyncKeyState(VK_DOWN) & 1) - (GetAsyncKeyState(VK_UP) & 1)) };

            if (counter <= 0)
                offsetSpeed = 0.5f;
            else
                offsetSpeed = std::min(5.f, offsetSpeed + 0.5f);

            if (previousMoveVec == moveVec)
                counter = 20;
            else
                counter = std::max(0, counter - 1);

            switch (debugState)
            {
                using enum EDebugTestState;

                case BossBarOffset:
                    BossPostureBarData::firstBossScreenX += moveVec.first * offsetSpeed;
                    BossPostureBarData::firstBossScreenY += moveVec.second * offsetSpeed;
                    break;
                case EntityBarOffset:
                    PostureBarData::offsetScreenX += moveVec.first * offsetSpeed;
                    PostureBarData::offsetScreenY += moveVec.second * offsetSpeed;
                    break;
                case GameScreenOffset:
                    ScreenParams::posX += moveVec.first * offsetSpeed;
                    ScreenParams::posY += moveVec.second * offsetSpeed;
                    break;
                case PosFixingMultiplier:
                    PostureBarData::positionFixingMultiplierX = std::clamp(PostureBarData::positionFixingMultiplierX + (moveVec.first * std::ceil(offsetSpeed) * 0.1), 0.0, 20.0);
                    PostureBarData::positionFixingMultiplierY = std::clamp(PostureBarData::positionFixingMultiplierY - (moveVec.second * std::ceil(offsetSpeed) * 0.1), 0.0, 20.0);
                    break;
            }

            if (GetAsyncKeyState(VK_INSERT) & 1)
                saveTestOffsetToIni();

            if (GetAsyncKeyState(VK_NEXT) & 1)
                debugState = cycleState(debugState, 1);

            if (GetAsyncKeyState(VK_PRIOR) & 1)
                debugState = cycleState(debugState, -1);

            if (moveVec.first != 0.f || moveVec.second != 0.f)
                previousMoveVec = moveVec;
        }

        std::this_thread::sleep_for(3ms);
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(500ms);
    FreeLibraryAndExitThread(g_Module, 0);
}