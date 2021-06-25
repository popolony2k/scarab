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
 * Viewport definition.
 */
typedef Rectangle  Viewport;

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
    float                      m_fWidth;
    float                      m_fHeight;
    int                        m_nTargetFps;
    float                      m_fLineThickness;
    float                      m_fZoomFactor;
    ZoomFactorList             m_vZoomFactorList;
    unsigned                   m_nCurrentZoomPos;
    unsigned                   m_nPreferredZoomPos;
    ZoomBorderLimits           m_ZoomBorderLimits;
    __AnimInfoList             m_AnimInfoList;
    Viewport                   m_Viewport;
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

    // Graphics primitives miscellaneous
    bool GetClippedArea( int32_t nSourceW,
                         int32_t nSourceH,
                         int32_t nDestX,
                         int32_t nDestY,
                         float& fViewportX,
                         float& fViewportY,
                         float& fWidth,
                         float& fHeight,
                         bool bResetViewOnNegative = false );

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
    void DrawRectangle( double offset_x,
                        double offset_y,
                        double width,
                        double height,
                        Color color );
    void MidPointEllipse( double fCoordX,
                          double fCoordY,
                          double fRadiusX,
                          double fRadiusY,
                          Color color );
    void DrawEllipse( double offset_x,
                      double offset_y,
                      double width,
                      double height,
                      Color color );
    void DrawTile( void *pImage,
                   int32_t nSourceX,
                   int32_t nSourceY,
                   int32_t nSourceW,
                   int32_t nSourceH,
                   int32_t nDestX,
                   int32_t nDestY,
                   float fOpacity );

    // High level primitive map handlers
    void DrawObjects( tmx_object_group *pObjgr );
    void DrawImageLayer( tmx_image *pImage );
    void DrawLayer( tmx_map *pMap, tmx_layer *pLayer );
    void DrawAllLayers( tmx_map *pMap, tmx_layer *pLayers );
    void RenderMap( void );

    bool UnloadMap( void );
    void InitializeZoomEngine( void );

    // User input handling
    void HandleUserInput( void );


    public:

    MapRenderer( float fWidth,
                 float fHeight,
                 const char* szTitle,
                 int nTargetFps = -1 );
    ~MapRenderer( void );

    // Object line rendering
    void SetLineThickness( float fLineThickness );
    float GetLineThickness( void );

    // Window behavior
    void SetExitKey( KeyboardKey key );
    void SetWindowResizeable( bool bResizeable );
    void SetClearBackground( bool bStatus );
    void SetDrawFPS( bool bDrawFPS );

    // View port control
    void SetViewControlMode( ViewControlMode mode );
    void SetViewport( Viewport viewport );
    void SetScrollStepSize( int nStepWidth, int nStepHeight );
    void SetEnableUserZoom( bool bEnabled );
    void SetMinZoom( unsigned nMinPos );
    void SetMaxZoom( unsigned nMaxPos );
    void SetPreferredZoom( unsigned nZoomPos );
    void SetZoom( unsigned nZoomPos );
    void ResetZoom( void );
    void ZoomIn( void );
    void ZoomOut( void );

    // Camera management
    void MoveCameraUp( void );
    void MoveCameraDown( void );
    void MoveCameraLeft( void );
    void MoveCameraRight( void );

    // Map file management
    void SetMapFile( const char *szTxMapFile );

    // Renderer flow control.
    bool Start( void );
    void Stop( void );
    bool Run( void );
};

#endif /* __MAPRENDERER_H__ */
