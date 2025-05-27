#pragma once

// little include hack
#include "SDK/Valve/UtlBuffer.hpp"
#include "SDK/Valve/UtlMemory.hpp"
#include "SDK/Valve/UtlVector.hpp"

#include "SDK/sdk.hpp"
#include "SDK/Valve/recv_swap.hpp"
#include <windows.h>
#include <shlobj.h>

extern bool on_cfg_load_gloves;
extern bool on_cfg_load_knives;

//#define MENU_DEV

class IClientMode {
public:
	virtual ~IClientMode( ) { }
	virtual int ClientModeCSNormal( void * ) = 0;
	virtual void Init( ) = 0;
	virtual void InitViewport( ) = 0;
	virtual void Shutdown( ) = 0;
	virtual void Enable( ) = 0;
	virtual void Disable( ) = 0;
	virtual void Layout( ) = 0;
	virtual IPanel *GetViewport( ) = 0;
	virtual void *GetViewportAnimationController( ) = 0;
	virtual void ProcessInput( bool bActive ) = 0;
	virtual bool ShouldDrawDetailObjects( ) = 0;
	virtual bool ShouldDrawEntity( C_BaseEntity *pEnt ) = 0;
	virtual bool ShouldDrawLocalPlayer( C_BaseEntity *pPlayer ) = 0;
	virtual bool ShouldDrawParticles( ) = 0;
	virtual bool ShouldDrawFog( void ) = 0;
	virtual void OverrideView( CViewSetup *pSetup ) = 0;
	virtual int KeyInput( int down, int keynum, const char *pszCurrentBinding ) = 0;
	virtual void StartMessageMode( int iMessageModeType ) = 0;
	virtual IPanel *GetMessagePanel( ) = 0;
	virtual void OverrideMouseInput( float *x, float *y ) = 0;
	virtual bool CreateMove( float flInputSampleTime, void *usercmd ) = 0;
	virtual void LevelInit( const char *newmap ) = 0;
	virtual void LevelShutdown( void ) = 0;
};


struct ServerClass {
	const char *m_pNetworkName;
	void **m_pTable;
	ServerClass *m_pNext;
	int	m_ClassID;
	int	m_InstanceBaselineIndex;
};

struct IServer {
	ServerClass *GetAllClasses( ) {
		using Fn = ServerClass * ( __thiscall * )( void * );
		return Memory::VCall<Fn>( this, 10 )( this );
	}
};

struct dlight_t {
	struct ColorRGBExp32 {
		byte r, g, b;
		signed char exponent;
	};

	int flags;
	Vector origin;
	float radius;
	ColorRGBExp32 color;
	float die; // stop lighting after this time
	float decay; // drop this each second
	float minLight; // don't add when contributing less
	int key;
	int style; // lightstyle

	// For spotlights. Use m_OuterAngle == 0 for point lights
	Vector m_direction; // center of the light cone
	float m_innerAngle;
	float m_outerAngle;

	// If this ptr is set, the dlight will only affect this particular client renderable 
	const IClientRenderable *m_pExclusiveLightReceiver;

	dlight_t( ) : m_pExclusiveLightReceiver( NULL ) { }
};

class IVEffects {
public:
	dlight_t *CL_AllocDlight( int key ) {
		typedef dlight_t *( __thiscall *CL_AllocDlight )( PVOID, int );
		return Memory::VCall<CL_AllocDlight>( this, 4 )( this, key );
	}
};

