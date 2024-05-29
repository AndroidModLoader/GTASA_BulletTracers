#include <mod/amlmod.h>
#include <mod/logger.h>

#include <tracers.h>
#include <shared.h>
#include <mINI/src/mini/ini.h>
#include <isautils.h>

#define FIX_BUGS
#define ARRAY_SIZE(_Array) (sizeof(_Array) / sizeof(_Array[0]))

#define RwIm3DVertexSetU(_a, _f) (_a)->texCoords.u = _f
#define RwIm3DVertexSetV(_a, _f) (_a)->texCoords.v = _f
#define RwIm3DVertexSetPos(_a, x, y, z) (_a)->position = { x, y, z }
#define RwIm3DVertexSetPosVector(_a, _vec) { CVector pp = (_vec); (_a)->position = { pp.x, pp.y, pp.z }; }
#define RwIm3DVertexSetRGBA(_a, r, g, b, a) (_a)->color = { r, g, b, a }

CBulletTrace CBulletTraces::aTraces[MAX_TRACES];
CTracesConfig CBulletTraces::configVC;
CTracesConfig CBulletTraces::configIII;
CTracesConfig CBulletTraces::configSA;

static RwTexture* gpTraceTexture = NULL;
static RwTexture* gpSmokeTrailTexture = NULL;
static RwIm3DVertex TraceVerticesVC[10];
static RwIm3DVertex TraceVerticesIII[10];
static RwIm3DVertex TraceVerticesSA[6];
static uint16_t TraceIndexListVC[48] =  { 0, 5, 7, 0, 7, 2, 0, 7, 5, 0, 2, 7, 0, 4, 9, 0,
                                          9, 5, 0, 9, 4, 0, 5, 9, 0, 1, 6, 0, 6, 5, 0, 6,
                                          1, 0, 5, 6, 0, 3, 8, 0, 8, 5, 0, 8, 3, 0, 5, 8 };
static uint16_t TraceIndexListIII[12] = { 0, 2, 1, 1, 2, 3, 2, 4, 3, 3, 4, 5 };
static uint16_t TraceIndexListSA[12] =  { 4, 1, 3, 1, 0, 3, 0, 2, 3, 3, 2, 5 };

