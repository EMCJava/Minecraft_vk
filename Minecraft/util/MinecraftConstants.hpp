//
// Created by loys on 5/29/22.
//

#ifndef MINECRAFT_VK_MINECRAFTCONSTANTS_HPP
#define MINECRAFT_VK_MINECRAFTCONSTANTS_HPP

#include "MinecraftType.h"
#include <Utility/Math/Math.hpp>

enum Direction : uint8_t {
    DirFront,
    DirBack,
    DirRight,
    DirLeft,
    DirHorizontalSize = DirLeft + 1,
    DirUp             = DirHorizontalSize,
    DirDown,
    DirSize
};

enum DirectionBit : uint8_t {
    DirFrontBit = 1 << DirFront,
    DirBackBit  = 1 << DirBack,
    DirRightBit = 1 << DirRight,
    DirLeftBit  = 1 << DirLeft,
    DirUpBit    = 1 << DirUp,
    DirDownBit  = 1 << DirDown,
    DirBitSize  = 6
};

static constexpr CoordinateType SectionUnitLength             = 16;
static constexpr CoordinateType SectionUnitLengthBinaryOffset = IntLog<SectionUnitLength, 2>::value;
static_assert( 1 << SectionUnitLengthBinaryOffset == SectionUnitLength );

static constexpr CoordinateType SectionSurfaceSize             = SectionUnitLength * SectionUnitLength;
static constexpr CoordinateType SectionSurfaceSizeBinaryOffset = SectionUnitLengthBinaryOffset + SectionUnitLengthBinaryOffset;
static constexpr CoordinateType SectionVolume                  = SectionSurfaceSize * SectionUnitLength;
static constexpr CoordinateType SectionVolumeBinaryOffset      = SectionSurfaceSizeBinaryOffset + SectionUnitLengthBinaryOffset;

static constexpr CoordinateType MaxSectionInChunk = 24;
static constexpr CoordinateType ChunkMaxHeight    = SectionUnitLength * MaxSectionInChunk;

static constexpr uint32_t ChunkThreadDelayPeriod = 100;

using IndexBufferType = uint32_t;

static constexpr uint32_t MaxMemoryAllocationPerVertexBuffer = 64 * 1024 * 1024;
using SolidChunkVertexIndexSizeCapConverter                  = ScaleToSecond_S<4, 1>;
using SCVISCC                                                = SolidChunkVertexIndexSizeCapConverter;
static constexpr uint32_t MaxMemoryAllocationPerIndexBuffer  = SCVISCC::ConvertToSecond( MaxMemoryAllocationPerVertexBuffer );

static constexpr uint32_t IndirectDrawBufferSizeStep = 20 * 1024;

#endif   // MINECRAFT_VK_MINECRAFTCONSTANTS_HPP
