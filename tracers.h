#ifdef AML32
    #include "GTASA_STRUCTS.h"
#else
    #include "GTASA_STRUCTS_210.h"
#endif

// 64 is enough.
#define MAX_TRACES 64

struct CTracesConfig
{
    bool  loaded;
    float thickness[512];
    int   lifetime[512];
    int   visibility[512];
};

class CBulletTraces
{
public:
    static void Init(void);
    static void InitTraces(void);
    static void Render(void);
    static void Update(void);
    static void Shutdown(void);
    static void AddTrace(CVector* start, CVector* end, float thickness, uint32_t lifeTime, uint8_t visibility);
    static void AddTrace2(CVector* start, CVector* end, eWeaponType weaponType, CEntity* shooter);
    static void AddTraceAfterLogic(CVector* start, CVector* end, eWeaponType weaponType, bool isInstantFire = false);

private:
    static void RenderSA(void);
    static void RenderVC(void);
    static void RenderIII(void);
    static void RenderCTW(void);
    static void ProcessEffects(CBulletTrace* trace);

public:
    static CBulletTrace aTraces[MAX_TRACES];
    static CTracesConfig configVC;
    static CTracesConfig configIII;
    static CTracesConfig configSA;
};