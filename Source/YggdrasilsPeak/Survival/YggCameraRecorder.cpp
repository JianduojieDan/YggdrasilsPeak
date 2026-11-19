#include "Survival/YggCameraRecorder.h"

// The view rig (camera spec §10.1) is being written in parallel: everything that touches it is behind this switch and
// the few calls of the fixed API (Find, Audit, GetView, GetEyeAboveFloor, GetLid). Getters beyond it (GetScripted,
// GetLeaseMask, WasCutRecently, IsNudging) are used only when the rig has them under these names.
#if defined(__has_include)
#if __has_include("Survival/YggViewRig.h")
#include "Survival/YggViewRig.h"
#define YGG_CAMREC_HAS_RIG 1
#endif
#endif
#ifndef YGG_CAMREC_HAS_RIG
#define YGG_CAMREC_HAS_RIG 0
#endif

#include "Survival/YggPlayBot.h"

#include "Async/Async.h"
#include "Camera/CameraModifier.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Containers/Queue.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "UnrealClient.h"
#include <atomic>
#include <type_traits>
#include <utility>

DEFINE_LOG_CATEGORY_STATIC(LogYggCamRec, Log, All);

// ------------------------------------------------------------------------------------------------ switches

static TAutoConsoleVariable<int32> CVarCamRec(TEXT("ygg.CamRec"), -1,
	TEXT("Camera flight recorder: 1 = on, 0 = off, -1 = auto (on in -game, off in the editor and in -YggNoSave test runs unless -YggCamRec)."));
static TAutoConsoleVariable<float> CVarCamRecSeconds(TEXT("ygg.CamRec.Seconds"), 6.f,
	TEXT("Seconds of frames kept and written before an incident (read when the recorder switches on)."));
static TAutoConsoleVariable<float> CVarCamRecPost(TEXT("ygg.CamRec.PostS"), 0.5f,
	TEXT("Seconds of frames after the incident that are waited for before it is written."));
static TAutoConsoleVariable<float> CVarCamRecRate(TEXT("ygg.CamRec.RateDeg"), 120.f,
	TEXT("Incident when the view (camera or control rotation) turns faster than this (deg/s) over the window while the mouse and stick are still."));
static TAutoConsoleVariable<float> CVarCamRecWindow(TEXT("ygg.CamRec.Window"), 0.15f,
	TEXT("Seconds the turn rate is measured over (every frame in it must be without input)."));
static TAutoConsoleVariable<float> CVarCamRecMinDeg(TEXT("ygg.CamRec.MinDeg"), 8.f,
	TEXT("A spin incident needs at least this many degrees turned in the window."));
static TAutoConsoleVariable<float> CVarCamRecMouseEps(TEXT("ygg.CamRec.MouseEps"), 0.01f,
	TEXT("Mouse delta (input value and raw Slate pixels) at or below this counts as a still mouse."));
static TAutoConsoleVariable<float> CVarCamRecRoll(TEXT("ygg.CamRec.RollDeg"), 3.f,
	TEXT("Incident when |roll| of the camera or the control rotation exceeds this outside a scripted fall / knock / fly-out."));
static TAutoConsoleVariable<float> CVarCamRecEye(TEXT("ygg.CamRec.EyeCm"), 156.f,
	TEXT("Normal standing eye height over the floor (cm; the rig's stand posture)."));
static TAutoConsoleVariable<float> CVarCamRecLow(TEXT("ygg.CamRec.LowCm"), 40.f,
	TEXT("Incident when the camera is this far below EyeCm for 0.3 s while the pawn walks faster than MoveCmS."));
static TAutoConsoleVariable<float> CVarCamRecMove(TEXT("ygg.CamRec.MoveCmS"), 60.f,
	TEXT("Horizontal pawn speed (cm/s) above which a low eye counts."));
static TAutoConsoleVariable<float> CVarCamRecVt(TEXT("ygg.CamRec.ViewTargetS"), 15.f,
	TEXT("Incident when the view target is not the pawn for this many seconds."));
static TAutoConsoleVariable<float> CVarCamRecHitch(TEXT("ygg.CamRec.HitchMs"), 250.f,
	TEXT("Incident for a frame longer than this (ms)."));
static TAutoConsoleVariable<float> CVarCamRecCooldown(TEXT("ygg.CamRec.Cooldown"), 3.f,
	TEXT("Seconds between incidents (later ones in that time are counted, not written)."));
static TAutoConsoleVariable<int32> CVarCamRecMax(TEXT("ygg.CamRec.Max"), 30,
	TEXT("Incidents written per session at most (ygg.CamRec.Dump does not count)."));
static TAutoConsoleVariable<float> CVarCamRecWarm(TEXT("ygg.CamRec.WarmS"), 5.f,
	TEXT("Seconds after the world begins play with no detection (loading, the arrival fade)."));
static TAutoConsoleVariable<int32> CVarCamRecShot(TEXT("ygg.CamRec.Shot"), 1,
	TEXT("1 = take a screenshot (shot.jpg) for each incident."));
static TAutoConsoleVariable<int32> CVarCamRecRig(TEXT("ygg.CamRec.Rig"), 1,
	TEXT("1 = read the view rig (posture, eye, eyelids, takeover, leases, Audit), 0 = never touch it."));
static TAutoConsoleVariable<float> CVarCamRecAuditEvery(TEXT("ygg.CamRec.AuditEvery"), 0.25f,
	TEXT("Seconds between reads of the view rig's Audit() text (kept when it changes)."));

// ------------------------------------------------------------------------------------------------ helpers

namespace
{
	FString GCamRecSession;           // one folder per process
	int32 GCamRecNumber = 0;          // numbering of the folders (incidents and dumps)
	int32 GCamRecIncidents = 0;       // incidents written (the Max applies to these)
	FCriticalSection GCamRecFileLock; // the index file is appended from the thread pool
	int32 GCamRecPendingTest = 0;     // ygg.CamRec.Test typed with no game world yet: 1 pitch, 2 yaw (the next recorder takes it)
	float GCamRecPendingDelay = 0.f;

	enum ECamRecKind : uint8 { CRK_None, CRK_SpinPitch, CRK_SpinYaw, CRK_PitchWrap, CRK_Roll, CRK_LowEye, CRK_ViewTarget, CRK_Hitch, CRK_Dump, CRK_Num };
	const TCHAR* const CamRecKindNames[] = { TEXT(""), TEXT("spin_pitch"), TEXT("spin_yaw"), TEXT("pitch_wrap"), TEXT("roll"), TEXT("low_eye"), TEXT("view_target"), TEXT("hitch"), TEXT("dump") };
	const TCHAR* const CamRecPostureNames[] = { TEXT("walk"), TEXT("lie"), TEXT("sit"), TEXT("kneel1"), TEXT("kneel2"), TEXT("crawl") };
	const TCHAR* const CamRecScriptedNames[] = { TEXT("none"), TEXT("fall"), TEXT("knock"), TEXT("fly") };
	// bit order of EYggLease (spec §10.1)
	const TCHAR* const CamRecLeaseNames[] = { TEXT("Move"), TEXT("Look"), TEXT("Pace"), TEXT("Fall"), TEXT("Knock"), TEXT("Fly"), TEXT("Inspect"), TEXT("Hands"),
		TEXT("Attention"), TEXT("FovBeat"), TEXT("StoryFov"), TEXT("Nudge"), TEXT("Lids"), TEXT("Breath"), TEXT("Exposure"), TEXT("Lens") };
	const TCHAR* const CamRecMoveNames[] = { TEXT("none"), TEXT("walking"), TEXT("navwalking"), TEXT("falling"), TEXT("swimming"), TEXT("flying"), TEXT("custom") };

	float CamRecNorm(float A)
	{
		A = FMath::Fmod(A + 180.f, 360.f);
		if (A < 0.f) { A += 360.f; }
		return A - 180.f;
	}
	float CamRecDelta(float From, float To) { return CamRecNorm(To - From); }

	const TCHAR* CamRecPosture(int32 P) { return P < 0 ? TEXT("-") : CamRecPostureNames[FMath::Clamp(P, 0, 5)]; }
	const TCHAR* CamRecScripted(int32 S) { return S < 0 ? TEXT("-") : CamRecScriptedNames[FMath::Clamp(S, 0, 3)]; }
	const TCHAR* CamRecMove(int32 M) { return CamRecMoveNames[FMath::Clamp(M, 0, 6)]; }

	FString CamRecLeases(int64 Mask)
	{
		if (Mask < 0) { return TEXT("-"); }
		FString S;
		for (int32 B = 0; B < int32(UE_ARRAY_COUNT(CamRecLeaseNames)); ++B)
		{
			if (Mask & (int64(1) << B)) { if (!S.IsEmpty()) { S += TEXT("|"); } S += CamRecLeaseNames[B]; }
		}
		return S.IsEmpty() ? FString(TEXT("none")) : S;
	}

	FString CamRecMods(const FYggCamRecFrame& F)
	{
		if (F.NumMods == 0) { return TEXT("none"); }
		FString S;
		for (int32 I = 0; I < F.NumMods; ++I)
		{
			if (I) { S += TEXT("|"); }
			S += FString::Printf(TEXT("%s:%s:%.2f"), *F.ModName[I].ToString(), (F.ModOffMask & (1 << I)) ? TEXT("off") : TEXT("on"), F.ModAlpha[I] / 255.f);
		}
		return S;
	}

