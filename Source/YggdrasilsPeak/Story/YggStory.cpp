#include "Story/YggStory.h"

#include "Survival/YggAssets.h"
#include "Survival/YggTime.h"
#include "Survival/YggUIStyle.h"
#include "AIController.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

bool UYggStorySubsystem::bPendingNewGame = false;

namespace
{
	const TCHAR* const NPCMeshPath = TEXT("/Game/Characters/Climber_Rigged/SKM_Climber_Rigged.SKM_Climber_Rigged");
	const TCHAR* const NPCAnimPath = TEXT("/Game/Characters/Climber_Rigged/Anims/ABP_Unarmed_Climber.ABP_Unarmed_Climber_C");

	float Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	/** 0 -> 1 in the first fifth, holds, 1 -> 0 in the last fifth: words, titles, flashes. */
	float Pulse(float T) { return Ease(T / 0.2f) * Ease((1.f - T) / 0.2f); }

	FString Str(const TSharedPtr<FJsonObject>& O, const TCHAR* K, const FString& Def = FString())
	{
		FString S;
		return O.IsValid() && O->TryGetStringField(K, S) ? S : Def;
	}
	float Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, float Def)
	{
		double D;
		return O.IsValid() && O->TryGetNumberField(K, D) ? float(D) : Def;
	}
	bool Bool(const TSharedPtr<FJsonObject>& O, const TCHAR* K, bool Def)
	{
		bool B;
		return O.IsValid() && O->TryGetBoolField(K, B) ? B : Def;
	}
	TArray<FString> Strs(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		TArray<FString> Out;
		const TArray<TSharedPtr<FJsonValue>>* Arr;
		if (O.IsValid() && O->TryGetArrayField(K, Arr)) { for (const TSharedPtr<FJsonValue>& V : *Arr) { Out.Add(V->AsString()); } }
		else { FString S; if (O.IsValid() && O->TryGetStringField(K, S)) { Out.Add(S); } }
		return Out;
	}

	UFont* StoryFace(int32 Which)
	{
		static const TCHAR* const Paths[2] = { TEXT("/Game/UI/Fonts/FF_Cinzel.FF_Cinzel"), TEXT("/Game/UI/Fonts/FF_Caveat.FF_Caveat") };
		static TStrongObjectPtr<UFont> Made[2];
		if (!Made[Which].IsValid()) { if (UFont* F = YggUI::MakeFace(GetTransientPackage(), Paths[Which])) { Made[Which].Reset(F); } }
		return Made[Which].Get();
	}
	FSlateFontInfo Face(int32 Which, int32 Size, int32 Track = 0)
	{
		UFont* F = StoryFace(Which);
		FSlateFontInfo I = F ? FSlateFontInfo(F, Size, TEXT("Regular")) : YggUI::SerifInfo(Size);
		I.LetterSpacing = Track;
		return I;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdStart(TEXT("ygg.Story.Start"), TEXT("ygg.Story.Start [Chapter1] - play a chapter (Content/Story/<name>.ygs)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr) { S->Start(Args.Num() ? Args[0] : TEXT("Chapter1")); }
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdJump(TEXT("ygg.Story.Jump"), TEXT("ygg.Story.Jump <beat> [Chapter1] - start the chapter (if needed) at that beat (testing)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr;
			if (!S || Args.Num() == 0) { return; }
			if (!S->IsRunning()) { S->Start(Args.Num() > 1 ? Args[1] : TEXT("Chapter1")); }
			S->JumpTo(Args[0]);
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdChoose(TEXT("ygg.Story.Choose"), TEXT("ygg.Story.Choose <1-4> - answer the open choice (testing)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr) { S->Choose(Args.Num() ? FCString::Atoi(*Args[0]) : 1); }
		}));
	FAutoConsoleCommandWithWorld CmdStop(TEXT("ygg.Story.Stop"), TEXT("Stop the story."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr) { S->Stop(); }
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdParam(TEXT("ygg.Story.Param"), TEXT("ygg.Story.Param <actor tag> <material parameter> <value> - set a scalar on every mesh of the tagged actors (testing: ygg.Story.Param plume Glow 1)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || Args.Num() < 3) { return; }
			const FName Tag(*Args[0]), Name(*Args[1]);
			const float Value = FCString::Atof(*Args[2]);
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->ActorHasTag(Tag)) { continue; }
				TInlineComponentArray<UMeshComponent*> Meshes(*It);
				for (UMeshComponent* Mesh : Meshes)
				{
					for (int32 m = 0; m < Mesh->GetNumMaterials(); ++m)
					{
						UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(m));
						if (!D) { D = Mesh->CreateAndSetMaterialInstanceDynamic(m); }
						if (D) { D->SetScalarParameterValue(Name, Value); }
					}
				}
			}
		}));
	TAutoConsoleVariable<int32> CVarStoryDebug(TEXT("ygg.Story.Debug"), 0, TEXT("1 = log the fly-out camera every frame."));
	TAutoConsoleVariable<int32> CVarStoryAuto(TEXT("ygg.Story.Auto"), 0,
		TEXT("Testing: 1 = choices answer themselves (first option) and cards close after 2 s, so a run can play the chapter unattended."));
}

// ------------------------------------------------------------------------------------------------ camera modifier

bool UYggStoryCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	InOutPOV.Location.Z -= DownCm;
	InOutPOV.Rotation += Spin;
	InOutPOV.FOV += FovAdd;
	return false;
}

// ------------------------------------------------------------------------------------------------ NPC

AYggStoryNPC::AYggStoryNPC()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Spawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	UCharacterMovementComponent* M = GetCharacterMovement();
	M->bOrientRotationToMovement = true;
	M->RotationRate = FRotator(0.f, 260.f, 0.f);
	M->MaxWalkSpeed = 190.f;
	M->MaxStepHeight = 60.f;
	M->SetWalkableFloorAngle(55.f);
	M->bUseRVOAvoidance = false;
	// stands on the drawn snow, which lies above its collision at the camps (2026-11-14 test shots)
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -82.f), FRotator(0.f, -90.f, 0.f));
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AYggStoryNPC::Dress(const FLinearColor& Tint, float Scale, bool bHood)
{
	if (USkeletalMesh* Body = YggAssets::Kept<USkeletalMesh>(NPCMeshPath))
	{
		GetMesh()->SetSkeletalMesh(Body);
	}
	if (UClass* Anim = LoadClass<UAnimInstance>(nullptr, NPCAnimPath))
	{
		GetMesh()->SetAnimInstanceClass(Anim);
	}
	// each person's clothing takes their colour (M_StoryNPC keeps the climber texture's folds and detail)
	UMaterialInterface* Look = YggAssets::Kept<UMaterialInterface>(TEXT("/Game/Story/Materials/M_StoryNPC.M_StoryNPC"));
	for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
	{
		UMaterialInstanceDynamic* D = Look ? UMaterialInstanceDynamic::Create(Look, this) : GetMesh()->CreateAndSetMaterialInstanceDynamic(i);
		if (D)
		{
			GetMesh()->SetMaterial(i, D);
			D->SetVectorParameterValue(TEXT("Tint"), Tint);
		}
	}
	SetActorScale3D(FVector(Scale));
	GetCharacterMovement()->MaxWalkSpeed = 190.f * FMath::Lerp(0.85f, 1.f, Scale);
}

