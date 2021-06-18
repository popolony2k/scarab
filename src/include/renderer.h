/*
 * renderer.h
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <tmx.h>
#include <raylib.h>
#include <string>
#include <queue>


class Renderer  {

    /**
     * Tile animation information.
     */
    struct __stTileAnimInfo  {
        int64_t      nMillis;
        int          nCounter;
        tmx_tile     *pNextTile;
    };
    typedef std ::deque<__stTileAnimInfo*> __AnimInfoList;

    int              m_nWidth;
    int              m_nHeight;
    int              m_nTargetFps;
    float            m_fLineThickness;
    __AnimInfoList   m_AnimInfoList;
    std :: string    m_strTitle;
    std :: string    m_strTmxMapFile;
    tmx_map          *m_pTmxMap;
    bool             m_bIsStarted;
    static bool      m_bInitialized;


    static void* TextureLoaderCallback( const char *szPath );
    static void TextureFreeCallback( void *pTexture );

    Color IntToColor( int color );
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
    void DrawObjects( tmx_object_group *objgr );
    void DrawImageLayer( tmx_image *pImage );
    void DrawTile( void *pImage,
                   unsigned int sx,
                   unsigned int sy,
                   unsigned int sw,
                   unsigned int sh,
                   unsigned int dx,
                   unsigned int dy,
                   float opacity,
                   unsigned int flags );
    void DrawLayer( tmx_map *pMap, tmx_layer *pLayer );
    void DrawAllLayers( tmx_map *pMap, tmx_layer *pLayers );
    void RenderMap( void );
    void ReleaseLayer( void );


    public:

    Renderer( int nWidth,
              int nHeight,
              const char* szTitle,
              const char *szTmxMapFile,
              int nTargetFps = -1);
    ~Renderer( void );

    void SetLineThickness( float fLineThickness );
    float GetLineThickness( void );

    bool Start( void );
    void Stop( void );
    bool Run( void );
};

#endif /* __RENDERER_H__ */
