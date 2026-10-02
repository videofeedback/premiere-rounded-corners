#include "AEConfig.h"
#include "AE_Effect.h"
#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <vector>

namespace {
std::vector<PF_ParamDef> controls;
bool cancelled = false;
PF_Err addParameter(PF_ProgPtr, PF_ParamIndex index, PF_ParamDefPtr def) {
    assert(index == -1);
    controls.push_back(*def);
    return PF_Err_NONE;
}
PF_Err abortRender(PF_ProgPtr) {
    return cancelled ? PF_Interrupt_CANCEL : PF_Err_NONE;
}
}

int main(int argc, char** argv) {
    assert(argc == 2);
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) { std::cerr << dlerror() << '\n'; return 1; }
    using Entry = PF_Err (*)(PF_Cmd, PF_InData*, PF_OutData*, PF_ParamDef*[], PF_LayerDef*, void*);
    auto entry = reinterpret_cast<Entry>(dlsym(library, "EffectMain"));
    assert(entry);
    PF_InData in{};
    PF_OutData out{};
    in.inter.add_param = addParameter;
    in.inter.abort = abortRender;
    in.downsample_x = {1, 1}; in.downsample_y = {1, 1}; in.pixel_aspect_ratio = {1, 1};
    assert(entry(PF_Cmd_GLOBAL_SETUP, &in, &out, nullptr, nullptr, nullptr) == PF_Err_NONE);
    assert(out.my_version == 98305 && out.out_flags == 0 && out.out_flags2 == 0);
    assert(entry(PF_Cmd_PARAMS_SETUP, &in, &out, nullptr, nullptr, nullptr) == PF_Err_NONE);
    assert(out.num_params == 11 && controls.size() == 10);
    for (int i = 0; i < 10; ++i) {
        assert(controls[i].param_type == PF_Param_FLOAT_SLIDER);
        assert(controls[i].uu.id == i + 1);
        assert(!(controls[i].flags & PF_ParamFlag_CANNOT_TIME_VARY));
        assert(controls[i].u.fs_d.value == (i == 0 ? 80 : i < 5 ? 100 : i < 9 ? 0 : 10));
        assert(bool(controls[i].ui_flags & PF_PUI_INVISIBLE) == (i < 3));
    }
    constexpr int width = 64, height = 32, sourceStride = 68, destStride = 72;
    const PF_Pixel color{128, 100, 50, 25}, sentinel{231, 232, 233, 234};
    std::vector<PF_Pixel> source(sourceStride * height, color);
    std::vector<PF_Pixel> destination(destStride * height, sentinel);
    PF_ParamDef input{};
    input.u.ld.data = source.data();
    input.u.ld.width = width; input.u.ld.height = height;
    input.u.ld.rowbytes = sourceStride * sizeof(PF_Pixel);
    PF_LayerDef result{};
    result.data = destination.data(); result.width = width; result.height = height;
    result.rowbytes = destStride * sizeof(PF_Pixel);
    PF_ParamDef* params[]{&input, &controls[0], &controls[1], &controls[2], &controls[3], &controls[4],
                          &controls[5], &controls[6], &controls[7], &controls[8], &controls[9]};
    controls[9].u.fs_d.value = 37.5;
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Err_NONE);
    assert(destination[0].alpha == 0 && destination[0].red == 0);
    assert(destination[16 * destStride + 32].alpha == 128);
    bool foundEdge = false;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto p = destination[y * destStride + x];
            assert(p.alpha <= 128 && p.red <= 100);
            foundEdge |= p.alpha > 0 && p.alpha < 128;
            assert(std::memcmp(&source[y * sourceStride + x], &color, sizeof(color)) == 0);
        }
        for (int x = width; x < destStride; ++x)
            assert(std::memcmp(&destination[y * destStride + x], &sentinel, sizeof(sentinel)) == 0);
    }
    assert(foundEdge);
    controls[3].u.fs_d.value = 50;
    controls[4].u.fs_d.value = 50;
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Err_NONE);
    assert(destination[9 * destStride + 19].alpha == 128);
    assert(destination[8 * destStride + 16].alpha == 0);
    assert(destination[16 * destStride + 15].alpha == 0);
    controls[3].u.fs_d.value = 100;
    controls[4].u.fs_d.value = 100;
    // Synthetic letterboxed video with a unique source color at every pixel.
    // Explicit edges remove black bars without moving or resampling the image.
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            source[y * sourceStride + x] = y < 8 || y >= 28 ? PF_Pixel{255,0,0,0}
                : PF_Pixel{255,static_cast<A_u_char>(x*3),static_cast<A_u_char>(y*7),37};
    controls[5].u.fs_d.value = 25;   // top = 8
    controls[6].u.fs_d.value = 12.5; // bottom = 28
    controls[7].u.fs_d.value = 12.5; // left = 8
    controls[8].u.fs_d.value = 6.25; // right = 60
    controls[9].u.fs_d.value = 20;   // radius = 4 (short side = 20)
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Err_NONE);
    assert(destination[7 * destStride + 32].alpha == 0);
    assert(destination[28 * destStride + 32].alpha == 0);
    assert(destination[16 * destStride + 7].alpha == 0);
    assert(destination[16 * destStride + 60].alpha == 0);
    assert(destination[8 * destStride + 8].alpha == 0);
    for (int y = 12; y < 24; ++y)
        for (int x = 12; x < 56; ++x)
            assert(std::memcmp(&destination[y * destStride + x],
                               &source[y * sourceStride + x], sizeof(PF_Pixel)) == 0);
    controls[5].u.fs_d.value = 90; // crossed top/bottom must be transparent
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Err_NONE);
    assert(destination[16 * destStride + 32].alpha == 0);
    for (int i = 5; i < 9; ++i) controls[i].u.fs_d.value = 0;
    std::fill(source.begin(), source.end(), color);
    // Negative row strides must behave like the same image with reversed storage.
    input.u.ld.data = source.data() + sourceStride * (height - 1);
    input.u.ld.rowbytes = -sourceStride * static_cast<int>(sizeof(PF_Pixel));
    result.data = destination.data() + destStride * (height - 1);
    result.rowbytes = -destStride * static_cast<int>(sizeof(PF_Pixel));
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Err_NONE);
    controls[9].u.fs_d.value = 0;
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Err_NONE);
    assert(destination[0].alpha == 128 && destination[0].red == 100);
    cancelled = true;
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Interrupt_CANCEL);
    cancelled = false;
    result.width = 63;
    assert(entry(PF_Cmd_RENDER, &in, &out, params, &result, nullptr) == PF_Err_BAD_CALLBACK_PARAM);
    dlclose(library);
    std::cout << "PASS: compiled plugin loads, SDK entry point, version, 7 visible animatable controls, "
                 "render, circular frame-relative radii, 4 edges, letterbox, unchanged source pixels, alpha, row padding, negative strides, zero radius, cancellation, bounds\n";
}