bool AYggStoryNPC::ReachedGoal(float Radius) const
{
	return FVector::Dist2D(GetActorLocation(), Goal) < Radius;
}

void AYggStoryNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!P)
	{
		return;
	}
	const FVector Me = GetActorLocation();
	FVector Target = Me;
	bool bMove = false;
	float Scale = Speed;
	switch (Mode)
	{
	case EMode::Follow:
	{
		const FVector F = P->GetActorForwardVector().GetSafeNormal2D();
		const FVector R = FVector::CrossProduct(FVector::UpVector, F);
		Target = P->GetActorLocation() - F * FollowBehind + R * FollowSide;
		const float D = FVector::Dist2D(Me, Target);
		bMove = D > 140.f;
		Scale *= D > 900.f ? 1.6f : (D > 400.f ? 1.2f : 1.f);
		// far behind and out of sight: catch up quietly (a person walking with you never gets lost)
		if (FVector::Dist2D(Me, P->GetActorLocation()) > 4500.f)
		{
			const FVector View = P->GetControlRotation().Vector().GetSafeNormal2D();
			if (FVector::DotProduct(View, (Target - P->GetActorLocation()).GetSafeNormal2D()) < 0.2f)
			{
				TeleportTo(Target + FVector(0.f, 0.f, 120.f), GetActorRotation());
			}
		}
		break;
	}
	case EMode::GoTo:
		Target = Goal;
		bMove = !ReachedGoal(110.f);
		break;
	case EMode::Lead:
		Target = Goal;
		bMove = bMayWalk && !ReachedGoal(110.f) && FVector::Dist2D(Me, P->GetActorLocation()) < 900.f;
		break;
	default:
		break;
	}
	bArrived = Mode == EMode::GoTo || Mode == EMode::Lead ? ReachedGoal(150.f) : false;
	if (bMove)
	{
		const FVector Dir = (Target - Me).GetSafeNormal2D();
		AddMovementInput(Dir, FMath::Clamp(Scale, 0.2f, 1.f));
		GetCharacterMovement()->MaxWalkSpeed = 190.f * FMath::Max(1.f, Scale) * GetActorScale3D().X;
		// stuck against a rock: hop round it
		StuckTime = FVector::Dist2D(Me, LastPos) < 15.f * DeltaSeconds * 10.f ? StuckTime + DeltaSeconds : 0.f;
		if (StuckTime > 1.5f)
		{
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir) * 140.f;
			TeleportTo(Me + Side + Dir * 120.f + FVector(0.f, 0.f, 60.f), Dir.Rotation());
			StuckTime = 0.f;
		}
	}
	else if (bFacePlayer && Mode != EMode::Sit && FVector::Dist2D(Me, P->GetActorLocation()) < 700.f)
	{
		const FRotator Want = (P->GetActorLocation() - Me).GetSafeNormal2D().Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Want.Yaw, 0.f), DeltaSeconds, 3.f));
	}
	LastPos = Me;
}

// ------------------------------------------------------------------------------------------------ overlay

TSharedRef<SWidget> UYggStoryOverlay::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	}
	return Super::RebuildWidget();
}

