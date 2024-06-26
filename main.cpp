// ---------------------------------------------------------------------------------------

// DISCLAIMER:
// GTA:CTW realisation is MY OWN attempt to bring it. The engines are different,
// and the rendering in GTA:CTW - is a single line that represents a trace.
// And this line is being drawed differently.

// ---------------------------------------------------------------------------------------


#include <mod/amlmod.h>
#include <mod/logger.h>

#include "isautils.h"
#include <tracers.h>


// ---------------------------------------------------------------------------------------


MYMOD(net.theartemmaps.rusjj.tracers, GTASA BulletTracers, 1.1, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.2.1)
    ADD_DEPENDENCY_VER(net.rusjj.gtasa.utils, 1.3.0)
END_DEPLIST()


// ---------------------------------------------------------------------------------------


uintptr_t pGTASA;
void* hGTASA;
ISAUtils* sautils;


// ---------------------------------------------------------------------------------------


#define DO_REF
#include "shared.h"

const char* aSwitchesType[TRACE_TYPE_MAX] = 
{
    "GTA:SA",
    "GTA:VC",
    "GTA:III",
    "GTA:CTW"
};


// ---------------------------------------------------------------------------------------


DECL_HOOKv(TracesInit)
{
    TracesInit();
    CBulletTraces::Init();
}

DECL_HOOKv(TracesUpdate)
{
    TracesUpdate();
    if(bModEnabled) CBulletTraces::Update();
}

DECL_HOOKv(TracesRender)
{
    TracesRender();
    if(bModEnabled) CBulletTraces::Render();
}

DECL_HOOKv(TracesShutdown)
{
    TracesShutdown();
    CBulletTraces::Shutdown();
}

DECL_HOOKv(TracesAdd1, CVector *pStart, CVector *pEnd, float SizeArg, UInt32 LifeTimeArg, UInt8 OpaquenessArg)
{
    if(!bModEnabled)
    {
        TracesAdd1(pStart, pEnd, SizeArg, LifeTimeArg, OpaquenessArg);
        return;
    }
    CBulletTraces::AddTrace(pStart, pEnd, SizeArg, LifeTimeArg, OpaquenessArg);
}

DECL_HOOKv(TracesAdd2, CVector *pStart, CVector *pEnd, eWeaponType WeaponType, CEntity *pFiredByEntity)
{
    if(!bModEnabled)
    {
        TracesAdd2(pStart, pEnd, WeaponType, pFiredByEntity);
        return;
    }
    CBulletTraces::AddTrace2(pStart, pEnd, WeaponType, pFiredByEntity);
}

DECL_HOOKv(Traces_FireOneInstantHitRound, CVector *pStart, CVector *pEnd, float SizeArg, UInt32 LifeTimeArg, UInt8 OpaquenessArg)
{
    if(!bModEnabled)
    {
        Traces_FireOneInstantHitRound(pStart, pEnd, SizeArg, LifeTimeArg, OpaquenessArg);
        return;
    }
    CBulletTraces::AddTraceAfterLogic(pStart, pEnd, WEAPON_MICRO_UZI);
}


// ---------------------------------------------------------------------------------------


