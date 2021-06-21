/*
 * maprenderer.h
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#ifndef __MAPRENDERER_H__
#define __MAPRENDERER_H__

#include <tmx.h>
#include <raylib.h>
#include <string>
#include <queue>
#include <vector>


/**
 * Viewport control mode (active and reactive)
 * Active, the view port reacts to a single key pressing continuously;
 * Reactive, the view port reacts only for each key pressing;
 */
enum ViewControlMode  {
    VIEW_CONTROL_MODE_ACTIVE,
    VIEW_CONTROL_MODE_REACTIVE
};

class MapRenderer  {

    /**
     * Tile animation information.
     */
    struct __stTileAnimInfo  {
        int64_t      nMillis;
        int          nCounter;
        tmx_tile     *pNextTile;
    };
    typedef std :: deque<__stTileAnimInfo*> __AnimInfoList;
    typedef std :: vector<float> ZoomFactorList;
    typedef std :: pair<unsigned, unsigned> ZoomBorderLimits;

    Vector2                    m_CameraPos;
    ViewControlMode            m_ViewControlMode;
    int                        m_nScrollStepWidth;
    int                        m_nScrollStepHeight;
    int                        m_nWidth;
    int                        m_nHeight;
    int                        m_nTargetFps;
    float                      m_fLineThickness;
    float                      m_fZoomFactor;
    ZoomFactorList             m_vZoomFactorList;
    unsigned                   m_nCurrentZoomPos;
    unsigned                   m_nPreferredZoomPos;
    ZoomBorderLimits           m_ZoomBorderLimits;
    __AnimInfoList             m_AnimInfoList;
    std :: string              m_strTitle;
    std :: string              m_strTxMapFile;
    tmx_map                    *m_pTmxMap;
    bool                       m_bClearBackground;
    bool                       m_bIsStarted;
    bool                       m_bEnabledUserZoom;
    bool                       m_bWindowResizeable;
    bool                       m_bDrawFPS;
    static bool                m_bInitialized;

    // TmxLib overrides
    static void* TextureLoaderCallback( const char *szPath );
    static void TextureFreeCallback( void *pTexture );

    // Color control
    Color IntToColor( int color );

    // Engine primitives
    void DrawPolyline( double offset_x,
                       double offset_y,
                       double **points,
                       int points_count,
                       Color color );
    void DrawPolygon( double offset_x,
                      double offset_y,
                      double **points,
                      int points_count,
                      Color color );
    void DrawTile( void *pImage,
                   unsigned int sx,
                   unsigned int sy,
                   unsigned int sw,
                   unsigned int sh,
                   unsigned int dx,
                   unsigned int dy,
                   float opacity,
                   unsigned int flags );

    // High level primitive map handlers
    void DrawObjects( tmx_object_group *pObjgr );
    void DrawImageLayer( tmx_image *pImage );
    void DrawLayer( tmx_map *pMap, tmx_layer *pLayer );
    void DrawAllLayers( tmx_map *pMap, tmx_layer *pLayers );
    void RenderMap( void );

    bool UnloadMap( void );
    void ResetZoom( void );
    void ZoomIn( void );
    void ZoomOut( void );
    void InitializeZoomEngine( void );

    // User input handling
    void HandleUserInput( void );


    public:

    MapRenderer( int nWidth,
                 int nHeight,
                 const char* szTitle,
                 int nTargetFps = -1 );
    ~MapRenderer( void );

    void SetLineThickness( float fLineThickness );
    float GetLineThickness( void );

    void SetExitKey( KeyboardKey key );

    void SetWindowResizeable( bool bResizeable );
    void SetClearBackground( bool bStatus );
    void SetDrawFPS( bool bDrawFPS );

    void SetScrollStepSize( int nStepWidth, int nStepHeight );
    void SetEnableUserZoom( bool bEnabled );
    void SetMinZoom( unsigned nMinPos );
    void SetMaxZoom( unsigned nMaxPos );
    void SetPreferredZoom( unsigned nZoomPos );
    void SetZoom( unsigned nZoomPos );
    void SetViewControlMode( ViewControlMode mode );

    void SetMapFile( const char *szTxMapFile );

    bool Start( void );
    void Stop( void );
    bool Run( void );
};

#endif /* __MAPRENDERER_H__ */
