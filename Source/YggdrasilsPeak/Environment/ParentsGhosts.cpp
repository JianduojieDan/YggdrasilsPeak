// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/ParentsGhosts.h"
#include "Survival/YggAssets.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

static FAutoConsoleCommandWithWorld CmdGhosts(TEXT("ygg.Ghosts"), TEXT("Shows the parents' ghosts ahead of the player."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { AParentsGhosts::SpawnAhead(World); }));

AParentsGhosts::AParentsGhosts()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	auto Make = [this](const TCHAR* Name, const FVector& Offset, float Scale)
	{
		USkeletalMeshComponent* M = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		M->SetupAttachment(GetRootComponent());
		M->SetRelativeLocation(Offset);
		M->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));     // the climber mesh faces +Y
		M->SetRelativeScale3D(FVector(Scale));
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		return M;
	};
	Father = Make(TEXT("Father"), FVector(0.f, -70.f, 0.f), 1.f);
	Mother = Make(TEXT("Mother"), FVector(-110.f, 75.f, 0.f), 0.94f);
}

namespace
{
	const TCHAR* const GhostsMeshPath = TEXT("/Game/Characters/Climber_Rigged/SKM_Climber_Rigged.SKM_Climber_Rigged");
	const TCHAR* const GhostsWalkPath = TEXT("/Game/Characters/Climber_Rigged/Anims/MF_Unarmed_Walk_Fwd_Climber.MF_Unarmed_Walk_Fwd_Climber");
	const TCHAR* const GhostsMaterialPath = TEXT("/Game/Environment/Story/M_Ghost.M_Ghost");
}

void AParentsGhosts::Preload()
{
	// 2026-11-14: the ghosts appear mid-play (a frayed mind); loading these at their BeginPlay was a wait for the disk on
	// the game thread at that very moment
	YggAssets::Kept<USkeletalMesh>(GhostsMeshPath);
	YggAssets::Kept<UAnimSequence>(GhostsWalkPath);
	YggAssets::Kept<UMaterialInterface>(GhostsMaterialPath);
}

void AParentsGhosts::BeginPlay()
{
	Super::BeginPlay();
	USkeletalMesh* Mesh = YggAssets::Kept<USkeletalMesh>(GhostsMeshPath);           // kept since Preload
	UAnimSequence* Walk = YggAssets::Kept<UAnimSequence>(GhostsWalkPath);
	UMaterialInterface* Ghost = YggAssets::Kept<UMaterialInterface>(GhostsMaterialPath);
	float Offset = 0.f;
	for (USkeletalMeshComponent* M : { Father.Get(), Mother.Get() })
	{
		M->SetSkeletalMesh(Mesh);
		if (Ghost)
		{
			for (int32 i = 0; i < M->GetNumMaterials(); ++i)
			{
				UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Ghost, this);
				D->SetScalarParameterValue(TEXT("Fade"), 0.f);
				M->SetMaterial(i, D);
				Ghostly.Add(D);
			}
		}
		if (Walk)
		{
			M->PlayAnimation(Walk, true);
			M->SetPlayRate(0.8f);
			M->SetPosition(Offset);       // out of step with each other
			Offset += 0.45f;
		}
	}
}

AParentsGhosts* AParentsGhosts::SpawnAhead(UWorld* World, float Distance)
{
	APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!Pawn)
	{
		return nullptr;
	}
	const float Yaw = Pawn->GetControlRotation().Yaw + FMath::FRandRange(-20.f, 20.f);
	const FVector Dir = FRotator(0.f, Yaw, 0.f).Vector();
	FVector At = Pawn->GetActorLocation() + Dir * Distance;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GhostGround), false, Pawn);
	if (World->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 5000.f), At - FVector(0.f, 0.f, 8000.f), ECC_Visibility, Params))
	{
		At = Hit.ImpactPoint;
	}
	// walking away from you, up the mountain
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AParentsGhosts>(At, FRotator(0.f, Yaw, 0.f), P);
}

void AParentsGhosts::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Age > Lifetime || (Pawn && FVector::Dist2D(Pawn->GetActorLocation(), GetActorLocation()) < VanishDistance))
	{
		bLeaving = true;
	}
	Fade = FMath::FInterpTo(Fade, bLeaving ? 0.f : 1.f, DeltaSeconds, bLeaving ? 1.6f : 0.7f);
	for (UMaterialInstanceDynamic* D : Ghostly)
	{
		D->SetScalarParameterValue(TEXT("Fade"), Fade);
	}
	if (bLeaving && Fade < 0.02f)
	{
		Destroy();
		return;
	}
	// walk on, feet on the snow
	FVector At = GetActorLocation() + GetActorForwardVector() * WalkSpeed * DeltaSeconds;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GhostWalk), false, this);
	if (Pawn)
	{
		Params.AddIgnoredActor(Pawn);
	}
	if (GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 300.f), At - FVector(0.f, 0.f, 600.f), ECC_Visibility, Params))
	{
		At.Z = Hit.ImpactPoint.Z;
	}
	SetActorLocation(At);
}
