//
// Created by lys on 8/18/22.
//

#ifndef MINECRAFT_VK_WORLDCHUNK_HPP
#define MINECRAFT_VK_WORLDCHUNK_HPP

#define GENERATE_DEBUG_CHUNK false

#include <list>
#include <vector>

#include "RenderableChunk.hpp"

class WorldChunk : public RenderableChunk
{
private:
    std::unordered_map<ChunkCoordinate, ChunkStatusTy> m_MissingEssentialChunks;

    MinecraftNoise m_ChunkNoise;

    // TODO: to not use shared & weak ptr
    std::vector<std::shared_ptr<Structure>> m_StructureStarts;
    std::list<std::weak_ptr<Structure>>     m_StructureReferences;

    ChunkStatus                                             m_Status = ChunkStatus::eEmpty;
    std::array<std::unique_ptr<int32_t[]>, eFullHeight + 1> m_StatusHeightMap { };

    inline void CopyHeightMapTo( HeightMapStatus status )
    {
        memcpy( m_StatusHeightMap[ status ].get( ), m_HeightMap.get( ), sizeof( m_HeightMap[ 0 ] ) * SectionSurfaceSize );
    }

    inline void ResetChunk( )
    {
        m_Status = ChunkStatus::eEmpty;

        m_StructureStarts.clear( );
        m_StructureReferences.clear( );
    }

    // return true if all chunk are upgraded
    bool UpgradeStatusAtLeastInRange( ChunkStatus targetStatus, int range );
    bool IsSavedChunksStatusAtLeastInRange( ChunkStatus targetStatus, int range ) const;

    void UpgradeChunk( ChunkStatus targetStatus );

    template <ChunkStatus status>
    [[nodiscard]] inline bool StatusCompletable( ) const;

    template <ChunkStatus status>
    inline bool AttemptCompleteStatus( );

    inline bool UpgradeSatisfied( ChunkStatus status ) const;

    /*
     *
     * Generation
     *
     * */
    void FillTerrain( const MinecraftNoise& generator );
    void FillBedRock( const MinecraftNoise& generator );

    /*
     *
     * Return and update the chunk reference
     *
     * */
    static inline int                                    GetRefIndexFromCenter( int dx, int dy ) { return ChunkReferenceRange * ( StructureReferenceStatusRange + dy ) + StructureReferenceStatusRange + dx; }
    std::weak_ptr<WorldChunk>                            TryGetChunkReference( uint32_t index ) const;
    std::weak_ptr<WorldChunk>&                           GetChunkReference( uint32_t index, const ChunkCoordinate& worldCoordinate );
    [[nodiscard]] std::vector<std::weak_ptr<WorldChunk>> GetChunkRefInRange( int range ) const;

    /*
     *
     * Generation dependencies
     *
     * */
    ChunkStatus                            m_RequiredStatus = ChunkStatus::eFull;
    uint32_t                               m_EmergencyLevel = std::numeric_limits<uint32_t>::max( );
    std::vector<std::weak_ptr<WorldChunk>> m_RequiredBy;

    static constexpr auto                                     ChunkReferenceRange = StructureReferenceStatusRange * 2 + 1;
    static constexpr auto                                     ChunkReferenceSize  = ChunkReferenceRange * ChunkReferenceRange;
    std::array<std::weak_ptr<WorldChunk>, ChunkReferenceSize> m_ChunkReferencesSaves;

public:
    explicit WorldChunk( class MinecraftWorld* world )
        : RenderableChunk( world )
    { }

    void        RegenerateChunk( ChunkStatus status );
    inline void TryUpgradeChunk( ) { UpgradeChunk( m_RequiredStatus ); }

    void SetCoordinate( const ChunkCoordinate& coordinate );

    CoordinateType GetHeight( uint32_t index, HeightMapStatus status = eFullHeight ) const;

    inline void SetExpectedStatus( ChunkStatus status ) { m_RequiredStatus = status; }
    inline void SetEmergencyLevel( uint32_t newEmergencyLevel ) { m_EmergencyLevel = newEmergencyLevel; }

    /*
     *
     * The lower, the more emergent
     *
     * */
    inline uint32_t GetEmergencyLevel( ) const
    {
        uint32_t emergencyLevel = m_EmergencyLevel;
        for ( auto& parent : m_RequiredBy )
            // chunk has not been deleted for whatever reason
            if ( !parent.expired( ) )
                emergencyLevel = std::min( emergencyLevel, parent.lock( )->GetEmergencyLevel( ) + 1 );

        return emergencyLevel;
    }

    inline const auto& GetStructureStarts( ) const { return m_StructureStarts; }

    inline auto CopyChunkNoise( ) const { return m_ChunkNoise; }
    inline auto GenerateChunkNoise( const MinecraftNoise& terrainGenerator ) const
    {
        auto noiseSeed = terrainGenerator.CopySeed( );

        const auto coordinateNoiseX = MinecraftNoise::FromUint64( GetMinecraftX( m_WorldCoordinate ) << 2 );
        const auto coordinateNoiseZ = MinecraftNoise::FromUint64( GetMinecraftZ( m_WorldCoordinate ) );

        noiseSeed.first ^= coordinateNoiseX.GetSeed( ).seed.first + coordinateNoiseZ.GetSeed( ).seed.second;
        noiseSeed.second ^= coordinateNoiseX.GetSeed( ).seed.second + coordinateNoiseZ.GetSeed( ).seed.first;

        return MinecraftNoise { noiseSeed };
    }

    bool StatusUpgradeAllSatisfied( ) const;
    bool NextStatusUpgradeSatisfied( ) const;

    inline const auto& GetStatus( ) const { return m_Status; }
    inline const auto& GetTargetStatus( ) const { return m_RequiredStatus; }
    inline const bool  IsAtLeastTargetStatus( ) const { return GetTargetStatus( ) <= m_Status; }
    inline bool        IsChunkStatusAtLeast( ChunkStatus status ) const { return m_Status >= status; }

    inline auto ExtractMissingEssentialChunks( )
    {
        auto backup = m_MissingEssentialChunks;
        m_MissingEssentialChunks.clear( );
        return backup;
    }

    size_t GetObjectSize( ) const;
};

#include "WorldChunk_Impl.hpp"

#endif   // MINECRAFT_VK_WORLDCHUNK_HPP