void OnSettingSwitch_Type(int oldVal, int newVal, void* data)
{
    clampint(0, TRACE_TYPE_MAX-1, &newVal);
    aml->MLSSetInt("TRACETYP", newVal);
    nTracesType = (eTracesType)newVal;
    CBulletTraces::InitTraces();
}
void OnSettingSwitch_Enabled(int oldVal, int newVal, void* data)
{
    clampint(0, 1, &newVal);
    //aml->MLSSetInt("TRACEWRK", newVal); // nah
    bModEnabled = (newVal!=0);
}
void OnSettingSwitch_Config(int oldVal, int newVal, void* data)
{
    clampint(0, 1, &newVal);
    aml->MLSSetInt("TRACECFG", newVal);
    bUseConfigValues = (newVal!=0);
}
extern "C" void OnModLoad()
{
    logger->SetTag("SA BulletTracers");
    
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");
    if(!pGTASA || !hGTASA)
    {
        logger->Error("Get a real GTA:SA first");
        return;
    }

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(!sautils)
    {
        logger->Error("SAUtils is missing");
        return;
    }

    // Hooks
  #ifdef AML32
    HOOKPLT(TracesInit, pGTASA + 0x66E580);
    HOOKPLT(TracesUpdate, pGTASA + 0x66FA4C);
    HOOKPLT(TracesRender, pGTASA + 0x66F868);
    HOOKPLT(TracesShutdown, pGTASA + 0x6710A4);
    HOOKPLT(TracesAdd1, pGTASA + 0x672088);
    HOOKPLT(TracesAdd2, pGTASA + 0x6740FC);
    HOOKBLX(Traces_FireOneInstantHitRound, pGTASA + 0x5E23E8);
  #else
    HOOKPLT(TracesInit, pGTASA + 0x83D918);
    HOOKPLT(TracesUpdate, pGTASA + 0x83FA90);
    HOOKPLT(TracesRender, pGTASA + 0x83F7A8);
    HOOKPLT(TracesShutdown, pGTASA + 0x841EB0);
    HOOKPLT(TracesAdd1, pGTASA + 0x8438E8);
    HOOKPLT(TracesAdd2, pGTASA + 0x846DB0);
    HOOKBL(Traces_FireOneInstantHitRound, pGTASA + 0x707D78);
  #endif

    // Game Funcs
    SET_TO(UpdateBulletTrace, aml->GetSym(hGTASA, "_ZN12CBulletTrace6UpdateEv"));
    SET_TO(FindPlayerPed, aml->GetSym(hGTASA, "_Z13FindPlayerPedi"));
    SET_TO(FindPlayerVehicle, aml->GetSym(hGTASA, "_Z17FindPlayerVehicleib"));
    SET_TO(RwTextureDestroy, aml->GetSym(hGTASA, "_Z16RwTextureDestroyP9RwTexture"));
    SET_TO(RwTextureRead, aml->GetSym(hGTASA, "_Z13RwTextureReadPKcS0_"));
    SET_TO(RwRenderStateSet, aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(RwIm3DTransform, aml->GetSym(hGTASA, "_Z15RwIm3DTransformP18RxObjSpace3DVertexjP11RwMatrixTagj"));
    SET_TO(RwIm3DRenderIndexedPrimitive, aml->GetSym(hGTASA, "_Z28RwIm3DRenderIndexedPrimitive15RwPrimitiveTypePti"));
    SET_TO(RwIm3DEnd, aml->GetSym(hGTASA, "_Z9RwIm3DEndv"));
    SET_TO(ReportFrontendAudioEvent, aml->GetSym(hGTASA, "_ZN12CAudioEngine24ReportFrontendAudioEventEiff"));

    // Game Vars
    SET_TO(TheCamera, aml->GetSym(hGTASA, "TheCamera"));
    SET_TO(m_snTimeInMilliseconds, aml->GetSym(hGTASA, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(AudioEngine, aml->GetSym(hGTASA, "AudioEngine"));

    // Our Vars
    tracesTextures = sautils->AddTextureDB("bullettraces", true);

    // Config
    bModEnabled = true;
    bDoAudioEffects = true;
    bUseConfigValues = true;
    nTracesType = TRACE_TYPE_SA;
    aml->MLSGetInt("TRACETYP", (int*)&nTracesType);
    aml->MLSGetInt("TRACECFG", (int*)&bUseConfigValues);
    //aml->MLSGetInt("TRACEWRK", (int*)&bModEnabled); // Nah

    static const char* pYesNo[] = 
    {
        "FEM_OFF",
        "FEM_ON"
    };
    sautils->AddClickableItem(eTypeOfSettings::SetType_Display, "Bullet Traces", nTracesType, 0, TRACE_TYPE_MAX-1, aSwitchesType, OnSettingSwitch_Type, NULL);
    sautils->AddClickableItem(eTypeOfSettings::SetType_Mods, "Enable BulletTraces", bUseConfigValues, 0, 1, pYesNo, OnSettingSwitch_Enabled, NULL);
    sautils->AddClickableItem(eTypeOfSettings::SetType_Mods, "Use Traces Config", bUseConfigValues, 0, 1, pYesNo, OnSettingSwitch_Config, NULL);

    // Final!
    logger->Info("The mod has been loaded");
}


// ---------------------------------------------------------------------------------------