	FString CamRecCsv(FString S)
	{
		S.ReplaceInline(TEXT("\""), TEXT("'"));
		S.ReplaceInline(TEXT("\r"), TEXT(""));
		S.ReplaceInline(TEXT("\n"), TEXT(" / "));
		return TEXT("\"") + S + TEXT("\"");
	}

	FString CamRecClock(double T) { return FString::Printf(TEXT("%d:%04.1f"), int32(T / 60.0), FMath::Fmod(T, 60.0)); }

#if YGG_CAMREC_HAS_RIG
	template<typename T, typename = void> struct TCamRecHasScripted : std::false_type {};
	template<typename T> struct TCamRecHasScripted<T, std::void_t<decltype(std::declval<const T&>().GetScripted())>> : std::true_type {};
	template<typename T, typename = void> struct TCamRecHasLeaseMask : std::false_type {};
	template<typename T> struct TCamRecHasLeaseMask<T, std::void_t<decltype(std::declval<const T&>().GetLeaseMask())>> : std::true_type {};
	template<typename T, typename = void> struct TCamRecHasCut : std::false_type {};
	template<typename T> struct TCamRecHasCut<T, std::void_t<decltype(std::declval<const T&>().WasCutRecently())>> : std::true_type {};
	template<typename T, typename = void> struct TCamRecHasNudging : std::false_type {};
	template<typename T> struct TCamRecHasNudging<T, std::void_t<decltype(std::declval<const T&>().IsNudging())>> : std::true_type {};

	/** -1 unknown, 0 none, 1 fall, 2 knock, 3 fly */
	template<typename T> int8 CamRecRigScripted(const T* R)
	{
		if constexpr (TCamRecHasScripted<T>::value)
		{
			const auto V = R->GetScripted();
			return int8(V == EYggScripted::Fall ? 1 : V == EYggScripted::Knock ? 2 : V == EYggScripted::Fly ? 3 : 0);
		}
		else { (void)R; return -1; }
	}
	template<typename T> int64 CamRecRigLeases(const T* R)
	{
		if constexpr (TCamRecHasLeaseMask<T>::value) { return int64(uint32(R->GetLeaseMask())); }
		else { (void)R; return -1; }
	}
	template<typename T> bool CamRecRigCut(const T* R)
	{
		if constexpr (TCamRecHasCut<T>::value) { return R->WasCutRecently(); }
		else { (void)R; return false; }
	}
	template<typename T> int8 CamRecRigNudging(const T* R)
	{
		if constexpr (TCamRecHasNudging<T>::value) { return int8(R->IsNudging() ? 1 : 0); }
		else { (void)R; return -1; }
	}
#endif

	/** Everything an incident's files are made of (copied off the game thread). */
	struct FCamRecJob
	{
		int32 N = 0;
		FString Kind, Values, Also, Dir, SessionDir, Map;
		uint64 Seq = 0;
		double DetectT = 0.0;
		TArray<FYggCamRecFrame> Frames;
		TArray<TPair<double, FString>> Audits;
		int32 AuditBase = 0;
		TArray<TPair<double, FString>> Logs;
		float EyeCm = 156.f, LowCm = 40.f, RollDeg = 3.f;
	};

	struct FCamRecChange { double Dt = 0.0; FString Text; };

	/** What changed from frame A to frame B that could move the view. */
	void CamRecChanges(const FYggCamRecFrame& A, const FYggCamRecFrame& B, double Dt, TArray<FCamRecChange>& Out)
	{
		const auto Add = [&Out, Dt](const FString& S) { FCamRecChange& C = Out.AddDefaulted_GetRef(); C.Dt = Dt; C.Text = S; };
		if (A.ViewTarget != B.ViewTarget) { Add(FString::Printf(TEXT("view target %s -> %s"), *A.ViewTarget.ToString(), *B.ViewTarget.ToString())); }
		if (A.PawnName != B.PawnName) { Add(FString::Printf(TEXT("pawn %s -> %s"), *A.PawnName.ToString(), *B.PawnName.ToString())); }
		// camera modifiers: added, removed, switched, faded in / out
		for (int32 I = 0; I < B.NumMods; ++I)
		{
			int32 J = INDEX_NONE;
			for (int32 K = 0; K < A.NumMods; ++K) { if (A.ModName[K] == B.ModName[I]) { J = K; break; } }
			if (J == INDEX_NONE) { Add(FString::Printf(TEXT("camera modifier added: %s"), *B.ModName[I].ToString())); continue; }
			const bool bOffA = (A.ModOffMask & (1 << J)) != 0, bOffB = (B.ModOffMask & (1 << I)) != 0;
			if (bOffA != bOffB) { Add(FString::Printf(TEXT("camera modifier %s %s"), *B.ModName[I].ToString(), bOffB ? TEXT("disabled") : TEXT("enabled"))); }
			if ((A.ModAlpha[J] == 0) != (B.ModAlpha[I] == 0)) { Add(FString::Printf(TEXT("camera modifier %s alpha %.2f -> %.2f"), *B.ModName[I].ToString(), A.ModAlpha[J] / 255.f, B.ModAlpha[I] / 255.f)); }
		}
		for (int32 K = 0; K < A.NumMods; ++K)
		{
			bool bFound = false;
			for (int32 I = 0; I < B.NumMods; ++I) { bFound |= A.ModName[K] == B.ModName[I]; }
			if (!bFound) { Add(FString::Printf(TEXT("camera modifier removed: %s"), *A.ModName[K].ToString())); }
		}
		if (A.Posture != B.Posture) { Add(FString::Printf(TEXT("rig posture %s -> %s"), CamRecPosture(A.Posture), CamRecPosture(B.Posture))); }
		if (A.Scripted != B.Scripted) { Add(FString::Printf(TEXT("rig takeover %s -> %s"), CamRecScripted(A.Scripted), CamRecScripted(B.Scripted))); }
		if (A.Nudging != B.Nudging && B.Nudging >= 0) { Add(B.Nudging ? TEXT("rig nudge started") : TEXT("rig nudge ended")); }
		if (A.Leases != B.Leases && A.Leases >= 0 && B.Leases >= 0)
		{
			const int64 Added = B.Leases & ~A.Leases, Removed = A.Leases & ~B.Leases;
			if (Added) { Add(TEXT("rig lease taken: ") + CamRecLeases(Added)); }
			if (Removed) { Add(TEXT("rig lease released: ") + CamRecLeases(Removed)); }
		}
		if (A.bLookIgnored != B.bLookIgnored) { Add(B.bLookIgnored ? TEXT("look input now IGNORED") : TEXT("look input accepted again")); }
		if (A.bMoveIgnored != B.bMoveIgnored) { Add(B.bMoveIgnored ? TEXT("move input now ignored") : TEXT("move input accepted again")); }
		if (A.bCursor != B.bCursor) { Add(B.bCursor ? TEXT("mouse cursor shown (menu / UI)") : TEXT("mouse cursor hidden (mouse recaptured by the game)")); }
		if (A.MoveMode != B.MoveMode) { Add(FString::Printf(TEXT("movement %s -> %s"), CamRecMove(A.MoveMode), CamRecMove(B.MoveMode))); }
		if (A.bPawnHidden != B.bPawnHidden) { Add(B.bPawnHidden ? TEXT("pawn hidden") : TEXT("pawn shown")); }
		if (B.bCutPcm) { Add(TEXT("camera cut (camera manager flag)")); }
		if (B.bCutRig && !A.bCutRig) { Add(TEXT("camera cut (view rig: teleport / reset / fly-out)")); }
		if (B.bTest && !A.bTest) { Add(TEXT("ygg.CamRec.Test started writing the control rotation")); }
		if (B.bBot != A.bBot) { Add(B.bBot ? FString::Printf(TEXT("robot player (ygg.Bot) started turning the view (%.0f, %.0f deg/s)"), B.BotYaw, B.BotPitch) : FString(TEXT("robot player stopped turning the view"))); }
		if (FMath::Abs(B.RawX) + FMath::Abs(B.RawY) > 150.f) { Add(FString::Printf(TEXT("raw mouse jump %.0f, %.0f px in one frame"), B.RawX, B.RawY)); }
		if (B.Ms > 100.f) { Add(FString::Printf(TEXT("slow frame %.0f ms"), B.Ms)); }
		if (FMath::Abs(B.Fov - A.Fov) > 2.f) { Add(FString::Printf(TEXT("FOV %.1f -> %.1f"), A.Fov, B.Fov)); }
		if (A.EyeMeasured >= 0.f && B.EyeMeasured >= 0.f && FMath::Abs(B.EyeMeasured - A.EyeMeasured) > 15.f)
		{
			Add(FString::Printf(TEXT("eye height jumped %.0f -> %.0f cm"), A.EyeMeasured, B.EyeMeasured));
		}
	}