void CBulletTraces::Init(void)
{
    // VC-part init
    RwIm3DVertexSetU(&TraceVerticesVC[0], 0.0);
    RwIm3DVertexSetV(&TraceVerticesVC[0], 0.0);
    RwIm3DVertexSetU(&TraceVerticesVC[1], 1.0);
    RwIm3DVertexSetV(&TraceVerticesVC[1], 0.0);
    RwIm3DVertexSetU(&TraceVerticesVC[2], 1.0);
    RwIm3DVertexSetV(&TraceVerticesVC[2], 0.0);
    RwIm3DVertexSetU(&TraceVerticesVC[3], 1.0);
    RwIm3DVertexSetV(&TraceVerticesVC[3], 0.0);
    RwIm3DVertexSetU(&TraceVerticesVC[4], 1.0);
    RwIm3DVertexSetV(&TraceVerticesVC[4], 0.0);
    RwIm3DVertexSetU(&TraceVerticesVC[5], 0.0);
    RwIm3DVertexSetU(&TraceVerticesVC[6], 1.0);
    RwIm3DVertexSetU(&TraceVerticesVC[7], 1.0);
    RwIm3DVertexSetU(&TraceVerticesVC[8], 1.0);
    RwIm3DVertexSetU(&TraceVerticesVC[9], 1.0);

    // III-part init
    RwIm3DVertexSetRGBA(&TraceVerticesIII[0], 20, 20, 20, 255);
    RwIm3DVertexSetRGBA(&TraceVerticesIII[1], 20, 20, 20, 255);
    RwIm3DVertexSetRGBA(&TraceVerticesIII[2], 70, 70, 70, 255);
    RwIm3DVertexSetRGBA(&TraceVerticesIII[3], 70, 70, 70, 255);
    RwIm3DVertexSetRGBA(&TraceVerticesIII[4], 10, 10, 10, 255);
    RwIm3DVertexSetRGBA(&TraceVerticesIII[5], 10, 10, 10, 255);
    RwIm3DVertexSetU(&TraceVerticesIII[0], 0.0);
    RwIm3DVertexSetV(&TraceVerticesIII[0], 0.0);
    RwIm3DVertexSetU(&TraceVerticesIII[1], 1.0);
    RwIm3DVertexSetV(&TraceVerticesIII[1], 0.0);
    RwIm3DVertexSetU(&TraceVerticesIII[2], 0.0);
    RwIm3DVertexSetV(&TraceVerticesIII[2], 0.5);
    RwIm3DVertexSetU(&TraceVerticesIII[3], 1.0);
    RwIm3DVertexSetV(&TraceVerticesIII[3], 0.5);
    RwIm3DVertexSetU(&TraceVerticesIII[4], 0.0);
    RwIm3DVertexSetV(&TraceVerticesIII[4], 1.0);
    RwIm3DVertexSetU(&TraceVerticesIII[5], 1.0);
    RwIm3DVertexSetV(&TraceVerticesIII[5], 1.0);

    // SA-part init
    RwIm3DVertexSetRGBA(&TraceVerticesSA[0], 255, 255, 128, 0);
    RwIm3DVertexSetRGBA(&TraceVerticesSA[1], 255, 255, 128, 0);
    RwIm3DVertexSetRGBA(&TraceVerticesSA[2], 255, 255, 128, 0);
    RwIm3DVertexSetRGBA(&TraceVerticesSA[3], 255, 255, 128, 0);
    RwIm3DVertexSetRGBA(&TraceVerticesSA[4], 255, 255, 128, 0);
    RwIm3DVertexSetRGBA(&TraceVerticesSA[5], 255, 255, 128, 0);

    // General init
    InitTraces();
    gpTraceTexture = RwTextureRead("trace", NULL);
    gpSmokeTrailTexture = RwTextureRead("smoketrail", NULL);

    // Traces.ini loading part (Vice City)
    {
        char path[256];
        snprintf(path, sizeof(path), "%s/VCBulletTrails_Weapons.ini", aml->GetConfigPath());
        mINI::INIFile file(path);
        mINI::INIStructure ini;
        file.read(ini);

        // Reading weapons params
        for (int i = 22; i < 512; ++i)
        {
            std::string name = "WEP";
            std::string formatted_str = name + std::to_string(i);
            const char* formatted_str2 = formatted_str.c_str();

            std::string strb = ini.get(formatted_str2).get("thickness");
            std::string strc = ini.get(formatted_str2).get("lifetime");
            std::string strd = ini.get(formatted_str2).get("visibility");

            CBulletTraces::configVC.thickness[i] =  std::atof( strb.c_str() );
            CBulletTraces::configVC.lifetime[i] =   std::atoi( strc.c_str() );
            CBulletTraces::configVC.visibility[i] = std::atoi( strd.c_str() );
        }
    }

    // Traces.ini loading part (San Andreas)
    {
        char path[256];
        snprintf(path, sizeof(path), "%s/SABulletTrails_Weapons.ini", aml->GetConfigPath());
        mINI::INIFile file(path);
        mINI::INIStructure ini;
        file.read(ini);

        // Reading weapons params
        for (int i = 22; i < 512; ++i)
        {
            std::string name = "WEP";
            std::string formatted_str = name + std::to_string(i);
            const char* formatted_str2 = formatted_str.c_str();

            std::string strb = ini.get(formatted_str2).get("thickness");
            std::string strc = ini.get(formatted_str2).get("lifetime");
            std::string strd = ini.get(formatted_str2).get("visibility");

            CBulletTraces::configSA.thickness[i] =  std::atof( strb.c_str() );
            CBulletTraces::configSA.lifetime[i] =   std::atoi( strc.c_str() );
            CBulletTraces::configSA.visibility[i] = std::atoi( strd.c_str() );
        }
    }
}

void CBulletTraces::InitTraces(void)
{
    memset(aTraces, 0, sizeof(aTraces));
}

