#include "pandora.hpp"

#include "Hooking/Hooked.hpp"
#include "Utils/InputSys.hpp"

#include <optional>
#include <signal.h>
#include "SDK/Classes/PropManager.hpp"
#include "SDK/Displacement.hpp"
//#include "Libraries/BASS/bass.h"
#include "Libraries/BASS/API.h"

#include <Virtualizer/C/VirtualizerSDK.h>

#include "SDK/Classes/Player.hpp"
#include "Features/Miscellaneous/Miscellaneous.hpp"

#include "Features/Miscellaneous/GameEvent.hpp"

#include "Renderer/Render.hpp"
#include "Features/Miscellaneous/SkinChanger.hpp"
#include "Features/Miscellaneous/KitParser.hpp"
#include "Features/Visuals/Glow.hpp"

#include "Hooking/hooker.hpp"

#include "Features/Visuals/Models.hpp"

#include "Features/Rage/EnginePrediction.hpp"
#include "Features/Rage/TickbaseShift.hpp"
#include "Loader/Exports.h"

#include <fstream>

#include "Utils/LogSystem.hpp"
#include "Features/Visuals/Visuals.hpp"

#include "Features/Miscellaneous/Communication.hpp"

#include "Features/Visuals/GSInterpreter.hpp"
#include "Utils/Threading/threading.h"

static Semaphore dispatchSem;
static SharedMutex smtx;

using ThreadIDFn = int( _cdecl * )( );

ThreadIDFn AllocateThreadID2;

int AllocateThreadIDWrapper2( ) {
	return AllocateThreadID2( );
}

template<typename T, T &Fn>
static void AllThreadsStub( void * ) {
	dispatchSem.Post( );
	smtx.rlock( );
	smtx.runlock( );
	Fn( );
}

template<typename T, T &Fn>
static void DispatchToAllThreads( void *data ) {
	smtx.wlock( );

	for( size_t i = 0; i < Threading::numThreads; i++ )
		Threading::QueueJobRef( AllThreadsStub<T, Fn>, data );

	for( size_t i = 0; i < Threading::numThreads; i++ )
		dispatchSem.Wait( );

	smtx.wunlock( );

	Threading::FinishQueue( false );
}

bool __fastcall Hooked::CheckAchievementsEnabled( void *ecx, void *edx ) {
	return true;
}

using CHudElement__ShouldDrawFn = bool( __thiscall * )( void * );
CHudElement__ShouldDrawFn oCHudElement__ShouldDraw;

// client.dll E8 ? ? ? ? 84 C0 74 F3
bool __fastcall CHudElement__ShouldDraw( void *ecx, void *edx ) {
	return false;
}


CHudElement__ShouldDrawFn oCanUnduck;


// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/shared/cstrike15/cs_gamemovement.cpp#L850
// ~superior valve code fixed~
bool __fastcall CanUnduck( void *ecx, void *edx ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return oCanUnduck( ecx );

	// Can't unduck if we are planting the bomb.
	if( pLocal->m_bDuckOverride( ) )
		return false;

	// Can always unduck if we are no-clipping
	if( pLocal->m_MoveType( ) == MOVETYPE_NOCLIP )
		return true;

	if( pLocal->m_hGroundEntity( ).Get( ) == nullptr )
		return oCanUnduck( ecx );

	static ConVar *mp_solid_teammates = g_pCVar->FindVar( XorStr( "mp_solid_teammates" ) );
	if( mp_solid_teammates ) {
		if( mp_solid_teammates->GetInt( ) == 1 )
			return oCanUnduck( ecx );
	}

	const Vector VEC_HULL_MIN( -16, -16, 0 );
	const Vector VEC_HULL_MAX( 16, 16, 72 );

	CGameTrace tr{ };
	CTraceFilterSkipTeammates filter{ pLocal };
	Ray_t ray{ pLocal->GetAbsOrigin( ), pLocal->GetAbsOrigin( ), VEC_HULL_MIN, VEC_HULL_MAX };

	g_pEngineTrace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &tr );

	if( tr.startsolid || ( tr.fraction != 1.0f ) )
		return false;

	return true;
}


using C_BasePlayer__PhysicsSimulate_ServerFn = void( __thiscall * )( void * );
C_BasePlayer__PhysicsSimulate_ServerFn oC_BasePlayer__PhysicsSimulate_Server;


// player + 0x126C = m_iTicksAllowed
// server.dll 55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 8B F1 8B 0D ? ? ? ? 57 89 74 24 34
// https://i.imgur.com/hw5OFMV.png
void __fastcall C_BasePlayer__PhysicsSimulate_Server( void *ecx, void *edx ) {
	if( !C_CSPlayer::GetLocalPlayer( ) )
		return oC_BasePlayer__PhysicsSimulate_Server( ecx );

	auto player = ( C_BasePlayer * )ecx;
	if( !ecx )
		return;

	if( !player )
		return oC_BasePlayer__PhysicsSimulate_Server( ecx );

	int ticks = *( int * )( uintptr_t( player ) + 0x126C );

	//if( ticks > 1 )
	//	printf( "ticks before: %i | ", ticks );

	oC_BasePlayer__PhysicsSimulate_Server( ecx );

	int ticks2 = *( int * )( uintptr_t( player ) + 0x126C );

	//if( ticks > 1 )
	//	printf( "ticks after: %i\n", ticks2 );
}

using CHudScope__PaintFn = bool( __thiscall * )( void * );
CHudScope__PaintFn oCHudScope__Paint;

// client.dll 55 8B EC 83 E4 F8 83 EC 78 56 57 8B 3D
void __fastcall CHudScope__Paint( void *ecx, void *edx ) {

}

using ClientModeCSNormal__UpdatePostProcessingEffectsFn = void( __thiscall * )( void * );
ClientModeCSNormal__UpdatePostProcessingEffectsFn oClientModeCSNormal__UpdatePostProcessingEffects;

void __fastcall ClientModeCSNormal__UpdatePostProcessingEffects( void *ecx, void *edx ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return oClientModeCSNormal__UpdatePostProcessingEffects( ecx );

	bool bBackup = pLocal->m_bIsScoped( );
	pLocal->m_bIsScoped( ) = false;
	oClientModeCSNormal__UpdatePostProcessingEffects( ecx );
	pLocal->m_bIsScoped( ) = bBackup;
}

using CClient_Precipitation__RenderFn = void( __thiscall * )( void * );
CClient_Precipitation__RenderFn oCClient_Precipitation__Render;

void __fastcall CClient_Precipitation__Render( void *ecx, void * ) {
	oCClient_Precipitation__Render( ecx );
}

//A1 ? ? ? ? 53 56 8B F1 B9 ? ? ? ? 57 FF 50 34
using CreateParticlePrecipFn = void( __thiscall * )( void * );
CreateParticlePrecipFn oCreateParticlePrecip;

void __fastcall CreateParticlePrecip( void *ecx, void * ) {
	// auto m_bParticlePrecipInitialized = *( bool* )( ( uintptr_t )ecx + 0xAA1 );

	// dont let the game call InitializeParticlePrecip ever
//	*( bool* )( ( uintptr_t )ecx + 0xAA1 ) = true;

	oCreateParticlePrecip( ecx );

	bool m_bActiveParticlePrecipEmitter = *( bool * )( ( uintptr_t )ecx + 0xAA0 );
	if( m_bActiveParticlePrecipEmitter ) {
		// this was here to make sure that DestroyInnerParticlePrecip is 100% not being called from CreateParticlePrecip
		// printf( "yep it was setup\n" );
	}
}

using UpdateParticlePrecipFn = void( __thiscall * )( void *, void *, int );
UpdateParticlePrecipFn oUpdateParticlePrecip;

void __fastcall UpdateParticlePrecip( void *ecx, void *, void *player, int idk ) {
	//*( float* )( uintptr_t( ecx ) + 2660 ) = 0.f;
	oUpdateParticlePrecip( ecx, player, idk );
}

//55 8B EC 53 8B 5D 0C 56
using CNewParticleEffect__SetControlPointFn = void( __thiscall * )( void *, int nWhichPoint, const Vector &v );
CNewParticleEffect__SetControlPointFn oCNewParticleEffect__SetControlPoint;

void __fastcall CNewParticleEffect__SetControlPoint( void *ecx, void *, int nWhichPoint, const Vector &v ) {
	//*( bool* )( ( uintptr_t )ecx + 0x3B1 ) = true;
	//*( bool* )( ( uintptr_t )ecx + 0x3B4 ) = 1;
	oCNewParticleEffect__SetControlPoint( ecx, nWhichPoint, v );
}

using CClient_Precipitation__SimulateFn = void( __thiscall * )( void *, float );
CClient_Precipitation__SimulateFn oCClient_Precipitation__Simulate;

void __fastcall CClient_Precipitation__Simulate( void *ecx, void *, float dt ) {
	if( !ecx )
		return;

	auto pPrecip = reinterpret_cast< C_BaseEntity * >( ecx );
	if( !pPrecip )
		return;

	pPrecip->m_nPrecipType( ) = PrecipitationType_t::PRECIPITATION_TYPE_PARTICLERAIN;

	oCClient_Precipitation__Simulate( ecx, dt );
}


#include "Features/Visuals/WeatherController.hpp"

using GetVCollideFn = vcollide_t * ( __thiscall * )( void *, int );
GetVCollideFn oGetVCollide;
vcollide_t *__fastcall GetVCollide( void *ecx, void *edx, int index ) {
	if( g_WeatherController.m_bCreatedCollision ) {
		if( index == -1 ) {
			return &g_WeatherController.m_vCollide;
		}
	}

	return oGetVCollide( ecx, index );
}

struct RenderableInstance_t {
	uint8 m_nAlpha;
};

#include "Features/Rage/Resolver.hpp"

using SVCMsg_VoiceData_t = bool( __thiscall * )( void *, void * );
SVCMsg_VoiceData_t oSVCMsg_VoiceData;
bool __fastcall SVCMsg_VoiceData( void *ecx, void *, void *mmsg ) {
	if( !mmsg )
		return oSVCMsg_VoiceData( ecx, mmsg );

	CSVCMsg_VoiceData *pReceived = ( CSVCMsg_VoiceData * )mmsg;
	int nSenderIndex = pReceived ? pReceived->client + 1 : -1;

	// yup
	if( g_GSInterpreter.ReceiveSharedPackets( ( voice_data * )mmsg ) )
		return true;

	return g_Communication.OnMessageHook( [&] ( ) { return oSVCMsg_VoiceData( ecx, mmsg ); }, mmsg );
}

using __MsgFunc_ReloadEffectFn = bool( __cdecl * )( int );
inline __MsgFunc_ReloadEffectFn o__MsgFunc_ReloadEffect;
bool __cdecl __MsgFunc_ReloadEffect( int msg ) {
	// thanks L3D
	if( ( *( char * )( msg + 36 ) & 1 ) != 0 ) {
		auto v1 = *( int * )( msg + 36 );
		if( ( v1 & 2 ) != 0 && ( v1 & 4 ) != 0 && ( v1 & 8 ) != 0 ) {
			auto v2 = *( int * )( msg + 8 );

			if( ( unsigned int )( v2 - 1 ) <= 63 ) {
				const Vector v3 = *( Vector * )( msg + 12 );

				// don't override if we are using a better method of overriding esp
				if( g_ExtendedVisuals.m_arrOverridePlayers.at( v2 - 1 ).m_eOverrideType <= EOverrideType::ESP_SHARED ) {
					g_ExtendedVisuals.m_arrOverridePlayers.at( v2 - 1 ).m_eOverrideType = EOverrideType::ESP_SHARED;
					g_ExtendedVisuals.m_arrOverridePlayers.at( v2 - 1 ).m_flReceiveTime = g_pGlobalVars->realtime;
					g_ExtendedVisuals.m_arrOverridePlayers.at( v2 - 1 ).m_vecLastOrigin = g_ExtendedVisuals.m_arrOverridePlayers.at( v2 - 1 ).m_vecOrigin;
					g_ExtendedVisuals.m_arrOverridePlayers.at( v2 - 1 ).m_vecOrigin = *( Vector * )( msg + 12 );;
				}
			}
		}
	}

	return o__MsgFunc_ReloadEffect( msg );
}

using mat_force_tonemap_scaleGetFloatFn = float( __thiscall * )( void * );
mat_force_tonemap_scaleGetFloatFn omat_force_tonemap_scaleGetFloat;
float __fastcall mat_force_tonemap_scaleGetFloat( void *, void * ) {
	return 1.f;
}

using CreateFontFn = HFONT( __stdcall * )( int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR );
inline CreateFontFn oCreateFontA;

HFONT __stdcall CreateFontA_( int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight,
							  DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision,
							  DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName ) {

	if( /*iQuality == ANTIALIASED_QUALITY*/false ) {
		return oCreateFontA( cHeight, cWidth, cEscapement, cOrientation, cWeight, bItalic, bUnderline, bStrikeOut,
							 DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
							 CLEARTYPE_NATURAL_QUALITY, VARIABLE_PITCH, pszFaceName );
	}

	return oCreateFontA( cHeight, cWidth, cEscapement, cOrientation, cWeight, bItalic, bUnderline, bStrikeOut,
						 iCharSet, iOutPrecision, iClipPrecision,
						 iQuality, iPitchAndFamily, pszFaceName );
}

std::optional<std::pair<Color, Color>> CompareColor( Color a, std::vector<std::pair<Color, Color>> b ) {
	for( auto col : b ) {
		if( a.r( ) == col.first.r( ) &&
			a.g( ) == col.first.g( ) &&
			a.b( ) == col.first.b( ) ) {
			return { {col.first, col.second} };
		}
	}

	return std::nullopt;
}

using RenderBoxFn = int( __cdecl * )( int a1, int a2, int a3, int a4, Color a5, int a6, char a7 );
inline RenderBoxFn oRenderBox;

// color of the line
int __cdecl RenderBox( int a1, int a2, int a3, int a4, Color a5, int a6, char a7 ) {
	return oRenderBox( a1, a2, a3, a4, a5, a6, a7 );
}

using RenderLineFn = int( __cdecl * )( int a1, int a2, Color a3, char a4 );
inline RenderLineFn oRenderLine;

// color of the glow
int __cdecl RenderLine( int a1, int a2, Color a3, char a4 ) {
	const Color grenadeWarningColor = g_Vars.esp.grenade_proximity_warning_color.ToRegularColor( );

	const Color bulletTracerLocalColor = g_Vars.esp.bullet_tracer_local_color.ToRegularColor( );
	const Color bulletTracerLocalGlowColor = g_Vars.esp.bullet_tracer_local_glow_color.ToRegularColor( );

	const Color bulletTracerEnemyColor = g_Vars.esp.bullet_tracer_enemy_color.ToRegularColor( );
	const Color bulletTracerEnemyGlowColor = g_Vars.esp.bullet_tracer_enemy_glow_color.ToRegularColor( );

	const Color smokeColor = Color( 220, 220, 220 );
	const Color molotovColor = Color( 248, 0, 0 );

	// first color = color to compare (regular color)
	// second color = color's paired color (wish glow color)
	auto col = CompareColor( a3, { {bulletTracerLocalColor, bulletTracerLocalGlowColor}, {bulletTracerEnemyColor, bulletTracerEnemyGlowColor}, {smokeColor, Color( 255,255,255,150 )}, {molotovColor, Color( 255,0,0,220 )}, { grenadeWarningColor, grenadeWarningColor } } );
	if( col.has_value( ) ) {
		const float flAlpha = col.value( ).second.a( ) / 255.f;
		Color glowColor = col.value( ).second;
		glowColor.RGBA[ 0 ] *= flAlpha;
		glowColor.RGBA[ 1 ] *= flAlpha;
		glowColor.RGBA[ 2 ] *= flAlpha;
		glowColor.RGBA[ 3 ] = a3.a( );

		return oRenderLine( a1, a2, glowColor, a4 );
	}

	return oRenderLine( a1, a2, a3, a4 );
}