	/** The likely culprit, best guess first; OneLine = the first guess, for the index. */
	TArray<FString> CamRecCulprit(const FCamRecJob& J, int32 I, const TArray<FCamRecChange>& Changes, FString& OneLine)
	{
		TArray<FString> L;
		const TArray<FYggCamRecFrame>& Fr = J.Frames;
		const FYggCamRecFrame& F = Fr[I];
		const double T = F.T;

		bool bTest = false;
		for (int32 K = I; K >= 0 && T - Fr[K].T <= 2.0; --K) { bTest |= Fr[K].bTest; }
		if (bTest) { L.Add(TEXT("ygg.CamRec.Test was turning the control rotation: this incident is the recorder's own self-test.")); }

		// over the last 0.5 s: how far did the camera and the control rotation turn, and was there input?
		float CamTravel = 0.f, CtrlTravel = 0.f, MaxRaw = 0.f, MaxBot = 0.f; int32 N = 0, Still = 0, BotN = 0; double RawT = 0.0;
		for (int32 K = I; K >= 0 && T - Fr[K].T <= 0.5; --K)
		{
			const FYggCamRecFrame& B = Fr[K];
			CamTravel += FMath::Abs(B.DCamPitch) + FMath::Abs(B.DCamYaw);
			CtrlTravel += FMath::Abs(B.DCtrlPitch) + FMath::Abs(B.DCtrlYaw);
			++N; Still += B.bStill ? 1 : 0;
			BotN += B.bBot ? 1 : 0;
			MaxBot = FMath::Max(MaxBot, FMath::Max(FMath::Abs(B.BotYaw), FMath::Abs(B.BotPitch)));
			const float Raw = FMath::Abs(B.RawX) + FMath::Abs(B.RawY);
			if (Raw > MaxRaw) { MaxRaw = Raw; RawT = B.T - T; }
		}
		const float StillFrac = N ? float(Still) / N : 1.f;
		FString ActiveMods;
		for (int32 M = 0; M < F.NumMods; ++M)
		{
			if (!(F.ModOffMask & (1 << M)) && F.ModAlpha[M] > 0) { ActiveMods += (ActiveMods.IsEmpty() ? TEXT("") : TEXT(", ")) + F.ModName[M].ToString(); }
		}
		if (ActiveMods.IsEmpty()) { ActiveMods = TEXT("none"); }

		const bool bTurnKind = J.Kind == TEXT("spin_pitch") || J.Kind == TEXT("spin_yaw") || J.Kind == TEXT("pitch_wrap") || J.Kind == TEXT("roll");
		if (bTurnKind && BotN > 0)
		{
			L.Add(FString::Printf(TEXT("The robot player (ygg.Bot) fed look input in %d of the last %d frames (up to %.0f deg/s): its camera stress drove this turn - a real fault only if the view went past what it asked (pitch beyond +-90, roll)."),
				BotN, N, MaxBot));
		}
		if (bTurnKind)
		{
			if (J.Kind == TEXT("roll"))
			{
				if (FMath::Abs(F.CtrlRoll) > J.RollDeg)
				{
					L.Add(FString::Printf(TEXT("The CONTROL rotation carries roll (%.1f deg): some code called SetControlRotation / ClientSetRotation with roll (the camera manager passes it on)."), F.CtrlRoll));
				}
				else
				{
					L.Add(FString::Printf(TEXT("Roll is on the camera only (cam %.1f, ctrl %.1f): added after the controller by a camera modifier (active: %s), the view target's camera, or the rig's ApplyView (takeover %s)."),
						F.CamRoll, F.CtrlRoll, *ActiveMods, CamRecScripted(F.Scripted)));
				}
			}
			if (CamTravel > 5.f && CtrlTravel < 0.3f * CamTravel)
			{
				L.Add(FString::Printf(TEXT("The control rotation stayed put (%.0f deg in 0.5 s) while the camera turned %.0f deg: the turn was added AFTER the controller - a camera modifier (active: %s), the view target (%s%s) or the rig's ApplyView (takeover %s, posture %s)."),
					CtrlTravel, CamTravel, *ActiveMods, *F.ViewTarget.ToString(), F.bVtIsPawn ? TEXT(", the pawn") : TEXT(", NOT the pawn"), CamRecScripted(F.Scripted), CamRecPosture(F.Posture)));
			}
			else if (CtrlTravel > 5.f && StillFrac > 0.9f)
			{
				L.Add(FString::Printf(TEXT("The CONTROL rotation itself turned %.0f deg in 0.5 s with no mouse or stick input: code wrote it - the rig's ProcessViewRotation (nudge %s, leases %s: nudge / reflex / hand pull / watchdog repair / HardReset), the story, or another SetControlRotation caller."),
					CtrlTravel, F.Nudging < 0 ? TEXT("-") : F.Nudging ? TEXT("ON") : TEXT("off"), *CamRecLeases(F.Leases)));
			}
			else if (MaxRaw > 150.f)
			{
				L.Add(FString::Printf(TEXT("A raw mouse jump of %.0f px at %+.2f s: the cursor being recaptured (window focus, a menu closing, show-cursor toggled) feeds one huge delta into the look."), MaxRaw, RawT));
			}
			else if (CamTravel > 0.5f || CtrlTravel > 0.5f)
			{
				L.Add(FString::Printf(TEXT("The mouse / stick was moving (%.0f%% of the last 0.5 s had input): player input - compare raw_x/raw_y with d_ctrl_* for a spike or a sensitivity / smoothing fault."), (1.f - StillFrac) * 100.f));
			}
		}
		else if (J.Kind == TEXT("low_eye"))
		{
			if (F.RigEye >= 0.f && F.RigEye < J.EyeCm - J.LowCm)
			{
				L.Add(FString::Printf(TEXT("The rig itself holds the eye low (rig eye %.0f cm, posture %s, takeover %s, leases %s): a posture, a sink or a stuck lease."),
					F.RigEye, CamRecPosture(F.Posture), CamRecScripted(F.Scripted), *CamRecLeases(F.Leases)));
			}
			else if (F.RigEye >= 0.f)
			{
				L.Add(FString::Printf(TEXT("The rig's eye is fine (%.0f cm) but the camera ends %.0f cm over the floor: the camera is placed by something else (attachment, view target, modifier: %s) or the floor under the capsule is wrong."),
					F.RigEye, F.EyeMeasured, *ActiveMods));
			}
			else
			{
				L.Add(FString::Printf(TEXT("No rig reading: the camera is %.0f cm over the floor while walking at %.0f cm/s (movement %s)."), F.EyeMeasured, F.SpeedH, CamRecMove(F.MoveMode)));
			}
		}
		else if (J.Kind == TEXT("view_target"))
		{
			L.Add(FString::Printf(TEXT("The view target is %s, not the pawn %s (rig takeover %s): a fly-out / camera actor that never handed the view back."),
				*F.ViewTarget.ToString(), *F.PawnName.ToString(), CamRecScripted(F.Scripted)));
		}
		else if (J.Kind == TEXT("hitch"))
		{
			L.Add(FString::Printf(TEXT("A %.0f ms frame: the game thread stalled - see the session monitor's stalls.txt and hitch events at this time, and the log lines below."), F.Ms));
		}

		// the nearest change before the incident (Changes are sorted nearest first)
		for (const FCamRecChange& C : Changes)
		{
			if (C.Dt <= 0.0001)
			{
				L.Add(FString::Printf(TEXT("Nearest change before it: %+.2f s  %s"), C.Dt, *C.Text));
				break;
			}
		}
		if (L.Num() == 0) { L.Add(TEXT("No change of view target, modifiers, rig state or input flags was seen in the frames before: read frames.csv.")); }
		OneLine = L[0].Left(160);
		return L;
	}

