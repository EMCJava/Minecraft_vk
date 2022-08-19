//
// Created by lys on 8/18/22.
//

#ifndef MINECRAFT_VK_WORLDCHUNK_HPP
#define MINECRAFT_VK_WORLDCHUNK_HPP

#include <vector>

#include "RenderableChunk.hpp"

class WorldChunk : public RenderableChunk
{
private:
    std::unordered_map<ChunkCoordinate, ChunkStatusTy> m_MissingEssentialChunks;

    MinecraftNoise m_ChunkNoise;

    // TODO: to not use shared & weak ptr
    std::vector<std::shared_ptr<Structure>> m_StructureStarts;
    std::vector<std::weak_ptr<Structure>>   m_StructureReferences;

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

    bool UpgradeStatusAtLeastInRange( ChunkStatus targetStatus, int range );
    bool IsStatusAtLeastInRange( ChunkStatus targetStatus, int range ) const;

    void UpgradeChunk( ChunkStatus targetStatus );

    bool CanRunStructureStart( ) const;
    bool CanRunStructureReference( ) const;
    bool CanRunNoise( ) const;
    bool CanRunFeature( ) const;

    bool AttemptRunStructureStart( );
    bool AttemptRunStructureReference( );
    bool AttemptRunNoise( );
    bool AttemptRunFeature( );

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
    std::weak_ptr<WorldChunk>              GetChunkReferenceConst( uint32_t index, const ChunkCoordinate& worldCoordinate ) const;
    std::weak_ptr<WorldChunk>&             GetChunkReference( uint32_t index, const ChunkCoordinate& worldCoordinate );
    std::vector<std::weak_ptr<WorldChunk>> GetChunkRefInRange( int range );

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

    ~WorldChunk( )
    {
        for ( int i = 0; i < DirHorizontalSize; ++i )
            if ( m_NearChunks[ i ] != nullptr ) m_NearChunks[ i ]->SyncChunkFromDirection( nullptr, static_cast<Direction>( i ^ 0b1 ) );
    }

    void        RegenerateChunk( ChunkStatus status );
    inline void TryUpgradeChunk( ) { UpgradeChunk( m_RequiredStatus ); }

    void SetCoordinate( const ChunkCoordinate& coordinate );

    CoordinateType GetHeight( uint32_t index, HeightMapStatus status = eFullHeight );

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

    inline auto CopyChunkNoise( ) { return m_ChunkNoise; }
    inline auto GetChunkNoise( const MinecraftNoise& terrainGenerator ) const
    {
        auto noiseSeed = terrainGenerator.CopySeed( );
        noiseSeed.first ^= GetMinecraftX( m_WorldCoordinate ) << 13;
        noiseSeed.second ^= GetMinecraftZ( m_WorldCoordinate ) << 7;

        return MinecraftNoise { noiseSeed };
    }

    bool StatusUpgradeAllSatisfied( ) const;
    bool NextStatusUpgradeSatisfied( ) const;

    inline const auto& GetStatus( ) const { return m_Status; }
    inline const auto& GetTargetStatus( ) const { return m_RequiredStatus; }
    inline const bool  IsAtLeastTargetStatus( ) const { return GetTargetStatus( ) <= m_Status; }
    inline bool        IsChunkStatusAtLeast( ChunkStatus status ) const { return m_Status >= status; }
};


#endif   // MINECRAFT_VK_WORLDCHUNK_HPP
