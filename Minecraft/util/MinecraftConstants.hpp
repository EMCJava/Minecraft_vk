//
// Created by loys on 5/29/22.
//

#ifndef MINECRAFT_VK_MINECRAFTCONSTANTS_HPP
#define MINECRAFT_VK_MINECRAFTCONSTANTS_HPP

#include "MinecraftType.h"
#include <Utility/Math/Math.hpp>


enum EightWayDirection : uint8_t {
    EWDirFront,
    EWDirBack,
    EWDirRight,
    EWDirLeft,
    EWDirFrontRight,
    EWDirBackLeft,
    EWDirFrontLeft,
    EWDirBackRight,

    EightWayDirectionSize
};

enum CubeDirection : uint8_t {
    DirFront,
    DirBack,
    DirRight,
    DirLeft,
    DirHorizontalSize = DirLeft + 1,
    DirUp             = DirHorizontalSize,
    DirDown,
    DirSize
};

enum DirectionBit : uint32_t {
    DirFrontBit = 1 << DirFront,
    DirBackBit  = 1 << DirBack,
    DirRightBit = 1 << DirRight,
    DirLeftBit  = 1 << DirLeft,
    DirUpBit    = 1 << DirUp,
    DirDownBit  = 1 << DirDown,
    DirFaceMask = ( DirDownBit << 1 ) - 1,

    DirFrontRightBit = 1 << 6,
    DirBackLeftBit   = 1 << 7,
    DirFrontLeftBit  = 1 << 8,
    DirBackRightBit  = 1 << 9,

    DirFrontUpBit   = 1 << 10,
    DirBackDownBit  = 1 << 11,
    DirBackUpBit    = 1 << 12,
    DirFrontDownBit = 1 << 13,
    DirRightUpBit   = 1 << 14,
    DirLeftDownBit  = 1 << 15,
    DirRightDownBit = 1 << 16,
    DirLeftUpBit    = 1 << 17,

    DirFrontRightUpBit   = 1 << 18,
    DirBackLeftDownBit   = 1 << 19,
    DirFrontLeftDownBit  = 1 << 20,
    DirBackRightUpBit    = 1 << 21,
    DirFrontLeftUpBit    = 1 << 22,
    DirBackRightDownBit  = 1 << 23,
    DirFrontRightDownBit = 1 << 24,
    DirBackLeftUpBit     = 1 << 25,

    DirBitSize
};

/*
 *
 * Chunk Sizes
 *
 * */
static constexpr CoordinateType SectionUnitLength             = 16;
static constexpr CoordinateType SectionUnitLengthBinaryOffset = IntLog<SectionUnitLength, 2>::value;
static_assert( 1 << SectionUnitLengthBinaryOffset == SectionUnitLength );

static constexpr CoordinateType SectionSurfaceSize             = SectionUnitLength * SectionUnitLength;
static constexpr CoordinateType SectionSurfaceSizeBinaryOffset = SectionUnitLengthBinaryOffset + SectionUnitLengthBinaryOffset;
static constexpr CoordinateType SectionVolume                  = SectionSurfaceSize * SectionUnitLength;
static constexpr CoordinateType SectionVolumeBinaryOffset      = SectionSurfaceSizeBinaryOffset + SectionUnitLengthBinaryOffset;

static constexpr CoordinateType MaxSectionInChunk = 24;
static constexpr CoordinateType ChunkMaxHeight    = SectionUnitLength * MaxSectionInChunk;

static constexpr CoordinateType ChunkVolume = MaxSectionInChunk * SectionVolume;

/*
 *
 * Generation
 *
 * */
static constexpr CoordinateType StructureReferenceStatusRange = 8;


/*
 *
 * Chunk storage
 *
 * */
static constexpr uint32_t ChunkThreadDelayPeriod = 100;
static constexpr uint32_t ChunkAccessCacheSize   = 8;

/*
 *
 * Memory
 *
 * */
static constexpr auto FaceVerticesCount = 4;
static constexpr auto FaceIndicesCount  = 6;

using IndexBufferType = uint32_t;

static constexpr uint32_t MaxMemoryAllocation = 128 * 1024 * 1024;
static constexpr uint32_t OptimalBufferGap    = 1024 * 1024;

static constexpr uint32_t IndirectDrawBufferSizeStep = 20 * 1024;

/*
 *
 * Game configuration
 *
 * */
#define SHOW_CHUNK_BARRIER false

#endif   // MINECRAFT_VK_MINECRAFTCONSTANTS_HPP