int32 UYggStoryOverlay::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 L = Super::NativePaint(Args, G, MyCullingRect, Out, LayerId, InWidgetStyle, bParentEnabled) + 1;
	const FVector2f S = FVector2f(G.GetLocalSize());
	const float U = FMath::Max(0.6f, S.Y / 1080.f);   // text grows with the window
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	auto Box = [&](int32 Layer, FVector2f Pos, FVector2f Size, const FLinearColor& C)
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, C);
	};
	auto Text = [&](int32 Layer, const FString& T, const FSlateFontInfo& F, FLinearColor C, FVector2f Centre)
	{
		const FVector2f Sz = FVector2f(Measure->Measure(T, F));
		const FVector2f Pos = Centre - Sz * 0.5f;
		FLinearColor Sh = YggUI::Ink; Sh.A = C.A * 0.5f;
		FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 2.f) * U)), T, F, ESlateDrawEffect::None, Sh);
		FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)), T, F, ESlateDrawEffect::None, C);
		return Sz;
	};
	auto Veil = [&](int32 Layer, float CentreY, float Height, float A)
	{
		// a soft band of night behind words on bright snow (no box edges: three stacked fades)
		for (int32 i = 0; i < 3; ++i)
		{
			const float H = Height * (1.f + i * 0.6f);
			Box(Layer, FVector2f(0.f, CentreY - H * 0.5f), FVector2f(S.X, H), YggUI::WithA(YggUI::Ink, A * 0.16f));
		}
	};

	if (Black > 0.001f)
	{
		Box(L, FVector2f(0.f, 0.f), S, YggUI::WithA(BlackColor, Black));
	}
	if (TitleA > 0.01f)
	{
		Text(L + 2, Title, Face(0, int32(46 * U), int32(260 * U)), YggUI::WithA(YggUI::Snow, TitleA), FVector2f(S.X * 0.5f, S.Y * 0.44f));
		if (!Sub.IsEmpty())
		{
			Text(L + 2, Sub, YggUI::SerifInfo(int32(17 * U)), YggUI::WithA(YggUI::Mist, TitleA), FVector2f(S.X * 0.5f, S.Y * 0.44f + 58.f * U));
		}
	}
	if (LineA > 0.01f && !Line.IsEmpty())
	{
		const float Y = S.Y * 0.84f;
		Veil(L + 1, Y, 92.f * U, LineA);
		if (!Who.IsEmpty())
		{
			Text(L + 2, Who.ToUpper(), Face(0, int32(11 * U), int32(220 * U)), YggUI::WithA(YggUI::Sun, LineA * 0.9f), FVector2f(S.X * 0.5f, Y - 26.f * U));
		}
		Text(L + 2, Line, YggUI::SerifInfo(int32(22 * U)), YggUI::WithA(YggUI::Snow, LineA), FVector2f(S.X * 0.5f, Y + 4.f * U));
	}
	if (HintA > 0.01f && !Hint.IsEmpty())
	{
		Text(L + 2, Hint, YggUI::SerifInfo(int32(15 * U)), YggUI::WithA(YggUI::Mist, HintA), FVector2f(S.X * 0.5f, S.Y * 0.93f));
	}
	if (ChoiceA > 0.01f && Options.Num() > 0)
	{
		const float Gap = 40.f * U;
		const float Y0 = S.Y * 0.66f - Gap * (Options.Num() - 1) * 0.5f;
		Veil(L + 1, S.Y * 0.66f, Gap * Options.Num() + 60.f * U, ChoiceA);
		if (!Prompt.IsEmpty())
		{
			Text(L + 2, Prompt, YggUI::SerifInfo(int32(17 * U)), YggUI::WithA(YggUI::Mist, ChoiceA), FVector2f(S.X * 0.5f, Y0 - Gap));
		}
		for (int32 i = 0; i < Options.Num(); ++i)
		{
			const FString T = FString::Printf(TEXT("%d   %s"), i + 1, *Options[i]);
			Text(L + 2, T, YggUI::SerifInfo(int32(20 * U)), YggUI::WithA(i == 0 ? YggUI::Snow : YggUI::Snow, ChoiceA), FVector2f(S.X * 0.5f, Y0 + Gap * i));
		}
		if (ChoiceTimeLeft >= 0.f)
		{
			const float W = 220.f * U * FMath::Clamp(ChoiceTimeLeft, 0.f, 1.f);
			Box(L + 3, FVector2f(S.X * 0.5f - W * 0.5f, Y0 + Gap * Options.Num()), FVector2f(W, 1.2f * U), YggUI::WithA(YggUI::Mist, ChoiceA * 0.6f));
		}
	}
	if (CardA > 0.01f)
	{
		Box(L + 4, FVector2f(0.f, 0.f), S, YggUI::WithA(YggUI::Ink, CardA * 0.72f));
		float Y = S.Y * 0.2f;
		if (!CardTitle.IsEmpty())
		{
			Text(L + 5, CardTitle.ToUpper(), Face(0, int32(14 * U), int32(240 * U)), YggUI::WithA(YggUI::Sun, CardA), FVector2f(S.X * 0.5f, Y));
			Y += 40.f * U;
		}
		if (CardImage)
		{
			const float H = S.Y * 0.42f;
			const float W = H * float(CardImage->GetSizeX()) / FMath::Max(1.f, float(CardImage->GetSizeY()));
			FSlateBrush Brush;
			Brush.SetResourceObject(CardImage);
			Brush.ImageSize = FVector2D(W, H);
			FSlateDrawElement::MakeBox(Out, L + 5, G.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(S.X * 0.5f - W * 0.5f, Y))), &Brush, ESlateDrawEffect::None, YggUI::WithA(FLinearColor::White, CardA));
			Y += H + 30.f * U;
		}
		TArray<FString> Rows;
		CardText.ParseIntoArray(Rows, TEXT("\n"), false);
		for (const FString& R : Rows)
		{
			Text(L + 5, R, Face(1, int32(30 * U)), YggUI::WithA(YggUI::Snow, CardA), FVector2f(S.X * 0.5f, Y + 18.f * U));
			Y += 40.f * U;
		}
		Text(L + 5, TEXT("[F]  put it down"), YggUI::SerifInfo(int32(13 * U)), YggUI::WithA(YggUI::Shade, CardA), FVector2f(S.X * 0.5f, S.Y * 0.92f));
	}
	if (PageA > 0.01f)
	{
		Box(L + 4, FVector2f(0.f, 0.f), S, YggUI::WithA(YggUI::Ink, PageA * 0.86f));
		float Y = S.Y * 0.16f;
		Text(L + 5, PageHead, Face(0, int32(16 * U), int32(260 * U)), YggUI::WithA(YggUI::Sun, PageA), FVector2f(S.X * 0.5f, Y));
		Y += 34.f * U;
		Box(L + 5, FVector2f(S.X * 0.5f - 60.f * U, Y), FVector2f(120.f * U, 1.f * U), YggUI::WithA(YggUI::Mist, PageA * 0.5f));
		Y += 30.f * U;
		for (const FString& R : Page)
		{
			Text(L + 5, R, Face(1, int32(29 * U)), YggUI::WithA(YggUI::Snow, PageA), FVector2f(S.X * 0.5f, Y + 18.f * U));
			Y += 42.f * U;
		}
		Text(L + 5, TEXT("[F]  close the journal"), YggUI::SerifInfo(int32(13 * U)), YggUI::WithA(YggUI::Shade, PageA), FVector2f(S.X * 0.5f, S.Y * 0.93f));
	}
	return L + 8;
}

// ------------------------------------------------------------------------------------------------ subsystem

bool UYggStorySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE) && Super::ShouldCreateSubsystem(Outer);
}

void UYggStorySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (bPendingNewGame && InWorld.GetMapName().Contains(TEXT("base_map")))
	{
		bPendingNewGame = false;
		Start(TEXT("Chapter1"));
	}
}

void UYggStorySubsystem::Deinitialize()
{
	Stop();
	Super::Deinitialize();
}

APawn* UYggStorySubsystem::Player() const { return UGameplayStatics::GetPlayerPawn(GetWorld(), 0); }
APlayerController* UYggStorySubsystem::PC() const { return UGameplayStatics::GetPlayerController(GetWorld(), 0); }

bool UYggStorySubsystem::LoadChapter(const FString& Chapter)
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Story") / (Chapter + TEXT(".ygs"));
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: cannot read %s"), *Path);
		return false;
	}
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: %s is not valid JSON"), *Path);
		return false;
	}
	Places.Reset();
	const TSharedPtr<FJsonObject>* P;
	if (Root->TryGetObjectField(TEXT("places"), P))
	{
		for (const auto& KV : (*P)->Values)
		{
			const TArray<TSharedPtr<FJsonValue>>& A = KV.Value->AsArray();
			if (A.Num() >= 3)
			{
				const float Yaw = A.Num() > 3 ? float(A[3]->AsNumber()) : 0.f;
				Places.Add(FName(*KV.Key), FTransform(FRotator(0.f, Yaw, 0.f), FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber())));
			}
		}
	}
	Beats.Reset();
	const TArray<TSharedPtr<FJsonValue>>* B;
	if (Root->TryGetArrayField(TEXT("beats"), B))
	{
		for (const TSharedPtr<FJsonValue>& V : *B)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			if (!O.IsValid()) { continue; }
			FBeat Beat;
			Beat.Id = Str(O, TEXT("id"));
			const TSharedPtr<FJsonObject>* W;
			if (O->TryGetObjectField(TEXT("when"), W)) { Beat.When = *W; }
			const TArray<TSharedPtr<FJsonValue>>* D;
			if (O->TryGetArrayField(TEXT("do"), D)) { Beat.Do = *D; }
			Beats.Add(MoveTemp(Beat));
		}
	}
	UE_LOG(LogTemp, Display, TEXT("Story: %s loaded, %d beats, %d places"), *Chapter, Beats.Num(), Places.Num());
	return Beats.Num() > 0;
}

bool UYggStorySubsystem::Start(const FString& Chapter)
{
	Stop();
	if (!LoadChapter(Chapter))
	{
		return false;
	}
	ChapterName = Chapter;
	Flags.Reset();
	Runs.Reset();
	Tweens.Reset();
	Day = 1;
	Clock = 0.0;
	bRunning = true;
	return true;
}