void CBulletTraces::AddTrace(CVector* start, CVector* end, float thickness, uint32_t lifeTime, uint8_t visibility)
{
    int32_t  enabledCount;
    uint32_t modifiedLifeTime;
    int32_t  nextSlot;

    if(nTracesType != TRACE_TYPE_III)
    {
        enabledCount = 0;
        for (int i = 0; i < MAX_TRACES; ++i)
        {
            if (aTraces[i].bIsUsed) ++enabledCount;
        }

        if      (enabledCount >= 10) modifiedLifeTime = lifeTime / 4;
        else if (enabledCount >= 5)  modifiedLifeTime = lifeTime / 2;
        else                         modifiedLifeTime = lifeTime;
    }
    else
    {
        modifiedLifeTime = 2000;
    }

    nextSlot = 0;
    for (int i = 0; nextSlot < MAX_TRACES && aTraces[i].bIsUsed; ++i) ++nextSlot;
    if (nextSlot < MAX_TRACES)
    {
        aTraces[nextSlot].Start = *start;
        aTraces[nextSlot].End = *end;
        aTraces[nextSlot].bIsUsed = true;
        aTraces[nextSlot].TimeCreated = *m_snTimeInMilliseconds;
        aTraces[nextSlot].Opaqueness = (nTracesType != TRACE_TYPE_III) ? visibility : (25 + rand() % 32);
        aTraces[nextSlot].Size = thickness;
        aTraces[nextSlot].LifeTime = modifiedLifeTime;
    }

    if(bDoAudioEffects)
    {
        CBulletTraces::ProcessEffects(&aTraces[nextSlot]);
    }
}

void CBulletTraces::AddTrace2(CVector* start, CVector* end, eWeaponType weaponType, CEntity* shooter)
{
    CPhysical* player;
    float speed;
    int16_t camMode;

    if (shooter == (CEntity*)FindPlayerPed(-1) || (FindPlayerVehicle(-1, false) != NULL && FindPlayerVehicle(-1, false) == (CVehicle*)shooter))
    {
      #ifdef AML32
        camMode = TheCamera->m_apCams[TheCamera->m_nCurrentActiveCam].Mode;
      #else // That's because im lazy.
        camMode = TheCamera->Cams[TheCamera->ActiveCam].Mode;
      #endif
        if (camMode == MODE_M16_1STPERSON
         || camMode == MODE_CAMERA
         || camMode == MODE_SNIPER
         || camMode == MODE_M16_1STPERSON_RUNABOUT
         || camMode == MODE_ROCKETLAUNCHER
         || camMode == MODE_ROCKETLAUNCHER_RUNABOUT
         || camMode == MODE_SNIPER_RUNABOUT
         || camMode == MODE_HELICANNON_1STPERSON)
        {
            player = FindPlayerVehicle(-1, false) ? (CPhysical*)FindPlayerVehicle(-1, false) : (CPhysical*)FindPlayerPed(-1);
            speed = player->m_vecMoveSpeed.Magnitude();
            if (speed < 0.05f) return;
        }
    }
    
    switch(nTracesType)
    {
        default: // SA
            if(bUseConfigValues) AddTrace(start, end, CBulletTraces::configSA.thickness[weaponType], CBulletTraces::configSA.lifetime[weaponType], CBulletTraces::configSA.visibility[weaponType]);
            else AddTrace(start, end, 0.01f, 300, 70);
            break;

        case TRACE_TYPE_III:
            AddTrace(start, end, 0, 0, 0);
            break;

        case TRACE_TYPE_VC:
            if(bUseConfigValues) AddTrace(start, end, CBulletTraces::configVC.thickness[weaponType], CBulletTraces::configVC.lifetime[weaponType], CBulletTraces::configVC.visibility[weaponType]);
            else
            {
                switch(weaponType)
                {
                    case WEAPON_DESERT_EAGLE:
                    case WEAPON_SHOTGUN:
                    case WEAPON_SAWNOFF_SHOTGUN:
                    case WEAPON_SPAS12_SHOTGUN:
                        AddTrace(start, end, 0.7f, 1000, 200);
                        break;

                    case WEAPON_COUNTRYRIFLE:
                    case WEAPON_SNIPERRIFLE:
                    case WEAPON_AK47:
                    case WEAPON_M4:
                    case WEAPON_MINIGUN:
                        AddTrace(start, end, 1.0f, 2000, 220);
                        break;

                    default:
                        AddTrace(start, end, 0.4f, 750, 150);
                        break;
                }
            }
            break;
    }
}