using CL_FireEventsFn = void( __cdecl * )( );
CL_FireEventsFn oCL_FireEvents;

struct CEventInfo {
	uint16_t classID;
	char pad_0002[ 2 ];
	float fire_delay;
	char pad_0008[ 4 ];
	void *pClientClass;
	void *pData;
	char pad_0014[ 36 ];
	CEventInfo *next;
	char pad_0038[ 8 ];
};

void __cdecl CL_FireEvents( ) {
	// remove fire delay :)
	// game calls CL_FireEvents in CL_ReadPackets => hooking the engine func isn't enough / proper to do this
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( pLocal ) {
		for( CEventInfo *i = *reinterpret_cast< CEventInfo ** >( uintptr_t( g_pClientState.Xor( ) ) + 0x4DEC );
			 i != nullptr;
			 i = i->next ) {
			if( !i->classID )
				continue;

			if( i->classID == CTEFireBullets || i->classID - 1 == CTEFireBullets )
				i->fire_delay = 0.0f;
		}
	}

	oCL_FireEvents( );
}


// E8 ? ? ? ? 8B 87 ? ? ? ? 89 87 ? ? ? ? A1
using CheckForSequenceChangeFn = void( __thiscall * )( void *, void *, int, bool, bool );
CheckForSequenceChangeFn oCheckForSequenceChange;
void __fastcall CheckForSequenceChange( void *ecx, void *, void *hdr, int nCurSequence, bool bForceNewSequence, bool bInterpolate ) {
	oCheckForSequenceChange( ecx, hdr, nCurSequence, bForceNewSequence, false );
}

void __fastcall Hooked::ClampBonesBBox( C_CSPlayer *ecx, void *, matrix3x4_t *pMatrix, int boneMask ) {
	if( !ecx )
		return;

	// DO IT ON LOCAL PLAYER :D
	if( ecx->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) {
		oClampBonesBBox( ecx, pMatrix, boneMask );
	}
}

using DrawStaticProps_FastFn = int( __thiscall * )( int a1, int a2, int a3, char a4, char a5 );
DrawStaticProps_FastFn oDrawStaticProps_Fast;

int __stdcall DrawStaticProps_Fast( int a1, int a2, int a3, char a4, char a5 ) {
	if( !GetAsyncKeyState( VK_MBUTTON ) )
		return oDrawStaticProps_Fast( a1, a2, a3, a4, a5 );
}

using Teleported_Fn = bool( __thiscall * )( void * );
Teleported_Fn oTeleported;

bool __fastcall Teleported( C_CSPlayer *ecx, void * ) {
	if( ecx->EntIndex( ) != g_pEngine->GetLocalPlayer( ) ) {
		return oTeleported( ecx );
	}


	if( g_Vars.globals.boneSertup )
		return false;

	auto ret = oTeleported( ecx );

	if( ret )
		printf( "lc\n" );

	return ret;
}

using PrecacheLightingFn = void( __thiscall * )( void *ecx );
PrecacheLightingFn oPrecacheLighting;

void __fastcall PrecacheLighting( void *a1, void *a2 ) {
	if( !a1 )
		return;

	int v4; // ebx
	int v6; // edi
	DWORD *v1; // esi
	int v3; // edi

	if( g_Misc.m_vecStaticProps.size( ) ) {
		//printf( "cleared %i props\n", g_Misc.m_vecStaticProps.size( ) );
		g_Misc.m_vecStaticProps.clear( );
	}

	v1 = ( DWORD * )a1;
	if( v1 ) {
		v3 = v1[ 11 ];
		if( v3 > 0 ) {
			v4 = 0;
			do {
				v6 = v4 + v1[ 8 ];

				g_Misc.m_vecStaticProps.push_back( Encrypted_t<CStaticProp>( ( CStaticProp * )v6 ) );

				v4 += 204;
				--v3;
			} while( v3 );
		}
	}

	//printf( "found %i props\n", g_Misc.m_vecStaticProps.size( ) );

	oPrecacheLighting( a1 );
}

bool bCallFromSVCMsg_GetCvarValue = false;
using SVCMsg_GetCvarValueFn = bool( __thiscall * )( void *ecx, void *msg );
SVCMsg_GetCvarValueFn oSVCMsg_GetCvarValue;

bool __fastcall SVCMsg_GetCvarValue( void *ecx, void *edx, void *msg ) {
	if( !g_Vars.misc.cl_lagcomp_bypass ) {
		return oSVCMsg_GetCvarValue( ecx, msg );
	}

	bCallFromSVCMsg_GetCvarValue = true;
	auto ret = oSVCMsg_GetCvarValue( ecx, msg );
	bCallFromSVCMsg_GetCvarValue = false;

	return ret;
}

using FindVarFn = ConVar * ( __thiscall * )( void *ecx, const char *var_name );
FindVarFn oFindVar;

ConVar *__fastcall FindVar( void *ecx, void *edx, const char *var_name ) {
	if( !g_Vars.misc.cl_lagcomp_bypass ) {
		return oFindVar( ecx, var_name );
	}

	if( bCallFromSVCMsg_GetCvarValue && strlen( var_name ) >= 7 ) {
		// printf( "incoming call from SVCMsg_GetCvarValue: %s\n", var_name );

		// cl_lagcompensation
		// ^  ^  ^
		if( var_name[ 0 ] == 'c' && var_name[ 3 ] == 'l' && var_name[ 6 ] == 'c' ) {
			// printf( "confirmed cl_lagcompensation\n" );

			// something that always returns 1
			static ConVar *cl_aggregate_particles = g_pCVar->FindVar( XorStr( "cl_aggregate_particles" ) );
			return cl_aggregate_particles;
		}
	}

	return oFindVar( ecx, var_name );
}

// 55 8B EC 8B 45 10 F3 0F 10 81
using OnBBoxChangeCallbackFn = void( __thiscall * )( C_CSPlayer *, Vector *, Vector *, const Vector *, Vector * );
OnBBoxChangeCallbackFn oOnBBoxChangeCallback;

void __fastcall OnBBoxChangeCallback( C_CSPlayer *ecx, void *edx, Vector *vecOldMins, Vector *vecNewMins, const Vector *vecOldMaxs, Vector *vecNewMaxs ) {
	// crazy method 

	if( *( float * )( uintptr_t( ecx ) + 0x9924 ) == 0.f || ecx->m_flSimulationTime( ) > *( float * )( uintptr_t( ecx ) + 0x9924 ) ) {
		*( float * )( uintptr_t( ecx ) + 0x9924 ) = ecx->m_flSimulationTime( );
	}
	else {
		int nFlow = FLOW_INCOMING;
		if( g_Vars.misc.extended_backtrack || ( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ) && !( C_CSPlayer::GetLocalPlayer( ) && C_CSPlayer::GetLocalPlayer( )->IsDead( ) ) ) {
			nFlow = FLOW_OUTGOING;
		}

		const float flLatency = g_pEngine->GetNetChannelInfo( ) ? g_pEngine->GetNetChannelInfo( )->GetLatency( nFlow ) : 0.f;
		const float flChangeTime = TICKS_TO_TIME( g_pGlobalVars->tickcount ) - flLatency;

		*( float * )( uintptr_t( ecx ) + 0x9924 ) = flChangeTime;
	}

	*( float * )( uintptr_t( ecx ) + 0x9920 ) = ecx->m_vecOrigin( ).z + vecNewMaxs->z;
}

using BuildRenderablesListForCSMViewFn = void( __thiscall * )( void *ecx, void *idk );
BuildRenderablesListForCSMViewFn oBuildRenderablesListForCSMView;

void __fastcall BuildRenderablesListForCSMView( void *ecx, void *, void *idk ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return oBuildRenderablesListForCSMView( ecx, idk );

	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		const auto player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player )
			continue;

		if( player->IsDead( ) || player->IsDormant( ) )
			continue;

		auto pRenderable = player->GetClientRenderable( );
		if( !pRenderable )
			continue;

		// we disable occlusion for local player by setting translucency type to RENDERABLE_IS_TRANSLUCENT
		// in frame_render_start, but that removes the rendering of our shadow, so set it to opaque here
		g_pClientLeafSystem->SetTranslucencyType( pRenderable->RenderHandle( ), RENDERABLE_IS_OPAQUE );
	}

	oBuildRenderablesListForCSMView( ecx, idk );

	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		const auto player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player )
			continue;

		if( player->IsDead( ) || player->IsDormant( ) )
			continue;

		auto pRenderable = player->GetClientRenderable( );
		if( !pRenderable )
			continue;

		// we disable occlusion for local player by setting translucency type to RENDERABLE_IS_TRANSLUCENT
		// in frame_render_start, but that removes the rendering of our shadow, so set it to opaque here
		g_pClientLeafSystem->SetTranslucencyType( pRenderable->RenderHandle( ), RENDERABLE_IS_TRANSLUCENT );
	}
}

using TE_EffectDispatch_PostDataUpdateFn = void( __thiscall * )( C_TEEffectDispatch *, int );
inline TE_EffectDispatch_PostDataUpdateFn oTE_EffectDispatch_PostDataUpdate;
#include "Features/Rage/Resolver.hpp"
void __fastcall TE_EffectDispatch_PostDataUpdate( C_TEEffectDispatch *thisptr, void *edx, int updateType ) {

	// g_pDebugOverlay->AddTextOverlay( thisptr->m_EffectData.m_vOrigin, 2.f, "%i\n", thisptr->m_EffectData.m_iEffectName );

	// ccsblood
	if( thisptr->m_EffectData.m_iEffectName == 4 ) {
		g_Resolver.OnSpawnBlood( thisptr );
	}

	oTE_EffectDispatch_PostDataUpdate( thisptr, updateType );
}

__declspec( naked ) void CreateMoveProxy( int sequence_number, float input_sample_frametime, bool active ) {
	__asm
	{
		push ebp
		mov  ebp, esp
		push ebx;
		push esp
			push dword ptr[ active ]
			push dword ptr[ input_sample_frametime ]
			push dword ptr[ sequence_number ]
			call Hooked::CreateMove
			pop  ebx
			pop  ebp
			retn 0Ch
	}
}

using isBoneAvailable = bool( __stdcall * )( int a1 );
isBoneAvailable oIsBoneAvailable;
bool __stdcall IsBoneAvailable( int a1 ) {

	static void *ret = ( void * )Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 0F 84 ? ? ? ? 8B 06 8B CE 8B 40 20" ), false );
	if( _ReturnAddress( ) == ret )
		return true;

	return oIsBoneAvailable( a1 );
}

using SFHudMoney__ShouldDrawFn = bool( __thiscall * )( void * );
SFHudMoney__ShouldDrawFn oSFHudMoney__ShouldDraw;
bool __fastcall SFHudMoney__ShouldDraw( void *ecx, void * ) {
	// 
	if( g_pGameRules.Xor( ) ) {
		using C_CSGameRules__IsBuyTimeElapsedFn = bool( __thiscall * )( void * );
		static auto oC_CSGameRules__IsBuyTimeElapsed = ( C_CSGameRules__IsBuyTimeElapsedFn )Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 51 56 8B F1 8B ? ? ? ? ? 8B 01 FF 50 20 83 F8 02 74 41" ) );
		if( oC_CSGameRules__IsBuyTimeElapsed( g_pGameRules.Xor( ) ) )
			return false;
	}

	return oSFHudMoney__ShouldDraw( ecx );
}

using CCSPlayer_FireBulletServerFn = void( __thiscall * )( void *ecx,
														   Vector vecSrc,	// shooting postion
														   const QAngle &shootAngles,  //shooting angle
														   float flDistance, // max distance 
														   float flPenetration, // the power of the penetration
														   int nPenetrationCount,
														   int iBulletType, // ammo type
														   int iDamage, // base damage
														   float flRangeModifier, // damage range modifier
														   C_CSPlayer *pevAttacker, // shooter
														   bool bDoEffects,
														   float xSpread, float ySpread );
CCSPlayer_FireBulletServerFn oCCSPlayer_FireBulletServer;

void __fastcall	CCSPlayer_FireBulletServer( void *ecx, void *,
											Vector vecSrc,	// shooting postion
											const QAngle &shootAngles,  //shooting angle
											float flDistance, // max distance 
											float flPenetration, // the power of the penetration
											int nPenetrationCount,
											int iBulletType, // ammo type
											int iDamage, // base damage
											float flRangeModifier, // damage range modifier
											C_CSPlayer *pevAttacker, // shooter
											bool bDoEffects,
											float xSpread, float ySpread ) {
	oCCSPlayer_FireBulletServer( ecx, vecSrc, shootAngles, flDistance, flPenetration, nPenetrationCount, iBulletType, iDamage, flRangeModifier, pevAttacker, bDoEffects, xSpread, ySpread );

	static uintptr_t gpGlobals = **( uintptr_t ** )( Memory::Scan( "server.dll", "8B 15 ? ? ? ? 33 C9 83 7A 18 01" ) + 0x2 );
	int tickcount = *( int * )( uintptr_t( gpGlobals ) + 0x1C );

	printf( "\tarrived on server tick: %i [server]\n", tickcount );

}

// 55 8B EC 53 56 8B F1 8B 0D ? ? ? ? 57 85 C9
using CClientState_NETMsg_TickFn = bool( __thiscall * )( void *, DWORD * );
CClientState_NETMsg_TickFn oCClientState_NETMsg_Tick;
bool CClientState_NETMsg_Tick( void *ecx, void *, DWORD *msg ) {
	if( !msg )
		return oCClientState_NETMsg_Tick( ecx, msg );

	auto msg_tick = msg[ 2 ];
	auto msg_flags = msg[ 6 ];

	if( g_pClientState->m_nLastCommandAck( ) != g_pEngine->GetLastAcknowledgedCommand( ) && msg_flags & 1 ) {
		int last_outgoing = g_pEngine->GetLastAcknowledgedCommand( );
		printf( "last outgoing %i %i\n", last_outgoing, last_outgoing % 150 );
		// c_hack::latency_record_t *latency_record = g_hack.get_latency_record( last_outgoing );

		/*if( *g_send_packet ) {
			int last_outgoing = g_pClientState->m_nLastOutgoingCommand( ) + 1;
			int current_cmd = cmd( )->m_number;

			if( current_cmd >= last_outgoing && ( current_cmd - last_outgoing ) < 150 ) {
				do {
					c_hack::latency_record_t *latency_record = g_hack.get_latency_record( last_outgoing );
					latency_record->m_tick_count = last_outgoing++;
					latency_record->m_server_tick = g_pEngine->GetServerTick( );
				} while( last_outgoing <= current_cmd );
			}
		}

		int *server_tick = latency_record->m_tick_count == last_outgoing ? &latency_record->m_server_tick : nullptr;
		if( server_tick ) {
			int tick_difference = std::clamp( msg_tick - *server_tick, 0, 64 );

			if( g_hack.m_latency.m_size != 16 )
				++g_hack.m_latency.m_size;
			g_hack.m_latency.m_count = ( g_hack.m_latency.m_count - 1 ) & 15;
			g_hack.m_latency.m_records[ g_hack.m_latency.m_count ] = tick_difference;
		}*/
	}

	return oCClientState_NETMsg_Tick( ecx, msg );
}