	void CamRecWriteJob(FCamRecJob J)
	{
		IFileManager::Get().MakeDirectory(*J.Dir, true);
		int32 I = J.Frames.Num() - 1;
		for (int32 K = 0; K < J.Frames.Num(); ++K) { if (J.Frames[K].Seq == J.Seq) { I = K; break; } }

		// ---- frames.csv
		FString Csv;
		Csv.Reserve(J.Frames.Num() * 420 + 1024);
		Csv += TEXT("t,t_rel,ms,dt_game,cam_x,cam_y,cam_z,cam_pitch,cam_yaw,cam_roll,ctrl_pitch,ctrl_yaw,ctrl_roll,d_cam_pitch,d_cam_yaw,d_ctrl_pitch,d_ctrl_yaw,")
			TEXT("fov,mouse_x,mouse_y,raw_x,raw_y,stick_x,stick_y,still,look_ignored,move_ignored,cursor,cut_pcm,cut_rig,cut,")
			TEXT("pawn,pawn_x,pawn_y,pawn_z,vel_x,vel_y,vel_z,speed_h,move_mode,view_target,vt_is_pawn,pawn_hidden,eye_measured,")
			TEXT("rig_posture,rig_eye,rig_lid,rig_scripted,rig_nudge,rig_leases,modifiers,test,bot_yaw,bot_pitch,incident,audit\n");
		const double T0 = I >= 0 ? J.Frames[I].T : 0.0;
		int32 LastAudit = -2;
		for (const FYggCamRecFrame& F : J.Frames)
		{
			FString Audit;
			if (F.AuditId != LastAudit)
			{
				LastAudit = F.AuditId;
				const int32 A = F.AuditId - J.AuditBase;
				if (J.Audits.IsValidIndex(A)) { Audit = CamRecCsv(J.Audits[A].Value); }
			}
			Csv += FString::Printf(TEXT("%.4f,%.4f,%.2f,%.4f,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.4f,%.4f,%.1f,%.1f,%.3f,%.3f,%d,%d,%d,%d,%d,%d,%d,"),
				F.T, F.T - T0, F.Ms, F.GameDt, F.CamLoc.X, F.CamLoc.Y, F.CamLoc.Z, F.CamPitch, F.CamYaw, F.CamRoll, F.CtrlPitch, F.CtrlYaw, F.CtrlRoll,
				F.DCamPitch, F.DCamYaw, F.DCtrlPitch, F.DCtrlYaw, F.Fov, F.MouseX, F.MouseY, F.RawX, F.RawY, F.StickX, F.StickY,
				F.bStill ? 1 : 0, F.bLookIgnored ? 1 : 0, F.bMoveIgnored ? 1 : 0, F.bCursor ? 1 : 0, F.bCutPcm ? 1 : 0, F.bCutRig ? 1 : 0, F.bCut ? 1 : 0);
			Csv += FString::Printf(TEXT("%s,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%s,%s,%d,%d,%.1f,%s,%.1f,%.3f,%s,%d,%s,%s,%d,%.0f,%.0f,%s,%s\n"),
				*F.PawnName.ToString(), F.PawnLoc.X, F.PawnLoc.Y, F.PawnLoc.Z, F.PawnVel.X, F.PawnVel.Y, F.PawnVel.Z, F.SpeedH, CamRecMove(F.MoveMode),
				*F.ViewTarget.ToString(), F.bVtIsPawn ? 1 : 0, F.bPawnHidden ? 1 : 0, F.EyeMeasured, CamRecPosture(F.Posture), F.RigEye, F.RigLid,
				CamRecScripted(F.Scripted), int32(F.Nudging), *CamRecLeases(F.Leases), *CamRecCsv(CamRecMods(F)), F.bTest ? 1 : 0, F.BotYaw, F.BotPitch,
				F.IncidentKind ? CamRecKindNames[FMath::Min<int32>(F.IncidentKind, CRK_Num - 1)] : TEXT(""), *Audit);
		}
		FFileHelper::SaveStringToFile(Csv, *(J.Dir / TEXT("frames.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

		// ---- summary.txt
		TArray<FCamRecChange> Changes;
		if (I >= 0)
		{
			for (int32 K = FMath::Min(I + 1, J.Frames.Num() - 1); K >= 1 && J.Frames[I].T - J.Frames[K].T <= 3.0; --K)
			{
				CamRecChanges(J.Frames[K - 1], J.Frames[K], J.Frames[K].T - J.Frames[I].T, Changes);
			}
		}
		FString OneLine = TEXT("-");
		FString S;
		S += FString::Printf(TEXT("CamRec incident #%d  %s\n"), J.N, *J.Kind);
		S += FString::Printf(TEXT("when:   %s into the recorder (frame %llu), map %s\n"), *CamRecClock(J.DetectT), (unsigned long long)J.Seq, *J.Map);
		S += FString::Printf(TEXT("values: %s\n"), *J.Values);
		if (!J.Also.IsEmpty()) { S += FString::Printf(TEXT("also:   %s\n"), *J.Also); }
		if (I >= 0)
		{
			const FYggCamRecFrame& F = J.Frames[I];
			S += FString::Printf(TEXT("camera: pitch %.1f yaw %.1f roll %.1f, FOV %.1f at (%.0f, %.0f, %.0f)\n"), F.CamPitch, F.CamYaw, F.CamRoll, F.Fov, F.CamLoc.X, F.CamLoc.Y, F.CamLoc.Z);
			S += FString::Printf(TEXT("control rotation: pitch %.1f yaw %.1f roll %.1f\n"), F.CtrlPitch, F.CtrlYaw, F.CtrlRoll);
			S += FString::Printf(TEXT("input: mouse %.3f, %.3f  raw %.1f, %.1f  stick %.2f, %.2f  still %d  look ignored %d  move ignored %d  cursor %d\n"),
				F.MouseX, F.MouseY, F.RawX, F.RawY, F.StickX, F.StickY, F.bStill ? 1 : 0, F.bLookIgnored ? 1 : 0, F.bMoveIgnored ? 1 : 0, F.bCursor ? 1 : 0);
			S += FString::Printf(TEXT("pawn: %s at (%.0f, %.0f, %.0f), %.0f cm/s, %s, eye over floor %.0f cm (normal %.0f)\n"),
				*F.PawnName.ToString(), F.PawnLoc.X, F.PawnLoc.Y, F.PawnLoc.Z, F.SpeedH, CamRecMove(F.MoveMode), F.EyeMeasured, J.EyeCm);
			S += FString::Printf(TEXT("view target: %s%s\n"), *F.ViewTarget.ToString(), F.bVtIsPawn ? TEXT(" (the pawn)") : TEXT(" (NOT the pawn)"));
			S += FString::Printf(TEXT("camera modifiers: %s\n"), *CamRecMods(F));
			S += FString::Printf(TEXT("rig: posture %s, eye %.0f cm, lid %.2f, takeover %s, nudge %d, leases %s\n"),
				CamRecPosture(F.Posture), F.RigEye, F.RigLid, CamRecScripted(F.Scripted), int32(F.Nudging), *CamRecLeases(F.Leases));
			S += TEXT("\n--- likely culprit ---\n");
			const TArray<FString> Guess = CamRecCulprit(J, I, Changes, OneLine);
			for (int32 G = 0; G < Guess.Num(); ++G) { S += FString::Printf(TEXT("%d. %s\n"), G + 1, *Guess[G]); }
		}
		S += TEXT("\n--- changes in the 3 s before (nearest first; t relative to the incident frame) ---\n");
		if (Changes.Num() == 0) { S += TEXT("none\n"); }
		for (int32 C = 0; C < Changes.Num() && C < 60; ++C) { S += FString::Printf(TEXT("%+7.3f s  %s\n"), Changes[C].Dt, *Changes[C].Text); }
		S += TEXT("\n--- log lines in the window (game categories + warnings; t relative to the incident) ---\n");
		if (J.Logs.Num() == 0) { S += TEXT("none\n"); }
		for (const TPair<double, FString>& Lg : J.Logs) { S += FString::Printf(TEXT("%+7.3f s  %s\n"), Lg.Key - J.DetectT, *Lg.Value.TrimEnd()); }
		S += TEXT("\n--- view rig Audit() changes in the window ---\n");
		if (J.Audits.Num() == 0) { S += TEXT("none (no rig, or ygg.CamRec.Rig 0)\n"); }
		for (const TPair<double, FString>& A : J.Audits) { S += FString::Printf(TEXT("%+7.3f s  %s\n"), A.Key - J.DetectT, *A.Value); }
		S += TEXT("\nframes.csv: one row per frame; d_* = change since the previous frame (0 on a cut); still = no mouse / raw mouse / stick / robot input;\n")
			TEXT("raw_* = Slate mouse move (pixels); bot_* = look the robot player fed (deg/s); incident = the frame it was detected on; audit = the rig's Audit() where it changed.\n");
		FFileHelper::SaveStringToFile(S, *(J.Dir / TEXT("summary.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

		// ---- incidents.md (running index)
		{
			FScopeLock Lock(&GCamRecFileLock);
			const FString Index = J.SessionDir / TEXT("incidents.md");
			if (!IFileManager::Get().FileExists(*Index))
			{
				const FString Head = FString::Printf(TEXT("# Camera incidents, session %s\n\n| # | time | kind | values | likely culprit | folder |\n|---|---|---|---|---|---|\n"), *FPaths::GetCleanFilename(J.SessionDir));
				FFileHelper::SaveStringToFile(Head, *Index, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
			}
			FString Row = FString::Printf(TEXT("| %d | %s | %s | %s | %s | %s |\n"), J.N, *CamRecClock(J.DetectT), *J.Kind,
				*J.Values.Replace(TEXT("|"), TEXT("/")), *OneLine.Replace(TEXT("|"), TEXT("/")), *FPaths::GetCleanFilename(J.Dir));
			FFileHelper::SaveStringToFile(Row, *Index, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
		}
	}
}

// ------------------------------------------------------------------------------------------------ input and log taps

/** The raw mouse as Slate sees it (GetInputMouseDelta reads the player input's key state, which Enhanced Input may not
 *  fill: both are recorded and either one moving means the mouse moved). */
class FYggCamRecInput : public IInputProcessor
{
public:
	float Dx = 0.f, Dy = 0.f;
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& E) override
	{
		const FVector2D D = E.GetCursorDelta();
		Dx += float(D.X);
		Dy += float(D.Y);
		return false;
	}
	virtual const TCHAR* GetDebugName() const override { return TEXT("YggCamRecInput"); }
};

/** The game's own log lines and every warning, queued from any thread, read by the game thread every frame. */
class FYggCamRecLog : public FOutputDevice
{
public:
	std::atomic<bool> bOn{ false };
	double StartReal = 0.0;
	TQueue<TPair<double, FString>, EQueueMode::Mpsc> Lines;

	FYggCamRecLog() { bAutoEmitLineTerminator = false; }
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (!bOn.load(std::memory_order_relaxed)) { return; }
		static const FName Own(TEXT("LogYggCamRec"));
		static const FName Keep[] = { TEXT("LogYggView"), TEXT("LogYggRig"), TEXT("LogYggStory"), TEXT("LogYggdrasilsPeak"), TEXT("LogSurvival"), TEXT("LogTemp"), TEXT("LogCheatManager") };
		if (Category == Own) { return; }
		bool bKeep = (Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::Warning;
		for (const FName& K : Keep) { bKeep |= Category == K; }
		if (bKeep) { Lines.Enqueue(TPair<double, FString>(FPlatformTime::Seconds() - StartReal, FString::Printf(TEXT("%s: %s"), *Category.ToString(), V))); }
	}
};

// ------------------------------------------------------------------------------------------------ console

static FAutoConsoleCommandWithWorldAndArgs GCamRecDumpCmd(TEXT("ygg.CamRec.Dump"),
	TEXT("Write the camera flight recorder's buffer now (Saved/Monitor/incidents/<session>/<n>_dump)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UYggCameraRecorder* R = UYggCameraRecorder::Get(World)) { R->Dump(FString::Join(Args, TEXT(" "))); }
		else { UE_LOG(LogYggCamRec, Display, TEXT("CamRec: no recorder in this world (game / PIE worlds only)")); }
	}));

static FAutoConsoleCommandWithWorldAndArgs GCamRecTestCmd(TEXT("ygg.CamRec.Test"),
	TEXT("ygg.CamRec.Test [pitch|yaw] [delay s]: fake a 360 degree spin through the control rotation for 1 s (switches ygg.CamRec on) to prove detection."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		bool bYaw = false; float Delay = 0.f;
		for (const FString& A : Args)
		{
			if (A.Equals(TEXT("yaw"), ESearchCase::IgnoreCase)) { bYaw = true; }
			else if (A.IsNumeric()) { Delay = FCString::Atof(*A); }
		}
		UYggCameraRecorder* R = UYggCameraRecorder::Get(World);
		if (!R)
		{
			// -ExecCmds can run before the game world exists: the first recorder that ticks takes the test
			GCamRecPendingTest = bYaw ? 2 : 1;
			GCamRecPendingDelay = Delay;
			UE_LOG(LogYggCamRec, Display, TEXT("CamRec: no recorder in this world yet - the test waits for the first game / PIE world"));
			return;
		}
		R->StartTest(bYaw, Delay);
	}));

// ------------------------------------------------------------------------------------------------ subsystem

UYggCameraRecorder* UYggCameraRecorder::Get(const UObject* Ctx)
{
	UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
	return W ? W->GetSubsystem<UYggCameraRecorder>() : nullptr;
}

bool UYggCameraRecorder::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE) && Super::ShouldCreateSubsystem(Outer);
}

bool UYggCameraRecorder::IsEnabled()
{
	const int32 V = CVarCamRec.GetValueOnGameThread();
	if (V >= 0) { return V > 0; }
	static const bool bAuto = []()
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("YggCamRec"))) { return true; }
#if UE_BUILD_SHIPPING
		return false;
#else
		return !GIsEditor && !FParse::Param(FCommandLine::Get(), TEXT("YggNoSave"));
#endif
	}();
	return bAuto;
}