void CBulletTraces::Update(void)
{
    if(nTracesType == TRACE_TYPE_III)
    {
        for (int i = 0; i < MAX_TRACES; ++i)
        {
            if (aTraces[i].bIsUsed)
            {
                if(aTraces[i].TimeCreated + aTraces[i].LifeTime < *m_snTimeInMilliseconds)
                {
                    aTraces[i].bIsUsed = false;
                    continue;
                }

                CVector diff = aTraces[i].Start - aTraces[i].End;
                float remaining = diff.Magnitude();
                if (remaining > 0.8f)
                {
                    aTraces[i].Start = aTraces[i].End + (remaining - 0.8f) / remaining * diff;
                }
                else
                {
                    aTraces[i].bIsUsed = false;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < MAX_TRACES; ++i)
        {
            if (aTraces[i].bIsUsed) UpdateBulletTrace(&aTraces[i]);
        }
    }
}

void CBulletTraces::Shutdown(void)
{
    if (gpTraceTexture)
    {
        RwTextureDestroy(gpTraceTexture);
        gpTraceTexture = NULL;
    }
    if (gpSmokeTrailTexture)
    {
        RwTextureDestroy(gpSmokeTrailTexture);
        gpSmokeTrailTexture = NULL;
    }
}

void CBulletTraces::Render(void)
{
    switch(nTracesType)
    {
        default:
            RenderSA();
            break;

        case TRACE_TYPE_VC:
            RenderVC();
            break;

        case TRACE_TYPE_III:
            RenderIII();
            break;
    }
}

void CBulletTraces::RenderVC(void)
{
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,  (void*)0);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,      (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,     (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)(gpSmokeTrailTexture->raster));

    for (int i = 0; i < MAX_TRACES; ++i)
    {
        CBulletTrace& trace = aTraces[i];
        if (!trace.bIsUsed) continue;
        
        float timeAlive = *m_snTimeInMilliseconds - trace.TimeCreated;

        float traceThickness = trace.Size * timeAlive / trace.LifeTime;

        CVector horizontalOffset = trace.End - trace.Start;
        VectorNormalise(&horizontalOffset);
        horizontalOffset *= traceThickness;

        // trace is dying = more transparency
        uint8_t nAlphaValue = trace.Opaqueness * (trace.LifeTime - timeAlive) / trace.LifeTime;

        CVector start = trace.Start;
        CVector end = trace.End;
        float startProj = DotProduct(start - TheCamera->GetPosition(), TheCamera->GetForward()) - 0.7f;
        float endProj = DotProduct(end - TheCamera->GetPosition(), TheCamera->GetForward()) - 0.7f;
        if (startProj < 0.0f && endProj < 0.0f) continue; // we dont need to render trace behind us

        // if start behind us, move it closer
        if (startProj < 0.0f)
        {
            float absStartProj = std::abs(startProj);
            float absEndProj = std::abs(endProj);
            start = (absEndProj * start + absStartProj * end) / (absStartProj + absEndProj);
        }
        else if (endProj < 0.0f)
        {
            float absStartProj = std::abs(startProj);
            float absEndProj = std::abs(endProj);
            end = (absEndProj * start + absStartProj * end) / (absStartProj + absEndProj);
        }

        // we divide trace at three parts
        CVector start2 = (7.0f * start + end) / 8;
        CVector end2 = (7.0f * end + start) / 8;

        RwIm3DVertexSetV(&TraceVerticesVC[5], 10.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[6], 10.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[7], 10.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[8], 10.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[9], 10.0f);

        RwIm3DVertexSetRGBA(&TraceVerticesVC[0], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[1], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[2], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[3], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[4], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[5], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[6], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[7], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[8], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[9], 255, 255, 255, nAlphaValue);

        // two points in center
        RwIm3DVertexSetPos(&TraceVerticesVC[0], start2.x, start2.y, start2.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[5], end2.x, end2.y, end2.z);

        // vertical planes
        RwIm3DVertexSetPos(&TraceVerticesVC[1], start2.x, start2.y, start2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[3], start2.x, start2.y, start2.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[6], end2.x, end2.y, end2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[8], end2.x, end2.y, end2.z - traceThickness);

        // horizontal planes
        RwIm3DVertexSetPos(&TraceVerticesVC[2], start2.x + horizontalOffset.y, start2.y - horizontalOffset.x, start2.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[7], end2.x + horizontalOffset.y, end2.y - horizontalOffset.x, end2.z);
      #ifdef FIX_BUGS // this point calculated wrong for some reason
        RwIm3DVertexSetPos(&TraceVerticesVC[4], start2.x - horizontalOffset.y, start2.y + horizontalOffset.x, start2.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[9], end2.x - horizontalOffset.y, end2.y + horizontalOffset.x, end2.z);
      #else
        RwIm3DVertexSetPos(&TraceVerticesVC[4], start2.x - horizontalOffset.y, start2.y - horizontalOffset.y, start2.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[9], end2.x - horizontalOffset.y, end2.y - horizontalOffset.y, end2.z);
      #endif

        if (RwIm3DTransform(TraceVerticesVC, ARRAY_SIZE(TraceVerticesVC), NULL, rwIM3D_VERTEXUV))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexListVC, ARRAY_SIZE(TraceIndexListVC));
            RwIm3DEnd();
        }

        RwIm3DVertexSetV(&TraceVerticesVC[5], 2.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[6], 2.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[7], 2.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[8], 2.0f);
        RwIm3DVertexSetV(&TraceVerticesVC[9], 2.0f);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[0], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[1], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[2], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[3], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVerticesVC[4], 255, 255, 255, 0);

        RwIm3DVertexSetPos(&TraceVerticesVC[0], start.x, start.y, start.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[1], start.x, start.y, start.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[3], start.x, start.y, start.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[2], start.x + horizontalOffset.y, start.y - horizontalOffset.x, start.z);

        RwIm3DVertexSetPos(&TraceVerticesVC[5], start2.x, start2.y, start2.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[6], start2.x, start2.y, start2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[8], start2.x, start2.y, start2.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[7], start2.x + horizontalOffset.y, start2.y - horizontalOffset.x, start2.z);