void UYggStorySubsystem::Stop()
{
	bRunning = false;
	bTimeHold = false;
	Runs.Reset();
	Tweens.Reset();
	for (auto& KV : People) { if (AYggStoryNPC* N = KV.Value.Get()) { N->Destroy(); } }
	People.Reset();
	for (UAudioComponent* A : Sounds) { if (A) { A->Stop(); } }
	Sounds.Reset();
	SetLocks(false, false);
	if (FlyCam) { FlyCam->Destroy(); FlyCam = nullptr; }
	if (Overlay) { Overlay->RemoveFromParent(); Overlay = nullptr; }
	if (CamMod) { CamMod->DownCm = 0.f; CamMod->FovAdd = 0.f; CamMod->Spin = FRotator::ZeroRotator; }
	HandNPC = nullptr;
}

bool UYggStorySubsystem::Ready() const
{
	return Player() && PC() && PC()->PlayerCameraManager;
}

void UYggStorySubsystem::JumpTo(const FString& BeatId)
{
	const int32 Index = Beats.IndexOfByPredicate([&](const FBeat& B) { return B.Id == BeatId; });
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: no beat %s"), *BeatId);
		return;
	}
	Runs.Reset();
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		Beats[i].bFired = i <= Index;
		Beats[i].bDone = i < Index;
		Beats[i].DoneAt = Clock;
		Beats[i].CondSince = -1.0;
	}
	FRun Run;
	Run.Beat = Index;
	Runs.Add(Run);
	if (Overlay) { Overlay->Black = 0.f; Overlay->CardA = Overlay->PageA = Overlay->ChoiceA = 0.f; }
	UE_LOG(LogTemp, Display, TEXT("Story: jump to %s"), *BeatId);
}

void UYggStorySubsystem::Choose(int32 Option)
{
	ChoiceResult = Option - 1;
	bCardClose = true;
}

bool UYggStorySubsystem::Where(const FString& Name, FVector& Out, FRotator* OutRot) const
{
	if (Name.IsEmpty()) { return false; }
	if (Name == TEXT("player"))
	{
		if (const APawn* P = Player()) { Out = P->GetActorLocation(); if (OutRot) { *OutRot = P->GetActorRotation(); } return true; }
		return false;
	}
	if (const TWeakObjectPtr<AYggStoryNPC>* N = People.Find(FName(*Name)))
	{
		if (N->IsValid()) { Out = (*N)->GetActorLocation(); if (OutRot) { *OutRot = (*N)->GetActorRotation(); } return true; }
	}
	if (const FTransform* T = Places.Find(FName(*Name)))
	{
		Out = T->GetLocation();
		if (OutRot) { *OutRot = T->Rotator(); }
		return true;
	}
	return false;
}

AYggStoryNPC* UYggStorySubsystem::Person(const FString& Name) const
{
	const TWeakObjectPtr<AYggStoryNPC>* N = People.Find(FName(*Name));
	return N ? N->Get() : nullptr;
}

void UYggStorySubsystem::Tween(float& Value, float To, float Seconds, bool bPulse)
{
	Tweens.RemoveAll([&](const FTween& T) { return T.Value == &Value; });
	if (Seconds <= 0.f && !bPulse) { Value = To; return; }
	FTween T;
	T.Value = &Value;
	T.From = bPulse ? 0.f : Value;
	T.To = To;
	T.Length = FMath::Max(0.01f, Seconds);
	T.bPulse = bPulse;
	Tweens.Add(T);
}

void UYggStorySubsystem::SetLocks(bool bMove, bool bLook)
{
	APlayerController* C = PC();
	if (!C) { bMoveLocked = bLookLocked = false; return; }
	if (bMove != bMoveLocked) { C->SetIgnoreMoveInput(bMove); bMoveLocked = bMove; }
	if (bLook != bLookLocked) { C->SetIgnoreLookInput(bLook); bLookLocked = bLook; }
}

FString UYggStorySubsystem::Expand(const FString& Text) const
{
	return Text.Replace(TEXT("{day}"), *FString::FromInt(Day));
}

void UYggStorySubsystem::Tick(float DeltaTime)
{
	if (!bRunning || !Ready())
	{
		return;
	}
	if (!Overlay)
	{
		Overlay = CreateWidget<UYggStoryOverlay>(PC(), UYggStoryOverlay::StaticClass());
		if (Overlay)
		{
			Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);
			Overlay->AddToViewport(60);
		}
	}
	if (!CamMod)
	{
		CamMod = Cast<UYggStoryCameraModifier>(PC()->PlayerCameraManager->AddNewCameraModifier(UYggStoryCameraModifier::StaticClass()));
	}
	Clock += DeltaTime;
	TickTriggers();
	TickRuns(DeltaTime);
	TickTweens(DeltaTime);
	TickCamera(DeltaTime);
	TickHand(DeltaTime);
	if (bTimeHold)
	{
		if (!Tweens.ContainsByPredicate([this](const FTween& T) { return T.Value == &TimeValue; })) { TimeValue += TimeRate * DeltaTime * 100.f / 60.f; }
		if (TimeValue >= 2400.f) { TimeValue -= 2400.f; }
		YggTime::Set(GetWorld(), TimeValue);
	}
	// material parameters on tagged actors (materials found once, when the "param" action starts)
	for (int32 i = 0; i < 8; ++i)
	{
		for (const TWeakObjectPtr<UMaterialInstanceDynamic>& D : ParamMIDs[i])
		{
			if (D.IsValid()) { D->SetScalarParameterValue(ParamName[i], ParamValue[i]); }
		}
	}
}

bool UYggStorySubsystem::Conditions(FBeat& Beat, bool& bLookPart)
{
	bLookPart = false;
	const TSharedPtr<FJsonObject>& W = Beat.When;
	if (!W.IsValid()) { return true; }
	if (Bool(W, TEXT("never"), false)) { return false; }
	double LastDone = 0.0;
	for (const FString& After : Strs(W, TEXT("after")))
	{
		const FBeat* Other = Beats.FindByPredicate([&](const FBeat& B) { return B.Id == After; });
		if (!Other || !Other->bDone) { return false; }
		LastDone = FMath::Max(LastDone, Other->DoneAt);
	}
	if (Clock - LastDone < Num(W, TEXT("delay"), 0.f)) { return false; }
	for (const FString& F : Strs(W, TEXT("flag"))) { if (!Flags.Contains(FName(*F))) { return false; } }
	for (const FString& F : Strs(W, TEXT("noflag"))) { if (Flags.Contains(FName(*F))) { return false; } }
	const APawn* P = Player();
	FVector At;
	const FString Near = Str(W, TEXT("near"));
	if (!Near.IsEmpty() && (!Where(Near, At) || FVector::Dist(P->GetActorLocation(), At) > Num(W, TEXT("r"), 600.f))) { return false; }
	const FString Far = Str(W, TEXT("far"));
	if (!Far.IsEmpty() && Where(Far, At) && FVector::Dist(P->GetActorLocation(), At) < Num(W, TEXT("r"), 600.f)) { return false; }
	const FString Look = Str(W, TEXT("look"));
	if (!Look.IsEmpty())
	{
		bLookPart = true;
		if (Beat.CondSince < 0.0) { Beat.CondSince = Clock; }
		if (Clock - Beat.CondSince >= Num(W, TEXT("timeout"), 8.f)) { return true; }
		if (!Where(Look, At)) { return true; }
		const FVector Eye = PC()->PlayerCameraManager->GetCameraLocation();
		const FVector View = PC()->PlayerCameraManager->GetCameraRotation().Vector();
		const float Cone = FMath::Cos(FMath::DegreesToRadians(Num(W, TEXT("cone"), 14.f)));
		return FVector::DotProduct(View, (At - Eye).GetSafeNormal()) > Cone;
	}
	return true;
}

