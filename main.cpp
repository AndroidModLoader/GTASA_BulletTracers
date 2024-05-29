// ---------------------------------------------------------------------------------------


#include <mod/amlmod.h>
#include <mod/logger.h>

#include "isautils.h"
#include <tracers.h>


// ---------------------------------------------------------------------------------------


MYMOD(net.theartemmaps.rusjj.tracers, GTASA BulletTracers, 1.0, RusJJ)
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


// ---------------------------------------------------------------------------------------


DECL_HOOKv(TracesInit)
{
    TracesInit();
    CBulletTraces::Init();
}

DECL_HOOKv(TracesUpdate)
{
    TracesUpdate();
    CBulletTraces::Update();
}

DECL_HOOKv(TracesRender)
{
    TracesRender();
    CBulletTraces::Render();
}

DECL_HOOKv(TracesAdd1, CVector *pStart, CVector *pEnd, float SizeArg, UInt32 LifeTimeArg, UInt8 OpaquenessArg)
{
    CBulletTraces::AddTrace(pStart, pEnd, SizeArg, LifeTimeArg, OpaquenessArg);
}

DECL_HOOKv(TracesAdd2, CVector *pStart, CVector *pEnd, Int32 WeaponType, CEntity *pFiredByEntity)
{
    CBulletTraces::AddTrace2(pStart, pEnd, WeaponType, pFiredByEntity);
}


// ---------------------------------------------------------------------------------------


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

  #ifdef AML32
    HOOKPLT(TracesInit, pGTASA + 0x66E580);
    HOOKPLT(TracesUpdate, pGTASA + 0x66FA4C);
    HOOKPLT(TracesRender, pGTASA + 0x66F868);
    HOOKPLT(TracesAdd1, pGTASA + 0x672088);
    HOOKPLT(TracesAdd2, pGTASA + 0x6740FC);
  #else
    HOOKPLT(TracesInit, pGTASA + 0x83D918);
    HOOKPLT(TracesUpdate, pGTASA + 0x83FA90);
    HOOKPLT(TracesRender, pGTASA + 0x83F7A8);
    HOOKPLT(TracesAdd1, pGTASA + 0x8438E8);
    HOOKBL(TracesAdd1, pGTASA + 0x6E6314);
    HOOKPLT(TracesAdd2, pGTASA + 0x846DB0);
  #endif

    SET_TO(UpdateBulletTrace, aml->GetSym(hGTASA, "_ZN12CBulletTrace6UpdateEv"));
    SET_TO(FindPlayerPed, aml->GetSym(hGTASA, "_Z13FindPlayerPedi"));
    SET_TO(FindPlayerVehicle, aml->GetSym(hGTASA, "_Z17FindPlayerVehicleib"));
    SET_TO(RwTextureDestroy, aml->GetSym(hGTASA, "_Z16RwTextureDestroyP9RwTexture"));
    SET_TO(RwTextureRead, aml->GetSym(hGTASA, "_Z13RwTextureReadPKcS0_"));
    SET_TO(RwRenderStateSet, aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(RwIm3DTransform, aml->GetSym(hGTASA, "_Z15RwIm3DTransformP18RxObjSpace3DVertexjP11RwMatrixTagj"));
    SET_TO(RwIm3DRenderIndexedPrimitive, aml->GetSym(hGTASA, "_Z28RwIm3DRenderIndexedPrimitive15RwPrimitiveTypePti"));
    SET_TO(RwIm3DEnd, aml->GetSym(hGTASA, "_Z9RwIm3DEndv"));

    SET_TO(TheCamera, aml->GetSym(hGTASA, "TheCamera"));
    SET_TO(m_snTimeInMilliseconds, aml->GetSym(hGTASA, "_ZN6CTimer22m_snTimeInMillisecondsE"));

    tracesTextures = sautils->AddTextureDB("bullettraces", true);

    logger->Info("The mod has been loaded");
}


// ---------------------------------------------------------------------------------------