#ifdef FIX_BUGS
        RwIm3DVertexSetPos(&TraceVerticesVC[4], start.x - horizontalOffset.y, start.y + horizontalOffset.x, start.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[9], start2.x - horizontalOffset.y, start2.y + horizontalOffset.x, start2.z);
#else
        RwIm3DVertexSetPos(&TraceVerticesVC[4], start.x - horizontalOffset.y, start.y - horizontalOffset.y, start.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[9], start2.x - horizontalOffset.y, start2.y - horizontalOffset.y, start2.z);
#endif

        if (RwIm3DTransform(TraceVerticesVC, ARRAY_SIZE(TraceVerticesVC), NULL, rwIM3D_VERTEXUV))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexListVC, ARRAY_SIZE(TraceIndexListVC));
            RwIm3DEnd();
        }

        RwIm3DVertexSetPos(&TraceVerticesVC[1], end.x, end.y, end.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[2], end.x, end.y, end.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[4], end.x, end.y, end.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[3], end.x + horizontalOffset.y, end.y - horizontalOffset.x, end.z);

        RwIm3DVertexSetPos(&TraceVerticesVC[5], end2.x, end2.y, end2.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[6], end2.x, end2.y, end2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[8], end2.x, end2.y, end2.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVerticesVC[7], end2.x + horizontalOffset.y, end2.y - horizontalOffset.x, end2.z);