#include "Features/Rage/FakePing.hpp"

typedef bool( __thiscall *fnGetBool )( void * );
fnGetBool oNetShowfragmentsGetBool;
bool __fastcall net_showfragments_get_bool( void *pConVar, void *edx ) {
	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) ) {
		return oNetShowfragmentsGetBool( pConVar );
	}

	auto netchannel = Encrypted_t<INetChannel>( g_pEngine->GetNetChannelInfo( ) );
	if( !netchannel.IsValid( ) )
		return oNetShowfragmentsGetBool( pConVar );

	static auto retReadSubChannelData = reinterpret_cast< uintptr_t * >( Engine::Displacement.Data.ReadSubChannelDataReturn );
	static auto retCheckReceivingList = reinterpret_cast< uintptr_t * >( Engine::Displacement.Data.CheckReceivingListReturn );

	static char *pSavedBuffer = nullptr;
	static int nCopyTick = 0;
	static size_t nLastSize = 0;

	if( _ReturnAddress( ) == retReadSubChannelData ) {
		const auto pData = &reinterpret_cast< uint32_t * >( netchannel.Xor( ) )[ 0x54 ];

		if( pData ) {
			if( nCopyTick > 0 &&
				// yes sirr
				nCopyTick + TIME_TO_TICKS( 0.5f ) >= g_pClientState->m_ClockDriftManager( ).m_nServerTick ) {

				if( ( char * )pData[ 0x42 ] && ( nLastSize == ( size_t )pData[ 0x43 ] ) ) {
					size_t current_size = ( size_t )pData[ 0x43 ];
					size_t saved_size = ( size_t )pSavedBuffer[ 0x43 ];

					// idk if this is good
					auto cmp_size = std::min( current_size, saved_size );

					if( memcmp( ( char * )pData[ 0x42 ], pSavedBuffer, cmp_size ) == 0 ) {
						auto &buffer = reinterpret_cast< uint32_t * >( pData )[ 0x42 ];
						buffer = 0;
					}
				}
			}
		}
	}

	if( _ReturnAddress( ) == retCheckReceivingList ) {
		const auto pData = &reinterpret_cast< uint32_t * >( netchannel.Xor( ) )[ 0x54 ];

		if( pData ) {
			if( ( char * )pData[ 0x42 ] ) {
				nLastSize = ( size_t )pData[ 0x43 ];

				char *buffer = ( char * )pData[ 0x42 ];

				// free this one
				if( pSavedBuffer ) {
					delete[ ] pSavedBuffer;
					pSavedBuffer = nullptr;
				}

				// allocate new mem to our buffer
				// then copy over current buffer
				pSavedBuffer = new char[ nLastSize ];
				memcpy( pSavedBuffer, buffer, nLastSize );

				// note when we last copied this fucker
				nCopyTick = g_pClientState->m_ClockDriftManager( ).m_nServerTick;
			}
		}
	}
	return oNetShowfragmentsGetBool( pConVar );
}

using SendDatagramFn = int( __thiscall * )( INetChannel *, void * );
SendDatagramFn oSendDatagram;
int __fastcall SendDatagram( INetChannel *ecx, void *edx, void *a3 ) {
	if( !ecx || a3 )
		return oSendDatagram( ecx, a3 );

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return oSendDatagram( ecx, a3 );

	//if( ecx->IsLoopback( ) )
	//	return oSendDatagram( ecx, a3 );

	if( !( g_Vars.misc.extended_backtrack || ( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ) ) )
		return oSendDatagram( ecx, a3 );

	if( pLocal->IsDead( ) )
		return oSendDatagram( ecx, a3 );

	const auto nBackupInSequenceNr = ecx->m_nInSequenceNr;
	const auto nBackupReliable = ecx->m_nInReliableState;

	g_FakePing.OnSendDatagram( ecx );

	auto oRet = oSendDatagram( ecx, a3 );

	ecx->m_nInSequenceNr = nBackupInSequenceNr;
	ecx->m_nInReliableState = nBackupReliable;

	return oRet;
}

using AllocKeyValuesMemoryFn = void *( __thiscall * )( void *, int );
AllocKeyValuesMemoryFn oAllocKeyValuesMemory;
void *__fastcall AllocKeyValuesMemory( void *ecx, void *, int size ) {

	//static const std::uintptr_t uAllocKeyValuesEngine = Memory::CallableFromRelative( Memory::Scan( XorStr( "engine.dll" ), XorStr( "E8 ? ? ? ? 83 C4 08 84 C0 75 10 FF 75 0C" ), false ) ) + 0x4A;
	//static const std::uintptr_t uAllocKeyValuesClient = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 83 C4 08 84 C0 75 10" ), false ) ) + 0x3E;

	//if( const std::uintptr_t uReturnAddress = reinterpret_cast< std::uintptr_t >( _ReturnAddress( ) ); uReturnAddress == uAllocKeyValuesEngine || uReturnAddress == uAllocKeyValuesClient )
	//	return nullptr;

	return oAllocKeyValuesMemory( ecx, size );
}

using SelectSequenceFromActivityModsFn = int( __thiscall * )( void *, int );
SelectSequenceFromActivityModsFn oSelectSequenceFromActivityMods;

int __fastcall SelectSequenceFromActivityMods( void *pServerState, void *edx, int act ) {
	auto ret = oSelectSequenceFromActivityMods( pServerState, act );
	if( act == ACT_CSGO_ALIVE_LOOP ) {

		const bool moving = *( float * )( uintptr_t( pServerState ) + 0xE8 ) > 0.25f;
		const bool crouch = *( float * )( uintptr_t( pServerState ) + 0x94 ) > 0.55000001f;

		// m_flSpeedAsPortionOfWalkTopSpeed
		if( moving && !crouch ) {
			printf( "moving: %i\n", ret );
		}

		// m_flAnimDuckAmount
		if( crouch && !moving ) {
			printf( "crouch: %i\n", ret );
		}

		if( moving && crouch )
			printf( "both: %i\n", ret );

		if( !moving && !crouch )
			printf( "none: %i\n", ret );
	}

	return ret;
}

using SetupAliveLoopFn = void( __thiscall * )( CCSGOPlayerAnimState *ecx );
inline SetupAliveLoopFn oSetupAliveLoop;

void __fastcall SetupAliveLoop( CCSGOPlayerAnimState *ecx, void *edx ) {
	if( !ecx || !ecx->m_pPlayer )
		return;

	// rebuild this shit 
	// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/shared/cstrike15/csgo_playeranimstate.cpp#L1167

	C_AnimationLayer *pLayer = &ecx->m_pPlayer->m_AnimOverlay( )[ ANIMATION_LAYER_ALIVELOOP ];
	if( !pLayer )
		return;

	//static auto SelectSequenceFromActivityMods = reinterpret_cast< int *( __thiscall * )( void *, int ) >( Memory::Scan( "server.dll", "55 8B EC 51 56 8B 35 ? ? ? ? 57 8B F9 8B CE" ) );

	if( ecx->m_pPlayer->GetSequenceActivity( pLayer->m_nSequence ) != ACT_CSGO_ALIVE_LOOP ) {
		// first time init
		MDLCACHE_CRITICAL_SECTION( );
		ecx->SetLayerSequence( pLayer, ACT_CSGO_ALIVE_LOOP );
		ecx->SetLayerCycle( pLayer, RandomFloat( 0.0f, 1.0f ) );

		float flNewRate = ecx->m_pPlayer->GetLayerSequenceCycleRate( pLayer, pLayer->m_nSequence );
		flNewRate *= RandomFloat( 0.8f, 1.1f );
		ecx->SetLayerRate( pLayer, flNewRate );
	}
	else {
		if( ecx->m_pWeapon != ecx->m_pWeaponLast ) {
			//re-roll act on weapon change
			float flRetainCycle = pLayer->m_flCycle;

			ecx->SetLayerSequence( pLayer, ACT_CSGO_ALIVE_LOOP );
			ecx->SetLayerCycle( pLayer, flRetainCycle );
		}
		else if( ecx->IsLayerSequenceFinished( pLayer, ecx->m_flLastUpdateIncrement ) ) {
			//re-roll rate
			//MDLCACHE_CRITICAL_SECTION( );

			float flNewRate = ecx->m_pPlayer->GetLayerSequenceCycleRate( pLayer, pLayer->m_nSequence );
			flNewRate *= RandomFloat( 0.8f, 1.1f );
			ecx->SetLayerRate( pLayer, flNewRate );
		}
		else {
			float flWeightOutPoseBreaker = Math::RemapValClamped( ecx->m_flSpeedAsPortionOfRunTopSpeed, 0.55f, 0.9f, 1.0f, 0.0f );
			ecx->SetLayerWeight( pLayer, flWeightOutPoseBreaker );
		}
	}

	ecx->IncrementLayerCycle( pLayer, true );
}

using PaintTraverseFn = void( __thiscall * )( void *ecx, VPANEL panel, bool forceRepaint, bool allowForce );
inline PaintTraverseFn oPaintTraverse;
void __fastcall PaintTraverse( void *ecx, void *edx, VPANEL panel, bool forceRepaint, bool allowForce ) {
	if( g_Vars.esp.remove_scope ) {
		if( !strcmp( XorStr( "HudZoom" ), g_pPanel->GetName( panel ) ) )
			return;
	}

	oPaintTraverse( ecx, panel, forceRepaint, allowForce );
}

// E8 ? ? ? ? E9 ? ? ? ? 83 BE

using UpdateAnimationStateFn = void( __thiscall * )( CCSGOPlayerAnimState *, float, float, float );
UpdateAnimationStateFn oUpdateAnimationState;
void __fastcall UpdateAnimationState( CCSGOPlayerAnimState *ecx, void *, float eyeYaw, float eyePitch, bool bForce ) {
	if( !ecx )
		return oUpdateAnimationState( ecx, eyeYaw, eyePitch, bForce );

	if( !ecx->m_pPlayer )
		return oUpdateAnimationState( ecx, eyeYaw, eyePitch, bForce );

	if( ecx->m_pPlayer->EntIndex( ) != g_pEngine->GetLocalPlayer( ) )
		bForce = true;

	if( g_Vars.globals.m_bUpdatingAnimations )
		oUpdateAnimationState( ecx, eyeYaw, eyePitch, bForce );
}

// 
using EstimateAbsVelocityFn = void( __thiscall * )( C_CSPlayer *ecx, Vector &vel );
EstimateAbsVelocityFn oEstimateAbsVelocity;
void __fastcall EstimateAbsVelocity( C_CSPlayer *ecx, void *edx, Vector &vel ) {
	if( !ecx ) {
		return oEstimateAbsVelocity( ecx, vel );
	}

	if( !ecx->IsPlayer( ) ) {

		return oEstimateAbsVelocity( ecx, vel );
	}

	vel = ecx->m_vecVelocity( );
}

std::array<std::string, 21> arrMask = {
	XorStr( "models/player/holiday/facemasks/facemask_anaglyph.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_battlemask.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_boar.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_bunny.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_bunny_gold.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_chains.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_chicken.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_dallas.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_devil_plastic.mdl" ),
	XorStr( "models/player/holiday/facemasks/evil_clown.mdl" ),

	XorStr( "models/player/holiday/facemasks/facemask_hoxton.mdl" ),
	XorStr( "models/player/holiday/facemasks/porcelain_doll.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_pumpkin.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_samurai.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_sheep_model.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_sheep_bloody.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_sheep_gold.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_skull.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_template.mdl" ),
	XorStr( "models/player/holiday/facemasks/facemask_wolf.mdl" ),
};

// https://i.imgur.com/qDhq8E2.png
// s/o dalkr
// 55 8B EC 51 53 56 57 E8 ? ? ? ? 83 F8
using AddonMaskWhateverFn = char *( __thiscall * )( void * );
AddonMaskWhateverFn oAddonMaskWhatever;
char *__fastcall AddonMaskWhatever( void *ecx, void * ) {
	if( !g_Vars.misc.mask_changer )
		return oAddonMaskWhatever( ecx );

	if( !g_Misc.PrecacheModel( arrMask[ g_Vars.misc.mask_changer_selection ].data( ) ) ) {
		return oAddonMaskWhatever( ecx );
	}

	return arrMask[ std::clamp( g_Vars.misc.mask_changer_selection, 0, 19 ) ].data( );
}

using ResetLatchedFn = void( __thiscall * )( void * );
ResetLatchedFn oResetLatched;
void __fastcall ResetLatched( void *ecx, void * ) {
	return;
}

using CBoneSnapshot__UpdateFn = void( __thiscall * )( void *, void *pEnt, bool bReadOnly );
CBoneSnapshot__UpdateFn oCBoneSnapshot__Update;
void __fastcall CBoneSnapshot__Update( void *ecx, void *, void *ent, bool read ) {
	return oCBoneSnapshot__Update( ecx, ent, read );
}

// 55 8B EC 83 EC ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE
using PerformPredictionFn = bool( __thiscall * )( void *, int, C_CSPlayer *, bool, int, int );
PerformPredictionFn oPerformPrediction;
bool __fastcall PerformPrediction( void *ecx, void *, int nSlot, C_CSPlayer *localPlayer, bool received_new_world_update, int incoming_acknowledged, int outgoing_command ) {
	return oPerformPrediction( ecx, nSlot, localPlayer, received_new_world_update, incoming_acknowledged, outgoing_command );
}

using RunSimulationFn = void( __thiscall * )( void *, int, CUserCmd *, size_t );
RunSimulationFn oRunSimulation;
void InvokeRunSimulation( void *this_, float curtime, int cmdnum, CUserCmd *cmd, size_t local ) {
	__asm {
		push local
		push cmd
		push cmdnum

		movss xmm2, curtime
		mov ecx, this_

		call oRunSimulation
	}
}

void __fastcall RunSimulation( void *ecx, void *, int iCommandNumber, CUserCmd *pCmd, size_t local ) {
	C_CSPlayer *player = ( C_CSPlayer * )local;

	if( !player )
		return;

	float curtime;
	__asm {
		movss curtime, xmm2
	}

	InvokeRunSimulation( ecx, curtime, iCommandNumber, pCmd, local );
}

void __vectorcall CL_ReadPackets( bool final_tick ) {
	if( g_Prediction.m_bReadPackets )
		return Hooked::oCL_ReadPackets( final_tick );
}

