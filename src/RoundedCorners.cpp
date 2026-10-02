#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "RoundedMask.hpp"
#include <cstdio>
#include <cstring>

namespace {
// Keep old disk IDs/types reserved so saved frame settings cannot become new controls.
enum Parameter { Input, LegacyRadius, LegacyCornerWidth, LegacyCornerHeight,
                 FrameWidth, FrameHeight, Top, Bottom, Left, Right, Radius, Count };

PF_Err legacySlider(PF_InData* in_data, const char* name, A_long id, double value, double maximum) {
    PF_ParamDef def{};
    def.param_type = PF_Param_FLOAT_SLIDER;
    std::snprintf(def.PF_DEF_NAME, sizeof(def.PF_DEF_NAME), "%s", name);
    def.uu.id = id;
    def.ui_flags = PF_PUI_INVISIBLE;
    def.u.fs_d.value = value;
    def.u.fs_d.dephault = static_cast<PF_FpShort>(value);
    def.u.fs_d.valid_max = def.u.fs_d.slider_max = static_cast<PF_FpShort>(maximum);
    def.u.fs_d.precision = PF_Precision_TENTHS;
    return PF_ADD_PARAM(in_data, -1, &def);
}

PF_Err parameters(PF_InData* in_data, PF_OutData* out_data) {
    PF_ParamDef def;
    PF_Err err = legacySlider(in_data, "Legacy Radius", LegacyRadius, 80, 10000);
    if (err) return err;
    err = legacySlider(in_data, "Legacy Corner Width", LegacyCornerWidth, 100, 1000);
    if (err) return err;
    err = legacySlider(in_data, "Legacy Corner Height", LegacyCornerHeight, 100, 1000);
    if (err) return err;
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Frame Width (%)", 0, 100, 0, 100, 100,
                         PF_Precision_TENTHS, 0, 0, FrameWidth);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Frame Height (%)", 0, 100, 0, 100, 100,
                         PF_Precision_TENTHS, 0, 0, FrameHeight);
    PF_ADD_FLOAT_SLIDERX("Frame Top (%)", 0, 100, 0, 50, 0,
                         PF_Precision_TENTHS, 0, 0, Top);
    PF_ADD_FLOAT_SLIDERX("Frame Bottom (%)", 0, 100, 0, 50, 0,
                         PF_Precision_TENTHS, 0, 0, Bottom);
    PF_ADD_FLOAT_SLIDERX("Frame Left (%)", 0, 100, 0, 50, 0,
                         PF_Precision_TENTHS, 0, 0, Left);
    PF_ADD_FLOAT_SLIDERX("Frame Right (%)", 0, 100, 0, 50, 0,
                         PF_Precision_TENTHS, 0, 0, Right);
    PF_ADD_FLOAT_SLIDERX("Corner Radius (% of frame)", 0, 50, 0, 50, 10,
                         PF_Precision_TENTHS, 0, 0, Radius);
    out_data->num_params = Count;
    return PF_Err_NONE;
}

double ratio(A_long numerator, A_long denominator) noexcept {
    return numerator > 0 && denominator > 0 ? double(numerator) / denominator : 1.0;
}

PF_Err render(PF_InData* in_data, PF_ParamDef* params[], PF_LayerDef* output) {
    const PF_LayerDef& input = params[Input]->u.ld;
    if (!input.data || !output->data) return PF_Err_BAD_CALLBACK_PARAM;
    if (input.width != output->width || input.height != output->height)
        return PF_Err_BAD_CALLBACK_PARAM;
    rounded::Settings settings;
    settings.radius = params[Radius]->u.fs_d.value;
    settings.frameWidth = params[FrameWidth]->u.fs_d.value;
    settings.frameHeight = params[FrameHeight]->u.fs_d.value;
    settings.top = params[Top]->u.fs_d.value;
    settings.bottom = params[Bottom]->u.fs_d.value;
    settings.left = params[Left]->u.fs_d.value;
    settings.right = params[Right]->u.fs_d.value;
    const rounded::Mask mask(input.width, input.height, settings,
        ratio(in_data->downsample_x.num, in_data->downsample_x.den),
        ratio(in_data->downsample_y.num, in_data->downsample_y.den),
        ratio(in_data->pixel_aspect_ratio.num, in_data->pixel_aspect_ratio.den));
    for (A_long y = 0; y < output->height; ++y) {
        if ((y & 31) == 0) {
            const PF_Err err = PF_ABORT(in_data);
            if (err) return err;
        }
        const auto* source = reinterpret_cast<const PF_Pixel*>(
            reinterpret_cast<const char*>(input.data) + std::ptrdiff_t(y) * input.rowbytes);
        auto* destination = reinterpret_cast<PF_Pixel*>(
            reinterpret_cast<char*>(output->data) + std::ptrdiff_t(y) * output->rowbytes);
        rounded::renderRow(source, destination, output->width, y, mask);
    }
    return PF_Err_NONE;
}
} // namespace

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data,
    PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output, void*) {
    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                    "Rounded Corners 0.3\rCircular corners inside independently trimmed frame edges.");
                break;
            case PF_Cmd_GLOBAL_SETUP:
                out_data->my_version = PF_VERSION(0, 3, 0, PF_Stage_DEVELOP, 1);
                // Baseline AE-compatible ARGB8 path. Additional native Premiere
                // formats must be explicitly registered before using other layouts.
                out_data->out_flags = 0;
                out_data->out_flags2 = 0;
                break;
            case PF_Cmd_PARAMS_SETUP: return parameters(in_data, out_data);
            case PF_Cmd_RENDER: return render(in_data, params, output);
            default: break;
        }
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return PF_Err_NONE;
}
