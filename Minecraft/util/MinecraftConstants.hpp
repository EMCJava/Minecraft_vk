//
// Created by loys on 5/29/22.
//

#ifndef MINECRAFT_VK_MINECRAFTCONSTANTS_HPP
#define MINECRAFT_VK_MINECRAFTCONSTANTS_HPP

#include "MinecraftType.h"
#include <Utility/Math/Math.hpp>

static constexpr CoordinateType SectionUnitLength         = 16;
static constexpr CoordinateType SectionBinaryOffsetLength = IntLog<SectionUnitLength, 2>::value;
static_assert( 1 << SectionBinaryOffsetLength == SectionUnitLength );

static constexpr CoordinateType SectionSurfaceSize   = SectionUnitLength * SectionUnitLength;
static constexpr CoordinateType TotalBlocksInSection = SectionSurfaceSize * SectionUnitLength;

static constexpr CoordinateType MaxSectionInChunk = 24;
static constexpr CoordinateType ChunkMaxHeight    = SectionUnitLength * MaxSectionInChunk;

#endif   // MINECRAFT_VK_MINECRAFTCONSTANTS_HPP