// DISCLAIMER: 
// I am lazy and just replacing our structs makes the FindInDatamap offset shit crash
// (and I cba to investigate or rewrite any of the disgusting functions ;-))
// so I just dropped the structs from the game src leak here :-)
// this hack and especially this file got disgusting anyway code-wise so fuck it
struct typedescription2_t {
	fieldtype_t			fieldType;
	const char *fieldName;
	int					fieldOffset; // Local offset value
	unsigned short		fieldSize;
	short				flags;
	// the name of the variable in the map/fgd data, or the name of the action
	const char *externalName;
	// pointer to the function set for save/restoring of custom data types
	ISaveRestoreOps *pSaveRestoreOps;
	// for associating function with string names
	inputfunc_t			inputFunc;
	// For embedding additional datatables inside this one
	datamap_t *td;

	// Stores the actual member variable size in bytes
	int					fieldSizeInBytes;

	// FTYPEDESC_OVERRIDE point to first baseclass instance if chains_validated has occurred
	struct typedescription2_t *override_field;

	// Used to track exclusion of baseclass fields
	int					override_count;

	// Tolerance for field errors for float fields
	float				fieldTolerance;

	// For raw fields (including children of embedded stuff) this is the flattened offset
	int					flatOffset[ TD_OFFSET_COUNT ];
	unsigned short		flatGroup;
};

struct datamap2_t {
	typedescription2_t *dataDesc;
	int					dataNumFields;
	char const *dataClassName;
	datamap2_t *baseMap;

	int					m_nPackedSize;
	void *m_pOptimizedDataMap;
};

// this shit got inlined apparently
bool PrepareDataMap( datamap2_t *dmap ) {
	bool bPerformedPrepare = false;
	if( dmap && !dmap->m_pOptimizedDataMap ) {
		bPerformedPrepare = true;

		//BuildFlattenedChains( dmap ); // E8 ? ? ? ? 8B D7 8B 7D ? 8B CF
		reinterpret_cast< void( __thiscall * )( datamap2_t * ) >( Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 8B D7 8B 7D ? 8B CF" ), false ) ) )( dmap );

		dmap = dmap->baseMap;
	}

	return bPerformedPrepare;
}

// C_CSPlayer::GetPredDescMap (!!!)
// B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 55 8B EC A1 ? ? ? ? 56 68 ? ? ? ? 8B 08 8B 01 FF 50 ? 85 C0 75 ? 33 F6 EB ? 8D 70 ? 83 E6 ? 89 46 ? 68 ? ? ? ? 6A ? 56 E8 ? ? ? ? 83 C4 ? 85 F6 74 ? 8B CE E8 ? ? ? ? 8B F0 85 F6 74 ? FF 75 ? 8B 16 8B CE FF 75 ? FF 92 ? ? ? ? 8D 46 ? 5E 5D C3 33 C0 5E 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 56 8B F1 E8 ? ? ? ? C7 06 ? ? ? ? C7 46

using GetPredDescMapFn = datamap2_t * ( __thiscall * )( void * );
GetPredDescMapFn oGetPredDescMap;
datamap2_t *__fastcall GetPredDescMapHook( C_CSPlayer *ecx, void *edx ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	// no clue if this is needed but Local Prediction Doesn't Care About Other Players Anyway?
	if( !ecx || !pLocal || pLocal->EntIndex( ) != ecx->EntIndex( ) )
		return oGetPredDescMap( ecx );

	// s/o UnknownCheats
	static datamap2_t ourMap;

	// https://youtu.be/vtNJMAyeP0s?t=56

	// Did We Set It Up Yet ?
	if( !ourMap.baseMap ) {
		// first of all grab the game's prepared datamap
		const auto pOriginalMap = oGetPredDescMap( ecx );

		// now copy it over to our local map
		std::memcpy( &ourMap, pOriginalMap, sizeof datamap2_t );

		// amazing way to allocate arrays at dynamic size on runtime (disgusting lang)
		static std::unique_ptr<typedescription2_t[ ]> data( new typedescription2_t[ ourMap.dataNumFields + 1 ] );

		// copy the typedescription of the copied map into the data LAL
		std::memcpy( data.get( ), ourMap.dataDesc, ourMap.dataNumFields * sizeof typedescription2_t );

		// setup our new datamap addition :-)
		typedescription2_t m_flVelocityModifier = {};
		m_flVelocityModifier.fieldType = FIELD_FLOAT;
		m_flVelocityModifier.fieldName = XorStr( "m_flVelocityModifier" );
		m_flVelocityModifier.fieldOffset = Engine::Displacement.DT_CSPlayer.m_flVelocityModifier;
		m_flVelocityModifier.fieldSize = 1;
		m_flVelocityModifier.flags = 0x100; // FTYPEDESC_INSENDTABLE
		m_flVelocityModifier.fieldSizeInBytes = 4;
		m_flVelocityModifier.fieldTolerance = 0.1f;

		// copy it over !!!
		std::memcpy( &data.get( )[ ourMap.dataNumFields ], &m_flVelocityModifier, sizeof typedescription2_t );

		// now override the dataDesc pointer to our custom map's pointer
		ourMap.dataDesc = data.get( );

		// ofc we need to tell the game this got bigger ??
		ourMap.dataNumFields += 1;

		// Need To Optimize it Again, So Game Wants To Use It :-)
		ourMap.m_pOptimizedDataMap = nullptr;

		// PrePare The DataMap
		PrepareDataMap( &ourMap );
	}

	// LOL WE RETURN OURS HAHAHAHHAHAHAHAHAHAHAAAAHAHAHHAHAHAHAHAHAHAHAHAHAHAHAAHAHAAHAHAH
	return &ourMap;
}

float flServerDuckAmount, flLastServerDuckAmount;

using SetupVelocityFn = void( __thiscall * )( void * );
// 55 8B EC 83 E4 ? 83 EC ? 56 57 8B 3D
SetupVelocityFn oSetupVelocityServer;
void __fastcall SetupVelocityServer( CCSGOPlayerAnimStateServer *ecx, void * ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto GetPlayerByIndex = [ ] ( int index ) -> C_CSPlayer * { //i dont need this shit func for anything else so it can be lambda
		typedef C_CSPlayer *( __fastcall *player_by_index )( int );
		static auto nIndex = reinterpret_cast< player_by_index >( Memory::Scan( XorStr( "server.dll" ), XorStr( "85 C9 7E 2A A1" ) ) );

		if( !nIndex )
			return false;

		return nIndex( index );
	};

	if( GetPlayerByIndex( g_pEngine->GetLocalPlayer( ) ) == ecx->m_pPlayer )
		return oSetupVelocityServer( ecx );


	if( ecx->m_flAnimDuckAmount != flLastServerDuckAmount ) {
		printf( "[server] %.6f -> %.6f\n", flLastServerDuckAmount, ecx->m_flAnimDuckAmount );
	}

	flLastServerDuckAmount = ecx->m_flAnimDuckAmount;

	oSetupVelocityServer( ecx );

	if( ecx->m_flVelocityLengthXY > 0.1f && false ) {
		printf( "[server] m_flVelocityLengthXY: %.1f\tm_flFootYaw: %.1f\tm_flEyeYaw: %.1f\n",
				ecx->m_flVelocityLengthXY,
				ecx->m_flFootYaw,
				ecx->m_flEyeYaw );

	}
}

SetupVelocityFn oSetupVelocityClient;
void __fastcall SetupVelocityClient( CCSGOPlayerAnimState *ecx, void * ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal == ecx->m_pPlayer )
		return oSetupVelocityClient( ecx );

	static float flLastDuckAmount = ecx->m_flAnimDuckAmount;
	if( ecx->m_flAnimDuckAmount != flLastDuckAmount ) {
		printf( "[client] %.6f -> %.6f\n", flLastDuckAmount, ecx->m_flAnimDuckAmount );
	}
	flLastDuckAmount = ecx->m_flAnimDuckAmount;

	if( ecx->m_flAnimDuckAmount != flServerDuckAmount ) {
		/*printf( "[client] duck amount differs to server!\n\tclient: %.5f\tserver: (%.5f %.5f)\tdiff: %.5f\tchange: %.5f\n",
				ecx->m_flAnimDuckAmount, flServerDuckAmount, flLastServerDuckAmount, fabsf( ecx->m_flAnimDuckAmount - flServerDuckAmount ), fabsf( flServerDuckAmount - flLastServerDuckAmount ) );

		printf( "[client] change: %.5f -> %.5f (%.5f diff)\n",
				flLastDuckAmount, ecx->m_flAnimDuckAmount, fabsf( ecx->m_flAnimDuckAmount - flLastDuckAmount ) );

		printf( "[server] change: %.5f -> %.5f (%.5f diff)\n",
				flLastServerDuckAmount, flServerDuckAmount, fabsf( flServerDuckAmount - flLastServerDuckAmount ) );*/

				//ecx->m_flAnimDuckAmount = flServerDuckAmount;
	}


	oSetupVelocityClient( ecx );

	if( ecx->m_flVelocityLengthXY > 0.1f && false ) {
		const float flEstimatedApproached = Math::ApproachAngle( ecx->m_pPlayer->m_flLowerBodyYawTarget( ), ecx->m_flFootYaw, ecx->m_flLastUpdateIncrement * ( 30.0f + 20.0f * ecx->m_flWalkToRunTransition ) );

		float flEstimatedBodyDelta = std::remainderf( ecx->m_pPlayer->m_flLowerBodyYawTarget( ) - ecx->m_flEyeYaw, 360.f );
		flEstimatedBodyDelta = std::clamp( flEstimatedBodyDelta, -58.f, 58.f ); // or 60.f

		const float flEstimatedSnapped = Math::AngleNormalize( std::remainderf( ecx->m_flEyeYaw + flEstimatedBodyDelta, 360.f ) );

		printf( "[client] m_flVelocityLengthXY: %.1f\tm_flFootYaw: %.1f\tm_flEyeYaw: %.1f\test appr: %.1f\test snap %.1f\n",
				ecx->m_flVelocityLengthXY,
				ecx->m_flFootYaw,
				ecx->m_flEyeYaw,
				flEstimatedApproached,
				flEstimatedSnapped );
	}
}

using SetupMovementFn = void( __thiscall * )( void * );

SetupMovementFn oSetupMovementServer;
void __fastcall SetupMovementServer( CCSGOPlayerAnimStateServer *ecx, void * ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto GetPlayerByIndex = [ ] ( int index ) -> C_CSPlayer * { //i dont need this shit func for anything else so it can be lambda
		typedef C_CSPlayer *( __fastcall *player_by_index )( int );
		static auto nIndex = reinterpret_cast< player_by_index >( Memory::Scan( XorStr( "server.dll" ), XorStr( "85 C9 7E 2A A1" ) ) );

		if( !nIndex )
			return false;

		return nIndex( index );
	};

	if( GetPlayerByIndex( g_pEngine->GetLocalPlayer( ) ) == ecx->m_pPlayer )
		return oSetupMovementServer( ecx );

	printf( "[server] 1 duration in air: %.5f\n", ecx->m_flDurationInAir );

	oSetupMovementServer( ecx );

	printf( "[server] 2 duration in air: %.5f\n", ecx->m_flDurationInAir );
}

//SetupMovementFn oSetupMovementClient;
//void __fastcall SetupMovementClient( CCSGOPlayerAnimState *ecx, void * ) {
//	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
//	if( !pLocal )
//		return;
//
//	if( pLocal == ecx->m_pPlayer )
//		return oSetupMovementClient( ecx );
//
//	oSetupMovementClient( ecx );
//}

// 56 8B F1 80 BE ? ? ? ? ? 0F 84 ? ? ? ? 80 BE ? ? ? ? ? 0F 85 ? ? ? ? 8B 0D
using sub_1019D440Fn = void *( __thiscall * )( void * );
// 55 8B EC 83 E4 ? 83 EC ? 56 57 8B 3D
sub_1019D440Fn osub_1019D440;

void *__fastcall sub_1019D440( void *ecx, void *edx ) {
	/*const bool bIsRagdoll =
		*( bool * )( uintptr_t( ecx ) + 0x9F4 ) &&
		*( bool * )( uintptr_t( ecx ) + 0x275 );

	if( bIsRagdoll ) {
			return NULL;
	}

	return osub_1019D440( ecx );*/
	/*auto player = ( C_CSPlayer * )( ( uintptr_t )ecx - 4 );
	if( !player || player->GetClientClass( )->m_ClassID != CCSRagdoll )
		return osub_1019D440( ecx );*/

	return NULL;
}

using CPaintKit__InitFromKeyValuesFn = bool *( __thiscall * )( void *ecx, KeyValues *pKVEntry, const void *pDefault, bool bHandleAbsolutePaths );
// 55 8B EC 83 E4 ? 83 EC ? 56 57 8B 3D
CPaintKit__InitFromKeyValuesFn oCPaintKit__InitFromKeyValues;

bool __fastcall CPaintKit__InitFromKeyValues( void *ecx, void *edx, KeyValues *pKVEntry, const void *pDefault, bool bHandleAbsolutePaths ) {
	auto bReturn = oCPaintKit__InitFromKeyValues( ecx, pKVEntry, pDefault, bHandleAbsolutePaths );

	if( pDefault ) {
		//printf( "%s\n", pDefault->sName.buffer );
	}

	return bReturn;
}

using add_renderableFn = void( __thiscall * )( void *ecx, void *renderable, bool render_with_viewmodels, int type, int model_type, int split_screen_enables );
// 55 8B EC 83 E4 ? 83 EC ? 56 57 8B 3D
add_renderableFn oadd_renderable;

void __fastcall add_renderable( void *ecx, void *edx, void *renderable, bool render_with_viewmodels, int type, int model_type, int split_screen_enables ) {
	auto renderable_addr = ( std::uintptr_t )renderable;
	if( !renderable_addr || renderable_addr == 0x4 )
		return oadd_renderable( ecx, renderable, render_with_viewmodels, type, model_type, split_screen_enables );

	auto entity = ( C_BaseEntity * )( renderable_addr - 0x4 );
	int index = *( int * )( ( std::uintptr_t )entity + 0x64 );

	if( index < 1 || index > 64 )
		return oadd_renderable( ecx, renderable, render_with_viewmodels, type, model_type, split_screen_enables );

	// set advanced transparency type for fixing z order (chams renders behind props)
	if( index == g_pEngine->GetLocalPlayer( ) )
		type = 1;
	else
		type = 2;

	oadd_renderable( ecx, renderable, render_with_viewmodels, type, model_type, split_screen_enables );
}

struct start_sound_params_t {
	int user_data;
	int sound_source;
	int ent_channel;
	c_sfx_table *sfx;
	Vector origin;
};

using start_sound_immediateFn = int( __thiscall * )( start_sound_params_t *ecx );
start_sound_immediateFn ostart_sound_immediate;