void UYggStorySubsystem::TickTriggers()
{
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		FBeat& B = Beats[i];
		if (B.bFired) { continue; }
		bool bLook = false;
		if (Conditions(B, bLook))
		{
			B.bFired = true;
			B.FiredAt = Clock;
			FRun Run;
			Run.Beat = i;
			Runs.Add(Run);
			UE_LOG(LogTemp, Display, TEXT("Story: beat %s"), *B.Id);
		}
		else if (!bLook)
		{
			B.CondSince = -1.0;
		}
	}
}

void UYggStorySubsystem::TickRuns(float Dt)
{
	for (int32 r = 0; r < Runs.Num(); ++r)
	{
		FRun& Run = Runs[r];
		if (Run.bBlocked)
		{
			if (!Unblocked(Run, Dt)) { continue; }
			Run.bBlocked = false;
		}
		Run.Wait -= Dt;
		while (Run.Wait <= 0.f && !Run.bBlocked && Runs.IsValidIndex(r) && Beats.IsValidIndex(Run.Beat) && Run.Step < Beats[Run.Beat].Do.Num())
		{
			const TSharedPtr<FJsonObject> A = Beats[Run.Beat].Do[Run.Step++]->AsObject();
			const float W = A.IsValid() ? DoAction(Run, A) : 0.f;
			if (!bRunning) { return; }
			if (W < 0.f) { Run.bBlocked = true; Run.BlockAction = A; BlockedSince = Clock; }
			else { Run.Wait += W; }
		}
		if (!Run.bBlocked && Run.Wait <= 0.f && Beats.IsValidIndex(Run.Beat) && Run.Step >= Beats[Run.Beat].Do.Num())
		{
			Beats[Run.Beat].bDone = true;
			Beats[Run.Beat].DoneAt = Clock;
			Runs.RemoveAt(r--);
		}
	}
}

