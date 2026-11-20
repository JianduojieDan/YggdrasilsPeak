// ygg.GroundProbe: measures how the player stands on the ground (development plan A6, "floating on slopes").
// Collision is found with a line trace (what the capsule stands on); the VISIBLE surface is read from a scene depth capture
// looking straight down (it includes Nanite tessellation displacement and anything else the renderer draws, which a trace
// never sees). Per call one line in the log and in Saved/Review/ground_probe.csv:
//   capsule bottom over the collision under its axis, visible surface minus collision (axis, both boots, eye), each boot's
//   sole over the visible surface, eye over the visible surface, slope, what the trace hit, powder sink and hips drop.
// "ygg.GroundProbe <label> grid" adds visible-minus-collision on a 5 x 5 grid, 1 m apart, around the player.
#include "CoreMinimal.h"
#include <limits>
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TextureResource.h"
#include "Survival/FirstPersonBodyComponent.h"
#include "Survival/YggViewRig.h"

namespace
{
	TWeakObjectPtr<USceneCaptureComponent2D> GProbeCapture;

	USceneCaptureComponent2D* ProbeCapture(ACharacter* C)
	{
		if (USceneCaptureComponent2D* Cap = GProbeCapture.Get())
		{
			return Cap;
		}
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(C);
		RT->RenderTargetFormat = RTF_RGBA32f;
		RT->ClearColor = FLinearColor(1e7f, 0.f, 0.f, 0.f);
		RT->InitAutoFormat(8, 8);
		RT->UpdateResourceImmediate(true);
		USceneCaptureComponent2D* Cap = NewObject<USceneCaptureComponent2D>(C, TEXT("YggGroundProbeCapture"));
		Cap->SetUsingAbsoluteLocation(true);
		Cap->SetUsingAbsoluteRotation(true);
		Cap->bCaptureEveryFrame = false;
		Cap->bCaptureOnMovement = false;
		Cap->CaptureSource = SCS_SceneDepth;
		Cap->FOVAngle = 1.f;
		Cap->TextureTarget = RT;
		Cap->HiddenActors.Add(C);
		Cap->RegisterComponent();
		GProbeCapture = Cap;
		return Cap;
	}

	/** the visible surface's height at P (x, y): depth straight down from 4 m above the collision there; NaN when nothing */
	double VisibleZ(ACharacter* C, const FVector& P, double FromZ)
	{
		USceneCaptureComponent2D* Cap = ProbeCapture(C);
		const FVector Eye(P.X, P.Y, FromZ);
		Cap->SetWorldLocationAndRotation(Eye, FRotator(-90.f, 0.f, 0.f));
		Cap->CaptureScene();
		FTextureRenderTargetResource* Res = Cap->TextureTarget ? Cap->TextureTarget->GameThread_GetRenderTargetResource() : nullptr;
		TArray<FLinearColor> Px;
		if (!Res || !Res->ReadLinearColorPixels(Px) || Px.Num() < 64)
		{
			return std::numeric_limits<double>::quiet_NaN();
		}
		// the middle 2 x 2 pixels (the view is 7 cm wide at 4 m)
		double D = 0.0;
		for (int32 i : { 27, 28, 35, 36 }) { D += Px[i].R; }
		D *= 0.25;
		return D > 1e6 ? std::numeric_limits<double>::quiet_NaN() : FromZ - D;
	}

	bool CollisionZ(UWorld* W, ACharacter* C, const FVector& P, double Top, FHitResult& Hit)
	{
		FCollisionQueryParams Q(SCENE_QUERY_STAT(YggGroundProbe), true, C);
		return W->LineTraceSingleByChannel(Hit, FVector(P.X, P.Y, Top), FVector(P.X, P.Y, Top - 2000.0), ECC_Visibility, Q);
	}

