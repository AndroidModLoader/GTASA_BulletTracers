#ifdef AML32
    #include "GTASA_STRUCTS.h"
#else
    #include "GTASA_STRUCTS_210.h"
#endif

#define MAX_TRACES 256

class CBulletTraces
{
public:
	static void         Init(void);
	static void         Render(void);
	static void         Update(void);
	static void         Shutdown(void);
	static void         AddTrace(CVector* start, CVector* end, float thickness, uint32_t lifeTime, uint8_t visibility);
	static void         AddTrace2(CVector* start, CVector* end, int32_t weaponType, class CEntity* shooter);

public:
	static CBulletTrace aTraces[MAX_TRACES];
	static float        thickness[512];
	static int          lifetime[512];
	static int          visibility[512];
};