void UYggCameraRecorder::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartReal = FPlatformTime::Seconds();
	if (GCamRecSession.IsEmpty()) { GCamRecSession = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")); }
	ImageWrappers = FModuleManager::LoadModulePtr<IImageWrapperModule>(TEXT("ImageWrapper"));
	if (FSlateApplication::IsInitialized())
	{
		Input = MakeShared<FYggCamRecInput>();
		FSlateApplication::Get().RegisterInputPreProcessor(Input);
	}
	LogDevice = MakeShared<FYggCamRecLog>();
	LogDevice->StartReal = StartReal;
	if (GLog) { GLog->AddOutputDevice(LogDevice.Get()); }   // the backlog it replays is dropped: bOn is still false
	PostTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UYggCameraRecorder::OnPostActorTick);
}

void UYggCameraRecorder::Deinitialize()
{
	// incidents still waiting for their after-frames are written with what there is
	TArray<FPending> Left = MoveTemp(Pending);
	Pending.Reset();
	for (const FPending& P : Left) { WritePending(P, true); }   // on the game thread: at quit the thread pool may already be gone
	if (PostTickHandle.IsValid()) { FWorldDelegates::OnWorldPostActorTick.Remove(PostTickHandle); PostTickHandle.Reset(); }
	if (ShotHandle.IsValid()) { UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle); ShotHandle.Reset(); }
	if (Input.IsValid() && FSlateApplication::IsInitialized()) { FSlateApplication::Get().UnregisterInputPreProcessor(Input); }
	if (LogDevice.IsValid())
	{
		LogDevice->bOn = false;
		if (GLog) { GLog->RemoveOutputDevice(LogDevice.Get()); }   // the device itself lives on with this object
	}
	if (bWasOn) { UE_LOG(LogYggCamRec, Display, TEXT("CamRec: world ended, %d incident(s) this session -> %s"), GCamRecIncidents, *(FPaths::ProjectSavedDir() / TEXT("Monitor/incidents") / GCamRecSession)); }
	Super::Deinitialize();
}

void UYggCameraRecorder::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BeginPlayReal = FPlatformTime::Seconds();
}

void UYggCameraRecorder::ResetRing()
{
	const float Keep = FMath::Clamp(CVarCamRecSeconds.GetValueOnGameThread(), 1.f, 30.f) + FMath::Clamp(CVarCamRecPost.GetValueOnGameThread(), 0.f, 5.f) + 1.f;
	const int32 Cap = FMath::Clamp(FMath::CeilToInt(Keep * 250.f), 600, 8192);    // 250 fps worth; faster frame rates keep a little less time
	Ring.Reset();
	Ring.SetNum(Cap);
	Head = Count = 0;
	bHavePrev = false;
	LowSince = VtSince = RollSince = -1.0;
	bLowFired = bVtFired = bRollFired = false;
}

const FYggCamRecFrame* UYggCameraRecorder::Back(int32 K) const
{
	if (K < 0 || K >= Count || Ring.Num() == 0) { return nullptr; }
	const int32 Cap = Ring.Num();
	return &Ring[((Head - 1 - K) % Cap + Cap) % Cap];
}

UActorComponent* UYggCameraRecorder::FindRig(APawn* Pawn)
{
#if YGG_CAMREC_HAS_RIG
	if (!Pawn || CVarCamRecRig.GetValueOnGameThread() == 0) { Rig.Reset(); RigPawn.Reset(); return nullptr; }
	if (RigPawn.Get() != Pawn) { RigPawn = Pawn; Rig.Reset(); NextRigFind = 0.0; }
	const double Now = FPlatformTime::Seconds();
	if (!Rig.IsValid() && Now >= NextRigFind)
	{
		Rig = UYggViewRigComponent::Find(Pawn);
		NextRigFind = Now + 1.0;
	}
	return Rig.Get();
#else
	(void)Pawn;
	return nullptr;
#endif
}

void UYggCameraRecorder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const double Now = FPlatformTime::Seconds();
	DrainLog();

	// the self-test writes the control rotation here: after the player controller's tick, before the camera manager
	// builds this frame's view (world tickables run between the two)
	UWorld* W = GetWorld();
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	if (GCamRecPendingTest != 0)
	{
		const bool bYaw = GCamRecPendingTest == 2;
		GCamRecPendingTest = 0;
		StartTest(bYaw, GCamRecPendingDelay);
	}
	const bool bWarm = BeginPlayReal > 0.0 && Now - BeginPlayReal >= CVarCamRecWarm.GetValueOnGameThread();
	// never under a pause: detection is off then and the test would prove nothing
	if (bTestArmed && Now >= TestAt && bWarm && PC && PC->GetPawn() && bWasOn && W && !W->IsPaused())
	{
		bTestArmed = false;
		bTestRunning = true;
		TestStart = Now;
		TestFrom = PC->GetControlRotation();
		UE_LOG(LogYggCamRec, Display, TEXT("CamRec: TEST spinning the control rotation 360 deg about %s for 1 s"), bTestYaw ? TEXT("yaw") : TEXT("pitch"));
	}
	if (bTestRunning)
	{
		if (!PC) { bTestRunning = false; }
		else
		{
			const float A = float((Now - TestStart) / 1.0);
			if (A >= 1.f)
			{
				PC->SetControlRotation(TestFrom);
				bTestRunning = false;
				UE_LOG(LogYggCamRec, Display, TEXT("CamRec: TEST done, control rotation put back"));
			}
			else
			{
				FRotator R = TestFrom;
				if (bTestYaw) { R.Yaw = TestFrom.Yaw + 360.f * A; }
				else { R.Pitch = CamRecNorm(float(TestFrom.Pitch)) + 360.f * A; }
				PC->SetControlRotation(R);
			}
		}
	}

	// incidents whose after-frames are in
	for (int32 I = Pending.Num() - 1; I >= 0; --I)
	{
		if (Now >= Pending[I].WriteAt)
		{
			const FPending P = Pending[I];
			Pending.RemoveAt(I);
			WritePending(P);
		}
	}
	if (ShotHandle.IsValid() && Now - ShotAt > 5.0)     // a request that never came back
	{
		UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle);
		ShotHandle.Reset();
		// drop the stale request too: served later with no handler bound, the engine would write a PNG on the game thread
		if (FScreenshotRequest::IsScreenshotRequested()) { FScreenshotRequest::Reset(); }
	}
}