#ifdef FIX_BUGS
        RwIm3DVertexSetPos(&TraceVerticesVC[5], end.x - horizontalOffset.y, end.y + horizontalOffset.x, end.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[9], end2.x - horizontalOffset.y, end2.y + horizontalOffset.x, end2.z);
#else
        RwIm3DVertexSetPos(&TraceVerticesVC[5], end.x - horizontalOffset.y, end.y - horizontalOffset.y, end.z);
        RwIm3DVertexSetPos(&TraceVerticesVC[9], end2.x - horizontalOffset.y, end2.y - horizontalOffset.y, end2.z);
#endif

        if (RwIm3DTransform(TraceVerticesVC, ARRAY_SIZE(TraceVerticesVC), NULL, rwIM3D_VERTEXUV))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexListVC, ARRAY_SIZE(TraceIndexListVC));
            RwIm3DEnd();
        }
    }
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,  (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,      (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,     (void*)rwBLENDINVSRCALPHA);
}

void CBulletTraces::RenderIII(void)
{
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,  (void*)0);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,      (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,     (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)(gpTraceTexture->raster));
  #ifdef FIX_BUGS
    // Raster has no transparent pixels so it relies on the raster format having alpha
    // to turn on blending. librw image conversion might get rid of it right now so let's
    // just force it on.
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
  #endif

    for (int i = 0; i < MAX_TRACES; i++)
    {
        CBulletTrace& trace = aTraces[i];
        if (!trace.bIsUsed) continue;

        CVector inf = trace.Start;
        CVector sup = trace.End;
        CVector center = (inf + sup) / 2;
        CVector supsubinf = (sup - inf);
        CVector width = CrossProduct(&TheCamera->GetForward(), &supsubinf);
        VectorNormalise(&width);
        width = width / 20.0f;
        uint8_t intensity = trace.Opaqueness;
        for (int i = 0; i < ARRAY_SIZE(TraceVerticesIII); ++i)
        {
            RwIm3DVertexSetRGBA(&TraceVerticesIII[i], intensity, intensity, intensity, 255);
        }
        RwIm3DVertexSetPos(&TraceVerticesIII[0], inf.x + width.x, inf.y + width.y, inf.z + width.z);
        RwIm3DVertexSetPos(&TraceVerticesIII[1], inf.x - width.x, inf.y - width.y, inf.z - width.z);
        RwIm3DVertexSetPos(&TraceVerticesIII[2], center.x + width.x, center.y + width.y, center.z + width.z);
        RwIm3DVertexSetPos(&TraceVerticesIII[3], center.x - width.x, center.y - width.y, center.z - width.z);
        RwIm3DVertexSetPos(&TraceVerticesIII[4], sup.x + width.x, sup.y + width.y, sup.z + width.z);
        RwIm3DVertexSetPos(&TraceVerticesIII[5], sup.x - width.x, sup.y - width.y, sup.z - width.z);
        if (RwIm3DTransform(TraceVerticesIII, ARRAY_SIZE(TraceVerticesIII), NULL, rwIM3D_VERTEXUV))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexListIII, ARRAY_SIZE(TraceIndexListIII));
            RwIm3DEnd();
        }
    }
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,     (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,    (void*)rwBLENDINVSRCALPHA);
}

