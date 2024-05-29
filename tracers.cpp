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
#define RwIm3DVertexSetRGBA(_a, r, g, b, a) (_a)->color = { r, g, b, a }

CBulletTrace CBulletTraces::aTraces[MAX_TRACES];
float CBulletTraces::thickness[512];
int CBulletTraces::lifetime[512];
int CBulletTraces::visibility[512];

RwTexture* gpTraceTexture = NULL;
RwTexture* gpSmokeTrailTexture = NULL;
RwIm3DVertex TraceVertices[10];
static uint16_t TraceIndexList[48] = { 0, 5, 7, 0, 7, 2, 0, 7, 5, 0, 2, 7, 0, 4, 9, 0,
                                       9, 5, 0, 9, 4, 0, 5, 9, 0, 1, 6, 0, 6, 5, 0, 6,
                                       1, 0, 5, 6, 0, 3, 8, 0, 8, 5, 0, 8, 3, 0, 5, 8 };

void CBulletTraces::Init(void)
{
    memset(aTraces, 0, sizeof(aTraces));

    gpTraceTexture = RwTextureRead("trace", NULL);
    gpSmokeTrailTexture = RwTextureRead("smoketrail", NULL);
    
    TraceVertices[0].texCoords.u = 0.0;
    TraceVertices[0].texCoords.v = 0.0;
    TraceVertices[1].texCoords.u = 1.0;
    TraceVertices[1].texCoords.v = 0.0;
    TraceVertices[2].texCoords.u = 1.0;
    TraceVertices[2].texCoords.v = 0.0;
    TraceVertices[3].texCoords.u = 1.0;
    TraceVertices[3].texCoords.v = 0.0;
    TraceVertices[4].texCoords.u = 1.0;
    TraceVertices[4].texCoords.v = 0.0;
    TraceVertices[5].texCoords.u = 0.0;
    TraceVertices[5].texCoords.v = 0.0;
    TraceVertices[6].texCoords.u = 1.0;
    TraceVertices[6].texCoords.v = 0.0;
    TraceVertices[7].texCoords.u = 1.0;
    TraceVertices[7].texCoords.v = 0.0;
    TraceVertices[8].texCoords.u = 1.0;
    TraceVertices[8].texCoords.v = 0.0;
    TraceVertices[9].texCoords.u = 1.0;
    TraceVertices[9].texCoords.v = 0.0;

    TraceIndexList[0] = 0;
    TraceIndexList[1] = 2;
    TraceIndexList[2] = 1;
    TraceIndexList[3] = 1;
    TraceIndexList[4] = 2;
    TraceIndexList[5] = 3;
    TraceIndexList[6] = 2;
    TraceIndexList[7] = 4;
    TraceIndexList[8] = 3;
    TraceIndexList[9] = 3;
    TraceIndexList[10] = 4;
    TraceIndexList[11] = 5;

    // Traces ini load
    char path[256];
    snprintf(path, sizeof(path), "%s/VCBulletTrails_Weapons.ini", aml->GetConfigPath());
    mINI::INIFile file(path);
    mINI::INIStructure ini;
    file.read(ini);

    // Reading weapons params
    for (int i = 22; i < 512; i++)
    {
        std::string name = "WEP";
        std::string formatted_str = name + std::to_string(i);
        const char* formatted_str2 = formatted_str.c_str();

        std::string strb = ini.get(formatted_str2).get("thickness");
        std::string strc = ini.get(formatted_str2).get("lifetime");
        std::string strd = ini.get(formatted_str2).get("visibility");

        thickness[i] =  std::atof( strb.c_str() );
        lifetime[i] =   std::atoi( strc.c_str() );
        visibility[i] = std::atoi( strd.c_str() );

        //logger->Info("Init Bullet Tracer for weapon #%d (%s), thickness=%f lifetime=%d visibility=%d", i, formatted_str.c_str(), thickness[i], lifetime[i], visibility[i]);
    }
}

