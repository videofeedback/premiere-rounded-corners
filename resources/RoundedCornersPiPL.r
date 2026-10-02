#include "AEConfig.h"
#include "AE_EffectVers.h"
#ifndef AE_OS_WIN
#include "AE_General.r"
#endif

resource 'PiPL' (16000) {
    {
        Kind { AEEffect },
        Name { "Rounded Corners" },
        Category { "Video Feedback" },
#ifdef AE_OS_WIN
        CodeWin64X86 { "EffectMain" },
#else
        CodeMacIntel64 { "EffectMain" },
        CodeMacARM64 { "EffectMain" },
#endif
        AE_PiPL_Version { 2, 0 },
        AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
        AE_Effect_Version { 98305 },
        AE_Effect_Info_Flags { 0 },
        AE_Effect_Global_OutFlags { 0 },
        AE_Effect_Global_OutFlags_2 { 0 },
        AE_Effect_Match_Name { "VideoFeedback Rounded Corners" },
        AE_Reserved_Info { 0 }
    }
};