float UYggStorySubsystem::DoAction(FRun& Run, const TSharedPtr<FJsonObject>& A)
{
	for (const FString& F : Strs(A, TEXT("if"))) { if (!Flags.Contains(FName(*F))) { return 0.f; } }
	for (const FString& F : Strs(A, TEXT("ifnot"))) { if (Flags.Contains(FName(*F))) { return 0.f; } }
	UWorld* World = GetWorld();
	APawn* P = Player();
	APlayerController* C = PC();
	const float T = Num(A, TEXT("t"), -1.f);
	const bool bWait = Bool(A, TEXT("wait"), true);
	FVector At = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;

	if (A->HasField(TEXT("fade")))
	{
		const float Len = T < 0.f ? 1.5f : T;
		Tween(Overlay->Black, Num(A, TEXT("fade"), 1.f), Len);
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("blink")))
	{
		const float Len = Num(A, TEXT("blink"), 0.7f);
		Tween(Overlay->Black, 1.f, Len, true);
		return Len;
	}
	if (A->HasField(TEXT("title")))
	{
		Overlay->Title = Str(A, TEXT("title"));
		Overlay->Sub = Expand(Str(A, TEXT("sub")));
		const float Len = T < 0.f ? 6.f : T;
		Tween(Overlay->TitleA, 1.f, Len, true);
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("say")))
	{
		Overlay->Line = Expand(Str(A, TEXT("say")));
		Overlay->Who = Str(A, TEXT("who"));
		const float Len = T < 0.f ? FMath::Clamp(1.4f + 0.055f * Overlay->Line.Len(), 2.5f, 9.f) : T;
		Tween(Overlay->LineA, 1.f, Len, true);
		UE_LOG(LogTemp, Display, TEXT("Story [VO] %s: %s"), *Overlay->Who, *Overlay->Line);
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("hint")))
	{
		Overlay->Hint = Expand(Str(A, TEXT("hint")));
		Tween(Overlay->HintA, 1.f, T < 0.f ? 5.f : T, true);
		return 0.f;
	}
	if (A->HasField(TEXT("choice")))
	{
		Overlay->Options = Strs(A, TEXT("choice"));
		Overlay->Prompt = Str(A, TEXT("prompt"));
		Overlay->ChoiceTimeLeft = Num(A, TEXT("timeout"), 0.f) > 0.f ? 1.f : -1.f;
		Tween(Overlay->ChoiceA, 1.f, 0.5f);
		ChoiceResult = -1;
		bCardClose = false;
		Run.BlockKind = TEXT("choice");
		return -1.f;
	}
	if (A->HasField(TEXT("teleport")))
	{
		if (Where(Str(A, TEXT("teleport")), At, &Rot) && P)
		{
			P->TeleportTo(At + FVector(0.f, 0.f, Num(A, TEXT("up"), 100.f)), FRotator(0.f, Rot.Yaw, 0.f));
			if (C) { C->SetControlRotation(FRotator(Num(A, TEXT("pitch"), 0.f), Rot.Yaw, 0.f)); C->SetViewTarget(P); }
			if (FlyCam) { FlyCam->Destroy(); FlyCam = nullptr; }
			if (CamMod) { CamMod->Spin = FRotator::ZeroRotator; CamMod->DownCm = 0.f; }
			TumbleLength = 0.f;
		}
		return 0.f;
	}
	if (A->HasField(TEXT("view")))
	{
		if (C && P) { C->SetViewTargetWithBlend(P, T < 0.f ? 0.f : T); }
		if (FlyCam) { FlyCam->SetLifeSpan(FMath::Max(0.1f, T + 0.1f)); FlyCam = nullptr; }
		return 0.f;
	}
	if (A->HasField(TEXT("time")))
	{
		TimeValue = Num(A, TEXT("time"), 900.f);
		YggTime::Set(World, TimeValue);
		return 0.f;
	}
	if (A->HasField(TEXT("timehold")))
	{
		// the story keeps the clock: from now it moves at "rate" game minutes per real second (0 = stands still)
		bTimeHold = Bool(A, TEXT("timehold"), true);
		TimeValue = YggTime::Get(World);
		TimeRate = Num(A, TEXT("rate"), 0.f);
		return 0.f;
	}
	if (A->HasField(TEXT("timeto")))
	{
		// glide the sun to a time over t seconds (no light pop)
		bTimeHold = true;
		TimeValue = YggTime::Get(World);
		Tween(TimeValue, Num(A, TEXT("timeto"), 1700.f), T < 0.f ? 20.f : T);
		return Bool(A, TEXT("wait"), false) ? (T < 0.f ? 20.f : T) : 0.f;
	}
	if (A->HasField(TEXT("cmd")))
	{
		for (const FString& Cmd : Strs(A, TEXT("cmd"))) { if (C) { C->ConsoleCommand(Cmd, false); } }
		return 0.f;
	}
	if (A->HasField(TEXT("npc")) || A->HasField(TEXT("npc_mode")))
	{
		const FString Id = A->HasField(TEXT("npc")) ? Str(A, TEXT("npc")) : Str(A, TEXT("npc_mode"));
		AYggStoryNPC* N = Person(Id);
		if (A->HasField(TEXT("npc")) && Where(Str(A, TEXT("at")), At, &Rot))
		{
			const FRotator Face(0.f, Num(A, TEXT("yaw"), Rot.Yaw), 0.f);
			const FVector Spot = At + FVector(0.f, 0.f, 100.f);
			if (!N)
			{
				FActorSpawnParameters SP;
				SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				N = World->SpawnActor<AYggStoryNPC>(AYggStoryNPC::StaticClass(), Spot, Face, SP);
				if (N) { People.Add(FName(*Id), N); N->Id = FName(*Id); }
			}
			else
			{
				N->TeleportTo(Spot, Face);
			}
			if (N)
			{
				const TArray<TSharedPtr<FJsonValue>>* Tint;
				FLinearColor Col(1.f, 1.f, 1.f);
				if (A->TryGetArrayField(TEXT("tint"), Tint) && Tint->Num() >= 3) { Col = FLinearColor((*Tint)[0]->AsNumber(), (*Tint)[1]->AsNumber(), (*Tint)[2]->AsNumber()); }
				N->Dress(Col, Num(A, TEXT("scale"), 1.f), Bool(A, TEXT("hood"), false));
				const float Sink = Num(A, TEXT("sink"), 0.f);
				if (Sink > 0.f) { N->GetMesh()->AddRelativeLocation(FVector(0.f, 0.f, -Sink)); }
			}
		}
		if (N)
		{
			const FString Mode = Str(A, TEXT("mode"), TEXT(""));
			if (Mode == TEXT("follow")) { N->Mode = AYggStoryNPC::EMode::Follow; }
			else if (Mode == TEXT("goto")) { N->Mode = AYggStoryNPC::EMode::GoTo; }
			else if (Mode == TEXT("lead")) { N->Mode = AYggStoryNPC::EMode::Lead; }
			else if (Mode == TEXT("sit")) { N->Mode = AYggStoryNPC::EMode::Sit; }
			else if (Mode == TEXT("idle")) { N->Mode = AYggStoryNPC::EMode::Idle; }
			if (Where(Str(A, TEXT("to")), At)) { N->Goal = At; N->bArrived = false; }
			N->FollowBehind = Num(A, TEXT("behind"), N->FollowBehind);
			N->FollowSide = Num(A, TEXT("side"), N->FollowSide);
			N->Speed = Num(A, TEXT("speed"), N->Speed);
			N->bFacePlayer = Bool(A, TEXT("face"), N->bFacePlayer);
			N->bMayWalk = true;
			if (A->HasField(TEXT("to")) && Bool(A, TEXT("arrive"), false)) { Run.BlockKind = TEXT("arrive"); return -1.f; }
		}
		return 0.f;
	}
	if (A->HasField(TEXT("npc_remove")))
	{
		if (AYggStoryNPC* N = Person(Str(A, TEXT("npc_remove")))) { N->Destroy(); }
		People.Remove(FName(*Str(A, TEXT("npc_remove"))));
		return 0.f;
	}
	if (A->HasField(TEXT("look")))
	{
		if (C && Where(Str(A, TEXT("look")), At))
		{
			const FVector Eye = C->PlayerCameraManager->GetCameraLocation();
			GazeFrom = C->GetControlRotation();
			const FRotator Want = (At + FVector(0.f, 0.f, Num(A, TEXT("up"), 0.f)) - Eye).Rotation();
			GazeTo = FQuat::Slerp(GazeFrom.Quaternion(), Want.Quaternion(), FMath::Clamp(Num(A, TEXT("strength"), 0.75f), 0.f, 1.f)).Rotator();
			GazeTo.Roll = 0.f;
			GazeTime = 0.f;
			GazeLength = T < 0.f ? 1.4f : T;
		}
		return Bool(A, TEXT("wait"), false) ? GazeLength : 0.f;
	}
	if (A->HasField(TEXT("kneel")))
	{
		const float Len = T < 0.f ? 0.9f : T;
		Tween(CamMod->DownCm, Num(A, TEXT("kneel"), 55.f), Len);
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("fov")))
	{
		Tween(CamMod->FovAdd, Num(A, TEXT("fov"), 4.f), T < 0.f ? 2.5f : T, true);
		return 0.f;
	}
	if (A->HasField(TEXT("tumble")))
	{
		TumbleLength = Num(A, TEXT("tumble"), 1.6f);
		TumbleTime = 0.f;
		return TumbleLength;
	}
	if (A->HasField(TEXT("flyout")) && C)
	{
		if (CamMod) { CamMod->Spin = FRotator::ZeroRotator; CamMod->DownCm = 0.f; }   // the slip's roll must not follow the camera out
		TumbleLength = 0.f;
		FlyLength = Num(A, TEXT("flyout"), 7.f);
		FlyTime = 0.f;
		FlyFrom = C->PlayerCameraManager->GetCameraLocation();
		FlyRotFrom = C->GetControlRotation();   // the clean view (the camera's own rotation still holds the slip's roll)
		FlyRotFrom.Roll = 0.f;
		const FVector Back = -FRotator(0.f, FlyRotFrom.Yaw, 0.f).Vector();
		FlyTo = FlyFrom + FVector(0.f, 0.f, Num(A, TEXT("rise"), 2500.f)) + Back * Num(A, TEXT("back"), 3000.f);
		FlyRotTo = Where(Str(A, TEXT("face")), At) ? (At - FlyTo).Rotation() : FRotator(4.f, FlyRotFrom.Yaw, 0.f);
		FlyRotTo.Roll = 0.f;
		if (!FlyCam)
		{
			FlyCam = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FlyFrom, FlyRotFrom);
			if (FlyCam) { FlyCam->GetCameraComponent()->bConstrainAspectRatio = false; FlyCam->GetCameraComponent()->SetFieldOfView(Num(A, TEXT("fovdeg"), 70.f)); }
		}
		if (FlyCam) { C->SetViewTargetWithBlend(FlyCam, 0.f); }
		return Bool(A, TEXT("wait"), true) ? FlyLength : 0.f;
	}
	if (A->HasField(TEXT("lock")))
	{
		SetLocks(Bool(A, TEXT("lock"), true), Bool(A, TEXT("look"), false));
		return 0.f;
	}
	if (A->HasField(TEXT("hand")))
	{
		HandNPC = Person(Str(A, TEXT("hand")));
		if (HandNPC.IsValid() && Where(Str(A, TEXT("to")), At))
		{
			HandNPC->Mode = AYggStoryNPC::EMode::Lead;
			HandNPC->Goal = At;
			HandNPC->bMayWalk = false;
			bHandHintShown = false;
			Overlay->Hint = Str(A, TEXT("hint"), TEXT("Hold  [F]  to hold her hand"));
			Run.BlockKind = TEXT("hand");
			return -1.f;
		}
		return 0.f;
	}
	if (A->HasField(TEXT("card")))
	{
		Overlay->CardTitle = Str(A, TEXT("title"));
		Overlay->CardText = Expand(Str(A, TEXT("card"))).Replace(TEXT("|"), TEXT("\n"));
		Overlay->CardImage = A->HasField(TEXT("image")) ? LoadObject<UTexture2D>(nullptr, *Str(A, TEXT("image"))) : nullptr;
		Tween(Overlay->CardA, 1.f, 0.6f);
		SetLocks(true, true);
		bCardClose = false;
		Run.BlockKind = TEXT("card");
		return -1.f;
	}
	if (A->HasField(TEXT("page")))
	{
		Overlay->PageHead = Expand(Str(A, TEXT("head"), TEXT("Day {day} / 49")));
		Overlay->Page.Reset();
		const TArray<TSharedPtr<FJsonValue>>* Lines;
		if (A->TryGetArrayField(TEXT("page"), Lines))
		{
			for (const TSharedPtr<FJsonValue>& V : *Lines)
			{
				if (V->Type == EJson::String) { Overlay->Page.Add(Expand(V->AsString())); continue; }
				const TSharedPtr<FJsonObject> O = V->AsObject();
				bool bOk = true;
				for (const FString& F : Strs(O, TEXT("if"))) { bOk &= Flags.Contains(FName(*F)); }
				for (const FString& F : Strs(O, TEXT("ifnot"))) { bOk &= !Flags.Contains(FName(*F)); }
				if (bOk) { Overlay->Page.Add(Expand(Str(O, TEXT("text")))); }
			}
		}
		Tween(Overlay->PageA, 1.f, 1.2f);
		SetLocks(true, true);
		bCardClose = false;
		Run.BlockKind = TEXT("page");
		return -1.f;
	}
	if (A->HasField(TEXT("day"))) { Day = int32(Num(A, TEXT("day"), 1.f)); return 0.f; }
	if (A->HasField(TEXT("sound")))
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Str(A, TEXT("sound"))))
		{
			const float Vol = Num(A, TEXT("vol"), 1.f);
			UAudioComponent* Comp = Where(Str(A, TEXT("at")), At)
				? UGameplayStatics::SpawnSoundAtLocation(World, Sound, At, FRotator::ZeroRotator, Vol)
				: UGameplayStatics::SpawnSound2D(World, Sound, Vol);
			if (Comp)
			{
				const float Fade = Num(A, TEXT("fadein"), 0.f);
				if (Fade > 0.f) { Comp->FadeIn(Fade, Vol); }
				Comp->ComponentTags.Add(FName(*Str(A, TEXT("id"), TEXT("sound"))));
				Sounds.Add(Comp);
			}
		}
		return 0.f;
	}
	if (A->HasField(TEXT("stopsound")))
	{
		const FName Id(*Str(A, TEXT("stopsound")));
		for (UAudioComponent* S : Sounds) { if (S && S->ComponentHasTag(Id)) { S->FadeOut(Num(A, TEXT("fadeout"), 1.5f), 0.f); } }
		return 0.f;
	}
	if (A->HasField(TEXT("param")))
	{
		const FName Tag(*Str(A, TEXT("tag"))), Name(*Str(A, TEXT("param")));
		int32 Slot = INDEX_NONE;
		for (int32 i = 0; i < 8 && Slot == INDEX_NONE; ++i) { if (ParamTag[i] == Tag && ParamName[i] == Name) { Slot = i; } }
		for (int32 i = 0; i < 8 && Slot == INDEX_NONE; ++i) { if (ParamTag[i].IsNone()) { Slot = i; } }
		if (Slot != INDEX_NONE)
		{
			ParamTag[Slot] = Tag;
			ParamName[Slot] = Name;
			ParamMIDs[Slot].Reset();
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->ActorHasTag(Tag)) { continue; }
				TInlineComponentArray<UMeshComponent*> Meshes(*It);
				for (UMeshComponent* Mesh : Meshes)
				{
					for (int32 m = 0; m < Mesh->GetNumMaterials(); ++m)
					{
						UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(m));
						if (!D) { D = Mesh->CreateAndSetMaterialInstanceDynamic(m); }
						if (D) { ParamMIDs[Slot].Add(D); }
					}
				}
			}
			Tween(ParamValue[Slot], Num(A, TEXT("to"), 1.f), T < 0.f ? 2.f : T, Bool(A, TEXT("pulse"), false));
		}
		return 0.f;
	}
	if (A->HasField(TEXT("show")) || A->HasField(TEXT("hide")))
	{
		const bool bShow = A->HasField(TEXT("show"));
		const FName Tag(*Str(A, bShow ? TEXT("show") : TEXT("hide")));
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag)) { It->SetActorHiddenInGame(!bShow); It->SetActorEnableCollision(bShow); }
		}
		return 0.f;
	}
	if (A->HasField(TEXT("flag"))) { for (const FString& F : Strs(A, TEXT("flag"))) { Flags.Add(FName(*F)); } return 0.f; }
	if (A->HasField(TEXT("unflag"))) { for (const FString& F : Strs(A, TEXT("unflag"))) { Flags.Remove(FName(*F)); } return 0.f; }
	if (A->HasField(TEXT("waitnear")))
	{
		Run.BlockKind = TEXT("near");
		return -1.f;
	}
	if (A->HasField(TEXT("end")))
	{
		UE_LOG(LogTemp, Display, TEXT("Story: %s finished"), *ChapterName);
		Stop();
		return 0.f;
	}
	// a plain pause last: other actions carry "wait" as an option ({"flyout": 11, "wait": false} is not a pause)
	if (A->HasField(TEXT("wait"))) { return Num(A, TEXT("wait"), 1.f); }
	UE_LOG(LogTemp, Warning, TEXT("Story: unknown action in beat %s"), Beats.IsValidIndex(Run.Beat) ? *Beats[Run.Beat].Id : TEXT("?"));
	return 0.f;
}