int __fastcall start_sound_immediate( start_sound_params_t *ecx, void *edx ) {
	if( !ecx || !ecx->sfx )
		return ostart_sound_immediate( ecx );

	if( ecx->sound_source <= 0 || ecx->sound_source >= 65 )
		return ostart_sound_immediate( ecx );

	/*char sound_name[ 260 ]{};
	ecx->sfx->getname( sound_name, 260 );

	const auto is_valid_sound = [ sound_name ] ( ) -> bool {
		return strstr( sound_name, ( "auto_semiauto_switch" ) )
			|| strstr( sound_name, ( "weapons" ) )
			|| strstr( sound_name, ( "player\\land" ) )
			|| strstr( sound_name, ( "player\\footsteps" ) )
			|| strstr( sound_name, ( "items\\pickup_ammo" ) )
			|| strstr( sound_name, ( "grenade_throw" ) )
			|| strstr( sound_name, ( "items\\pickup_weapon" ) );
	};

	if( is_valid_sound( ) || ( ecx->sound_source > 0 && ecx->sound_source < 65 ) ) {
		auto origin = ecx->origin;
		if( std::strstr( sound_name, ( "weapons" ) ) || std::strstr( sound_name, ( "pickup_" ) ) || std::strstr( sound_name, ( "grenade_throw" ) ) )
			origin.z -= std::strstr( sound_name, ( "auto_semiauto_switch" ) ) ? 32.f : 64.f;

		Sounds_t sound;
		sound.m_sound_idx = ecx->sound_source;
		sound.m_sound_origin = origin;
		sound.m_sound_time = g_pGlobalVars->realtime;

		g_Vars.globals.m_vecSounds.push_back( sound );
	}*/

	if( g_ExtendedVisuals.m_arrOverridePlayers.at( ecx->sound_source ).m_eOverrideType <= EOverrideType::ESP_SHARED ) {
		g_ExtendedVisuals.m_arrOverridePlayers.at( ecx->sound_source ).m_eOverrideType = EOverrideType::ESP_SHARED;
		//g_ExtendedVisuals.m_arrOverridePlayers.at( ecx->sound_source ).m_flReceiveTime = g_pGlobalVars->realtime;
		g_ExtendedVisuals.m_arrOverridePlayers.at( ecx->sound_source ).m_vecLastOrigin = g_ExtendedVisuals.m_arrOverridePlayers.at( ecx->sound_source ).m_vecOrigin;
		g_ExtendedVisuals.m_arrOverridePlayers.at( ecx->sound_source ).m_vecOrigin = ecx->origin;
		g_ExtendedVisuals.m_arrOverridePlayers.at( ecx->sound_source ).m_bSus = true;
	}

	return ostart_sound_immediate( ecx );
}

// 55 8B EC 53 56 8B 75 ? 8B DA 8B 84 B1

using CRASHmyFuckingHackValveLOLFn = void( __thiscall * )( void *&renderList, IClientRenderable *pRenderable,
														   int iLeaf, int group, int nModelType, uint8 nAlphaModulation, bool bShadowDepthNoCache, bool bTwoPass );
CRASHmyFuckingHackValveLOLFn oCRASHmyFuckingHackValveLOL;

void __fastcall CRASHmyFuckingHackValveLOL( void *&renderList, void *edx, IClientRenderable *pRenderable,
											int iLeaf, int group, int nModelType, uint8 nAlphaModulation, bool bShadowDepthNoCache, bool bTwoPass ) {

	oCRASHmyFuckingHackValveLOL( renderList, pRenderable, iLeaf, group, nModelType, nAlphaModulation, bShadowDepthNoCache, bTwoPass );
}

Encrypted_t<IBaseClientDLL> g_pClient = nullptr;
Encrypted_t<IClientEntityList> g_pEntityList = nullptr;
Encrypted_t<IGameMovement> g_pGameMovement = nullptr;
Encrypted_t<IPrediction> g_pPrediction = nullptr;
Encrypted_t<IMoveHelper> g_pMoveHelper = nullptr;
Encrypted_t<IInput> g_pInput = nullptr;
Encrypted_t<CGlobalVars>  g_pGlobalVars = nullptr;
Encrypted_t<ISurface> g_pSurface = nullptr;
Encrypted_t<IVEngineClient> g_pEngine = nullptr;
Encrypted_t<IClientMode> g_pClientMode = nullptr;
Encrypted_t<ICVar> g_pCVar = nullptr;
Encrypted_t<IPanel> g_pPanel = nullptr;
Encrypted_t<IGameEventManager> g_pGameEvent = nullptr;
Encrypted_t<IVModelRender> g_pModelRender = nullptr;
Encrypted_t<IMaterialSystem> g_pMaterialSystem = nullptr;
Encrypted_t<ISteamClient> g_pSteamClient = nullptr;
Encrypted_t<ISteamGameCoordinator> g_pSteamGameCoordinator = nullptr;
Encrypted_t<ISteamMatchmaking> g_pSteamMatchmaking = nullptr;
Encrypted_t<ISteamUser> g_pSteamUser = nullptr;
Encrypted_t<ISteamFriends> g_pSteamFriends = nullptr;
Encrypted_t<IPhysicsSurfaceProps> g_pPhysicsSurfaceProps = nullptr;
Encrypted_t<IEngineTrace> g_pEngineTrace = nullptr;
Encrypted_t<CGlowObjectManager> g_pGlowObjectManager = nullptr;
Encrypted_t<IVModelInfo> g_pModelInfo = nullptr;
Encrypted_t<CClientState>  g_pClientState = nullptr;
Encrypted_t<IVDebugOverlay> g_pDebugOverlay = nullptr;
Encrypted_t<IEngineSound> g_pEngineSound = nullptr;
Encrypted_t<IMemAlloc> g_pMemAlloc = nullptr;
Encrypted_t<IViewRenderBeams> g_pViewRenderBeams = nullptr;
Encrypted_t<ILocalize> g_pLocalize = nullptr;
Encrypted_t<IStudioRender> g_pStudioRender = nullptr;
Encrypted_t<ICenterPrint> g_pCenterPrint = nullptr;
Encrypted_t<IVRenderView> g_pRenderView = nullptr;
Encrypted_t<IClientLeafSystem> g_pClientLeafSystem = nullptr;
Encrypted_t<IMDLCache> g_pMDLCache = nullptr;
Encrypted_t<IViewRender> g_pViewRender = nullptr;
Encrypted_t<IInputSystem> g_pInputSystem = nullptr;
Encrypted_t<INetGraphPanel> g_pNetGraphPanel = nullptr;
Encrypted_t<CCSGameRules> g_pGameRules = nullptr;
Encrypted_t<CFontManager> g_pFontManager = nullptr;
Encrypted_t<IWeaponSystem> g_pWeaponSystem = nullptr;
Encrypted_t<CSPlayerResource *> g_pPlayerResource = nullptr;
Encrypted_t<IVEffects> g_pEffects = nullptr;
Encrypted_t<CPhysicsCollision> g_pVPhysicsCollision = nullptr;
Encrypted_t<IServer> g_pServer = nullptr;
Encrypted_t<C_TEEffectDispatch> g_pTE_EffectDispatch = nullptr;
Encrypted_t<IEngineVGui> g_pEngineVGui = nullptr;
Encrypted_t<INetworkStringTableContainer> g_pNetworkStringTableContainer = nullptr;
Encrypted_t<SFHudDeathNoticeAndBotStatus> g_pDeathNotices = nullptr;
Encrypted_t<CHud> g_pHud = nullptr;

RecvPropHook::Shared m_flVelocityModifierSwap = nullptr;

namespace Interfaces {
	WNDPROC oldWindowProc;
	HWND hWindow = nullptr;

