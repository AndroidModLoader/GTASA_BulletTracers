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
EXTERNAL bool (*RwIm3DTransform)(RwIm3DVertex *pVerts, RwUInt32 numVerts, RwMatrix *ltm, RwUInt32 flags);
EXTERNAL void (*RwIm3DRenderIndexedPrimitive)(RwPrimitiveType primType, uint16_t *indices, RwInt32 numIndices);
EXTERNAL void (*RwIm3DEnd)();

EXTERNAL CCamera* TheCamera;
EXTERNAL uint32_t *m_snTimeInMilliseconds;