/**<main.cpp>
 * Scarab main entry point source file - builds the window/engine host and
 * runs whichever game argv[1] points at (Caravellius today, but nothing
 * here is Caravellius-specific).
 *
 *  Created on: Jun 30, 2021
 *      Author: popolony2k
 */
#include <cstdio>
#include <string>
#include <filesystem>
#include <CLI/CLI.hpp>
#include "main.h"
#include "enginehost.h"
#include "renderer/tilemaprenderer.h"
#include "engines/enginefactory.h"
#include "filesystem/filesystemfactory.h"


int main( int argc, char **argv ) {

    /*
     * Entry point argument parsing (Phase 12, archive-distribution).
     * The positional "entry" argument is one of:
     *   - a .lua script, run directly (unchanged from before this phase);
     *   - a .json project file, whose "main_script" field is resolved
     *     (unchanged from before this phase);
     *   - a .zip archive bundling a whole project (project.json + every
     *     resource it references) into one file.
     * --entry/-e names the actual entry point when that's not simply
     * "entry itself" - told apart by it's own extension, same as entry:
     *   - inside a .zip, this is where the real entry point lives (default,
     *     if omitted: "project.json" at the archive's own root) - a .lua
     *     path here is used directly (no project.json indirection at all),
     *     anything else is read as a .json project file, same as the loose
     *     .json case below;
     *   - as an outright override of *which* .json to use, when entry
     *     itself is a .json project file (a plain .lua entry ignores this -
     *     EngineHost::ResolveEntryScript reports that combination as a
     *     usage error, since there's no project-file concept to override).
     * See EngineHost::ResolveEntryScript for how these are actually
     * resolved - this is parsing only, no filesystem access happens here.
     */
    CLI :: App  app{ "Scarab - a thin, game-agnostic 2D engine host built on sunlight" };

    std :: string  strEntryPath;
    std :: string  strEntryOverride;

    app.add_option( "path", strEntryPath,
                    "Entry point: a .lua script, a .json project file, or a .zip bundle" )
       ->required()
       ->check( CLI :: ExistingFile );

    app.add_option( "-e,--entry", strEntryOverride,
                    "Actual entry point location - inside the .zip when entry is an archive "
                    "(a .lua path there runs directly, anything else is read as project.json "
                    "- default: project.json at its root), or overriding entry itself when "
                    "it's already a .json project file" );

    CLI11_PARSE( app, argc, argv );

    SunLight :: Renderer :: TileMapRenderer  renderer( DISPLAY_W,
                                                       DISPLAY_H,
                                                       APP_NAME,
                                                       FRAMES_PER_SECOND,
                                                       false );
    Scarab :: Host :: EngineHost                engineHost( &renderer, &renderer, strEntryPath, strEntryOverride );
    SunLight :: TileMap :: stDimension2D       viewport;

    /*
     * Mounts the executable's own real directory (Mount()'s source
     * argument, safe in whatever native OS format GetApplicationDirectory()
     * returns) at that same directory's own IFileSystem::ToVirtualPath()
     * equivalent - NOT at the real path itself. PHYSFS_mount()'s
     * destination/mountPoint argument must already be a legal virtual path
     * (see IFileSystem's own "virtual path" contract, ifilesystem.h); a raw
     * Windows path's drive-letter ':' trips the underlying PhysFS backend's
     * legality check outright, a real bug found live on Windows (never on
     * Mac/Linux - see ToVirtualPath's own doc comment for the story). Every
     * absolute path this game already constructs (APP_DIR-relative - the
     * BASE_PATH convention for resources/, or the separate SRC_PATH one for
     * src/, main.lua's own globals) resolves through SunLight::FileSystem
     * unchanged as a result, since LuaEngine::Init()
     * exposes this exact same ToVirtualPath() conversion as the Lua APP_DIR
     * global - no other path-construction code anywhere needs to know any
     * of this exists. This is the loose-directory case; when entry is a
     * .zip, EngineHost::ResolveEntryScript mounts that archive at this SAME
     * virtual mount point, searched first (bAppendToPath=false) - so a game
     * running from an archive still resolves every APP_DIR-anchored path
     * unchanged, with this loose mount only ever acting as a fallback for
     * anything the archive doesn't itself contain (there shouldn't be
     * anything, in a well-formed bundle).
     */
    std :: string  strAppDir = SunLight :: Engines :: EngineFactory :: GetEngine().GetApplicationDirectory();

    SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Init( argv[0] );
    SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Mount( strAppDir,
                                                                          SunLight :: FileSystem :: IFileSystem :: ToVirtualPath( strAppDir ),
                                                                          true );

    /*
     * Also mounts the process's own current working directory at the
     * virtual root, separately from strAppDir above - a real gap found
     * live building Phase 12's archive support: EngineHost::
     * ResolveEntryScript's .json branch deliberately resolves
     * "main_script" *relative to the project file itself*, not APP_DIR
     * (see its own doc comment - a project should be relocatable as a
     * whole). Given a bare relative entry path (the documented, normal
     * invocation - "./scarab project.json" from the build output
     * directory), that resolution produces a bare CWD-relative path (eg.
     * "src/main.lua"), not an APP_DIR-anchored one - the
     * strAppDir mount above only ever answers virtual paths that start
     * with that exact absolute prefix, so a bare relative path matches
     * neither mount, and every load of the game's own very first script
     * failed outright once LuaEngine::RunFile started routing through
     * SunLight::FileSystem too. Mounting CWD at "/" (matching exactly
     * what SunLight::FileSystem's own lazy fallback would have done on
     * its own, had this Mount() call not already pre-empted it - see
     * PhysFsFileSystem::EnsureReady's own doc comment) restores that
     * pre-Phase-12 "a relative path just resolves against CWD" behavior
     * plain fopen() always gave, for every relative path anywhere in this
     * codebase, not only this one call site.
     */
    SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Mount( std :: filesystem :: current_path().string(), "/", true );


    renderer.SetScrollStepSize( W_SCROLL_STEP_SIZE, H_SCROLL_STEP_SIZE );
    renderer.SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );
    renderer.AddTileMapListener( &engineHost );

    viewport.pos.x = VIEWPORT_POS_X;
    viewport.pos.y = VIEWPORT_POS_Y;
    viewport.size.nWidth  = VIEWPORT_WIDTH;
    viewport.size.nHeight = VIEWPORT_HEIGHT;

    renderer.GetViewport().SetZoom( DEFAULT_ZOOM_SCALE_POS );
    renderer.GetViewport().SetDimension2D( viewport );
    renderer.SetDrawFPS( false );
    renderer.SetWindowResizeable( true );
    renderer.Start();
    renderer.Run();
    renderer.Stop();

    return EXIT_SUCCESS;
}
