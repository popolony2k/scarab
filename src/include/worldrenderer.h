/*
 * worldrenderer.h
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#ifndef __WORLDRENDERER_H__
#define __WORLDRENDERER_H__

#include <raylib.h>
#include <string>
#include <queue>
#include <vector>
#include <array>
#include "iworldlistener.h"


/*
 * This is already defined when using baselibrary (maybe if this engine
 * integrates baselibrary, or just start using it, consider removing
 * this definition by using it's definition already defined in include
 * <corefoundation/callable.h>.
 */
#ifndef CALL_MEMBER_FN
/*
 * Macro definition for calling member classes functions followed
 * by parameters.
 * Eg:
 * CALL_MEMBER_FN( *this, pfnFunc)( 1, 2 );
 */
#define CALL_MEMBER_FN( obj, ptr_fn_member )  ( ( obj ).*( ptr_fn_member ) )
#endif /* CALL_MEMBER_FN */

/*
 * Maximum key array.
 */
#define MAX_KEYS          400

/**
 * Primitive structures definition.
 */
typedef Rectangle  stViewport;
typedef Vector2    stVector;

/**
 * View port control mode (active and reactive)
 * Active, the view port reacts to a single key pressing continuously;
 * Reactive, the view port reacts only for each key pressing;
 */
enum ViewControlMode  {
    VIEW_CONTROL_MODE_ACTIVE,
    VIEW_CONTROL_MODE_REACTIVE
};

class WorldRenderer : public IWorld  {

    /**
     * Tile animation information.
     */
    struct __stTileAnimInfo  {
        int64_t      nMillis;
        uint16_t     nCounter;
        tmx_tile     *pNextTile;
    };
    typedef std :: deque<__stTileAnimInfo*> __AnimInfoList;
    typedef std :: vector<float> ZoomFactorList;
    typedef std :: pair<unsigned, unsigned> ZoomBorderLimits;
    typedef std :: deque<IWorldListener*> WorldListenerList;


    typedef void ( WorldRenderer :: *KEY_EVENT_HANDLER )( void );
    typedef std :: array<KEY_EVENT_HANDLER, MAX_KEYS> KeyBindingEventHandler;

    WorldListenerList          m_WorldListenerList;
    KeyBindingEventHandler     m_UserEventHandlers;
    stVector                   m_CameraPos;
    ViewControlMode            m_ViewControlMode;
    uint16_t                   m_nMapWidth;
    uint16_t                   m_nMapHeight;
    int                        m_nScrollStepWidth;
    int                        m_nScrollStepHeight;
    float                      m_fWindowWidth;
    float                      m_fWindowHeight;
    int                        m_nTargetFps;
    float                      m_fZoomFactor;
    ZoomFactorList             m_vZoomFactorList;
    unsigned                   m_nCurrentZoomPos;
    unsigned                   m_nPreferredZoomPos;
    ZoomBorderLimits           m_ZoomBorderLimits;
    __AnimInfoList             m_AnimInfoList;
    stViewport                 m_Viewport;
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

    // TmxLib miscellaneous
    tmx_layer* GetLayer( int nLayerId );
    tmx_layer* GetLayer( const char *szLayerName );

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
                         float& fHeight );
    void SetPixel( int nCoordX, int nCoordY, Color color );
    void MidPointEllipse( double fCoordX,
                          double fCoordY,
                          double fRadiusX,
                          double fRadiusY,
                          Color color );
    void LineBresenham( int nX0,
                        int nY0,
                        int nX1,
                        int nY1,
                        Color color );

    // Engine primitives
    void DrawPolyline( double fOffset_x,
                       double fOffset_y,
                       double **fPoints,
                       int nPointsCount,
                       Color color );
    void DrawPolygon( double fOffset_x,
                      double fOffset_y,
                      double **fPoints,
                      int nPointsCount,
                      Color color );
    void DrawRectangle( double offset_x,
                        double offset_y,
                        double width,
                        double height,
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
    void DrawObjects( tmx_layer *pLayer );
    void DrawImageLayer( tmx_layer *pLayer );
    void DrawLayer( tmx_map *pMap, tmx_layer *pLayer );
    void DrawAllLayers( tmx_map *pMap, tmx_layer *pLayer );
    void RenderMap( void );
    bool UnloadMap( void );

    // General engine handlers
    void InitializeZoomEngine( void );
    void InitializeControllerHandlers( void );

    // User interaction handlers
    void HandleUserInput( void );
    void HandleUserUpdate( void );

    // Internal layer handlers
    void CopyLayerToTmx( tmx_layer *pTmxLayer, stLayer& layer );
    void CopyTmxToLayer( stLayer& layer, tmx_layer *pTmxLayer );


    public:

    WorldRenderer( float fWidth,
                   float fHeight,
                   const char* szTitle,
                   int nTargetFps = -1 );
    virtual ~WorldRenderer( void );

    // World user listener interaction
    void AddWorldListener( IWorldListener *pListener );
    void RemoveWorldListener( IWorldListener *pListener );

    // Window behavior
    void SetExitKey( KeyboardKey key );
    void SetWindowResizeable( bool bResizeable );
    void SetClearBackground( bool bStatus );
    void SetDrawFPS( bool bDrawFPS );

    // View port control
    void SetViewControlMode( ViewControlMode mode );
    void SetViewport( stViewport viewport );
    void SetScrollStepSize( int nStepWidth, int nStepHeight );

    // User input handling
     int GetKeyPressed( void );

    // Camera management
    void SetEnableUserZoom( bool bEnabled );
    void SetMinZoom( unsigned nMinPos );
    void SetMaxZoom( unsigned nMaxPos );
    void SetPreferredZoom( unsigned nZoomPos );
    void SetZoom( unsigned nZoomPos );
    void ResetZoom( void );
    void ZoomIn( void );
    void ZoomOut( void );
    void ResetCamera( void );
    void MoveCameraUp( void );
    void MoveCameraDown( void );
    void MoveCameraLeft( void );
    void MoveCameraRight( void );

    // Layer management
    bool SetLayer( int nLayerId, stLayer& layer );
    bool SetLayer( const char *szLayerName, stLayer &layer );
    bool GetLayer( int nLayerId, stLayer &layer );
    bool GetLayer( const char *szLayerName, stLayer &layer );

    // Tile management
    bool GetTile( const stMatrixPosition& pos,
                  const stLayer& layer,
                  stTile& tile );

    // Coordinate conversion
    bool WorldToTileMatrix( const stCoordinate2D& coord,
                            stMatrixPosition& pos );

    // Map management
    void SetMapFile( const char *szTxMapFile );
    bool GetMapInfo( stMapInfo& mapInfo );

    // Renderer flow control.
    bool Start( void );
    void Stop( void );
    bool Run( void );
};

#endif /* __WORLDRENDERER_H__ */