bool UYggStorySubsystem::Unblocked(FRun& Run, float Dt)
{
	APlayerController* C = PC();
	const TSharedPtr<FJsonObject>& A = Run.BlockAction;
	const bool bAuto = CVarStoryAuto.GetValueOnGameThread() != 0;
	if (Run.BlockKind == TEXT("choice"))
	{
		const float Timeout = Num(A, TEXT("timeout"), 0.f);
		if (Timeout > 0.f) { Overlay->ChoiceTimeLeft = 1.f - float(Clock - BlockedSince) / Timeout; }
		static const FKey Keys[4] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
		for (int32 i = 0; i < 4 && C; ++i) { if (C->WasInputKeyJustPressed(Keys[i])) { ChoiceResult = i; } }
		if (bAuto && Clock - BlockedSince > 1.5) { ChoiceResult = 0; }
		const TArray<FString> Opts = Strs(A, TEXT("choice"));
		if (ChoiceResult < 0 && Timeout > 0.f && Clock - BlockedSince > Timeout) { ChoiceResult = int32(Num(A, TEXT("default"), -1.f)); if (ChoiceResult < 0) { ChoiceResult = 99; } }
		if (ChoiceResult < 0) { return false; }
		const TArray<FString> Fl = Strs(A, TEXT("flags"));
		if (Fl.IsValidIndex(ChoiceResult) && !Fl[ChoiceResult].IsEmpty()) { Flags.Add(FName(*Fl[ChoiceResult])); }
		if (Opts.IsValidIndex(ChoiceResult) && Bool(A, TEXT("echo"), false))
		{
			Overlay->Who = TEXT("Sangye");
			Overlay->Line = Opts[ChoiceResult];
			Tween(Overlay->LineA, 1.f, 2.8f, true);
		}
		Tween(Overlay->ChoiceA, 0.f, 0.4f);
		ChoiceResult = -1;
		return true;
	}
	if (Run.BlockKind == TEXT("card") || Run.BlockKind == TEXT("page"))
	{
		const bool bKey = C && (C->WasInputKeyJustPressed(EKeys::F) || C->WasInputKeyJustPressed(EKeys::E) || C->WasInputKeyJustPressed(EKeys::Enter)
			|| C->WasInputKeyJustPressed(EKeys::SpaceBar) || C->WasInputKeyJustPressed(EKeys::LeftMouseButton));
		if (!(Clock - BlockedSince > 0.8 && (bKey || bCardClose)) && !(bAuto && Clock - BlockedSince > 2.0)) { return false; }
		Tween(Run.BlockKind == TEXT("card") ? Overlay->CardA : Overlay->PageA, 0.f, 0.6f);
		SetLocks(false, false);
		bCardClose = false;
		return true;
	}
	if (Run.BlockKind == TEXT("hand"))
	{
		if (!HandNPC.IsValid() || HandNPC->ReachedGoal(160.f))
		{
			if (HandNPC.IsValid()) { HandNPC->Mode = AYggStoryNPC::EMode::Idle; }
			HandNPC = nullptr;
			return true;
		}
		return false;
	}
	if (Run.BlockKind == TEXT("arrive"))
	{
		AYggStoryNPC* N = Person(Str(A, TEXT("npc_mode")).IsEmpty() ? Str(A, TEXT("npc")) : Str(A, TEXT("npc_mode")));
		return !N || N->ReachedGoal(160.f) || Clock - BlockedSince > Num(A, TEXT("max"), 60.f);
	}
	if (Run.BlockKind == TEXT("near"))
	{
		FVector At;
		return !Player() || !Where(Str(A, TEXT("waitnear")), At) || FVector::Dist(Player()->GetActorLocation(), At) < Num(A, TEXT("r"), 500.f);
	}
	return true;
}