extern Encrypted_t<IBaseClientDLL> g_pClient;
extern Encrypted_t<IClientEntityList> g_pEntityList;
extern Encrypted_t<IGameMovement> g_pGameMovement;
extern Encrypted_t<IPrediction> g_pPrediction;
extern Encrypted_t<IMoveHelper> g_pMoveHelper;
extern Encrypted_t<IInput> g_pInput;
extern Encrypted_t< CGlobalVars >  g_pGlobalVars;
extern Encrypted_t<ISurface> g_pSurface;
extern Encrypted_t<IVEngineClient> g_pEngine;
extern Encrypted_t<IClientMode> g_pClientMode;
extern Encrypted_t<ICVar> g_pCVar;
extern Encrypted_t<IPanel> g_pPanel;
extern Encrypted_t<IGameEventManager> g_pGameEvent;
extern Encrypted_t<IVModelRender> g_pModelRender;
extern Encrypted_t<IMaterialSystem> g_pMaterialSystem;
extern Encrypted_t<ISteamClient> g_pSteamClient;
extern Encrypted_t<ISteamGameCoordinator> g_pSteamGameCoordinator;
extern Encrypted_t<ISteamMatchmaking> g_pSteamMatchmaking;
extern Encrypted_t<ISteamUser> g_pSteamUser;
extern Encrypted_t<ISteamFriends> g_pSteamFriends;
extern Encrypted_t<IPhysicsSurfaceProps> g_pPhysicsSurfaceProps;
extern Encrypted_t<IEngineTrace> g_pEngineTrace;
extern Encrypted_t<CGlowObjectManager> g_pGlowObjectManager;
extern Encrypted_t<IVModelInfo> g_pModelInfo;
extern Encrypted_t< CClientState >  g_pClientState;
extern Encrypted_t<IVDebugOverlay> g_pDebugOverlay;
extern Encrypted_t<IEngineSound> g_pEngineSound;
extern Encrypted_t<IMemAlloc> g_pMemAlloc;
extern Encrypted_t<IViewRenderBeams> g_pViewRenderBeams;
extern Encrypted_t<ILocalize> g_pLocalize;
extern Encrypted_t<IStudioRender> g_pStudioRender;
extern Encrypted_t<ICenterPrint> g_pCenterPrint;
extern Encrypted_t<IVRenderView> g_pRenderView;
extern Encrypted_t<IClientLeafSystem> g_pClientLeafSystem;
extern Encrypted_t<IMDLCache> g_pMDLCache;
extern Encrypted_t<IViewRender> g_pViewRender;
extern Encrypted_t<IInputSystem> g_pInputSystem;
extern Encrypted_t<INetGraphPanel> g_pNetGraphPanel;
extern Encrypted_t<CCSGameRules> g_pGameRules;
extern Encrypted_t<CFontManager> g_pFontManager;
extern Encrypted_t<IWeaponSystem> g_pWeaponSystem;
extern Encrypted_t<CSPlayerResource *> g_pPlayerResource;
extern Encrypted_t<IVEffects> g_pEffects;
extern Encrypted_t<CPhysicsCollision> g_pVPhysicsCollision;
extern Encrypted_t<IServer> g_pServer;
extern Encrypted_t<C_TEEffectDispatch> g_pTE_EffectDispatch;
extern Encrypted_t<IEngineVGui> g_pEngineVGui;
extern Encrypted_t<INetworkStringTableContainer> g_pNetworkStringTableContainer;
extern Encrypted_t<SFHudDeathNoticeAndBotStatus> g_pDeathNotices;
extern Encrypted_t<CHud> g_pHud;

// netvar proxies
extern RecvPropHook::Shared m_flVelocityModifierSwap;

namespace Interfaces {
	extern WNDPROC oldWindowProc;
	extern HWND hWindow;

	bool Create( void *reserved );
	void Destroy( );

#ifdef DEV
	void *CreateInterface( const std::string &image_name, const std::string &name );
#else
	void *CreateInterface( const std::pair< uint32_t, std::string> hash );
#endif

}


#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

__forceinline std::string GetHitsoundsDirectory( ) {
	namespace fs = std::experimental::filesystem;
	fs::path full_path( fs::current_path( ) );

	std::wstring cheat_folder = full_path.wstring( ) + XorStr( L"\\naphack" );
	std::wstring hitsounds_folder = cheat_folder + XorStr( L"\\hitsounds\\" );
	return std::string( hitsounds_folder.begin( ), hitsounds_folder.end( ) );
}

__forceinline std::string GetScriptsDirectory( ) {
	namespace fs = std::experimental::filesystem;
	fs::path full_path( fs::current_path( ) );

	std::wstring cheat_folder = full_path.wstring( ) + XorStr( L"\\naphack" );
	std::wstring scripts_folder = cheat_folder + XorStr( L"\\scripts\\" );
	return std::string( scripts_folder.begin( ), scripts_folder.end( ) );
}