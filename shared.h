#ifdef DO_REF
    #define EXTERNAL extern
#else
    #define EXTERNAL
#endif

EXTERNAL uintptr_t *tracesTextures;

EXTERNAL void (*UpdateBulletTrace)(CBulletTrace*);
EXTERNAL CPlayerPed* (*FindPlayerPed)(int);
EXTERNAL CVehicle* (*FindPlayerVehicle)(int, bool);
EXTERNAL void (*RwTextureDestroy)(RwTexture*);
EXTERNAL RwTexture* (*RwTextureRead)(const char*, const char*);
EXTERNAL void (*RwRenderStateSet)(int, void*);
EXTERNAL void* (*RwIm3DTransform)(RwIm3DVertex *pVerts, RwUInt32 numVerts, RwMatrix *ltm, RwUInt32 flags);
EXTERNAL void (*RwIm3DRenderIndexedPrimitive)(RwPrimitiveType primType, uint16_t *indices, RwInt32 numIndices);
EXTERNAL void (*RwIm3DEnd)();
EXTERNAL void (*ReportFrontendAudioEvent)(void *self, eAudioEvents AudioEvent, float fVolumeOffsetdB, float fFrequencyScaling);

EXTERNAL CCamera* TheCamera;
EXTERNAL uint32_t *m_snTimeInMilliseconds;
EXTERNAL void *AudioEngine;

enum eTracesType : int
{
    TRACE_TYPE_SA = 0,
    TRACE_TYPE_VC,
    TRACE_TYPE_III,

    TRACE_TYPE_MAX
};
EXTERNAL eTracesType nTracesType;
EXTERNAL bool bDoAudioEffects;