void CBulletTraces::AddTrace(CVector* start, CVector* end, float thickness, uint32_t lifeTime, uint8_t visibility)
{
    int32_t  enabledCount;
    uint32_t modifiedLifeTime;
    int32_t  nextSlot;

    enabledCount = 0;
    for (int i = 0; i < MAX_TRACES; ++i)
    {
        if (aTraces[i].bIsUsed) ++enabledCount;
    }

    if      (enabledCount >= 10) modifiedLifeTime = lifeTime / 4;
    else if (enabledCount >= 5)  modifiedLifeTime = lifeTime / 2;
    else                         modifiedLifeTime = lifeTime;

    nextSlot = 0;
    for (int i = 0; nextSlot < MAX_TRACES && aTraces[i].bIsUsed; ++i) ++nextSlot;
    if (nextSlot < MAX_TRACES)
    {
        aTraces[nextSlot].Start = *start;
        aTraces[nextSlot].End = *end;
        aTraces[nextSlot].bIsUsed = true;
        aTraces[nextSlot].TimeCreated = *m_snTimeInMilliseconds;
        aTraces[nextSlot].Opaqueness = visibility;
        aTraces[nextSlot].Size = thickness;
        aTraces[nextSlot].LifeTime = modifiedLifeTime;
    }

    float startProjFwd = DotProduct(TheCamera->GetForward(), *start - TheCamera->GetPosition());
    float endProjFwd = DotProduct(TheCamera->GetForward(), *end - TheCamera->GetPosition());
    if (startProjFwd * endProjFwd < 0.0f) // if one of point behind us and second before us
    {
        float fStartDistFwd = abs(startProjFwd) / (abs(startProjFwd) + abs(endProjFwd));

        float startProjUp = DotProduct(TheCamera->GetUp(), *start - TheCamera->GetPosition());
        float endProjUp = DotProduct(TheCamera->GetUp(), *end - TheCamera->GetPosition());
        float distUp = (endProjUp - startProjUp) * fStartDistFwd + startProjUp;

        float startProjRight = DotProduct(TheCamera->GetRight(), *start - TheCamera->GetPosition());
        float endProjRight = DotProduct(TheCamera->GetRight(), *end - TheCamera->GetPosition());
        float distRight = (endProjRight - startProjRight) * fStartDistFwd + startProjRight;

        float dist = sqrt(sq(distUp) + sq(distRight));
    }
}

void CBulletTraces::AddTrace2(CVector* start, CVector* end, int32_t weaponType, class CEntity* shooter)
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
    AddTrace(start, end, thickness[weaponType], lifetime[weaponType], visibility[weaponType]);
}

void CBulletTraces::Update(void)
{
    for (int i = 0; i < MAX_TRACES; i++)
    {
        if (aTraces[i].bIsUsed) UpdateBulletTrace(&aTraces[i]);
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
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)(gpSmokeTrailTexture->raster));

    for (int i = 0; i < MAX_TRACES; ++i)
    {
        if (!aTraces[i].bIsUsed) continue;
        
        float timeAlive = *m_snTimeInMilliseconds - aTraces[i].TimeCreated;

        float traceThickness = aTraces[i].Size * timeAlive / aTraces[i].LifeTime;

        CVector horizontalOffset = aTraces[i].End - aTraces[i].Start;
        VectorNormalise(&horizontalOffset);
        horizontalOffset *= traceThickness;

        // then closer trace to die then it more transparent
        uint8_t nAlphaValue = aTraces[i].Opaqueness * (aTraces[i].LifeTime - timeAlive) / aTraces[i].LifeTime;

        CVector start = aTraces[i].Start;
        CVector end = aTraces[i].End;
        float startProj = DotProduct(start - TheCamera->GetPosition(), TheCamera->GetForward()) - 0.7f;
        float endProj = DotProduct(end - TheCamera->GetPosition(), TheCamera->GetForward()) - 0.7f;
        if (startProj < 0.0f && endProj < 0.0f) continue; // we dont need render trace behind us

        // if strat behind us move it closer
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

        RwIm3DVertexSetV(&TraceVertices[5], 10.0f);
        RwIm3DVertexSetV(&TraceVertices[6], 10.0f);
        RwIm3DVertexSetV(&TraceVertices[7], 10.0f);
        RwIm3DVertexSetV(&TraceVertices[8], 10.0f);
        RwIm3DVertexSetV(&TraceVertices[9], 10.0f);

        RwIm3DVertexSetRGBA(&TraceVertices[0], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[1], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[2], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[3], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[4], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[5], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[6], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[7], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[8], 255, 255, 255, nAlphaValue);
        RwIm3DVertexSetRGBA(&TraceVertices[9], 255, 255, 255, nAlphaValue);

        // two points in center
        RwIm3DVertexSetPos(&TraceVertices[0], start2.x, start2.y, start2.z);
        RwIm3DVertexSetPos(&TraceVertices[5], end2.x, end2.y, end2.z);

        // vertical planes
        RwIm3DVertexSetPos(&TraceVertices[1], start2.x, start2.y, start2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[3], start2.x, start2.y, start2.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[6], end2.x, end2.y, end2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[8], end2.x, end2.y, end2.z - traceThickness);

        // horizontal planes
        RwIm3DVertexSetPos(&TraceVertices[2], start2.x + horizontalOffset.y, start2.y - horizontalOffset.x, start2.z);
        RwIm3DVertexSetPos(&TraceVertices[7], end2.x + horizontalOffset.y, end2.y - horizontalOffset.x, end2.z);
      #ifdef FIX_BUGS // this point calculated wrong for some reason
        RwIm3DVertexSetPos(&TraceVertices[4], start2.x - horizontalOffset.y, start2.y + horizontalOffset.x, start2.z);
        RwIm3DVertexSetPos(&TraceVertices[9], end2.x - horizontalOffset.y, end2.y + horizontalOffset.x, end2.z);
      #else
        RwIm3DVertexSetPos(&TraceVertices[4], start2.x - horizontalOffset.y, start2.y - horizontalOffset.y, start2.z);
        RwIm3DVertexSetPos(&TraceVertices[9], end2.x - horizontalOffset.y, end2.y - horizontalOffset.y, end2.z);
      #endif

        if (RwIm3DTransform(TraceVertices, ARRAY_SIZE(TraceVertices), nullptr, 1))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexList, ARRAY_SIZE(TraceIndexList));
            RwIm3DEnd();
        }

        RwIm3DVertexSetV(&TraceVertices[5], 2.0f);
        RwIm3DVertexSetV(&TraceVertices[6], 2.0f);
        RwIm3DVertexSetV(&TraceVertices[7], 2.0f);
        RwIm3DVertexSetV(&TraceVertices[8], 2.0f);
        RwIm3DVertexSetV(&TraceVertices[9], 2.0f);
        RwIm3DVertexSetRGBA(&TraceVertices[0], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVertices[1], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVertices[2], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVertices[3], 255, 255, 255, 0);
        RwIm3DVertexSetRGBA(&TraceVertices[4], 255, 255, 255, 0);

        RwIm3DVertexSetPos(&TraceVertices[0], start.x, start.y, start.z);
        RwIm3DVertexSetPos(&TraceVertices[1], start.x, start.y, start.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[3], start.x, start.y, start.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[2], start.x + horizontalOffset.y, start.y - horizontalOffset.x, start.z);

        RwIm3DVertexSetPos(&TraceVertices[5], start2.x, start2.y, start2.z);
        RwIm3DVertexSetPos(&TraceVertices[6], start2.x, start2.y, start2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[8], start2.x, start2.y, start2.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[7], start2.x + horizontalOffset.y, start2.y - horizontalOffset.x, start2.z);
