#include <mod/amlmod.h>
#include <mod/logger.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
#else
    #include "GTASA_STRUCTS_210.h"
#endif
#include "isautils.h"

MYMOD(net.theartemmaps.rusjj.tracers, GTASA BulletTracers, 1.0, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.2.1)
END_DEPLIST()


// ---------------------------------------------------------------------------------------


uintptr_t pGTASA;
void* hGTASA;
ISAUtils* sautils;


// ---------------------------------------------------------------------------------------





// ---------------------------------------------------------------------------------------


extern "C" void OnModLoad()
{
    logger->SetTag("SA BulletTracers");
    
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");
    
    if(!pGTASA || !hGTASA)
    {
        logger->Info("Get a real GTA:SA first");
        return;
    }
    sautils = (ISAUtils*)GetInterface("SAUtils");
}