	// a plain teleport (no ghost, collision stays on), the way the story, the route check and camp travel move the climber
	FAutoConsoleCommandWithWorldAndArgs GGroundProbeGo(
		TEXT("ygg.GroundProbe.Go"),
		TEXT("ygg.GroundProbe.Go x y z [yaw] [pitch]: TeleportTo there (collision on, unlike BugItGo's ghost), facing yaw, looking at pitch."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
		{
			APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
			ACharacter* C = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
			if (!C || Args.Num() < 3) { return; }
			FVector To(FCString::Atod(*Args[0]), FCString::Atod(*Args[1]), FCString::Atod(*Args[2]));
			if (Args[2] == TEXT("g"))
			{
				// on the ground there: the highest blocking surface under 8 km of air
				FHitResult G;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(YggGroundProbeGo), true, C);
				if (!W->LineTraceSingleByChannel(G, FVector(To.X, To.Y, 900000.0), FVector(To.X, To.Y, -300000.0), ECC_Visibility, Q))
				{
					UE_LOG(LogTemp, Warning, TEXT("GroundProbe.Go: no ground at %.0f %.0f"), To.X, To.Y);
					return;
				}
				To.Z = G.ImpactPoint.Z + C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 15.0;
			}
			const float Yaw = Args.Num() > 3 ? FCString::Atof(*Args[3]) : C->GetActorRotation().Yaw;
			const float Pitch = Args.Num() > 4 ? FCString::Atof(*Args[4]) : 0.f;
			const bool bOk = C->TeleportTo(To, FRotator(0.f, Yaw, 0.f));
			// (2026-11-20) a teleport is a camera cut and a rig reset, as the bots do it (HardReset sets the control rotation)
			if (bOk)
			{
				if (PC->PlayerCameraManager) { PC->PlayerCameraManager->SetGameCameraCutThisFrame(); }
				if (UYggViewRigComponent* R = UYggViewRigComponent::Find(C)) { R->HardReset(FRotator(Pitch, Yaw, 0.f), EYggView::Walk); }
				else { PC->SetControlRotation(FRotator(Pitch, Yaw, 0.f)); }
			}
			UE_LOG(LogTemp, Display, TEXT("GroundProbe.Go: %s %s"), *To.ToCompactString(), bOk ? TEXT("ok") : TEXT("FAILED (blocked)"));
		}));

	FAutoConsoleCommandWithWorldAndArgs GGroundProbe(
		TEXT("ygg.GroundProbe"),
		TEXT("ygg.GroundProbe [label] [grid]: capsule, boots and eye against the collision (trace) and the visible surface (depth capture); log + Saved/Review/ground_probe.csv"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
		{
			APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
			ACharacter* C = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
			UCharacterMovementComponent* M = C ? C->GetCharacterMovement() : nullptr;
			if (!C || !M)
			{
				UE_LOG(LogTemp, Warning, TEXT("GroundProbe: no player character"));
				return;
			}
			const FString Label = Args.Num() > 0 ? Args[0] : TEXT("-");
			const bool bGrid = Args.Num() > 1 && Args[1] == TEXT("grid");
			const FVector Cap = C->GetCapsuleComponent()->GetComponentLocation();
			const float Half = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const float Radius = C->GetCapsuleComponent()->GetScaledCapsuleRadius();
			const double CapBottom = Cap.Z - Half;
			const double Top = Cap.Z + 300.0;
			const double From = Cap.Z + 400.0;
			const double NaN = std::numeric_limits<double>::quiet_NaN();

			FHitResult H;
			const bool bHit = CollisionZ(W, C, Cap, Top, H);
			const double ColZ = bHit ? H.ImpactPoint.Z : NaN;
			const float Slope = bHit ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(H.ImpactNormal.Z), -1.f, 1.f))) : -1.f;
			const double VisZ = VisibleZ(C, Cap, From);

			FVector SL(Cap), SR(Cap);
			const UFirstPersonBodyComponent* B = C->FindComponentByClass<UFirstPersonBodyComponent>();
			const bool bSoles = B && B->GetOwnSoles(SL, SR);
			auto Under = [&](const FVector& P, double& Col, double& Vis)
			{
				// (2026-11-20) start higher the further out the cell is (an uphill cell on a steep slope was over Top), and
				// capture from 4 m above the cell's own collision, as VisibleZ says
				FHitResult Hh;
				const double Reach = FVector::Dist2D(P, Cap) * 2.0;
				Col = CollisionZ(W, C, P, Top + Reach, Hh) ? Hh.ImpactPoint.Z : NaN;
				Vis = VisibleZ(C, P, FMath::IsNaN(Col) ? From + Reach : Col + 400.0);
			};
			double ColL = NaN, VisL = NaN, ColR = NaN, VisR = NaN, ColE = NaN, VisE = NaN;
			if (bSoles) { Under(SL, ColL, VisL); Under(SR, ColR, VisR); }
			const FVector EyeLoc = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : Cap;
			Under(EyeLoc, ColE, VisE);

			const float Sink = B ? B->GetSinkDepth() : 0.f;
			const float Drop = B ? B->GetFootDrop() : 0.f;
			const float FloorDist = M->CurrentFloor.bBlockingHit ? M->CurrentFloor.FloorDist : -1.f;
			const FString What = bHit ? FString::Printf(TEXT("%s/%s"), *GetNameSafe(H.GetActor()), *GetNameSafe(H.GetComponent())) : TEXT("nothing");
			const FString Line = FString::Printf(
				TEXT("%s,%.0f,%.0f,%.0f,%.1f,%s,%s,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f"),
				*Label, Cap.X, Cap.Y, Cap.Z, Slope, *What, *UEnum::GetValueAsString(M->MovementMode.GetValue()),
				CapBottom - ColZ,                // capsule bottom over the collision under its axis
				VisZ - ColZ,                     // visible surface minus collision, axis
				VisL - ColL, VisR - ColR,        // ... under each boot
				SL.Z - VisL, SR.Z - VisR,        // each sole over the visible surface (+ = floating)
				SL.Z - ColL, SR.Z - ColR,        // each sole over the collision
				EyeLoc.Z - VisE, VisE - ColE,    // eye over the visible surface under it; visible minus collision there
				Sink, Drop, FloorDist, Radius);
			UE_LOG(LogTemp, Display, TEXT("GroundProbe: %s"), *Line);
			UE_LOG(LogTemp, Display, TEXT("GroundProbe %s: slope %.1f deg on %s | capsule bottom %+.1f cm over collision | visible-collision axis %+.1f, boots %+.1f / %+.1f, eye %+.1f | soles over visible %+.1f / %+.1f (over collision %+.1f / %+.1f) | eye over visible %.1f | sink %.1f, hips drop %.1f, floor dist %.1f, radius %.0f"),
				*Label, Slope, *What, CapBottom - ColZ, VisZ - ColZ, VisL - ColL, VisR - ColR, VisE - ColE, SL.Z - VisL, SR.Z - VisR, SL.Z - ColL, SR.Z - ColR,
				EyeLoc.Z - VisE, Sink, Drop, FloorDist, Radius);
			// what holds the capsule: its floor, a capsule sweep down, the next surface under the collision, the hit's response to pawns
			{
				const FHitResult& F = M->CurrentFloor.HitResult;
				UCapsuleComponent* CC = C->GetCapsuleComponent();
				FHitResult Sw;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(YggGroundProbeSweep), false, C);
				FCollisionResponseParams RP;
				CC->InitSweepCollisionParams(Q, RP);
				const bool bSw = W->SweepSingleByChannel(Sw, Cap + FVector(0, 0, 60), Cap - FVector(0, 0, 300), FQuat::Identity, CC->GetCollisionObjectType(),
					CC->GetCollisionShape(), Q, RP);
				FHitResult Lo2;
				FCollisionQueryParams Q2(SCENE_QUERY_STAT(YggGroundProbe2), true, C);
				if (bHit && H.GetActor()) { Q2.AddIgnoredActor(H.GetActor()); }
				const bool bLo2 = W->LineTraceSingleByChannel(Lo2, FVector(Cap.X, Cap.Y, Top), FVector(Cap.X, Cap.Y, Top - 3000.0), ECC_Visibility, Q2);
				UPrimitiveComponent* HC = bHit ? H.GetComponent() : nullptr;
				UE_LOG(LogTemp, Display, TEXT("GroundProbe %s holds: floor %s %s walkable %d dist %.1f | capsule sweep (%s) %s at bottom %+.1f over collision, start-penetrating %d (depth %.1f) | next surface under the collision: %s %+.1f cm | hit's response to the capsule %d, collision enabled %d, complex-as-simple %d"),
					*Label, M->CurrentFloor.bBlockingHit ? TEXT("on") : TEXT("none"), *GetNameSafe(F.GetComponent()), M->CurrentFloor.IsWalkableFloor() ? 1 : 0, M->CurrentFloor.FloorDist,
					*UEnum::GetValueAsString(CC->GetCollisionObjectType()), bSw ? *GetNameSafe(Sw.GetComponent()) : TEXT("nothing"),
					bSw ? (Sw.Location.Z - Half) - ColZ : 0.0, bSw && Sw.bStartPenetrating ? 1 : 0, bSw ? Sw.PenetrationDepth : 0.f,
					bLo2 ? *GetNameSafe(Lo2.GetComponent()) : TEXT("nothing"), bLo2 ? Lo2.ImpactPoint.Z - ColZ : 0.0,
					HC ? int32(HC->GetCollisionResponseToChannel(CC->GetCollisionObjectType())) : -1, HC ? int32(HC->GetCollisionEnabled()) : -1,
					HC && HC->GetBodySetup() ? int32(HC->GetBodySetup()->CollisionTraceFlag) : -1);
			}
			UE_LOG(LogTemp, Display, TEXT("GroundProbe %s movement: velocity %s, gravity scale %.2f, capsule collision %d (pawn response to the tile %d), cheat flying %d, CMC active %d, half height %.1f, time dilation %.2f"),
				*Label, *M->Velocity.ToCompactString(), M->GravityScale, int32(C->GetCapsuleComponent()->GetCollisionEnabled()),
				bHit && H.GetComponent() ? int32(C->GetCapsuleComponent()->GetCollisionResponseToChannel(H.GetComponent()->GetCollisionObjectType())) : -1,
				M->bCheatFlying ? 1 : 0, M->IsActive() ? 1 : 0, Half, C->CustomTimeDilation);
			{
				// what the capsule touches where it is now (1 cm larger), with each one's push-out
				UCapsuleComponent* CC = C->GetCapsuleComponent();
				TArray<FOverlapResult> Ov;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(YggGroundProbeOv), true, C);
				FCollisionResponseParams RP;
				CC->InitSweepCollisionParams(Q, RP);
				W->OverlapMultiByChannel(Ov, Cap, FQuat::Identity, CC->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius + 1.f, Half + 1.f), Q, RP);
				FString L;
				for (const FOverlapResult& O : Ov)
				{
					UPrimitiveComponent* P = O.GetComponent();
					FMTDResult Mtd;
					const bool bPen = P && P->ComputePenetration(Mtd, FCollisionShape::MakeCapsule(Radius, Half), Cap, FQuat::Identity);
					L += FString::Printf(TEXT(" [%s/%s block %d pen %s %.1f cm]"), *GetNameSafe(P ? P->GetOwner() : nullptr), *GetNameSafe(P), O.bBlockingHit ? 1 : 0,
						bPen ? *Mtd.Direction.ToCompactString() : TEXT("-"), bPen ? Mtd.Distance : 0.f);
				}
				UE_LOG(LogTemp, Display, TEXT("GroundProbe %s touches:%s"), *Label, L.IsEmpty() ? TEXT(" nothing") : *L);
			}
			FString Grid;
			if (bGrid)
			{
				double Sum = 0.0, Lo = 1e9, Hi = -1e9;
				int32 N = 0;
				for (int32 y = -2; y <= 2; ++y)
				{
					for (int32 x = -2; x <= 2; ++x)
					{
						const FVector P = Cap + FVector(x * 100.0, y * 100.0, 0.0);
						double Col, Vis;
						Under(P, Col, Vis);
						if (FMath::IsNaN(Col) || FMath::IsNaN(Vis)) { continue; }
						const double D = Vis - Col;
						Sum += D; Lo = FMath::Min(Lo, D); Hi = FMath::Max(Hi, D); ++N;
						Grid += FString::Printf(TEXT(" %+.0f"), D);
					}
					Grid += TEXT(" |");
				}
				UE_LOG(LogTemp, Display, TEXT("GroundProbe %s grid (visible - collision, cm, 5x5 at 1 m):%s mean %+.1f min %+.1f max %+.1f"), *Label, *Grid,
					N ? Sum / N : 0.0, Lo, Hi);
			}
			const FString Dir = FPaths::ProjectSavedDir() / TEXT("Review");
			IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
			PF.CreateDirectoryTree(*Dir);
			const FString File = Dir / TEXT("ground_probe.csv");
			if (!PF.FileExists(*File))
			{
				FFileHelper::SaveStringToFile(TEXT("label,x,y,z,slope,hit,mode,cap_over_col,vis_minus_col,vis_minus_col_l,vis_minus_col_r,sole_over_vis_l,sole_over_vis_r,sole_over_col_l,sole_over_col_r,eye_over_vis,vis_minus_col_eye,sink,hips_drop,floor_dist,radius,grid\n"), *File);
			}
			FFileHelper::SaveStringToFile(Line + TEXT(",") + Grid.Replace(TEXT(","), TEXT(";")) + TEXT("\n"), *File, FFileHelper::EEncodingOptions::AutoDetect,
				&IFileManager::Get(), FILEWRITE_Append);
		}));
}