#ifdef FIX_BUGS
        RwIm3DVertexSetPos(&TraceVertices[4], start.x - horizontalOffset.y, start.y + horizontalOffset.x, start.z);
        RwIm3DVertexSetPos(&TraceVertices[9], start2.x - horizontalOffset.y, start2.y + horizontalOffset.x, start2.z);
#else
        RwIm3DVertexSetPos(&TraceVertices[4], start.x - horizontalOffset.y, start.y - horizontalOffset.y, start.z);
        RwIm3DVertexSetPos(&TraceVertices[9], start2.x - horizontalOffset.y, start2.y - horizontalOffset.y, start2.z);
#endif

        if (RwIm3DTransform(TraceVertices, ARRAY_SIZE(TraceVertices), nullptr, rwIM3D_VERTEXUV))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexList, ARRAY_SIZE(TraceIndexList));
            RwIm3DEnd();
        }

        RwIm3DVertexSetPos(&TraceVertices[1], end.x, end.y, end.z);
        RwIm3DVertexSetPos(&TraceVertices[2], end.x, end.y, end.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[4], end.x, end.y, end.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[3], end.x + horizontalOffset.y, end.y - horizontalOffset.x, end.z);

        RwIm3DVertexSetPos(&TraceVertices[5], end2.x, end2.y, end2.z);
        RwIm3DVertexSetPos(&TraceVertices[6], end2.x, end2.y, end2.z + traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[8], end2.x, end2.y, end2.z - traceThickness);
        RwIm3DVertexSetPos(&TraceVertices[7], end2.x + horizontalOffset.y, end2.y - horizontalOffset.x, end2.z);
#ifdef FIX_BUGS
        RwIm3DVertexSetPos(&TraceVertices[5], end.x - horizontalOffset.y, end.y + horizontalOffset.x, end.z);
        RwIm3DVertexSetPos(&TraceVertices[9], end2.x - horizontalOffset.y, end2.y + horizontalOffset.x, end2.z);
#else
        RwIm3DVertexSetPos(&TraceVertices[5], end.x - horizontalOffset.y, end.y - horizontalOffset.y, end.z);
        RwIm3DVertexSetPos(&TraceVertices[9], end2.x - horizontalOffset.y, end2.y - horizontalOffset.y, end2.z);
#endif

        if (RwIm3DTransform(TraceVertices, ARRAY_SIZE(TraceVertices), nullptr, rwIM3D_VERTEXUV))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TraceIndexList, ARRAY_SIZE(TraceIndexList));
            RwIm3DEnd();
        }
    }
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}