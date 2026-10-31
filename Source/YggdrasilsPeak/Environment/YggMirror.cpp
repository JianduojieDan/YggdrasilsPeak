// Copyright Epic Games, Inc. All Rights Reserved.

#include "YggMirror.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AYggMirror::AYggMirror()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the camera has moved this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MirrorMaterial(TEXT("/Game/Props/Mirror/M_Mirror.M_Mirror"));

	// Root faces +X (the glass normal); the plane mesh faces +Z, so it is turned up onto the root's X axis.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Glass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Glass"));
	Glass->SetupAttachment(Root);
	Glass->SetStaticMesh(PlaneMesh.Object);
	Glass->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Glass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glass->SetCastShadow(false);

	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(Root);
	Frame->SetStaticMesh(CubeMesh.Object);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->CaptureSource = SCS_FinalColorLDR;
	Capture->bEnableClipPlane = true;
	// Keep the capture's view state between frames: Lumen (bounce light off the snow) and exposure need history,
	// without it the reflected character is lit by the sun alone and comes out almost black.
	Capture->bAlwaysPersistRenderingState = true;

	GlassMaterial = MirrorMaterial.Object;
}

void AYggMirror::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Plane mesh is 100 x 100; its X runs along the root's -Z after the pitch, its Y along the root's Y.
	Glass->SetRelativeScale3D(FVector(Size.Y / 100.f, Size.X / 100.f, 1.f));
	Glass->SetRelativeLocation(FVector(0.f, 0.f, Size.Y * 0.5f));
	Frame->SetRelativeScale3D(FVector(0.04f, (Size.X + 10.f) / 100.f, (Size.Y + 10.f) / 100.f));
	Frame->SetRelativeLocation(FVector(-3.f, 0.f, Size.Y * 0.5f));
}

void AYggMirror::BeginPlay()
{
	Super::BeginPlay();
	Capture->HideComponent(Glass);
	Capture->HideComponent(Frame);
	if (GlassMaterial)
	{
		GlassInstance = UMaterialInstanceDynamic::Create(GlassMaterial, this);
		Glass->SetMaterial(0, GlassInstance);
	}
}

void AYggMirror::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Camera || !Viewport)
	{
		return;
	}

	const FVector Eye = Camera->GetCameraLocation();
	const FVector Normal = GetActorForwardVector();
	const FVector Plane = Glass->GetComponentLocation();
	const float EyeSide = FVector::DotProduct(Eye - Plane, Normal);
	if (EyeSide <= 0.f || FVector::Dist(Eye, Plane) > CaptureDistance)
	{
		return;   // behind the mirror or far away: nothing to show
	}

	// Render target follows the screen's shape and size (scaled), so the screen-space lookup on the glass is 1:1.
	FVector2D ScreenSize;
	Viewport->GetViewportSize(ScreenSize);
	const int32 Width = FMath::Max(64, FMath::RoundToInt(ScreenSize.X * ResolutionScale));
	const int32 Height = FMath::Max(64, FMath::RoundToInt(ScreenSize.Y * ResolutionScale));
	if (!Target || Target->SizeX != Width || Target->SizeY != Height)
	{
		Target = UKismetRenderingLibrary::CreateRenderTarget2D(this, Width, Height, RTF_RGBA16f);
		Capture->TextureTarget = Target;
		if (GlassInstance)
		{
			GlassInstance->SetTextureParameterValue(TEXT("Reflection"), Target);
		}
	}

	// Reflect the eye and its view axes across the glass. The mirrored axes are a left-handed frame; building the
	// rotation from forward + up flips the image left-right, which the glass material flips back.
	const FMatrix View = FRotationMatrix(Camera->GetCameraRotation());
	const auto Reflect = [&Normal](const FVector& V) { return V - 2.f * FVector::DotProduct(V, Normal) * Normal; };
	const FVector MirroredEye = Eye - 2.f * EyeSide * Normal;
	const FVector Forward = Reflect(View.GetUnitAxis(EAxis::X));
	const FVector Up = Reflect(View.GetUnitAxis(EAxis::Z));
	Capture->SetWorldLocationAndRotation(MirroredEye, FRotationMatrix::MakeFromXZ(Forward, Up).Rotator());
	Capture->FOVAngle = Camera->GetFOVAngle();
	// Only what is in front of the glass appears in it.
	Capture->ClipPlaneBase = Plane;
	Capture->ClipPlaneNormal = Normal;
	Capture->CaptureScene();
}