void UYggCameraRecorder::DrainLog()
{
	if (!LogDevice.IsValid()) { return; }
	LogDevice->bOn = bWasOn;
	TPair<double, FString> L;
	while (LogDevice->Lines.Dequeue(L))
	{
		if (RecentLog.Num() >= 400) { RecentLog.RemoveAt(0, 100, EAllowShrinking::No); }
		RecentLog.Add(MoveTemp(L));
	}
	const double Cut = FPlatformTime::Seconds() - StartReal - (CVarCamRecSeconds.GetValueOnGameThread() + 3.0);
	int32 Old = 0;
	while (Old < RecentLog.Num() && RecentLog[Old].Key < Cut) { ++Old; }
	if (Old) { RecentLog.RemoveAt(0, Old, EAllowShrinking::No); }
}

void UYggCameraRecorder::OnPostActorTick(UWorld* InWorld, ELevelTick TickType, float DeltaSeconds)
{
	// after every actor and the camera managers: this frame's POV is final
	if (InWorld != GetWorld() || TickType == LEVELTICK_ViewportsOnly) { return; }
	Sample(DeltaSeconds);
}

// UCameraModifier::Alpha is protected; it is a UPROPERTY, so read it through reflection (property looked up once)
static float CamRecModAlpha(const UCameraModifier* M)
{
	static const FFloatProperty* AlphaProp = FindFProperty<FFloatProperty>(UCameraModifier::StaticClass(), TEXT("Alpha"));
	return (M && AlphaProp) ? AlphaProp->GetPropertyValue_InContainer(M) : 1.f;
}


void UYggCameraRecorder::Sample(float GameDt)
{
	const double Now = FPlatformTime::Seconds();
	float RawX = 0.f, RawY = 0.f;
	if (Input.IsValid()) { RawX = Input->Dx; RawY = Input->Dy; Input->Dx = Input->Dy = 0.f; }

	if (!IsEnabled())
	{
		if (bWasOn) { bWasOn = false; UE_LOG(LogYggCamRec, Display, TEXT("CamRec: off")); }
		return;
	}
	if (!bWasOn)
	{
		bWasOn = true;
		ResetRing();
		LastReal = Now;
		UE_LOG(LogYggCamRec, Display, TEXT("CamRec: recording the camera (%d frame ring) -> %s"), Ring.Num(), *(FPaths::ProjectSavedDir() / TEXT("Monitor/incidents") / GCamRecSession));
	}

	UWorld* W = GetWorld();
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* PCM = PC ? PC->PlayerCameraManager.Get() : nullptr;
	if (!PC || !PCM || !PC->IsLocalController())
	{
		bHavePrev = false;
		LastReal = Now;
		return;
	}

	FYggCamRecFrame F;
	F.Seq = NextSeq++;
	F.T = Now - StartReal;
	F.Ms = float((Now - LastReal) * 1000.0);
	LastReal = Now;
	F.GameDt = GameDt;

	const FMinimalViewInfo& V = PCM->GetCameraCacheView();
	F.CamLoc = V.Location;
	F.CamPitch = CamRecNorm(float(V.Rotation.Pitch));
	F.CamYaw = CamRecNorm(float(V.Rotation.Yaw));
	F.CamRoll = CamRecNorm(float(V.Rotation.Roll));
	F.Fov = V.FOV;
	const FRotator CR = PC->GetControlRotation();
	F.CtrlPitch = CamRecNorm(float(CR.Pitch));
	F.CtrlYaw = CamRecNorm(float(CR.Yaw));
	F.CtrlRoll = CamRecNorm(float(CR.Roll));

	PC->GetInputMouseDelta(F.MouseX, F.MouseY);
	F.RawX = RawX; F.RawY = RawY;
	PC->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, F.StickX, F.StickY);
	// the robot player turns the view through AddYawInput / AddPitchInput (no mouse): its look is input too. Held 0.15 s,
	// because its input reaches the view a frame later (it is fed after the player controller's tick)
	if (const UYggPlayBot* Bot = W->GetSubsystem<UYggPlayBot>())
	{
		F.bBot = Bot->GetLookFeed(0.15, F.BotYaw, F.BotPitch);
	}
	const float Eps = CVarCamRecMouseEps.GetValueOnGameThread();
	F.bStill = FMath::Abs(F.MouseX) <= Eps && FMath::Abs(F.MouseY) <= Eps && FMath::Abs(F.RawX) <= Eps && FMath::Abs(F.RawY) <= Eps
		&& FMath::Abs(F.StickX) <= 0.15f && FMath::Abs(F.StickY) <= 0.15f && !F.bBot;
	F.bLookIgnored = PC->IsLookInputIgnored();
	F.bMoveIgnored = PC->IsMoveInputIgnored();
	F.bCursor = PC->bShowMouseCursor;
	F.bCutPcm = PCM->bGameCameraCutThisFrame;

	APawn* P = PC->GetPawn();
	AActor* VT = PC->GetViewTarget();
	F.ViewTarget = VT ? VT->GetFName() : NAME_None;
	F.bVtIsPawn = !P || VT == P;
	if (P)
	{
		F.PawnName = P->GetFName();
		F.PawnLoc = P->GetActorLocation();
		const FVector Vel = P->GetVelocity();
		F.PawnVel = FVector3f(Vel);
		F.SpeedH = float(FVector2D(Vel.X, Vel.Y).Size());
		F.bPawnHidden = P->IsHidden();
		if (const ACharacter* C = Cast<ACharacter>(P))
		{
			if (const UCharacterMovementComponent* M = C->GetCharacterMovement())
			{
				F.MoveMode = uint8(M->MovementMode.GetValue());
				F.CustomMode = M->CustomMovementMode;
				if (const UCapsuleComponent* Capsule = C->GetCapsuleComponent())
				{
					if (M->CurrentFloor.bBlockingHit && M->MovementMode == MOVE_Walking)
					{
						const double Floor = C->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight() - M->CurrentFloor.FloorDist;
						F.EyeMeasured = float(F.CamLoc.Z - Floor);
					}
				}
			}
		}
	}

	PCM->ForEachCameraModifier([&F](UCameraModifier* M) -> bool
	{
		if (M && F.NumMods < FYggCamRecFrame::MaxMods)
		{
			const int32 I = F.NumMods++;
			F.ModName[I] = M->GetFName();
			if (M->IsDisabled()) { F.ModOffMask |= uint8(1 << I); }
			F.ModAlpha[I] = uint8(FMath::Clamp(CamRecModAlpha(M), 0.f, 1.f) * 255.f + 0.5f);
		}
		return true;
	});

#if YGG_CAMREC_HAS_RIG
	if (UYggViewRigComponent* R = Cast<UYggViewRigComponent>(FindRig(P)))
	{
		F.Posture = int8(uint8(R->GetView()));
		F.RigEye = R->GetEyeAboveFloor();
		F.RigLid = R->GetLid();
		F.Scripted = CamRecRigScripted(R);
		F.Leases = CamRecRigLeases(R);
		F.bCutRig = CamRecRigCut(R);
		F.Nudging = CamRecRigNudging(R);
		if (Now >= NextAudit)
		{
			NextAudit = Now + FMath::Max(0.05f, CVarCamRecAuditEvery.GetValueOnGameThread());
			FString A = R->Audit();
			A.ReplaceInline(TEXT("\r"), TEXT(""));
			A.ReplaceInline(TEXT("\n"), TEXT(" / "));
			A.TrimEndInline();
			if (A != LastAudit)
			{
				LastAudit = A;
				Audits.Add(TPair<double, FString>(F.T, A));
			}
		}
		// keep the audits of the last Seconds + a little (ids stay stable: AuditBase counts the dropped ones)
		const double Keep = F.T - (CVarCamRecSeconds.GetValueOnGameThread() + CVarCamRecPost.GetValueOnGameThread() + 2.0);
		int32 Old = 0;
		while (Old < Audits.Num() - 1 && Audits[Old].Key < Keep) { ++Old; }
		if (Old) { Audits.RemoveAt(0, Old, EAllowShrinking::No); AuditBase += Old; }
		F.AuditId = Audits.Num() ? AuditBase + Audits.Num() - 1 : -1;
	}