// https://github.com/gta-reversed/gta-reversed-modern/blob/master/source/game_sa/BulletTraces.cpp#L166
void CBulletTraces::RenderSA(void)
{
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      (void*)0);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATECULLMODE,          (void*)rwCULLMODECULLNONE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     NULL);

    for (int i = 0; i < MAX_TRACES; i++)
    {
        CBulletTrace& trace = aTraces[i];
        if (!trace.bIsUsed) continue;

        const float t = 1.0f - (float)(*m_snTimeInMilliseconds - trace.TimeCreated) / (float)trace.LifeTime;
        
        CVector camToOriginDir = (trace.Start - TheCamera->GetPosition());
        VectorNormalise(&camToOriginDir);

        CVector direction = trace.End - trace.Start;
        VectorNormalise(&direction);

        CVector up = camToOriginDir.Cross(direction);
        VectorNormalise(&up);

        CVector sizeVec = up * (trace.Size * t);
        CVector currPosOnTrace = trace.End - (trace.End - trace.Start) * t;

        RwIm3DVertexSetPosVector(&TraceVerticesSA[0], currPosOnTrace);
        RwIm3DVertexSetPosVector(&TraceVerticesSA[1], currPosOnTrace + sizeVec);
        RwIm3DVertexSetPosVector(&TraceVerticesSA[2], currPosOnTrace - sizeVec);
        RwIm3DVertexSetPosVector(&TraceVerticesSA[3], trace.End);
        RwIm3DVertexSetPosVector(&TraceVerticesSA[4], trace.End + sizeVec);
        RwIm3DVertexSetPosVector(&TraceVerticesSA[5], trace.End - sizeVec);

        TraceVerticesSA[3].color.alpha = (RwUInt8)(t * trace.Opaqueness);
        
        if (RwIm3DTransform(TraceVerticesSA, ARRAY_SIZE(TraceVerticesSA), NULL, rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexListSA, ARRAY_SIZE(TraceIndexListSA));
            RwIm3DEnd();
        }
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,     (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,    (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATECULLMODE,     (void*)rwCULLMODECULLBACK);
}

void CBulletTraces::ProcessEffects(CBulletTrace* trace)
{
    CMatrix& camMat = *(TheCamera->GetMatrix());
    const CVector& camPos = camMat.GetPosition();

    // Make their position relative to the camera's
    const auto fromRelToCam = trace->Start - camPos;
    const auto toRelToCam = trace->End - camPos;

    // Transform both points into the camera's space ((C)cam (S)pace - CS)
    const float fromCSY = DotProduct(fromRelToCam, camMat.GetForward());

    const float toCSY = DotProduct(toRelToCam, camMat.GetForward());

    if (std::signbit(toCSY) == std::signbit(fromCSY)) { // Originally: toCSY * fromCSY < 0.0f - Check if signs differ
        return; // Both points are either in front or behind us
    }

    // They do, in this case points are on opposite sides (one behind, one in front of the camera)

    // Now calculate the remaining coordinates
    const float fromCSX = DotProduct(fromRelToCam, camMat.GetRight());
    const float fromCSZ = DotProduct(fromRelToCam, camMat.GetUp());

    const float toCSX = DotProduct(toRelToCam, camMat.GetRight());
    const float toCSZ = DotProduct(toRelToCam, camMat.GetUp());

    // Calculate distance to point on line that is on the same Y axis as the camera
    // (This point on line is basically the bullet when passing by the camera)
        
    // Interpolation on line
    const float t = fabs(fromCSY) / (fabs(fromCSY) + fabs(toCSY));

    const float pointOnLineZ = fromCSZ + (toCSZ - fromCSZ) * t;
    const float pointOnLineX = fromCSX + (toCSX - fromCSX) * t;

    // Calculate distance from camera to point on line
    const float camToLineDist = std::hypotf(pointOnLineZ, pointOnLineX);

    if (camToLineDist >= 2.0f) {
        return; // Point too far from camera
    }

    const auto ReportBulletAudio = [&](auto event) {
        const float volDistFactor = 1.0f - camToLineDist * 0.5f;
        const float volumeChange  = volDistFactor == 0.0f ? -100.0f : std::log10(volDistFactor);
        ReportFrontendAudioEvent(AudioEngine, event, volumeChange, 1.0f);
    };

    const bool isComingFromBehind = fromCSY <= 0.0f; // Is the bullet coming from behind us?
    if (0.f <= pointOnLineX) { // Is bullet passing on the right of the camera?
        ReportBulletAudio(isComingFromBehind ? AE_FRONTEND_BULLET_PASS_RIGHT_REAR : AE_FRONTEND_BULLET_PASS_RIGHT_FRONT);
    } else { // Bullet passing on left of the camera.
        ReportBulletAudio(isComingFromBehind ? AE_FRONTEND_BULLET_PASS_LEFT_REAR : AE_FRONTEND_BULLET_PASS_LEFT_FRONT);
    }
}