void UYggStorySubsystem::TickTweens(float Dt)
{
	for (int32 i = Tweens.Num() - 1; i >= 0; --i)
	{
		FTween& T = Tweens[i];
		T.Time += Dt;
		const float K = FMath::Clamp(T.Time / T.Length, 0.f, 1.f);
		*T.Value = T.bPulse ? T.To * Pulse(K) : FMath::Lerp(T.From, T.To, Ease(K));
		if (K >= 1.f) { *T.Value = T.bPulse ? 0.f : T.To; Tweens.RemoveAt(i); }
	}
}

void UYggStorySubsystem::TickCamera(float Dt)
{
	APlayerController* C = PC();
	if (GazeLength > 0.f && C)
	{
		GazeTime += Dt;
		const float K = Ease(GazeTime / GazeLength);
		C->SetControlRotation(FRotator(FMath::Lerp(GazeFrom.Pitch, GazeTo.Pitch, K), GazeFrom.Yaw + FRotator::NormalizeAxis(GazeTo.Yaw - GazeFrom.Yaw) * K, 0.f));
		if (GazeTime >= GazeLength) { GazeLength = 0.f; }
	}
	if (TumbleLength > 0.f && CamMod)
	{
		TumbleTime += Dt;
		const float K = FMath::Clamp(TumbleTime / TumbleLength, 0.f, 1.f);
		const float E = K * K;
		CamMod->Spin = FRotator(-75.f * E, 35.f * E, 140.f * E);
		CamMod->DownCm = 140.f * E;
		if (K >= 1.f) { TumbleLength = 0.f; }
	}
	if (FlyCam && FlyLength > 0.f)
	{
		// the game hands the view back to the climber on its own (camera auto-management): keep it on the fly-out camera
		if (C && C->GetViewTarget() != FlyCam) { C->SetViewTarget(FlyCam); }
		if (CVarStoryDebug.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Story fly: target %s cam %s rot %s"), C ? *GetNameSafe(C->GetViewTarget()) : TEXT("-"),
				*FlyCam->GetActorLocation().ToString(), *FlyCam->GetActorRotation().ToString());
		}
		FlyTime = FMath::Min(FlyTime + Dt, FlyLength);
		const float K = FlyTime / FlyLength;
		const float E = 1.f - FMath::Pow(1.f - K, 2.2f);
		// pitch and yaw blended on their own, roll held at 0 (a quaternion blend between two level views rolls the horizon)
		const float RK = Ease(K);
		const FRotator R(FMath::Lerp(FlyRotFrom.Pitch, FlyRotTo.Pitch, RK), FlyRotFrom.Yaw + FRotator::NormalizeAxis(FlyRotTo.Yaw - FlyRotFrom.Yaw) * RK, 0.f);
		FlyCam->SetActorLocationAndRotation(FMath::Lerp(FlyFrom, FlyTo, E), R);
	}
}

void UYggStorySubsystem::TickHand(float Dt)
{
	AYggStoryNPC* N = HandNPC.Get();
	APawn* P = Player();
	APlayerController* C = PC();
	if (!N || !P || !C)
	{
		return;
	}
	const bool bHeld = C->IsInputKeyDown(EKeys::F) || CVarStoryAuto.GetValueOnGameThread() != 0;
	const float D = FVector::Dist2D(N->GetActorLocation(), P->GetActorLocation());
	N->bMayWalk = bHeld && D < 320.f;
	if (bHeld && D < 600.f)
	{
		// she pulls you along beside her
		const FVector Pull = (N->GetActorLocation() + N->GetActorForwardVector() * 60.f - P->GetActorLocation()).GetSafeNormal2D();
		P->AddMovementInput(Pull, D > 160.f ? 0.8f : 0.45f);
		Overlay->HintA = FMath::FInterpTo(Overlay->HintA, 0.f, Dt, 3.f);
	}
	else
	{
		Overlay->HintA = FMath::FInterpTo(Overlay->HintA, 1.f, Dt, 3.f);
	}
}