#endif
	F.bTest = bTestRunning;

	// deltas since the last frame (none across a cut, a new pawn or a restart)
	const FYggCamRecFrame* Prev = bHavePrev ? Back(0) : nullptr;
	F.bCut = !Prev || F.bCutPcm || F.bCutRig || Prev->PawnName != F.PawnName;
	if (Prev && !F.bCut)
	{
		F.DCamPitch = CamRecDelta(Prev->CamPitch, F.CamPitch);
		F.DCamYaw = CamRecDelta(Prev->CamYaw, F.CamYaw);
		F.DCtrlPitch = CamRecDelta(Prev->CtrlPitch, F.CtrlPitch);
		F.DCtrlYaw = CamRecDelta(Prev->CtrlYaw, F.CtrlYaw);
	}

	const int32 Cap = Ring.Num();
	Ring[Head] = F;
	FYggCamRecFrame& Stored = Ring[Head];
	Head = (Head + 1) % Cap;
	Count = FMath::Min(Count + 1, Cap);
	bHavePrev = true;

	const bool bWarm = BeginPlayReal > 0.0 && Now - BeginPlayReal >= CVarCamRecWarm.GetValueOnGameThread();
	if (bWarm && P && !W->IsPaused())
	{
		Detect(Stored);
	}
}

void UYggCameraRecorder::Detect(FYggCamRecFrame& F)
{
	const FYggCamRecFrame* Prev = Back(1);
	TArray<TPair<uint8, FString>, TInlineAllocator<4>> Hits;

	// ---- pitch wrapping across +-90 (camera or control rotation), whatever the mouse does
	FString WrapWhy;
	if (FMath::Abs(F.CtrlPitch) > 90.5f) { WrapWhy = FString::Printf(TEXT("control pitch %.1f is beyond +-90"), F.CtrlPitch); }
	else if (FMath::Abs(F.CamPitch) > 90.5f) { WrapWhy = FString::Printf(TEXT("camera pitch %.1f is beyond +-90"), F.CamPitch); }
	if (!F.bCut && Prev)
	{
		FString Why = WrapWhy;
		const auto Check = [&Why](const TCHAR* Who, float P, float PrevP, float DYaw, float DRoll)
		{
			if (!Why.IsEmpty()) { return; }
			if (FMath::Abs(P) > 90.5f) { Why = FString::Printf(TEXT("%s pitch %.1f is beyond +-90"), Who, P); }
			else if ((PrevP > 60.f && P < -60.f) || (PrevP < -60.f && P > 60.f)) { Why = FString::Printf(TEXT("%s pitch jumped %.1f -> %.1f in one frame"), Who, PrevP, P); }
			else if (FMath::Abs(DYaw) > 150.f && FMath::Abs(DRoll) > 150.f) { Why = FString::Printf(TEXT("%s flipped over the pole (yaw %+.0f, roll %+.0f in one frame)"), Who, DYaw, DRoll); }
		};
		Check(TEXT("camera"), F.CamPitch, Prev->CamPitch, F.DCamYaw, CamRecDelta(Prev->CamRoll, F.CamRoll));
		Check(TEXT("control"), F.CtrlPitch, Prev->CtrlPitch, F.DCtrlYaw, CamRecDelta(Prev->CtrlRoll, F.CtrlRoll));
		if (Why.IsEmpty())
		{
			// net pitch travel in the last second: a clamped pitch can never travel more than 180 one way
			float NetCam = 0.f, NetCtrl = 0.f;
			for (int32 K = 0; K < Count; ++K)
			{
				const FYggCamRecFrame* B = Back(K);
				if (!B || F.T - B->T > 1.0 || B->bCut) { break; }
				NetCam += B->DCamPitch; NetCtrl += B->DCtrlPitch;
			}
			if (FMath::Abs(NetCam) > 185.f) { Why = FString::Printf(TEXT("camera pitch travelled %+.0f deg one way in 1 s"), NetCam); }
			else if (FMath::Abs(NetCtrl) > 185.f) { Why = FString::Printf(TEXT("control pitch travelled %+.0f deg one way in 1 s"), NetCtrl); }
		}
		if (!Why.IsEmpty()) { Hits.Add(TPair<uint8, FString>(uint8(CRK_PitchWrap), Why)); }
	}
	else if (!WrapWhy.IsEmpty())
	{
		// a cut (rig teleport / reset / watchdog, new pawn) must not hide a pitch past the pole
		Hits.Add(TPair<uint8, FString>(uint8(CRK_PitchWrap), WrapWhy + TEXT(" (on a cut frame)")));
	}

	// ---- turning with no input
	{
		const float Win = FMath::Max(0.03f, CVarCamRecWindow.GetValueOnGameThread());
		float CP = 0.f, CY = 0.f, KP = 0.f, KY = 0.f; double Span = 0.0; int32 N = 0; bool bOk = true;
		for (int32 K = 0; K < Count; ++K)
		{
			const FYggCamRecFrame* B = Back(K);
			if (!B || F.T - B->T > Win) { break; }
			if (B->bCut || !B->bStill) { bOk = false; break; }
			CP += FMath::Abs(B->DCamPitch); CY += FMath::Abs(B->DCamYaw); KP += FMath::Abs(B->DCtrlPitch); KY += FMath::Abs(B->DCtrlYaw);
			Span += B->Ms / 1000.0; ++N;
		}
		if (bOk && N >= 2 && Span >= Win * 0.6)
		{
			const float Rate = CVarCamRecRate.GetValueOnGameThread(), MinDeg = CVarCamRecMinDeg.GetValueOnGameThread();
			const auto Spin = [&](uint8 Kind, const TCHAR* Axis, float Cam, float Ctrl)
			{
				const float M = FMath::Max(Cam, Ctrl);
				if (M > MinDeg && M / Span > Rate)
				{
					const TCHAR* Src = Cam > MinDeg && Ctrl > MinDeg ? TEXT("camera + control") : Cam >= Ctrl ? TEXT("camera only") : TEXT("control only");
					Hits.Add(TPair<uint8, FString>(Kind, FString::Printf(TEXT("%s turned %.0f deg/s (%.0f deg in %.2f s, %s; cam %.0f, ctrl %.0f) with the mouse and stick still"),
						Axis, M / Span, M, Span, Src, Cam, Ctrl)));
				}
			};
			Spin(uint8(CRK_SpinPitch), TEXT("pitch"), CP, KP);
			Spin(uint8(CRK_SpinYaw), TEXT("yaw"), CY, KY);
		}
	}

	// ---- roll outside a scripted fall / knock / fly-out
	{
		const float RollDeg = CVarCamRecRoll.GetValueOnGameThread();
		// a fall / knock / fly-out (or its lease): the rig's roll is wanted, and it fades out for a while after GetScripted()
		// is back to none, so the check stays off until 3 s after it
		const int64 TakeoverLeases = (int64(1) << 3) | (int64(1) << 4) | (int64(1) << 5);   // EYggLease Fall, Knock, Fly
		if (F.Scripted > 0 || (F.Leases > 0 && (F.Leases & TakeoverLeases) != 0)) { ScriptedT = F.T; }
		const bool bScripted = F.T - ScriptedT < 3.0;
		const bool bRoll = !bScripted && (FMath::Abs(F.CamRoll) > RollDeg || FMath::Abs(F.CtrlRoll) > RollDeg);
		if (bRoll)
		{
			if (RollSince < 0.0) { RollSince = F.T; }
			if (!bRollFired && F.T - RollSince >= 0.05)
			{
				bRollFired = true;
				Hits.Add(TPair<uint8, FString>(uint8(CRK_Roll), FString::Printf(TEXT("roll cam %.1f / ctrl %.1f deg (limit %.1f), takeover %s"), F.CamRoll, F.CtrlRoll, RollDeg, CamRecScripted(F.Scripted))));
			}
		}
		else { RollSince = -1.0; bRollFired = false; }
	}

	// ---- stuck low while walking
	{
		const float EyeCm = CVarCamRecEye.GetValueOnGameThread(), LowCm = CVarCamRecLow.GetValueOnGameThread();
		const bool bLow = F.bVtIsPawn && F.EyeMeasured >= 0.f && F.SpeedH > CVarCamRecMove.GetValueOnGameThread() && F.MoveMode == MOVE_Walking
			&& F.Posture <= 0 && F.Scripted <= 0 && F.EyeMeasured < EyeCm - LowCm;
		if (bLow)
		{
			if (LowSince < 0.0) { LowSince = F.T; }
			if (!bLowFired && F.T - LowSince >= 0.3)
			{
				bLowFired = true;
				Hits.Add(TPair<uint8, FString>(uint8(CRK_LowEye), FString::Printf(TEXT("eye %.0f cm over the floor (normal %.0f) for %.1f s while walking %.0f cm/s; rig eye %.0f, posture %s"),
					F.EyeMeasured, EyeCm, F.T - LowSince, F.SpeedH, F.RigEye, CamRecPosture(F.Posture))));
			}
		}
		else { LowSince = -1.0; bLowFired = false; }
	}

	// ---- view target away from the pawn
	if (!F.bVtIsPawn)
	{
		if (VtSince < 0.0) { VtSince = F.T; }
		if (!bVtFired && F.T - VtSince >= CVarCamRecVt.GetValueOnGameThread())
		{
			bVtFired = true;
			Hits.Add(TPair<uint8, FString>(uint8(CRK_ViewTarget), FString::Printf(TEXT("view target %s (not the pawn %s) for %.1f s"), *F.ViewTarget.ToString(), *F.PawnName.ToString(), F.T - VtSince)));
		}
	}
	else { VtSince = -1.0; bVtFired = false; }

	// ---- a long frame
	if (F.Ms > CVarCamRecHitch.GetValueOnGameThread())
	{
		Hits.Add(TPair<uint8, FString>(uint8(CRK_Hitch), FString::Printf(TEXT("frame %.0f ms"), F.Ms)));
	}

	if (Hits.Num() == 0) { return; }
	// one incident per frame: the order of the checks above is the priority; the rest go in "also"
	FString Also;
	for (int32 I = 1; I < Hits.Num(); ++I) { Also += FString::Printf(TEXT("%s%s: %s"), I > 1 ? TEXT("; ") : TEXT(""), CamRecKindNames[Hits[I].Key], *Hits[I].Value); }
	Fire(F, Hits[0].Key, Hits[0].Value, Also);
}

