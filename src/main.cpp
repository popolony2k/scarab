/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/**<main.cpp>
 * Scarab main entry point source file - builds the window/engine host and
 * runs whichever game argv[1] points at. Nothing here is specific to any
 * one game.
 */
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <filesystem>
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include "main.h"
#include "enginehost.h"
#include "lua/luacryptoapi.h"
#include "lua/luaengine.h"
#include "renderer/tilemaprenderer.h"
#include "engines/enginefactory.h"
#include "filesystem/filesystemfactory.h"


/**
 * @brief Cross-platform single-variable environment setter -
 * SCARAB_PACK_SOURCE_DIR/SCARAB_PACK_OUTPUT (see the --pack handling in
 * main() below) are handed to tools/pack.lua this way rather than as a
 * new mount/Lua-global plumbed through EngineHost/LuaEngine, since
 * that's the one thing Lua's own standard library (os.getenv) already
 * reads with zero engine changes needed on the Lua side. POSIX's
 * setenv() and MSVC's _putenv_s() aren't unified anywhere in the C/C++
 * standard library, hence the #ifdef - matches this project's existing
 * precedent of small, explicit platform ifdefs for OS APIs (never
 * Autotools/etc - see CLAUDE.md's "Build" section).
 */
static void SetPackEnvVar( const char *szName, const std :: string &strValue )  {
#ifdef _WIN32
    _putenv_s( szName, strValue.c_str() );
#else
    setenv( szName, strValue.c_str(), 1 );
#endif
}


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

    // SCARAB_VERSION/SCARAB_SUNLIGHT_VERSION are compiler-defined string
    // literals (target_compile_definitions, root CMakeLists.txt) - adjacent
    // string literal concatenation joins them into one literal at compile
    // time, so this always reflects the exact sunlight tag actually fetched,
    // not a hand-copied value that could drift out of sync.
    //
    // LUA_RELEASE (eg. "Lua 5.4.7") is different - not a Scarab-owned
    // define at all, but Lua's own compile-time macro (lua.h, transitively
    // included via lua/luaengine.h above), read directly rather than
    // hand-copied into a new CMake variable for exactly the same
    // drift-avoidance reason. This matters concretely here: the default
    // Lua backend (walterschell/Lua, GIT_TAG master) tracks whatever Lua
    // release that fork currently packages, not a version pinned anywhere
    // in this repo - confirmed live to actually be 5.4.7 despite the
    // *alternate* backend (SCARAB_USE_OFFICIAL_LUA_FTP, the official FTP
    // tarball) being hardcoded to 5.4.6 in its own URL. Reading LUA_RELEASE
    // straight from whichever lua.h actually got compiled against means
    // this always matches reality for either backend automatically, with
    // no separate tracking needed for the FTP path either.
    app.set_version_flag( "--version,-v", "scarab v" SCARAB_VERSION " (sunlight v" SCARAB_SUNLIGHT_VERSION ", " LUA_RELEASE ")" );

    std :: string  strEntryPath;
    std :: string  strEntryOverride;
    std :: string  strPackConfigPath;

    CLI :: Option  *pPathOption = app.add_option( "path", strEntryPath,
                    "Entry point: a .lua script, a .json project file, or a .zip bundle" )
       ->check( CLI :: ExistingFile );

    CLI :: Option  *pEntryOption = app.add_option( "-e,--entry", strEntryOverride,
                    "Actual entry point location - inside the .zip when entry is an archive "
                    "(a .lua path there runs directly, anything else is read as project.json "
                    "- default: project.json at its root), or overriding entry itself when "
                    "it's already a .json project file" );

    /*
     * --pack is sugar for running the (compiled-in) content-encryption
     * packaging tool - tools/pack.lua, copied next to this executable by
     * scarab_copy_binaries (root CMakeLists.txt) - without needing to
     * `cd` into this repo/build's own directory first, or hand-author a
     * pack-config.json there (see tools/README.md's own two-invocation-
     * styles note). No positional entry point applies in this mode, so
     * it's mutually exclusive with both "path" and "--entry" - combining
     * them is a usage error CLI11 itself reports (via excludes()),
     * matching the "a bad argument combination fails before any window
     * opens" philosophy ResolveEntryScript's own --entry/.lua check
     * already follows.
     */
    CLI :: Option  *pPackOption = app.add_option( "--pack", strPackConfigPath,
                    "Encrypt + zip a game's own source directory (see README.md's "
                    "own \"Content encryption\" section) and exit - no window opens. "
                    "<config.json>: {\"source_dir\": \"...\", \"output\": \"...\"} - "
                    "relative paths inside it resolve against the config file's own "
                    "directory, not the current working directory." )
       ->check( CLI :: ExistingFile );

    pPathOption->excludes( pPackOption );
    pPackOption->excludes( pPathOption );
    pEntryOption->excludes( pPackOption );
    pPackOption->excludes( pEntryOption );

    CLI11_PARSE( app, argc, argv );

    if( strEntryPath.empty() && strPackConfigPath.empty() )  {
        fprintf( stderr, "scarab: either the entry-point positional or --pack <config.json> is required "
            "(see --help).\n" );
        return EXIT_FAILURE;
    }

    /*
     * SCARAB_CONTENT_KEY_IS_SHARED is only ever 1 on an official
     * release.yml-built binary (see CMakeLists.txt's own doc comment) -
     * printed unconditionally, every invocation (both --pack and the
     * normal windowed path reach this same line), so the "as-is,
     * educational only" framing in README.md's own "Content encryption"
     * section is restated live at the point of use, not just in a doc
     * someone could skip past. A build with no key configured, or a
     * real private one someone set themselves, never sets this - stays
     * silent either way.
     */
    if( SCARAB_CONTENT_KEY_IS_SHARED )  {
        fprintf( stderr, "WARNING: this scarab build was compiled with a SHARED, PUBLIC content-encryption "
            "key (SCARAB_CONTENT_KEY) - the same key every official release of this version ships with, not "
            "a private one. Anyone can extract it from this exact binary. As-is, educational use only - never "
            "rely on it for real content protection. Build your own copy from source with your own "
            "SCARAB_CONTENT_KEY for production/professional use - see README.md's own \"Content encryption\" "
            "section.\n" );
    }

    /*
     * strAppDir/strVirtualAppDir computed early (moved up from where this
     * exact GetApplicationDirectory() call used to sit, right before the
     * APP_DIR Mount() below) - --pack needs the virtual (ToVirtualPath()-
     * converted) form *before* EngineHost is constructed, to synthesize
     * an APP_DIR-anchored strEntryPath pointing at tools/pack.lua. See
     * that Mount() call's own doc comment below for why the virtual form,
     * not the raw real-OS-path one, is what actually resolves through
     * SunLight::FileSystem.
     */
    std :: string  strAppDir        = SunLight :: Engines :: EngineFactory :: GetEngine().GetApplicationDirectory();
    std :: string  strVirtualAppDir = SunLight :: FileSystem :: IFileSystem :: ToVirtualPath( strAppDir );

    if( !strPackConfigPath.empty() )  {

        /*
         * Read directly via std::ifstream/nlohmann::json - a real,
         * arbitrary native OS path (wherever the user's own config.json
         * actually lives), same as ResolveEntryScript's own loose
         * project.json branch (enginehost.cpp) - deliberately NOT routed
         * through SunLight::FileSystem, since nothing has been
         * mounted/resolved yet at this point in main(), and this config
         * is native-path-shaped by convention anyway (matching
         * LuaPackApi's own primitives - see docs/content-encryption-
         * plan.md).
         */
        std :: filesystem :: path  packConfigPath( strPackConfigPath );
        std :: ifstream            packConfigFile( packConfigPath );

        if( !packConfigFile.is_open() )  {
            fprintf( stderr, "scarab: --pack: could not open config file => %s\n", strPackConfigPath.c_str() );
            return EXIT_FAILURE;
        }

        nlohmann :: json  packConfigData = nlohmann :: json :: parse( packConfigFile, nullptr, false );

        if( packConfigData.is_discarded() || !packConfigData.contains( "source_dir" ) || !packConfigData.contains( "output" ) )  {
            fprintf( stderr, "scarab: --pack: malformed config (needs \"source_dir\" and \"output\") => %s\n",
                strPackConfigPath.c_str() );
            return EXIT_FAILURE;
        }

        std :: filesystem :: path  packConfigDir = std :: filesystem :: absolute( packConfigPath ).parent_path();

        // Relative source_dir/output resolve against the config file's own
        // directory (relocatable, like project.json's main_script - not
        // against whatever the caller's own CWD happens to be); an
        // already-absolute path is used unchanged.
        auto ResolvePackPath = [ &packConfigDir ]( const std :: string &strRaw ) -> std :: string  {
            std :: filesystem :: path  rawPath( strRaw );

            return ( rawPath.is_absolute() ? rawPath : ( packConfigDir / rawPath ) ).string();
        };

        SetPackEnvVar( "SCARAB_PACK_SOURCE_DIR", ResolvePackPath( packConfigData[ "source_dir" ].get<std :: string>() ) );
        SetPackEnvVar( "SCARAB_PACK_OUTPUT",     ResolvePackPath( packConfigData[ "output" ].get<std :: string>() ) );

        // APP_DIR-anchored, exactly matching what LuaEngine::Init() itself
        // later exposes to Lua as APP_DIR - see the identity Mount() call
        // just below, which is what actually makes this resolvable.
        strEntryPath = strVirtualAppDir + "tools/pack.lua";
    }

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
     * anything, in a well-formed bundle). Done before constructing
     * TileMapRenderer/EngineHost below (moved up from where this call used
     * to sit) - mounting itself only needs strAppDir/strVirtualAppDir,
     * already computed above, so it has no actual ordering dependency on
     * either of those objects existing yet, and --pack mode below needs
     * these same mounts in place before it can resolve tools/pack.lua's
     * own path, without ever constructing either object at all.
     */
    SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Init( argv[0] );

    /*
     * Content encryption (see README.md's own "Content encryption"
     * section) - registers LuaCryptoApi::TryDecryptBytes as sunlight's
     * own generic IFileSystem::SetReadFilter hook (sunlight v0.18.0+),
     * so every read sunlight itself performs - textures (RaylibEngine),
     * sounds (RayLibSound), tilemaps (TileMapRenderer::LoadMap) -
     * transparently decrypts content packed by tools/pack.lua the exact
     * same way every read path Scarab's own code owns already does
     * (LuaEngine::RunFile/LuaJsonApi::LoadJson/LuaFileSystemApi::DoFile/
     * EngineHost::ResolveEntryScript's own .zip-branch project.json
     * read). Registered before
     * any Mount() call below so it's in effect for every read from the
     * very first one; sunlight itself has no crypto dependency or
     * awareness of what this callback does (see IFileSystem::
     * SetReadFilter's own doc comment) - unset, this would be a pure
     * no-op, byte-identical to today's behavior. TryDecryptBytes already
     * silently falls back to "not encrypted, use the raw bytes" (returns
     * false) when this build has no SCARAB_CONTENT_KEY configured, so
     * this registration is unconditional - it costs nothing for a build
     * with no key at all.
     */
    SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().SetReadFilter(
        []( const std :: vector<unsigned char> &in, std :: vector<unsigned char> &out ) {
            return Scarab :: Engine :: Lua :: LuaCryptoApi :: TryDecryptBytes( in, out );
        } );

    SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Mount( strAppDir,
                                                                          strVirtualAppDir,
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

    if( !strPackConfigPath.empty() )  {
        /*
         * Genuinely headless --pack: no TileMapRenderer/EngineHost is ever
         * constructed, so no window ever opens - there was never an actual
         * requirement for one. tools/pack.lua only calls primitives
         * LuaEngine's own constructor already registers unconditionally
         * (LuaScriptingApi/LuaJsonApi/LuaCryptoApi/LuaPackApi/
         * LuaFileSystemApi - see LuaEngine::RegisterCalls), all of which
         * need no ITileMap/SoundManager/SpritePool at all; only the
         * primitives Init() adds (camera/input/tilemap/sound/sprite/
         * collision/app) need those, and tools/pack.lua calls none of
         * them (app_set_name is nil-guarded there for exactly this
         * reason). ScriptProcessor's own constructor takes no arguments
         * and touches nothing window-related either - it exists here
         * purely to satisfy LuaEngine's constructor signature; nothing
         * ever drains its queue (no per-frame loop exists to), so
         * tools/pack.lua's sp_wait(1) call is a harmless no-op in this
         * mode, not a requirement the way it is for a normal windowed
         * entry script (see ScriptProcessor::Compile()'s own "queue
         * must not be empty" check, in EngineHost::LoadLuaScript, which
         * this path never calls at all).
         */
        SunLight :: Scripting :: ScriptProcessor  packScriptProcessor;
        Scarab :: Lua :: LuaEngine                packLuaEngine( &packScriptProcessor );

        bool  bPackOk = packLuaEngine.RunFile( strEntryPath );

        return bPackOk ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    SunLight :: Renderer :: TileMapRenderer  renderer( DISPLAY_W,
                                                       DISPLAY_H,
                                                       APP_NAME,
                                                       FRAMES_PER_SECOND,
                                                       false );
    Scarab :: Host :: EngineHost                engineHost( &renderer, &renderer, strEntryPath, strEntryOverride );
    SunLight :: TileMap :: stDimension2D       viewport;

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
