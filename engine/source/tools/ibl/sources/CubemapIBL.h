/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IBL_CUBEMAPIBL_H
#define IBL_CUBEMAPIBL_H

#include "core/math/moyu_math2.h"

#include <vector>

#include <stdint.h>
#include <stddef.h>

namespace MoYu {
namespace ibl {

class Cubemap;
class Image;

/**
 * Generates cubemaps for the IBL.
 */
class CubemapIBL {
public:
    /**
     * Computes a roughness LOD using prefiltered importance sampling GGX
     *
     * @param dst               the destination cubemap
     * @param levels            a list of prefiltered lods of the source environment
     * @param linearRoughness   roughness
     * @param maxNumSamples     number of samples for importance sampling
     * @param updater           a callback for the caller to track progress
     */
    static void roughnessFilter(Cubemap& dst, const std::vector<Cubemap>& levels,
            float linearRoughness, size_t maxNumSamples, glm::float3 mirror, bool prefilter);

    //! Computes the "DFG" term of the "split-sum" approximation and stores it in a 2D image
    static void DFG(Image& dst, bool multiscatter, bool cloth);

    /**
     * Computes the diffuse irradiance using prefiltered importance sampling GGX
     *
     * @note Usually this is done using spherical harmonics instead.
     *
     * @param dst               the destination cubemap
     * @param levels            a list of prefiltered lods of the source environment
     * @param maxNumSamples     number of samples for importance sampling
     * @param updater           a callback for the caller to track progress
     *
     * @see CubemapSH
     */
    static void diffuseIrradiance(Cubemap& dst, const std::vector<Cubemap>& levels, size_t maxNumSamples = 1024);
};

} // namespace ibl
} // namespace MoYu

#endif /* IBL_CUBEMAPIBL_H */