void UYggCameraRecorder::Fire(FYggCamRecFrame& F, uint8 Kind, const FString& Values, const FString& Also)
{
	const FString KindName = CamRecKindNames[Kind];
	if (GCamRecIncidents >= CVarCamRecMax.GetValueOnGameThread())
	{
		return;
	}
	if (F.T - LastIncidentT < CVarCamRecCooldown.GetValueOnGameThread())
	{
		SuppressedKinds.FindOrAdd(KindName)++;
		return;
	}
	LastIncidentT = F.T;
	F.IncidentKind = Kind;
	++GCamRecIncidents;
	FString AllAlso = Also;
	if (SuppressedKinds.Num())
	{
		FString Sup;
		for (const TPair<FString, int32>& KV : SuppressedKinds) { Sup += FString::Printf(TEXT("%s%s x%d"), Sup.IsEmpty() ? TEXT("") : TEXT(", "), *KV.Key, KV.Value); }
		AllAlso += FString::Printf(TEXT("%sheld back by the cooldown since the last incident: %s"), AllAlso.IsEmpty() ? TEXT("") : TEXT("; "), *Sup);
		SuppressedKinds.Reset();
	}
	Queue(KindName, Values, AllAlso, F.Seq, F.T, CVarCamRecPost.GetValueOnGameThread());
	if (GCamRecIncidents >= CVarCamRecMax.GetValueOnGameThread())
	{
		UE_LOG(LogYggCamRec, Warning, TEXT("CamRec: %d incidents this session - no more are written (ygg.CamRec.Max)"), GCamRecIncidents);
	}
}

void UYggCameraRecorder::Queue(const FString& Kind, const FString& Values, const FString& Also, uint64 Seq, double T, float PostS)
{
	FPending P;
	P.N = ++GCamRecNumber;
	P.Kind = Kind;
	P.Values = Values;
	P.Also = Also;
	P.Seq = Seq;
	P.DetectT = T;
	P.WriteAt = FPlatformTime::Seconds() + FMath::Max(0.f, PostS);
	P.Dir = FPaths::ProjectSavedDir() / TEXT("Monitor/incidents") / GCamRecSession / FString::Printf(TEXT("%d_%s"), P.N, *Kind);
	Pending.Add(P);
	UE_LOG(LogYggCamRec, Warning, TEXT("CamRec: INCIDENT %s %s -> %s"), *Kind, *Values, *P.Dir);
	if (CVarCamRecShot.GetValueOnGameThread() != 0) { RequestShot(P.Dir / TEXT("shot.jpg")); }
}

void UYggCameraRecorder::Dump(const FString& Why)
{
	const FYggCamRecFrame* Newest = Back(0);
	if (!bWasOn || !Newest)
	{
		UE_LOG(LogYggCamRec, Display, TEXT("CamRec: nothing recorded (ygg.CamRec is off or no frames yet)"));
		return;
	}
	Queue(CamRecKindNames[CRK_Dump], Why.IsEmpty() ? FString(TEXT("ygg.CamRec.Dump")) : TEXT("ygg.CamRec.Dump: ") + Why, FString(), Newest->Seq, Newest->T, 0.f);
}

void UYggCameraRecorder::StartTest(bool bYaw, float Delay)
{
	if (!IsEnabled())
	{
		if (IConsoleVariable* CV = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.CamRec"))) { CV->Set(1, ECVF_SetByConsole); }
		UE_LOG(LogYggCamRec, Display, TEXT("CamRec: switched on for the test (ygg.CamRec 1)"));
	}
	bTestYaw = bYaw;
	bTestArmed = true;
	TestAt = FPlatformTime::Seconds() + FMath::Max(0.f, Delay);
	const double WarmLeft = BeginPlayReal > 0.0 ? CVarCamRecWarm.GetValueOnGameThread() - (FPlatformTime::Seconds() - BeginPlayReal) : -1.0;
	UE_LOG(LogYggCamRec, Display, TEXT("CamRec: TEST armed (%s spin) in %.1f s%s"), bYaw ? TEXT("yaw") : TEXT("pitch"), FMath::Max(0.f, Delay),
		WarmLeft > 0.0 ? *FString::Printf(TEXT(", after the %.1f s warm-up"), WarmLeft) : TEXT(""));
}

void UYggCameraRecorder::WritePending(const FPending& P, bool bSync)
{
	FCamRecJob J;
	J.N = P.N;
	J.Kind = P.Kind;
	J.Values = P.Values;
	J.Also = P.Also;
	J.Dir = P.Dir;
	J.SessionDir = FPaths::ProjectSavedDir() / TEXT("Monitor/incidents") / GCamRecSession;
	J.Seq = P.Seq;
	J.DetectT = P.DetectT;
	J.EyeCm = CVarCamRecEye.GetValueOnGameThread();
	J.LowCm = CVarCamRecLow.GetValueOnGameThread();
	J.RollDeg = CVarCamRecRoll.GetValueOnGameThread();
	const UWorld* W = GetWorld();
	J.Map = W ? W->GetMapName() : FString(TEXT("-"));
	const double From = P.DetectT - FMath::Clamp(CVarCamRecSeconds.GetValueOnGameThread(), 1.f, 30.f);
	J.Frames.Reserve(Count);
	for (int32 K = Count - 1; K >= 0; --K)
	{
		const FYggCamRecFrame* F = Back(K);
		if (F && F->T >= From) { J.Frames.Add(*F); }
	}
	for (int32 I = 0; I < Audits.Num(); ++I)
	{
		if (Audits[I].Key >= From || I == Audits.Num() - 1 || (I + 1 < Audits.Num() && Audits[I + 1].Key >= From))   // the one in force at the start too
		{
			if (J.Audits.Num() == 0) { J.AuditBase = AuditBase + I; }
			J.Audits.Add(Audits[I]);
		}
	}
	for (const TPair<double, FString>& L : RecentLog) { if (L.Key >= From) { J.Logs.Add(L); } }
	if (J.Frames.Num() == 0) { return; }
	if (bSync) { CamRecWriteJob(MoveTemp(J)); return; }
	Async(EAsyncExecution::ThreadPool, [Job = MoveTemp(J)]() mutable { CamRecWriteJob(MoveTemp(Job)); });
}

void UYggCameraRecorder::RequestShot(const FString& Path)
{
	// one at a time; the pixels come to OnShotCaptured (no engine PNG on the game thread). If the session monitor asked
	// for a shot in the same frame, both get the same pixels.
	if (ShotHandle.IsValid() || !ImageWrappers) { return; }
	ShotPath = Path;
	ShotAt = FPlatformTime::Seconds();
	ShotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UYggCameraRecorder::OnShotCaptured);
	FScreenshotRequest::RequestScreenshot(Path, false, false);
}

void UYggCameraRecorder::OnShotCaptured(int32 W, int32 H, const TArray<FColor>& Px)
{
	UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle);
	ShotHandle.Reset();
	if (W <= 0 || H <= 0 || Px.Num() < W * H || !ImageWrappers) { return; }
	const int32 Step = FMath::Max(1, FMath::DivideAndRoundUp(W, 1280));
	const int32 OW = W / Step, OH = H / Step;
	TArray<FColor> Small;
	Small.SetNumUninitialized(OW * OH);
	for (int32 Y = 0; Y < OH; ++Y)
	{
		const FColor* Row = Px.GetData() + int64(Y * Step) * W;
		for (int32 X = 0; X < OW; ++X) { FColor C = Row[X * Step]; C.A = 255; Small[Y * OW + X] = C; }
	}
	IImageWrapperModule* Mod = ImageWrappers;
	Async(EAsyncExecution::ThreadPool, [Mod, File = ShotPath, OW, OH, Small = MoveTemp(Small)]()
	{
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
		const TSharedPtr<IImageWrapper> Jpg = Mod->CreateImageWrapper(EImageFormat::JPEG);
		if (Jpg.IsValid() && Jpg->SetRaw(Small.GetData(), int64(Small.Num()) * sizeof(FColor), OW, OH, ERGBFormat::BGRA, 8))
		{
			FFileHelper::SaveArrayToFile(Jpg->GetCompressed(85), *File);
		}
	});
}