	bool Create( void *reserved ) {
	#ifdef DEV
		g_pPanel = ( IPanel * )CreateInterface( XorStr( "vgui2.dll" ), XorStr( "VGUI_Panel009" ) );
	#else
		g_pPanel = ( IPanel * )CreateInterface( loader_hash( XorStr( "vgui2.dll::VGUI_Panel009" ) ) );
	#endif
		if( !g_pPanel.IsValid( ) ) {
			return false;
		}

		g_pFontManager = *( CFontManager ** )( Memory::Scan( XorStr( "vguimatsurface.dll" ), XorStr( "74 1D 8B 0D ? ? ? ? 68" ), false ) + 0x4 );
		if( !g_pFontManager.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pSurface = ( ISurface * )CreateInterface( XorStr( "vguimatsurface.dll" ), XorStr( "VGUI_Surface031" ) );
	#else
		g_pSurface = ( ISurface * )CreateInterface( loader_hash( XorStr( "vguimatsurface.dll::VGUI_Surface031" ) ) );
	#endif
		if( !g_pSurface.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pClient = ( IBaseClientDLL * )CreateInterface( XorStr( "client.dll" ), XorStr( "VClient018" ) );
	#else
		g_pClient = ( IBaseClientDLL * )CreateInterface( loader_hash( XorStr( "client.dll::VClient018" ) ) );
	#endif
		if( !g_pClient.IsValid( ) ) {
			return false;
		}

		g_pGlobalVars = ( **reinterpret_cast< CGlobalVars *** >( ( *reinterpret_cast< uintptr_t ** >( g_pClient.Xor( ) ) )[ 11 ] + 10 ) );
		if( !g_pGlobalVars.IsValid( ) ) {
			return false;
		}


	#ifdef DEV
		g_pEngineVGui = ( IEngineVGui * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VEngineVGui001" ) );
	#else
		g_pEngineVGui = ( IEngineVGui * )CreateInterface( loader_hash( XorStr( "engine.dll::VEngineVGui001" ) ) );
	#endif
		if( !g_pEngineVGui.IsValid( ) ) {
			return false;
		}

	#ifdef MENU_DEV
		MH_Initialize( );

		if( MH_CreateHook( Memory::VCall<decltype( Hooked::oPaint )>( g_pEngineVGui.Xor( ), 14 ), &Hooked::EngineVGUI_Paint, ( LPVOID * )&Hooked::oPaint ) == MH_OK ) {
			MH_EnableHook( Memory::VCall<decltype( Hooked::oPaint )>( g_pEngineVGui.Xor( ), 14 ) );
		}
	#endif

		// HAHA INLINED COZ LOADER DOESN'T SUPPORT IT YET
		auto SERVERDLL = GetModuleHandleA( XorStr( "server.dll" ) );
		if( !SERVERDLL )
			return false;

		auto SERVERDLL_CREATEINTERFACE = ( CreateInterfaceFn )( GetProcAddress( SERVERDLL, XorStr( "CreateInterface" ) ) );
		if( !SERVERDLL_CREATEINTERFACE )
			return false;

		g_pServer = ( IServer * )SERVERDLL_CREATEINTERFACE( XorStr( "ServerGameDLL005" ), nullptr );

		if( !g_pServer.IsValid( ) )
			return false;

		if( !Engine::g_PropManager.Create( g_pClient.Xor( ) ) ) {
			return false;
		}

	#ifdef DEV
		g_pEntityList = ( IClientEntityList * )CreateInterface( XorStr( "client.dll" ), XorStr( "VClientEntityList003" ) );
	#else
		g_pEntityList = ( IClientEntityList * )CreateInterface( loader_hash( XorStr( "client.dll::VClientEntityList003" ) ) );
	#endif
		if( !g_pEntityList.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pGameMovement = ( IGameMovement * )CreateInterface( XorStr( "client.dll" ), XorStr( "GameMovement001" ) );
	#else
		g_pGameMovement = ( IGameMovement * )CreateInterface( loader_hash( XorStr( "client.dll::GameMovement001" ) ) );
	#endif
		if( !g_pGameMovement.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pPrediction = ( IPrediction * )CreateInterface( XorStr( "client.dll" ), XorStr( "VClientPrediction001" ) );
	#else
		g_pPrediction = ( IPrediction * )CreateInterface( loader_hash( XorStr( "client.dll::VClientPrediction001" ) ) );
	#endif
		if( !g_pPrediction.IsValid( ) ) {
			return false;
		}

		g_pInput = *reinterpret_cast< IInput ** > ( ( *reinterpret_cast< uintptr_t ** >( g_pClient.Xor( ) ) )[ 15 ] + 0x1 );
		if( !g_pInput.IsValid( ) ) {
			return false;
		}

		g_pWeaponSystem = *( IWeaponSystem ** )( Memory::Scan( XorStr( "client.dll" ), XorStr( "8B 35 ? ? ? ? FF 10 0F B7 C0" ) ) + 2 );
		if( !g_pWeaponSystem.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pEngine = ( IVEngineClient * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VEngineClient014" ) );
	#else
		g_pEngine = ( IVEngineClient * )CreateInterface( loader_hash( XorStr( "engine.dll::VEngineClient014" ) ) );
	#endif
		if( !g_pEngine.IsValid( ) ) {
			return false;
		}

		g_pClientMode = **( IClientMode *** )( ( *( DWORD ** )g_pClient.Xor( ) )[ 10 ] + 0x5 );
		if( !g_pClientMode.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pCVar = ( ICVar * )CreateInterface( XorStr( "vstdlib.dll" ), XorStr( "VEngineCvar007" ) );
	#else
		g_pCVar = ( ICVar * )CreateInterface( loader_hash( XorStr( "vstdlib.dll::VEngineCvar007" ) ) );
	#endif
		if( !g_pCVar.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pGameEvent = ( IGameEventManager * )CreateInterface( XorStr( "engine.dll" ), XorStr( "GAMEEVENTSMANAGER002" ) );
	#else
		g_pGameEvent = ( IGameEventManager * )CreateInterface( loader_hash( XorStr( "engine.dll::GAMEEVENTSMANAGER002" ) ) );
	#endif
		if( !g_pGameEvent.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pModelRender = ( IVModelRender * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VEngineModel016" ) );
	#else
		g_pModelRender = ( IVModelRender * )CreateInterface( loader_hash( XorStr( "engine.dll::VEngineModel016" ) ) );
	#endif
		if( !g_pModelRender.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pMaterialSystem = ( IMaterialSystem * )CreateInterface( XorStr( "materialsystem.dll" ), XorStr( "VMaterialSystem080" ) );
	#else
		g_pMaterialSystem = ( IMaterialSystem * )CreateInterface( loader_hash( XorStr( "materialsystem.dll::VMaterialSystem080" ) ) );
	#endif
		if( !g_pMaterialSystem.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pPhysicsSurfaceProps = ( IPhysicsSurfaceProps * )CreateInterface( XorStr( "vphysics.dll" ), XorStr( "VPhysicsSurfaceProps001" ) );
	#else
		g_pPhysicsSurfaceProps = ( IPhysicsSurfaceProps * )CreateInterface( loader_hash( XorStr( "vphysics.dll::VPhysicsSurfaceProps001" ) ) );
	#endif
		if( !g_pPhysicsSurfaceProps.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pEngineTrace = ( IEngineTrace * )CreateInterface( XorStr( "engine.dll" ), XorStr( "EngineTraceClient004" ) );
	#else
		g_pEngineTrace = ( IEngineTrace * )CreateInterface( loader_hash( XorStr( "engine.dll::EngineTraceClient004" ) ) );
	#endif
		if( !g_pEngineTrace.IsValid( ) ) {
			return false;
		}

		//	g_Log.Log( XorStr( ".nap" ), XorStr( "val" ) );
		if( !Engine::CreateDisplacement( reserved ) ) {
			return false;
		}

		//g_Log.Log( XorStr( ".nap" ), XorStr( "val done" ) );

		g_pMoveHelper = ( IMoveHelper * )( Engine::Displacement.Data.m_uMoveHelper );
		if( !g_pMoveHelper.IsValid( ) ) {
			return false;
		}

		g_pGlowObjectManager = ( CGlowObjectManager * )Engine::Displacement.Data.m_uGlowObjectManager;
		if( !g_pGlowObjectManager.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pModelInfo = ( IVModelInfo * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VModelInfoClient004" ) );
	#else
		g_pModelInfo = ( IVModelInfo * )CreateInterface( loader_hash( XorStr( "engine.dll::VModelInfoClient004" ) ) );
	#endif
		if( !g_pModelInfo.IsValid( ) ) {
			return false;
		}

		// A1 FC BC 58 10  mov eax, g_ClientState
		g_pClientState = Encrypted_t<CClientState>( **( CClientState *** )( ( *( std::uintptr_t ** )g_pEngine.Xor( ) )[ 14 ] + 0x1 ) );
		if( !g_pClientState.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pDebugOverlay = ( IVDebugOverlay * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VDebugOverlay004" ) );
	#else
		g_pDebugOverlay = ( IVDebugOverlay * )CreateInterface( loader_hash( XorStr( "engine.dll::VDebugOverlay004" ) ) );
	#endif
		if( !g_pDebugOverlay.IsValid( ) ) {
			return false;
		}

		g_pMemAlloc = *( IMemAlloc ** )( GetProcAddress( GetModuleHandle( XorStr( "tier0.dll" ) ), XorStr( "g_pMemAlloc" ) ) );
		if( !g_pMemAlloc.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pEngineSound = ( IEngineSound * )CreateInterface( XorStr( "engine.dll" ), XorStr( "IEngineSoundClient003" ) );
	#else
		g_pEngineSound = ( IEngineSound * )CreateInterface( loader_hash( XorStr( "engine.dll::IEngineSoundClient003" ) ) );
	#endif
		if( !g_pEngineSound.IsValid( ) ) {
			return false;
		}

		g_pViewRenderBeams = *( IViewRenderBeams ** )( Engine::Displacement.Data.m_uRenderBeams );
		if( !g_pViewRenderBeams.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pLocalize = ( ILocalize * )CreateInterface( XorStr( "localize.dll" ), XorStr( "Localize_001" ) );
	#else
		g_pLocalize = ( ILocalize * )CreateInterface( loader_hash( XorStr( "localize.dll::Localize_001" ) ) );
	#endif
		if( !g_pLocalize.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pStudioRender = ( IStudioRender * )CreateInterface( XorStr( "studiorender.dll" ), XorStr( "VStudioRender026" ) );
	#else
		g_pStudioRender = ( IStudioRender * )CreateInterface( loader_hash( XorStr( "studiorender.dll::VStudioRender026" ) ) );
	#endif
		if( !g_pStudioRender.IsValid( ) ) {
			return false;
		}

		g_pCenterPrint = *( ICenterPrint ** )( Engine::Displacement.Data.m_uCenterPrint );
		// unused, so this should not fuck us up if it gets outdated
	//	if( !g_pCenterPrint.IsValid( ) ) {
	//		return false;
	//	}

	#ifdef DEV
		g_pRenderView = ( IVRenderView * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VEngineRenderView014" ) );
	#else
		g_pRenderView = ( IVRenderView * )CreateInterface( loader_hash( XorStr( "engine.dll::VEngineRenderView014" ) ) );
	#endif
		if( !g_pRenderView.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pNetworkStringTableContainer = ( INetworkStringTableContainer * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VEngineClientStringTable001" ) );
	#else
		g_pNetworkStringTableContainer = ( INetworkStringTableContainer * )CreateInterface( loader_hash( XorStr( "engine.dll::VEngineClientStringTable001" ) ) );
	#endif
		if( !g_pNetworkStringTableContainer.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pClientLeafSystem = ( IClientLeafSystem * )CreateInterface( XorStr( "client.dll" ), XorStr( "ClientLeafSystem002" ) );
	#else
		g_pClientLeafSystem = ( IClientLeafSystem * )CreateInterface( loader_hash( XorStr( "client.dll::ClientLeafSystem002" ) ) );
	#endif
		if( !g_pClientLeafSystem.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pMDLCache = ( IMDLCache * )CreateInterface( XorStr( "datacache.dll" ), XorStr( "MDLCache004" ) );
	#else
		g_pMDLCache = ( IMDLCache * )CreateInterface( loader_hash( XorStr( "datacache.dll::MDLCache004" ) ) );
	#endif
		if( !g_pMDLCache.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pInputSystem = ( IInputSystem * )CreateInterface( XorStr( "inputsystem.dll" ), XorStr( "InputSystemVersion001" ) );
	#else
		g_pInputSystem = ( IInputSystem * )CreateInterface( loader_hash( XorStr( "inputsystem.dll::InputSystemVersion001" ) ) );
	#endif
		if( !g_pInputSystem.IsValid( ) ) {
			return false;
		}

	#ifdef DEV
		g_pVPhysicsCollision = ( CPhysicsCollision * )CreateInterface( XorStr( "vphysics.dll" ), XorStr( "VPhysicsCollision007" ) );
	#else
		g_pVPhysicsCollision = ( CPhysicsCollision * )CreateInterface( loader_hash( XorStr( "vphysics.dll::VPhysicsCollision007" ) ) );
	#endif
		if( !g_pVPhysicsCollision.IsValid( ) ) {
			return false;
		}

		g_pViewRender = **( IViewRender *** )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 50 4C 8B 06 8D 4D F4" ) ) - 6 );
		if( !g_pViewRender.IsValid( ) ) {
			return false;
		}

		g_pTE_EffectDispatch = *( C_TEEffectDispatch ** )( Memory::Scan( XorStr( "client.dll" ), XorStr( "B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 55 8B EC 83 E4 F8 83 EC 20" ) ) + 1 );
		if( !g_pTE_EffectDispatch.IsValid( ) ) {
			return false;
		}

		g_pHud = *( CHud ** )( Memory::Scan( XorStr( "client.dll" ), XorStr( "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08" ) ) + 1 );
		if( !g_pHud.IsValid( ) )
			return false;

		g_pDeathNotices = g_pHud->FindHudElement< SFHudDeathNoticeAndBotStatus * >( XorStr( "SFHudDeathNoticeAndBotStatus" ) );
		if( !g_pDeathNotices.IsValid( ) )
			return false;

	#ifdef DEV
		g_pEffects = ( IVEffects * )CreateInterface( XorStr( "engine.dll" ), XorStr( "VEngineEffects001" ) );
	#else
		g_pEffects = ( IVEffects * )CreateInterface( loader_hash( XorStr( "engine.dll::VEngineEffects001" ) ) );
	#endif

		auto D3DDevice9 = **( IDirect3DDevice9 *** )Engine::Displacement.Data.m_D3DDevice;
		if( !D3DDevice9 ) {
			return false;
		}


		//g_Log.Log( XorStr( ".nap" ), XorStr( "done" ) );
		if( !g_InputSystem.Initialize( D3DDevice9 ) ) {
			return false;
		}

		Memory::hTheFuckingWindow = g_InputSystem.m_hTargetWindow;

		auto pSteamAPI = GetModuleHandleA( XorStr( "steam_api.dll" ) );
		g_pSteamClient = ( ( ISteamClient * ( __cdecl * )( void ) )GetProcAddress( pSteamAPI, XorStr( "SteamClient" ) ) )( );
		if( !g_pSteamClient.IsValid( ) ) {
			return false;
		}

		HSteamUser hSteamUser = reinterpret_cast< HSteamUser( __cdecl * ) ( void ) >( GetProcAddress( pSteamAPI, XorStr( "SteamAPI_GetHSteamUser" ) ) )( );
		HSteamPipe hSteamPipe = reinterpret_cast< HSteamPipe( __cdecl * ) ( void ) >( GetProcAddress( pSteamAPI, XorStr( "SteamAPI_GetHSteamPipe" ) ) )( );
		g_pSteamGameCoordinator = ( ISteamGameCoordinator * )g_pSteamClient->GetISteamGenericInterface( hSteamUser, hSteamPipe, XorStr( "SteamGameCoordinator001" ) );
		if( !g_pSteamGameCoordinator.IsValid( ) ) {
			return false;
		}

		g_pSteamUser = g_pSteamClient->GetISteamUser( hSteamUser, hSteamPipe, XorStr("SteamUser019" ) );
		if( !g_pSteamUser.IsValid( ) ) {
			return false;
		}

		BASS::bass_lib_handle = BASS::bass_lib.LoadFromMemory( bass_dll_image, sizeof( bass_dll_image ) );

		if( BASS_INIT_ONCE( ) )
			BASS::bass_init = TRUE;

	#ifndef MENU_DEV
		g_GameEvent.Register( );
	#endif

		g_KitParser.Initialize( );

	#ifndef MENU_DEV
		g_SkinChanger.Create( );
	#endif

		for( auto clientclass = g_pClient->GetAllClasses( );
			 clientclass != nullptr;
			 clientclass = clientclass->m_pNext ) {

			if( !strcmp( clientclass->m_pNetworkName, XorStr( "CPlayerResource" ) ) ) {
				RecvTable *pClassTable = clientclass->m_pRecvTable;

				for( int nIndex = 0; nIndex < pClassTable->m_nProps; nIndex++ ) {
					RecvProp *pProp = &pClassTable->m_pProps[ nIndex ];

					if( !pProp || strcmp( pProp->m_pVarName, XorStr( "m_iTeam" ) ) )
						continue;

					g_pPlayerResource = Encrypted_t<CSPlayerResource *>( *reinterpret_cast< CSPlayerResource *** >( std::uintptr_t( pProp->m_pDataTable->m_pProps->m_ProxyFn ) + 0x10 ) );
					break;
				}

				continue;
			}
		}

		// init config system
		g_Vars.Create( );
	#ifndef MENU_DEV
		// setup variable stuff
		g_Vars.viewmodel_fov->fnChangeCallback.m_Size = 0;
		g_Vars.viewmodel_offset_x->fnChangeCallback.m_Size = 0;
		g_Vars.viewmodel_offset_y->fnChangeCallback.m_Size = 0;
		g_Vars.viewmodel_offset_z->fnChangeCallback.m_Size = 0;

		g_Vars.mat_ambient_light_r->fnChangeCallback.m_Size = 0;
		g_Vars.mat_ambient_light_g->fnChangeCallback.m_Size = 0;
		g_Vars.mat_ambient_light_b->fnChangeCallback.m_Size = 0;

		g_Vars.sv_showlagcompensation_duration->fnChangeCallback.m_Size = 0;
		g_Vars.sv_showlagcompensation_duration->nFlags &= ~FCVAR_CHEAT;

		g_Vars.sv_showimpacts_time->fnChangeCallback.m_Size = 0;
		g_Vars.sv_showimpacts_time->nFlags &= ~FCVAR_CHEAT;

		g_Vars.name->fnChangeCallback.m_Size = 0;

		//// disable extrapolation, i guess
		g_Vars.cl_extrapolate->fnChangeCallback.m_Size = 0;
		g_Vars.cl_extrapolate->SetValue( 0 );

		// no foot shadows on inject only cos kys scythe
		// g_Vars.cl_foot_contact_shadows->fnChangeCallback.m_Size = 0;
		// g_Vars.cl_foot_contact_shadows->SetValue( 0 );

		// setup proxy hooks
		RecvProp *prop = nullptr;

		Engine::g_PropManager.GetProp( XorStr( "DT_CSPlayer" ), XorStr( "m_flVelocityModifier" ), &prop );
		m_flVelocityModifierSwap = std::make_shared<RecvPropHook>( prop, &Hooked::m_flVelocityModifier );

		oSendDatagram = Hooked::HooksManager.CreateHook<decltype( oSendDatagram ) >( &SendDatagram, ( void * )Engine::Displacement.Data.SendDatagram );

		// vmt hooks
		// oTE_EffectDispatch_PostDataUpdate = Hooked::HooksManager.HookVirtual<decltype( oTE_EffectDispatch_PostDataUpdate )>( g_pTE_EffectDispatch.Xor( ), &TE_EffectDispatch_PostDataUpdate, 7 );
		oPaintTraverse = Hooked::HooksManager.HookVirtual<decltype( oPaintTraverse )>( g_pPanel.Xor( ), &PaintTraverse, 41 );
		Hooked::oFireEvents = Hooked::HooksManager.HookVirtual<decltype( Hooked::oFireEvents )>( g_pEngine.Xor( ), &Hooked::FireEvents, 59 );
		Hooked::oGetScreenAspectRatio = Hooked::HooksManager.HookVirtual<decltype( Hooked::oGetScreenAspectRatio )>( g_pEngine.Xor( ), &Hooked::GetScreenAspectRatio, Index::EngineClient::GetScreenAspectRatio );
		Hooked::oIsBoxVisible = Hooked::HooksManager.HookVirtual<decltype( Hooked::oIsBoxVisible )>( g_pEngine.Xor( ), &Hooked::IsBoxVisible, 32 );

		Hooked::oListLeavesInBox = Hooked::HooksManager.HookVirtual<decltype( Hooked::oListLeavesInBox )>( g_pEngine.Xor( )->GetBSPTreeQuery( ), &Hooked::ListLeavesInBox, Index::BSPTreeQuery::ListLeavesInBox );

		Hooked::oIsHltv = Hooked::HooksManager.HookVirtual<decltype( Hooked::oIsHltv )>( g_pEngine.Xor( ), &Hooked::IsHltv, Index::EngineClient::IsHltv );
		Hooked::oIsConnected = Hooked::HooksManager.HookVirtual<decltype( Hooked::oIsConnected )>( g_pEngine.Xor( ), &Hooked::IsConnected, 27 );

		//Hooked::oDispatchUserMessage = Hooked::HooksManager.HookVirtual<decltype( Hooked::oDispatchUserMessage )>( g_pClient.Xor( ), &Hooked::DispatchUserMessage, 38 );
		Hooked::oFrameStageNotify = Hooked::HooksManager.HookVirtual<decltype( Hooked::oFrameStageNotify )>( g_pClient.Xor( ), &Hooked::FrameStageNotify, Index::IBaseClientDLL::FrameStageNotify );

		// 11
		Hooked::oCreateMove = Hooked::HooksManager.HookVirtual<decltype( Hooked::oCreateMove )>( g_pClientMode.Xor( ), &Hooked::CreateMove, Index::CClientModeShared::CreateMove );
		Hooked::oDoPostScreenEffects = Hooked::HooksManager.HookVirtual<decltype( Hooked::oDoPostScreenEffects )>( g_pClientMode.Xor( ), &Hooked::DoPostScreenEffects, Index::CClientModeShared::DoPostScreenSpaceEffects );
		Hooked::oOverrideView = Hooked::HooksManager.HookVirtual<decltype( Hooked::oOverrideView )>( g_pClientMode.Xor( ), &Hooked::OverrideView, Index::CClientModeShared::OverrideView );

		//Hooked::oDrawSetColor = Hooked::HooksManager.HookVirtual<decltype( Hooked::oDrawSetColor )>( g_pSurface.Xor( ), &Hooked::DrawSetColor, 15 );
		Hooked::oLockCursor = Hooked::HooksManager.HookVirtual<decltype( Hooked::oLockCursor )>( g_pSurface.Xor( ), &Hooked::LockCursor, Index::VguiSurface::LockCursor );

		Hooked::oBeginFrame = Hooked::HooksManager.HookVirtual<decltype( Hooked::oBeginFrame )>( g_pMaterialSystem.Xor( ), &Hooked::BeginFrame, Index::MatSystem::BeginFrame );
		Hooked::oOverrideConfig = Hooked::HooksManager.HookVirtual<decltype( Hooked::oOverrideConfig )>( g_pMaterialSystem.Xor( ), &Hooked::OverrideConfig, 21 );

		MH_Initialize( );

		if( MH_CreateHook( Memory::VCall<decltype( Hooked::oPaint )>( g_pEngineVGui.Xor( ), 14 ), &Hooked::EngineVGUI_Paint, ( LPVOID * )&Hooked::oPaint ) == MH_OK ) {
			MH_EnableHook( Memory::VCall<decltype( Hooked::oPaint )>( g_pEngineVGui.Xor( ), 14 ) );
		}

		//Hooked::oEmitSound = Hooked::HooksManager.HookVirtual<decltype( Hooked::oEmitSound )>( g_pEngineSound.Xor( ), &Hooked::EmitSound, 5 );
		Hooked::oRunCommand = Hooked::HooksManager.HookVirtual<decltype( Hooked::oRunCommand )>( g_pPrediction.Xor( ), &Hooked::RunCommand, Index::IPrediction::RunCommand );

		Hooked::oProcessMovement = Hooked::HooksManager.HookVirtual<decltype( Hooked::oProcessMovement )>( g_pGameMovement.Xor( ), &Hooked::ProcessMovement, Index::IGameMovement::ProcessMovement );

		//Hooked::oSetupMove = Hooked::HooksManager.HookVirtual<decltype( Hooked::oSetupMove )>( g_pPrediction.Xor( ), &Hooked::SetupMove, 20 );

		Hooked::oClipRayCollideable = Hooked::HooksManager.HookVirtual<decltype( Hooked::oClipRayCollideable )>( g_pEngineTrace.Xor( ), &Hooked::ClipRayCollideable, 4 );
		//Hooked::oTraceRay = Hooked::HooksManager.HookVirtual<decltype( Hooked::oTraceRay )>( g_pEngineTrace.Xor( ), &Hooked::TraceRay, 5 );

		Hooked::oDrawModelExecute = Hooked::HooksManager.HookVirtual<decltype( Hooked::oDrawModelExecute )>( g_pModelRender.Xor( ), &Hooked::DrawModelExecute, Index::ModelDraw::DrawModelExecute );
		//	Hooked::oSetColorModulation = Hooked::HooksManager.HookVirtual<decltype( Hooked::oSetColorModulation )>( g_pStudioRender.Xor( ), &Hooked::SetColorModulation, 27 );

			//Hooked::oAddBoxOverlay = Hooked::HooksManager.HookVirtual<decltype( Hooked::oAddBoxOverlay )>( g_pDebugOverlay.Xor( ), &Hooked::AddBoxOverlay, 1 );

			//Hooked::oRetrieveMessage = Hooked::HooksManager.HookVirtual<decltype( Hooked::oRetrieveMessage )>( g_pSteamGameCoordinator.Xor( ), &Hooked::RetrieveMessage, 2 );

			//Hooked::oWriteUsercmdDeltaToBuffer = Hooked::HooksManager.HookVirtual<decltype( Hooked::oWriteUsercmdDeltaToBuffer )>( g_pClient.Xor( ), &Hooked::WriteUsercmdDeltaToBuffer, 24 );


		// detours
		auto CalcViewBobAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC A1 ? ? ? ? 83 EC 10 56 8B F1 B9" ) );
		Hooked::oCalcViewBob = Hooked::HooksManager.CreateHook<decltype( Hooked::oCalcViewBob ) >( &Hooked::CalcViewBob, ( void * )CalcViewBobAddr );

		auto CalcView = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 53 8B 5D 08 56 57 FF 75 18 8B F1" ) );
		Hooked::oCalcView = Hooked::HooksManager.CreateHook<decltype( Hooked::oCalcView )>( &Hooked::CalcView, ( void * )CalcView );

		auto CheckAchievementsEnabledAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "A1 ? ? ? ? 56 8B F1 B9 ? ? ? ? FF 50 34 85 C0 0F 85" ) );
		Hooked::oCheckAchievementsEnabled = Hooked::HooksManager.CreateHook<decltype( Hooked::oCheckAchievementsEnabled ) >( &Hooked::CheckAchievementsEnabled, ( void * )CheckAchievementsEnabledAddr );

		auto EstimateAbsVelocityAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 ? 83 EC ? 56 8B F1 85 F6 74 ? 8B 06 8B 80 ? ? ? ? FF D0 84 C0 74 ? 8A 86" ) );
		oEstimateAbsVelocity = Hooked::HooksManager.CreateHook<decltype( oEstimateAbsVelocity ) >( &EstimateAbsVelocity, ( void * )EstimateAbsVelocityAddr );

		//auto CRASHmyFuckingHackValveLOLAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 53 56 8B 75 ? 8B DA 8B 84 B1" ) );
		//oCRASHmyFuckingHackValveLOL = Hooked::HooksManager.CreateHook<decltype( oCRASHmyFuckingHackValveLOL ) >( &CRASHmyFuckingHackValveLOL, ( void * )CRASHmyFuckingHackValveLOLAddr );

		//#ifdef DEV
		//	auto ReportHitAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 8B 55 08 83 EC 1C F6 42 1C 01" ) );
		//#else
		//	auto ReportHitAddr = Memory::GetAddress( loader_hash( XorStr( "55 8B EC 8B 55 08 83 EC 1C F6 42 1C 01" ) ) );
		//#endif
			//Hooked::oReportHit = Hooked::HooksManager.CreateHook<decltype( Hooked::oReportHit ) >( &Hooked::ReportHit, ( void * )ReportHitAddr );

		auto CL_MoveAddr = Memory::Scan( XorStr( "engine.dll" ), XorStr( "55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D ? ? ? ? 8A" ) );
		Hooked::oCL_Move = Hooked::HooksManager.CreateHook<decltype( Hooked::oCL_Move ) >( &Hooked::CL_Move, ( void * )CL_MoveAddr );

		auto ModifyEyePositionAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 8B 06 8B CE FF 90 ? ? ? ? 85 C0 74 4E" ) ) );
		Hooked::oModifyEyePosition = Hooked::HooksManager.CreateHook<decltype( Hooked::oModifyEyePosition ) >( &Hooked::ModifyEyePosition, ( void * )ModifyEyePositionAddr );

		auto PhysicsSimulateAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "56 8B F1 8B 8E ? ? ? ? 83 F9 FF 74 21" ) );
		Hooked::oPhysicsSimulate = Hooked::HooksManager.CreateHook<decltype( Hooked::oPhysicsSimulate ) >( &Hooked::PhysicsSimulate, ( void * )PhysicsSimulateAddr );

		//auto TeleportedAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "8B 91 ? ? ? ? 83 FA ? 74 ? 0F B7 C2 C1 E0 ? 05 ? ? ? ? C1 EA ? 39 50 ? 75 ? 8B 00 EB ? 33 C0 56" ) );
		//oTeleported = Hooked::HooksManager.CreateHook<decltype( oTeleported ) >( &Teleported, ( void * )TeleportedAddr );

		auto DoProceduralFootPlantAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F0 83 EC 78 56 8B F1 57 8B 56" ) );
		Hooked::oDoProceduralFootPlant = Hooked::HooksManager.CreateHook<decltype( Hooked::oDoProceduralFootPlant )>( &Hooked::DoProceduralFootPlant, ( void * )DoProceduralFootPlantAddr );

		auto IsUsingStaticPropDebugModeAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "engine.dll" ), XorStr( "E8 ? ? ? ? 84 C0 8B 45 08" ) ) );
		Hooked::oIsUsingStaticPropDebugMode = Hooked::HooksManager.CreateHook<decltype( Hooked::oIsUsingStaticPropDebugMode ) >( &Hooked::IsUsingStaticPropDebugMode, ( void * )IsUsingStaticPropDebugModeAddr );

		auto UpdateClientSideAnimationAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 51 56 8B F1 80 BE ? ? ? ? ? 74 36" ) );
		Hooked::oUpdateClientSideAnimation = Hooked::HooksManager.CreateHook<decltype( Hooked::oUpdateClientSideAnimation ) >( &Hooked::UpdateClientSideAnimation, ( void * )UpdateClientSideAnimationAddr );

		//	auto standard_blending_rules_adr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6" ) );
		//	Hooked::oStandardBlendingRules = Hooked::HooksManager.CreateHook<decltype( Hooked::oStandardBlendingRules ) >( &Hooked::StandardBlendingRules, ( void * )standard_blending_rules_adr );

			//auto oCBoneSnapshot__UpdateAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC A1 ?? ?? ?? ?? 0F 57 DB" ) );
			//oCBoneSnapshot__Update = Hooked::HooksManager.CreateHook<decltype( oCBoneSnapshot__Update ) >( &CBoneSnapshot__Update, ( void * )oCBoneSnapshot__UpdateAddr );


	#ifdef DEV
		//auto VertexBufferLockAddr = Memory::Scan( XorStr( "shaderapidx9.dll" ), XorStr( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 57" ) );
	#else
		//auto VertexBufferLockAddr = Memory::GetAddress( loader_hash( XorStr( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 57" ) ) );
	#endif
		//Hooked::oVertexBufferLock = Hooked::HooksManager.CreateHook<decltype( Hooked::oVertexBufferLock ) >( &Hooked::VertexBufferLock, ( void * )VertexBufferLockAddr );

		//auto IsBoneAvailableAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? 8D 44 24 18" ), false ) );
		//oIsBoneAvailable = Hooked::HooksManager.CreateHook<decltype( oIsBoneAvailable )>( &IsBoneAvailable, ( void * )IsBoneAvailableAddr );

	/*#ifdef DEV
		auto CalcRenderableWorldSpaceAABB_BloatedAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? F3 0F 10 47 ? 0F 2E 45 D8" ) ) );
	#else
		auto CalcRenderableWorldSpaceAABB_BloatedAddr = Memory::CallableFromRelative( Memory::GetAddress( loader_hash( XorStr( "E8 ? ? ? ? F3 0F 10 47 ? 0F 2E 45 D8" ) ) ) );
	#endif
		Hooked::oCalcRenderableWorldSpaceAABB_Bloated = Hooked::HooksManager.CreateHook<decltype( Hooked::oCalcRenderableWorldSpaceAABB_Bloated ) >(
			&Hooked::CalcRenderableWorldSpaceAABB_Bloated, ( void * )CalcRenderableWorldSpaceAABB_BloatedAddr );*/

		auto ClientStateAddr = ( void * )( uintptr_t( g_pClientState.Xor( ) ) + 0x8 );
		Hooked::oPacketStart = Hooked::HooksManager.HookVirtual<decltype( Hooked::oPacketStart )>( ClientStateAddr, &Hooked::PacketStart, 5 );
		Hooked::oPacketEnd = Hooked::HooksManager.HookVirtual<decltype( Hooked::oPacketEnd )>( ClientStateAddr, &Hooked::PacketEnd, 6 );
		//Hooked::oProcessTempEntities = Hooked::HooksManager.HookVirtual<decltype( Hooked::oProcessTempEntities )>( ClientStateAddr, &Hooked::ProcessTempEntities, 36 );;
		oSVCMsg_VoiceData = Hooked::HooksManager.HookVirtual<decltype( oSVCMsg_VoiceData )>( ClientStateAddr, &SVCMsg_VoiceData, 24 );

		Hooked::oSendNetMsg = Hooked::HooksManager.CreateHook<decltype( Hooked::oSendNetMsg ) >( &Hooked::SendNetMsg, ( void * )Engine::Displacement.Data.m_SendNetMsg );

		Hooked::oInterpolateServerEntities = Hooked::HooksManager.CreateHook<decltype( Hooked::oInterpolateServerEntities ) >( &Hooked::InterpolateServerEntities, ( void * )Engine::Displacement.Data.m_InterpolateServerEntities );
		Hooked::oProcessInterpolatedList = Hooked::HooksManager.CreateHook<decltype( Hooked::oProcessInterpolatedList ) >( &Hooked::ProcessInterpolatedList, ( void * )Engine::Displacement.Data.m_ProcessInterpolatedList );

		//Hooked::oIsRenderableInPvs = Hooked::HooksManager.HookVirtual<decltype( Hooked::oIsRenderableInPvs ) >( g_pClientLeafSystem.Xor( ), &Hooked::IsRenderableInPVS, 8 );

		//oGetModelIndex = Hooked::HooksManager.HookVirtual<decltype( oGetModelIndex ) >( g_pModelInfo.Xor( ), &GetModelIndex, 2 );

		Hooked::oProcessPacket = Hooked::HooksManager.CreateHook<decltype( Hooked::oProcessPacket )>( &Hooked::ProcessPacket, ( void * )Engine::Displacement.Data.ProcessPacket );

	#ifdef DEV
		//auto GetItemNameAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 80 7D 08 00 B8 ? ? ? ? 56" ) );
	#else
		//	auto GetItemNameAddr = Memory::GetAddress( loader_hash( XorStr( "55 8B EC 80 7D 08 00 B8 ? ? ? ? 56" ) ) );
	#endif
	//	Hooked::oGetItemName = Hooked::HooksManager.CreateHook<decltype( Hooked::oGetItemName )>( &Hooked::GetItemName, ( void * )GetItemNameAddr );

		static auto CHudScope__Paintaddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 83 EC 78 56 57 8B 3D" ) );
		oCHudScope__Paint = Hooked::HooksManager.CreateHook<decltype( oCHudScope__Paint )>( &CHudScope__Paint, ( void * )CHudScope__Paintaddr );

		Hooked::oIsPaused = Hooked::HooksManager.HookVirtual<decltype( Hooked::oIsPaused )>( g_pEngine.Xor( ), &Hooked::IsPaused, 90 );

		oGetVCollide = Hooked::HooksManager.HookVirtual<decltype( oGetVCollide )>( g_pModelInfo.Xor( ), &GetVCollide, 4 );

		auto CanUnduckAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 EC 78 57" ) );
		oCanUnduck = Hooked::HooksManager.CreateHook<decltype( oCanUnduck )>( &CanUnduck, ( void * )CanUnduckAddr );

		//auto shitaddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 83 EC 0C 53 56 8B F1 57 83 BE" ) );
		//Hooked::oInterpolateViewmodel = Hooked::HooksManager.CreateHook<decltype( Hooked::oInterpolateViewmodel )>( &Hooked::InterpolateViewmodel, ( void * )shitaddr );

		auto RenderLineAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 83 C4 10 EB 79" ) ) );
		oRenderLine = Hooked::HooksManager.CreateHook<decltype( oRenderLine )>( &RenderLine, ( void * )RenderLineAddr );

		auto __MsgFunc_ReloadEffectAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 8B 4D 08 83 EC 0C 8B 51 08" ) );
		o__MsgFunc_ReloadEffect = Hooked::HooksManager.CreateHook<decltype( o__MsgFunc_ReloadEffect )>( &__MsgFunc_ReloadEffect, ( void * )__MsgFunc_ReloadEffectAddr );

		//auto DestroyOuterParticlePrecipAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? F3 0F 10 4D ? 8D 04 7F" ) ) );
		//oDestroyOuterParticlePrecip = Hooked::HooksManager.CreateHook<decltype( oDestroyOuterParticlePrecip )>( &DestroyOuterParticlePrecip, ( void* )DestroyOuterParticlePrecipAddr );

		//auto SpawnAddr = Memory::Scan( XorStr( "server.dll" ), XorStr( "55 8B EC 51 53 56 57 6A 20" ) );
		//oSpawn = Hooked::HooksManager.CreateHook<decltype( oSpawn )>( &Spawn, ( void* )SpawnAddr );

		//auto C_BasePlayer__PhysicsSimulate_Server_func = Memory::Scan( "server.dll", "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 8B F1 8B 0D ? ? ? ? 57 89 74 24 34" );
		//oC_BasePlayer__PhysicsSimulate_Server = Hooked::HooksManager.CreateHook<decltype( oC_BasePlayer__PhysicsSimulate_Server )>( &C_BasePlayer__PhysicsSimulate_Server, ( void* )C_BasePlayer__PhysicsSimulate_Server_func );

		auto CL_FireEventsAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "engine.dll" ), XorStr( "E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? 8B 0D" ) ) );
		oCL_FireEvents = Hooked::HooksManager.CreateHook<decltype( oCL_FireEvents )>( &CL_FireEvents, reinterpret_cast< void * >( CL_FireEventsAddr ) );

		auto setupbonesadd = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ) );
		Hooked::oSetupBones = Hooked::HooksManager.CreateHook<decltype( Hooked::oSetupBones ) >( &Hooked::SetupBones, reinterpret_cast< void * >( setupbonesadd ) );

		auto DoExtraBoneProcessingAdd = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 8B F1 57 89 74 24 1C" ) );
		Hooked::oDoExtraBoneProcessing = Hooked::HooksManager.CreateHook<decltype( Hooked::oDoExtraBoneProcessing ) >( &Hooked::DoExtraBoneProcessing, reinterpret_cast< void * >( DoExtraBoneProcessingAdd ) );

		// render ragdolls in dme
		auto sub_1019D440Addr = Memory::Scan( XorStr( "client.dll" ), XorStr( "56 8B F1 80 BE ? ? ? ? ? 0F 84 ? ? ? ? 80 BE ? ? ? ? ? 0F 85 ? ? ? ? 8B 0D" ) );
		osub_1019D440 = Hooked::HooksManager.CreateHook<decltype( osub_1019D440 ) >( &sub_1019D440, reinterpret_cast< void * >( sub_1019D440Addr ) );

		// find by looking at xrefs of animstate::reset
		// or ref: "mp_guardian_bomb_plant_custom_x_mark_location: unable to find the ground from coordinate"
		auto c_csplayer_postdataupdate = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 80 BE ? ? ? ? ? 74 28" ) ) );
		Hooked::oPostDataUpdate = Hooked::HooksManager.CreateHook<decltype( Hooked::oPostDataUpdate ) >( &Hooked::PostDataUpdate, reinterpret_cast< void * >( c_csplayer_postdataupdate ) );

		auto PrecacheLightingAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "engine.dll" ), XorStr( "E8 ? ? ? ? FF 76 60" ) ) );
		oPrecacheLighting = Hooked::HooksManager.CreateHook<decltype( oPrecacheLighting ) >( &PrecacheLighting, reinterpret_cast< void * >( PrecacheLightingAddr ) );

		//	auto SVCMsg_GetCvarValueAddr = Memory::Scan( XorStr( "engine.dll" ), XorStr( "55 8B EC 83 E4 C0 81 EC ? ? ? ? 53 56 8B D9" ) );
		//	oSVCMsg_GetCvarValue = Hooked::HooksManager.CreateHook<decltype( oSVCMsg_GetCvarValue ) >( &SVCMsg_GetCvarValue, reinterpret_cast< void * >( SVCMsg_GetCvarValueAddr ) );

			//auto FindVarAddr = Memory::Scan( XorStr( "vstdlib.dll" ), XorStr( "55 8B EC 8B 01 56 FF 75 08 FF 50 3C 8B F0 85 F6 74 1E" ) );
			//oFindVar = Hooked::HooksManager.CreateHook<decltype( oFindVar ) >( &FindVar, reinterpret_cast< void * >( FindVarAddr ) );

		auto CL_ReadPacketsAddr = Engine::Displacement.Function.m_CLReadPackets;
		Hooked::oCL_ReadPackets = Hooked::HooksManager.CreateHook<decltype( Hooked::oCL_ReadPackets ) >( &CL_ReadPackets, reinterpret_cast< void * >( CL_ReadPacketsAddr ) );

		//auto SelectSequenceFromActivityModsAddr = Memory::Scan( "server.dll", "55 8B EC 51 56 8B 35 ? ? ? ? 57 8B F9 8B CE" );
		//oSelectSequenceFromActivityMods = Hooked::HooksManager.CreateHook<decltype( oSelectSequenceFromActivityMods ) >( &SelectSequenceFromActivityMods, reinterpret_cast< void * >( SelectSequenceFromActivityModsAddr ) );

		//auto PerformScreenOverlayAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 51 A1 ? ? ? ? 53 56 8B D9" ) );
		//Hooked::oPerformScreenOverlay = Hooked::HooksManager.CreateHook<decltype( Hooked::oPerformScreenOverlay )>( &Hooked::PerformScreenOverlay, reinterpret_cast< void * >( PerformScreenOverlayAddr ) );

		//auto OnBBoxChangeCallbackAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 8B 45 10 F3 0F 10 81" ) );
		//oOnBBoxChangeCallback = Hooked::HooksManager.CreateHook<decltype( oOnBBoxChangeCallback )>( &OnBBoxChangeCallback, reinterpret_cast< void * >( OnBBoxChangeCallbackAddr ) );

		//auto SetupAliveLoopAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 8B 47 60 83 B8" ) ) );
		//oSetupAliveLoop = Hooked::HooksManager.CreateHook<decltype( oSetupAliveLoop )>( &SetupAliveLoop, ( void * )SetupAliveLoopAddr );

		// 57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 ? 5
	//	auto ResetLatchedAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "56 8B F1 57 8B BE ? ? ? ? 85 FF 74 ? 8B CF E8 ? ? ? ? 68" ), false );
	//	oResetLatched = Hooked::HooksManager.CreateHook<decltype( oResetLatched )>( &ResetLatched, ( void * )ResetLatchedAddr );

		auto GetPredDescMapAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 55 8B EC A1 ? ? ? ? 56 68 ? ? ? ? 8B 08 8B 01 FF 50 ? 85 C0 75 ? 33 F6 EB ? 8D 70 ? 83 E6 ? 89 46 ? 68 ? ? ? ? 6A ? 56 E8 ? ? ? ? 83 C4 ? 85 F6 74 ? 8B CE E8 ? ? ? ? 8B F0 85 F6 74 ? FF 75 ? 8B 16 8B CE FF 75 ? FF 92 ? ? ? ? 8D 46 ? 5E 5D C3 33 C0 5E 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 56 8B F1 E8 ? ? ? ? C7 06 ? ? ? ? C7 46" ), false );
		oGetPredDescMap = Hooked::HooksManager.CreateHook<decltype( oGetPredDescMap )>( &GetPredDescMapHook, ( void * )GetPredDescMapAddr );

		auto SFHudMoney__ShouldDrawAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "57 8B F9 E8 ?? ?? ?? ?? 84 C0 74 ?? 32 C0 5F C3 83 3D" ) );
		oSFHudMoney__ShouldDraw = Hooked::HooksManager.CreateHook<decltype( oSFHudMoney__ShouldDraw )>( &SFHudMoney__ShouldDraw, ( void * )SFHudMoney__ShouldDrawAddr );

		auto add_renderableAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 56 8B 75 08 57 FF 75 18" ) );
		oadd_renderable = Hooked::HooksManager.CreateHook<decltype( oadd_renderable )>( &add_renderable, ( void * )add_renderableAddr );

		//auto CL_SendMoveAddr = Memory::Scan( XorStr( "engine.dll" ), XorStr( "55 8B EC A1 ?? ?? ?? ?? 81 EC ?? ?? ?? ?? B9 ?? ?? ?? ?? 53 8B 98" ) );
		//oCL_SendMove = Hooked::HooksManager.CreateHook<decltype( oCL_SendMove )>( &CL_SendMove, ( void * )CL_SendMoveAddr );

		//auto S_StartSoundAddr = Memory::Scan( XorStr( "engine.dll" ), XorStr( "55 8B EC 81 EC ? ? ? ? B8 ? ? ? ? 53 56 57" ) );
		//oS_StartSound = Hooked::HooksManager.CreateHook<decltype( oS_StartSound )>( &S_StartSound, ( void * )S_StartSoundAddr );
		//
		//auto start_sound_immediateAddr = Memory::Scan( XorStr( "engine.dll" ), XorStr( "55 8B EC 83 E4 ? A1 ? ? ? ? 81 EC ? ? ? ? 53 56 57" ) );
		//ostart_sound_immediate = Hooked::HooksManager.CreateHook<decltype( ostart_sound_immediate )>( &start_sound_immediate, ( void * )start_sound_immediateAddr );

		//auto CCSPlayer_FireBulletServerAddr = Memory::Scan( XorStr( "server.dll" ), XorStr( "55 8B EC 83 E4 ?? 81 EC ?? ?? ?? ?? 66 0F 6E 45" ) );
		//oCCSPlayer_FireBulletServer = Hooked::HooksManager.CreateHook<decltype( oCCSPlayer_FireBulletServer )>( &CCSPlayer_FireBulletServer, ( void * )CCSPlayer_FireBulletServerAddr );

		//auto CClientState_NETMsg_TickAddr = Memory::Scan( XorStr( "engine.dll" ), XorStr( "55 8B EC 53 56 8B F1 8B 0D ?? ?? ?? ?? 57 85 C9" ) );
		//oCClientState_NETMsg_Tick = Hooked::HooksManager.CreateHook<decltype( oCClientState_NETMsg_Tick )>( &CClientState_NETMsg_Tick, ( void * )CClientState_NETMsg_TickAddr );

		//auto SetupVelocityServerAddr = Memory::Scan( XorStr( "server.dll" ), XorStr( "55 8B EC 83 E4 ? 83 EC ? 56 57 8B 3D" ), false );
		//oSetupVelocityServer = Hooked::HooksManager.CreateHook<decltype( oSetupVelocityServer )>( &SetupVelocityServer, ( void * )SetupVelocityServerAddr );
		//
		//auto SetupVelocityClientAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 ? 83 EC ? 56 57 8B 3D ? ? ? ? 8B F1" ), false );
		//oSetupVelocityClient = Hooked::HooksManager.CreateHook<decltype( oSetupVelocityClient )>( &SetupVelocityClient, ( void * )SetupVelocityClientAddr );
		//
		//auto SetupMovementServerAddr = Memory::Scan( XorStr( "server.dll" ), XorStr( "55 8B EC 83 E4 ? 81 EC ? ? ? ? 56 57 8B 3D" ), false );
		//oSetupMovementServer = Hooked::HooksManager.CreateHook<decltype( oSetupMovementServer )>( &SetupMovementServer, ( void * )SetupMovementServerAddr );

		// 55 8B EC 83 EC ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE
		//auto PerformPredictionAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 EC ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ), false );
		//oPerformPrediction = Hooked::HooksManager.CreateHook<decltype( oPerformPrediction )>( &PerformPrediction, ( void * )PerformPredictionAddr );

		//auto UpdateAnimationStateAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24 0C" ) );
		//oUpdateAnimationState = Hooked::HooksManager.CreateHook<decltype( oUpdateAnimationState )>( &UpdateAnimationState, ( void * )UpdateAnimationStateAddr );

		//auto BuildRenderablesListForCSMViewAddr = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 8D A5 ? ? ? ? 5F 5E 5B 8B E5 5D C2 04 00 8B 35" ) ) );
		//oBuildRenderablesListForCSMView = Hooked::HooksManager.CreateHook<decltype( oBuildRenderablesListForCSMView ) >( &BuildRenderablesListForCSMView, reinterpret_cast< void * >( BuildRenderablesListForCSMViewAddr ) );

		// 55 8B EC 83 EC ? 53 8B 5D ? 56 57 FF 75 ? 8B F1
		//auto oRunSimulationAddr = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 EC ? 53 8B 5D ? 56 57 FF 75 ? 8B F1" ), false );
		//oRunSimulation = Hooked::HooksManager.CreateHook<decltype( oRunSimulation )>( &RunSimulation, ( void * )oRunSimulationAddr );
	#endif
		//auto tier0 = GetModuleHandleA( XorStr( "tier0.dll" ) );
		//AllocateThreadID2 = ( ThreadIDFn )GetProcAddress( tier0, XorStr( "AllocateThreadID" ) );

		//Threading::InitThreads( );

		//DispatchToAllThreads<decltype( AllocateThreadIDWrapper2 ), AllocateThreadIDWrapper2>( nullptr );

		Threading::bDone = true;

		g_initialised = true;
	#ifndef MENU_DEV
		Hooked::HooksManager.Enable( );
	#endif

		return true;
	}

	void Destroy( ) {
	#ifndef MENU_DEV
		Hooked::HooksManager.Restore( );

		g_GameEvent.Shutdown( );
		g_SkinChanger.Destroy( );
	#endif
		g_InputSystem.Destroy( );

		MH_DisableHook( Memory::VCall<decltype( Hooked::oPaint )>( g_pEngineVGui.Xor( ), 14 ) );

		MH_Uninitialize( );

		g_pInputSystem->EnableInput( true );
		//g_pClientState->m_nDeltaTick( ) = -1;
	}

#ifndef DEV
	void *CreateInterface( const std::pair< uint32_t, std::string> hash ) {
		//char buf[ 128 ] = { 0 };
		//wsprintfA( buf, XorStr( "%s::%s" ), image_name.data( ), name.data( ) );
		auto iface = interfaces_get_interface( hash.first );

	#ifdef BETA_MODE
		if( !iface ) {
			LI_FN( MessageBoxA )( HWND( 0 ), XorStr( "Failed to grab interface" ), XorStr( "ERROR" ), 0 );
		}
	#endif

		return reinterpret_cast< void * >( iface );
	#else
	void *CreateInterface( const std::string & image_name, const std::string & name ) {
		auto image = GetModuleHandleA( image_name.c_str( ) );
		if( !image )
			return nullptr;

		auto fn = ( CreateInterfaceFn )( GetProcAddress( image, XorStr( "CreateInterface" ) ) );
		if( !fn )
			return nullptr;

		return fn( name.c_str( ), nullptr );
	#endif
	}
}