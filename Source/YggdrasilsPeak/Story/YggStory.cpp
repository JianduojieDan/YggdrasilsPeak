#include "Story/YggStory.h"

#include "Survival/YggViewRig.h"
#include "Survival/CompassComponent.h"
#include "Survival/YggAssets.h"
#include "Survival/YggSettings.h"
#include "Survival/YggTime.h"
#include "Survival/YggUIStyle.h"
#include "AIController.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Survival/SnowTrailComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/AssetManager.h"
#include "Engine/OverlapResult.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "TextureResource.h"
#include <limits>
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"

bool UYggStorySubsystem::bPendingNewGame = false;

// Helper names carry a Story prefix: in a unity build this file shares one translation unit with others (and with the
// view rig), and two anonymous "Ease" helpers would clash.
namespace
{
	const TCHAR* const NPCMeshPath = TEXT("/Game/Characters/Climber_Rigged/SKM_Climber_Rigged.SKM_Climber_Rigged");
	const TCHAR* const NPCAnimPath = TEXT("/Game/Characters/Climber_Rigged/Anims/ABP_Unarmed_Climber.ABP_Unarmed_Climber_C");
	const TCHAR* const KhataMeshPath = TEXT("/Game/Story/Meshes/SM_Khata/StaticMeshes/SM_Khata.SM_Khata");
	/** Lease owner of ygg.Story.CamTest (beats own theirs by index). */
	constexpr int32 CamTestOwner = 9000;

	/** Landscape or a detail tile: what people stand on (props, tents and rooms are never "ground" here). */
	bool IsStoryTerrain(const FHitResult& H)
	{
		const AActor* A = H.GetActor();
		if (A && A->GetClass()->GetName().Contains(TEXT("Landscape"))) { return true; }
		const UStaticMeshComponent* C = Cast<UStaticMeshComponent>(H.GetComponent());
		return C && C->GetStaticMesh() && C->GetStaticMesh()->GetPathName().Contains(TEXT("/DetailTiles/"));
	}

	/** The drawn ground under a point. At the camps the detail tiles lie ~1 m ABOVE the landscape, so the first terrain
	 *  hit from above wins; anything else in the way (tents, props, people) is stepped through. */
	bool StoryGround(UWorld* World, const FVector& At, float Up, float Down, FVector& Out, const AActor* Ignore)
	{
		if (!World) { return false; }
		FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryGround), true, Ignore);
		FVector From = At + FVector(0.f, 0.f, Up);
		const FVector To = At - FVector(0.f, 0.f, Down);
		for (int32 i = 0; i < 6; ++i)
		{
			FHitResult H;
			if (!World->LineTraceSingleByChannel(H, From, To, ECC_Visibility, Q)) { return false; }
			if (IsStoryTerrain(H)) { Out = H.ImpactPoint; return true; }
			if (H.GetActor()) { Q.AddIgnoredActor(H.GetActor()); } else { From = H.ImpactPoint - FVector(0.f, 0.f, 2.f); }
		}
		return false;
	}

	float StoryEase(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	/** 0 -> 1 in the first fifth, holds, 1 -> 0 in the last fifth: words, titles, flashes. */
	float StoryPulse(float T) { return StoryEase(T / 0.2f) * StoryEase((1.f - T) / 0.2f); }
	/** Seconds to read a line. */
	float StoryReadTime(const FString& S) { return FMath::Clamp(1.4f + 0.055f * S.Len(), 2.5f, 9.f); }

	FString JStr(const TSharedPtr<FJsonObject>& O, const TCHAR* K, const FString& Def = FString())
	{
		FString S;
		return O.IsValid() && O->TryGetStringField(K, S) ? S : Def;
	}
	float JNum(const TSharedPtr<FJsonObject>& O, const TCHAR* K, float Def)
	{
		double D;
		return O.IsValid() && O->TryGetNumberField(K, D) ? float(D) : Def;
	}
	bool JBool(const TSharedPtr<FJsonObject>& O, const TCHAR* K, bool Def)
	{
		bool B;
		return O.IsValid() && O->TryGetBoolField(K, B) ? B : Def;
	}
	TArray<FString> JStrs(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		TArray<FString> Out;
		const TArray<TSharedPtr<FJsonValue>>* Arr;
		if (O.IsValid() && O->TryGetArrayField(K, Arr)) { for (const TSharedPtr<FJsonValue>& V : *Arr) { if (V.IsValid() && V->Type == EJson::String) { Out.Add(V->AsString()); } } }
		else { FString S; if (O.IsValid() && O->TryGetStringField(K, S)) { Out.Add(S); } }
		return Out;
	}
	/** A field whatever its type, lower case: true / false / keep / end / a number / a name (false when missing or null). */
	FString JField(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		const TSharedPtr<FJsonValue> V = O.IsValid() ? O->TryGetField(K) : nullptr;
		if (!V.IsValid() || V->Type == EJson::Null) { return TEXT("false"); }
		if (V->Type == EJson::Boolean) { return V->AsBool() ? TEXT("true") : TEXT("false"); }
		if (V->Type == EJson::Number) { return V->AsNumber() == 0.0 ? TEXT("false") : FString::SanitizeFloat(V->AsNumber()); }
		if (V->Type == EJson::String) { return V->AsString().ToLower(); }
		return TEXT("true");
	}

	UFont* StoryFontFace(int32 Which)
	{
		static const TCHAR* const Paths[2] = { TEXT("/Game/UI/Fonts/FF_Cinzel.FF_Cinzel"), TEXT("/Game/UI/Fonts/FF_Caveat.FF_Caveat") };
		static TStrongObjectPtr<UFont> Made[2];
		if (!Made[Which].IsValid()) { if (UFont* F = YggUI::MakeFace(GetTransientPackage(), Paths[Which])) { Made[Which].Reset(F); } }
		return Made[Which].Get();
	}
	FSlateFontInfo StoryFont(int32 Which, int32 Size, int32 Track = 0)
	{
		UFont* F = StoryFontFace(Which);
		FSlateFontInfo I = F ? FSlateFontInfo(F, Size, TEXT("Regular")) : YggUI::SerifInfo(Size);
		I.LetterSpacing = Track;
		return I;
	}

	// ---- postures (spec 4.3): names, eye above the floor, band, down / up seconds
	bool StoryPose(const FString& In, EYggView& Out)
	{
		const FString N = In.ToLower();
		if (N == TEXT("stand") || N == TEXT("walk")) { Out = EYggView::Walk; return true; }
		if (N == TEXT("lie")) { Out = EYggView::Lie; return true; }
		if (N == TEXT("sit")) { Out = EYggView::Sit; return true; }
		if (N == TEXT("kneel1")) { Out = EYggView::Kneel1; return true; }
		if (N == TEXT("kneel2")) { Out = EYggView::Kneel2; return true; }
		if (N == TEXT("crawl")) { Out = EYggView::Crawl; return true; }
		return false;
	}
	const TCHAR* StoryPoseName(EYggView V)
	{
		switch (V)
		{
		case EYggView::Lie: return TEXT("lie");
		case EYggView::Sit: return TEXT("sit");
		case EYggView::Kneel1: return TEXT("kneel1");
		case EYggView::Kneel2: return TEXT("kneel2");
		case EYggView::Crawl: return TEXT("crawl");
		default: return TEXT("stand");
		}
	}
	float StoryPoseEye(EYggView V)
	{
		switch (V)
		{
		case EYggView::Lie: return 32.f;
		case EYggView::Sit: return 78.f;
		case EYggView::Kneel1: return 108.f;
		case EYggView::Kneel2: return 110.f;
		case EYggView::Crawl: return 60.f;   // (#77, was 68: closer to the sit's 78)
		default: return 156.f;
		}
	}
	float StoryPoseBand(EYggView V) { return V == EYggView::Walk ? 5.f : (V == EYggView::Crawl ? 8.f : 6.f); }
	float StoryPoseDown(EYggView V)
	{
		switch (V)
		{
		case EYggView::Lie: return 2.5f;
		case EYggView::Sit: return 2.0f;
		case EYggView::Kneel1: return 0.9f;
		case EYggView::Kneel2: return 1.0f;
		case EYggView::Crawl: return 0.8f;
		default: return 1.0f;
		}
	}
	float StoryPoseUp(EYggView V)
	{
		switch (V)
		{
		case EYggView::Lie: return 2.2f;
		case EYggView::Sit: return 1.6f;
		case EYggView::Kneel1: return 1.0f;
		case EYggView::Kneel2: return 1.2f;
		case EYggView::Crawl: return 1.4f;
		default: return 1.0f;
		}
	}

	/** Eyelid curves (spec 6.6), used here only when there is no view rig (the rig runs its own). */
	float StoryLidCurve(EYggLidMode M, float E, float T, float Hold, float From, bool& bDone)
	{
		bDone = false;
		switch (M)
		{
		case EYggLidMode::Blink:
			if (E < T) { return FMath::Lerp(From, 1.f, StoryEase(E / FMath::Max(T, 0.01f))); }
			if (E < T + Hold) { return 1.f; }
			if (E < T + Hold + 0.6f) { return 1.f - StoryEase((E - T - Hold) / 0.6f); }
			bDone = true;
			return 0.f;
		case EYggLidMode::Closed:
			if (T <= 0.f || E >= T) { bDone = true; return 1.f; }
			return FMath::Lerp(From, 1.f, StoryEase(E / T));
		case EYggLidMode::Open:
			if (T <= 0.f || E >= T) { bDone = true; return 0.f; }
			return FMath::Lerp(From, 0.f, StoryEase(E / T));
		case EYggLidMode::Wake:
			// 35% open at 0.6 s, shut again at 1.1 s, fully open at t
			if (E < 0.6f) { return 1.f - 0.35f * StoryEase(E / 0.6f); }
			if (E < 1.1f) { return 0.65f + 0.35f * StoryEase((E - 0.6f) / 0.5f); }
			if (E < T) { return 1.f - StoryEase((E - 1.1f) / FMath::Max(0.1f, T - 1.1f)); }
			bDone = true;
			return 0.f;
		case EYggLidMode::Sleep:
		{
			// one flutter: to 60% at 40% of t, back to 30%, then closed
			const float K = E / FMath::Max(T, 0.1f);
			if (K < 0.4f) { return FMath::Lerp(From, 0.6f, StoryEase(K / 0.4f)); }
			if (K < 0.55f) { return FMath::Lerp(0.6f, 0.3f, StoryEase((K - 0.4f) / 0.15f)); }
			if (K < 1.f) { return FMath::Lerp(0.3f, 1.f, StoryEase((K - 0.55f) / 0.45f)); }
			bDone = true;
			return 1.f;
		}
		default:
			bDone = true;
			return 0.f;
		}
	}

	/** The first-person camera of the climber (the rig re-parents it to the capsule; things in the hands hang from it). */
	UCameraComponent* StoryCamera(const APawn* P)
	{
		if (!P) { return nullptr; }
		TInlineComponentArray<UCameraComponent*> Cams(P);
		for (UCameraComponent* Cam : Cams) { if (Cam && Cam->GetName().Contains(TEXT("FirstPerson"))) { return Cam; } }
		return Cams.Num() ? Cams[0] : nullptr;
	}

	/** "Interact" -> the player's bound key; otherwise a key name ("F"). */
	FKey StoryKey(const FString& Name)
	{
		if (Name.IsEmpty() || Name.Equals(TEXT("Interact"), ESearchCase::IgnoreCase)) { return YggSettings::KeyFor(TEXT("Interact"), EKeys::E); }
		const FKey K{ FName(*Name) };
		return K.IsValid() ? K : EKeys::E;
	}

	/** (#77) The keyboard key that walks forward: the move action's mapping that swizzles the axis onto Y without negating
	 *  it (W in the template's IA_Move), read from the player's live Enhanced Input mappings, so a remapped key shows in the
	 *  hints; a letter wins over the arrow keys. W when none is found. */
	FKey StoryForwardKey(const APlayerController* PC)
	{
		const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
		const UEnhancedInputLocalPlayerSubsystem* Sub = LP ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP) : nullptr;
		const UEnhancedPlayerInput* In = Sub ? Sub->GetPlayerInput() : nullptr;
		if (!In) { return EKeys::W; }
		FKey Found;
		for (const FEnhancedActionKeyMapping& M : In->GetEnhancedActionMappingsView())
		{
			if (!M.Action || !M.Key.IsValid() || M.Key.IsGamepadKey() || M.Key.IsMouseButton() || M.Key.IsTouch()
				|| M.Action->ValueType != EInputActionValueType::Axis2D || !M.Action->GetName().Contains(TEXT("Move")))
			{
				continue;
			}
			bool bSwizzle = false, bNegate = false;
			for (const UInputModifier* Mod : M.Modifiers)
			{
				if (const UInputModifierSwizzleAxis* S = Cast<UInputModifierSwizzleAxis>(Mod)) { bSwizzle |= S->Order == EInputAxisSwizzle::YXZ || S->Order == EInputAxisSwizzle::ZXY; }
				else if (const UInputModifierNegate* N = Cast<UInputModifierNegate>(Mod)) { bNegate |= N->bX || N->bY; }
			}
			if (!bSwizzle || bNegate) { continue; }
			const bool bArrow = M.Key == EKeys::Up;
			if (!Found.IsValid() || (Found == EKeys::Up && !bArrow)) { Found = M.Key; }
		}
		return Found.IsValid() ? Found : EKeys::W;
	}

	/** Content/Story/Cast.ygs, written by Tools/Story/mh_cast (editor): person -> body, face, grooms, garment. */
	TSharedPtr<FJsonObject> StoryCast()
	{
		static TSharedPtr<FJsonObject> Cast;
		// (2026-11-20) re-read when the file changes, so "regenerate the cast, then play" works in one editor session
		static FDateTime Stamp;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Story/Cast.ygs");
		const FDateTime Now = IFileManager::Get().GetTimeStamp(*Path);
		if (!Cast.IsValid() || Now != Stamp)
		{
			Stamp = Now;
			Cast.Reset();
			FString Text;
			if (FFileHelper::LoadFileToString(Text, *Path))
			{
				TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Text);
				FJsonSerializer::Deserialize(R, Cast);
			}
		}
		return Cast;
	}

	/** "/Game/A/B" -> "/Game/A/B.B" so LoadObject finds the asset inside its package. */
	FString StoryObjectPath(const FString& P)
	{
		if (P.IsEmpty() || P.Contains(TEXT("."))) { return P; }
		return P + TEXT(".") + FPaths::GetBaseFilename(P);
	}

	FAutoConsoleCommandWithWorldAndArgs StoryCmdStart(TEXT("ygg.Story.Start"), TEXT("ygg.Story.Start [Chapter1] - play a chapter (Content/Story/<name>.ygs)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr) { S->Start(Args.Num() ? Args[0] : TEXT("Chapter1")); }
		}));
	FAutoConsoleCommandWithWorldAndArgs StoryCmdJump(TEXT("ygg.Story.Jump"), TEXT("ygg.Story.Jump <beat> [Chapter1] - start the chapter (if needed) at that beat (testing)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr;
			if (!S || Args.Num() == 0) { return; }
			if (!S->IsRunning()) { S->Start(Args.Num() > 1 ? Args[1] : TEXT("Chapter1")); }
			S->JumpTo(Args[0]);
		}));
	FAutoConsoleCommandWithWorldAndArgs StoryCmdChoose(TEXT("ygg.Story.Choose"), TEXT("ygg.Story.Choose <1-4> - answer the open choice (testing)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr) { S->Choose(Args.Num() ? FCString::Atoi(*Args[0]) : 1); }
		}));
	FAutoConsoleCommandWithWorld StoryCmdStop(TEXT("ygg.Story.Stop"), TEXT("Stop the story."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr) { S->Stop(); }
		}));
	FAutoConsoleCommandWithWorld StoryCmdCamTest(TEXT("ygg.Story.CamTest"),
		TEXT("Camera self-test (spec V3): every posture, nudge, reflex, knock, fall, blink + teleport, fly-out, inspect and reset; PASS / FAIL in the log and Saved/Review/camtest.txt. Story must be stopped."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr) { S->StartCamTest(); }
		}));
	FAutoConsoleCommandWithWorldAndArgs StoryCmdFrame(TEXT("ygg.Cam.Frame"),
		TEXT("ygg.Cam.Frame <place> [lie|sit|kneel1|kneel2|crawl] - cut to a chapter place, take the pose, save Saved/Review/frames/<place>[_<pose>].png (spec V13)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UYggStorySubsystem* S = World ? World->GetSubsystem<UYggStorySubsystem>() : nullptr;
			if (S && Args.Num() > 0) { S->StartFrameShot(Args[0], Args.Num() > 1 ? Args[1] : FString()); }
		}));
	FAutoConsoleCommandWithWorldAndArgs StoryCmdParam(TEXT("ygg.Story.Param"), TEXT("ygg.Story.Param <actor tag> <material parameter> <value> - set a scalar on every mesh of the tagged actors (testing: ygg.Story.Param plume Glow 1)."),
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
	TAutoConsoleVariable<int32> CVarStoryDebug(TEXT("ygg.Story.Debug"), 0, TEXT("1 = log the view rig's lease audit each time a beat ends (spec V4)."));
	TAutoConsoleVariable<int32> CVarStoryAuto(TEXT("ygg.Story.Auto"), 0,
		TEXT("Testing: 1 = choices answer themselves (first option), inspects and the page close after 2 s, press prompts press themselves, so a run can play the chapter unattended."));
	TAutoConsoleVariable<int32> CVarStoryNPCDebug(TEXT("ygg.Story.NPCDebug"), 0,
		TEXT("Testing: 1 = every story person logs twice a second: mode, movement, speed, acceleration, the walk blueprint's GroundSpeed / ShouldMove / IsFalling, foot heights, and (#84) the lower boot's sole over the drawn snow (sole=, cm, - = in it), sink= drop= trail= snow= base= fit=; 2 = also a depth capture of the real drawn surface under that boot within 30 m (solev= visgap=; a render flush each, hitches)."));
	TAutoConsoleVariable<int32> CVarStoryNPCWalkBS(TEXT("ygg.Story.NPCWalkBS"), 1,
		TEXT("1 = MetaHuman story people walk with BS_Idle_Walk_Run_MH played directly (feet move); 0 = the old ABP_Unarmed_MH (its foot IK pins the feet: they glide). Read when a person is dressed."));
	TAutoConsoleVariable<int32> CVarStoryNPCPrints(TEXT("ygg.Story.NPCPrints"), 1,
		TEXT("1 = story people's planted feet press prints into the player's snow trail map (within 60 m)."));
	TAutoConsoleVariable<float> CVarStoryHeadlampLm(TEXT("ygg.Story.HeadlampLm"), 6.f,   // 600 burned a hard white disc into the night snow (2026-11-20); 35 and 15 still clipped to a white disc ("very powerful"), 6 keeps the snow readable in the pool (2026-11-21)
		TEXT("The player's story headlamp (light headlamp): lumens. Spot 38 degrees with a soft 16-degree centre, 25 m, low specular, no shadows (spec 6.15)."));
	TAutoConsoleVariable<float> CVarStoryHeadlampSpec(TEXT("ygg.Story.HeadlampSpec"), 0.3f,
		TEXT("Specular scale of the player's headlamp: 1 = full glare on wet fabric, brass and glass, 0.3 = the soft matte look (2026-11-21)."));
	TAutoConsoleVariable<int32> CVarStoryPinPosed(TEXT("ygg.Story.PinPosed"), 1,
		TEXT("1 = a story posture right after a cut (lie / sit / kneel) holds the climber on the cut spot (movement off until he walks or crawls), so the physics cannot push him off it; 0 = old behaviour."));
	TAutoConsoleVariable<int32> CVarStoryNPCSoles(TEXT("ygg.Story.NPCSoles"), 1,
		TEXT("(#84) 1 = story people's boots on the drawn snow: soles measured per body, lowered into the player's trench and their own prints (read back from the trail map) and into fresh snow by the player's sink rule; 0 = the old fixed mesh offset (6 cm over the capsule bottom), to compare."));
	TAutoConsoleVariable<float> CVarStoryNPCSoleLift(TEXT("ygg.Story.NPCSoleLift"), 0.f,
		TEXT("(#84) Tuning: cm the story people's soles sit above where the fit puts them (+ = higher)."));
	TAutoConsoleVariable<float> CVarStoryNPCSoleContact(TEXT("ygg.Story.NPCSoleContact"), 1.5f,
		TEXT("(#84) cm a story person's boot goes into snow so it clearly touches it (the player's SnowContactCm); rock and props: 0."));

	/** ygg.Story.NPCDebug 2 (#84): the drawn surface's height straight under P, from a scene depth capture 4 m above it (it sees
	 *  the Nanite displacement and the trench, which no trace does), the people and the player hidden. NaN when nothing. A
	 *  render flush per call: testing only. */
	double StoryNPCVisibleZ(AActor* Owner, APawn* Player, const FVector& P, double FromZ)
	{
		static TWeakObjectPtr<USceneCaptureComponent2D> Shared;
		USceneCaptureComponent2D* Cap = Shared.Get();
		if (!Cap && Owner)
		{
			UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Owner);
			RT->RenderTargetFormat = RTF_RGBA32f;
			RT->ClearColor = FLinearColor(1e7f, 0.f, 0.f, 0.f);
			RT->InitAutoFormat(8, 8);
			RT->UpdateResourceImmediate(true);
			Cap = NewObject<USceneCaptureComponent2D>(Owner, TEXT("StoryNPCSoleCapture"));
			Cap->SetUsingAbsoluteLocation(true);
			Cap->SetUsingAbsoluteRotation(true);
			Cap->bCaptureEveryFrame = false;
			Cap->bCaptureOnMovement = false;
			Cap->CaptureSource = SCS_SceneDepth;
			Cap->FOVAngle = 1.f;
			Cap->TextureTarget = RT;
			Cap->RegisterComponent();
			Shared = Cap;
		}
		if (!Cap || !Cap->GetWorld()) { return std::numeric_limits<double>::quiet_NaN(); }
		Cap->HiddenActors.Reset();
		for (TActorIterator<AYggStoryNPC> It(Cap->GetWorld()); It; ++It) { Cap->HiddenActors.Add(*It); }
		if (Player) { Cap->HiddenActors.Add(Player); }
		Cap->SetWorldLocationAndRotation(FVector(P.X, P.Y, FromZ), FRotator(-90.f, 0.f, 0.f));
		Cap->CaptureScene();
		FTextureRenderTargetResource* Res = Cap->TextureTarget ? Cap->TextureTarget->GameThread_GetRenderTargetResource() : nullptr;
		TArray<FLinearColor> Px;
		if (!Res || !Res->ReadLinearColorPixels(Px) || Px.Num() < 64) { return std::numeric_limits<double>::quiet_NaN(); }
		double D = 0.0;
		for (int32 i : { 27, 28, 35, 36 }) { D += Px[i].R; }   // the middle 2 x 2 pixels (7 cm across at 4 m)
		D *= 0.25;
		return D > 1e6 ? std::numeric_limits<double>::quiet_NaN() : FromZ - D;
	}
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
	// a person's start and stop take a moment (default 2048 cm/s2 snapped to full pace in a tenth of a second)
	M->MaxAcceleration = 650.f;
	M->BrakingDecelerationWalking = 380.f;
	M->GroundFriction = 4.f;
	M->bUseSeparateBrakingFriction = true;      // a stop settles over ~0.5 s (ground friction 4 stopped a walk dead in 0.2 s)
	M->BrakingFriction = 1.f;
	M->MaxStepHeight = 60.f;
	M->SetWalkableFloorAngle(55.f);
	M->bUseRVOAvoidance = false;
	// stands on the drawn snow, which lay above its collision at the camps (2026-11-14 test shots). Since 2026-11-21 (#84)
	// only until the body is dressed: FitSoles measures each body and TickSoles puts its boots on the drawn snow
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
	FitSoles();
}

void AYggStoryNPC::SetStorySink(float Cm)
{
	StorySink = Cm;
	AppliedMeshZ = 1e9f;
	ApplyMeshZ();
}

void AYggStoryNPC::FitSoles()
{
	bSoleFit = false;
	SoleBones[0] = SoleBones[1] = NAME_None;
	SoleDrop = SoleNeedNow = SoleNeedLast = SoleWindowAge = 0.f;
	SoleGroundCheck = 0.f;
	MeshBaseZ = -82.f;
	AppliedMeshZ = 1e9f;
	USkeletalMeshComponent* M = GetMesh();
	USkeletalMesh* BodyMesh = M ? M->GetSkeletalMeshAsset() : nullptr;
	if (!BodyMesh)
	{
		ApplyMeshZ();
		return;
	}
	// the lowest point of the body in its reference pose, or of a garment's boots under it (a stray garment vertex far
	// below never counts: at most 8 cm under the body). Meshy and MetaHuman bodies have their soles at ~0 (glb min 0).
	const FBoxSphereBounds B = BodyMesh->GetImportedBounds();
	const float BodyBottom = float(B.Origin.Z - B.BoxExtent.Z);
	float Bottom = BodyBottom;
	for (USceneComponent* C : LookParts)
	{
		const USkeletalMeshComponent* G = Cast<USkeletalMeshComponent>(C);
		if (G && G->GetName().StartsWith(TEXT("Garment")) && G->GetSkeletalMeshAsset())
		{
			const FBoxSphereBounds GB = G->GetSkeletalMeshAsset()->GetImportedBounds();
			// (2026-11-21) an outfit build deletes the feet under the boots (the body's bottom is the ankle): the boots count
			Bottom = FMath::Min(Bottom, FMath::Max(float(GB.Origin.Z - GB.BoxExtent.Z), BodyBottom > 5.f ? -5.f : BodyBottom - 8.f));
		}
	}
	Bottom = FMath::Clamp(Bottom, -20.f, 20.f);
	// the foot bones (UE mannequin / MetaHuman, the Mixamo-named climber) and the ankle over that bottom in the reference
	// pose (FirstPersonBodyComponent::MeasureSoleHeight does the same for the player)
	const FReferenceSkeleton& Ref = BodyMesh->GetRefSkeleton();
	static const TCHAR* const Left[] = { TEXT("foot_l"), TEXT("LeftFoot"), TEXT("mixamorig:LeftFoot") };
	static const TCHAR* const Right[] = { TEXT("foot_r"), TEXT("RightFoot"), TEXT("mixamorig:RightFoot") };
	for (int32 k = 0; k < 3; ++k)
	{
		if (SoleBones[0].IsNone() && Ref.FindBoneIndex(FName(Left[k])) != INDEX_NONE) { SoleBones[0] = FName(Left[k]); }
		if (SoleBones[1].IsNone() && Ref.FindBoneIndex(FName(Right[k])) != INDEX_NONE) { SoleBones[1] = FName(Right[k]); }
	}
	AnkleOverSoleCm = 10.f;
	if (!SoleBones[0].IsNone())
	{
		const int32 Bone = Ref.FindBoneIndex(SoleBones[0]);
		FTransform Xf = Ref.GetRefBonePose()[Bone];
		for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent)) { Xf = Xf * Ref.GetRefBonePose()[Parent]; }
		AnkleOverSoleCm = FMath::Clamp(float(Xf.GetLocation().Z) - Bottom, 3.f, 25.f);
	}
	SoleBottomCm = Bottom;
	// soles on the capsule bottom (the capsule's own half height: the mesh is in capsule space, scaled with it)
	MeshBaseZ = -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - Bottom;
	bSoleFit = true;
	ApplyMeshZ();
	UE_LOG(LogTemp, Display, TEXT("Story NPC %s: %s soles at %.1f cm (mesh space), ankle %s %.1f cm over them -> mesh at %.1f (was -82)"),
		*Id.ToString(), *GetNameSafe(BodyMesh), Bottom, *SoleBones[0].ToString(), AnkleOverSoleCm, MeshBaseZ);
}

void AYggStoryNPC::ApplyMeshZ()
{
	USkeletalMeshComponent* M = GetMesh();
	if (!M) { return; }
	const float Scale = FMath::Max(float(GetActorScale3D().Z), 0.1f);
	const bool bFit = bSoleFit && CVarStoryNPCSoles.GetValueOnGameThread() != 0;
	// the drop is world cm; the mesh sits in the (scaled) capsule's space
	const float Z = (bFit ? MeshBaseZ - SoleDrop / Scale : -82.f) - StorySink;
	if (FMath::Abs(Z - AppliedMeshZ) < 0.05f) { return; }
	FVector L = M->GetRelativeLocation();
	L.Z = Z;
	M->SetRelativeLocation(L);
	AppliedMeshZ = Z;
}

void AYggStoryNPC::TickSoles(float DeltaSeconds, APawn* P)
{
	if (!bSoleFit || CVarStoryNPCSoles.GetValueOnGameThread() == 0 || DeltaSeconds <= 0.f)
	{
		ApplyMeshZ();   // a switched cvar takes the old offset back at once
		return;
	}
	const FVector Me = GetActorLocation();
	// far away no one sees the boots: the drop stays as it is (no trace, no read-back)
	if (FVector::DistSquared2D(Me, P->GetActorLocation()) > FMath::Square(6000.f))
	{
		return;
	}
	UCharacterMovementComponent* CM = GetCharacterMovement();
	USkeletalMeshComponent* M = GetMesh();
	const float Scale = FMath::Max(float(GetActorScale3D().Z), 0.1f);
	const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	USnowTrailComponent* Snow = PlayerSnow.Get();
	if (!Snow) { Snow = P->FindComponentByClass<USnowTrailComponent>(); PlayerSnow = Snow; }
	// what the ground is, 4x a second: one trace down the axis with the faces and materials IsSnow reads (snow, how soft)
	SoleGroundCheck -= DeltaSeconds;
	if (SoleGroundCheck <= 0.f)
	{
		SoleGroundCheck = 0.25f;
		FHitResult H;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryNPCSoles), false, this);
		Q.bReturnPhysicalMaterial = true;
		Q.bReturnFaceIndex = true;
		Q.AddIgnoredActor(P);
		bSoleSnow = GetWorld()->LineTraceSingleByChannel(H, Me, Me - FVector(0.f, 0.f, Half + 60.f), ECC_Visibility, Q) && USnowTrailComponent::IsSnow(H);
		SoleSink = bSoleSnow && Snow ? Snow->SinkDepthFor(H) : 0.f;
	}
	const bool bGround = CM && CM->IsMovingOnGround() && CM->CurrentFloor.bBlockingHit && !bHover;
	float Need = 0.f;
	if (bGround)
	{
		// the collision under any point near the feet: the plane of the floor the movement stands on (no extra traces)
		const FHitResult& F = CM->CurrentFloor.HitResult;
		const FVector N = F.ImpactNormal.Z > 0.5f ? FVector(F.ImpactNormal) : FVector::UpVector;
		const FVector At = F.ImpactPoint;
		auto ColZ = [&N, &At](const FVector& X) { return float(At.Z - (N.X * (X.X - At.X) + N.Y * (X.Y - At.Y)) / N.Z); };
		if (Snow && bSoleSnow) { Snow->RequestTrailPatch(this, Me); }
		const float Contact = bSoleSnow ? CVarStoryNPCSoleContact.GetValueOnGameThread() : 0.f;
		const float Lift = CVarStoryNPCSoleLift.GetValueOnGameThread();
		// where a sole belongs: on the drawn snow (the collision plus the trench or bank drawn there), into it by the
		// player's sink rule as far as the print or the trench under it is not that deep already
		auto Target = [&](const FVector& Sole, float& OutTrail, bool& bOutKnown)
		{
			OutTrail = 0.f;
			bOutKnown = false;
			if (bSoleSnow && Snow) { OutTrail = Snow->TrailOffsetCm(this, Sole, &bOutKnown); }
			const float Pit = FMath::Max(0.f, -OutTrail);
			return ColZ(Sole) + OutTrail - Contact - FMath::Max(0.f, SoleSink - Pit) + Lift;
		};
		// the soles' rest height with no drop is the capsule bottom (FitSoles); now (drop applied) it is the mesh's bottom
		const float RestNoDrop = float(Me.Z) - Half;
		const float RestNow = float(M->GetComponentLocation().Z) + SoleBottomCm * Scale;
		const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
		float Best = 0.f, LowestH = 1e9f, MinAnimH = 1e9f;
		int32 Count = 0;
		for (int32 i = 0; i < 2; ++i)
		{
			if (SoleBones[i].IsNone()) { continue; }
			const FVector Foot = M->GetSocketLocation(SoleBones[i]);
			// the animated sole over the rest height: ~0 planted, up in the swing; a swinging boot counts with the ground under
			// it (as the player's: the lower ground under either boot decides), a planted one at its own height
			const float AnimH = float(Foot.Z - RestNow) - AnkleOverSoleCm * Scale;
			MinAnimH = FMath::Min(MinAnimH, AnimH);
			const float Resting = AnimH * (1.f - FMath::SmoothStep(2.f, 8.f, AnimH));
			const FVector Sole = Foot + Fwd * (6.f * Scale);     // the sole's middle, in front of the ankle (SoleForwardCm)
			float Trail;
			bool bKnown;
			const float TZ = Target(Sole, Trail, bKnown);
			const float FootNeed = RestNoDrop + Resting - TZ;
			if (Count == 0 || FootNeed > Best) { Best = FootNeed; }
			++Count;
			if (AnimH < LowestH)
			{
				// the debug log's boot: the lower one, its sole over the drawn snow under it (cm, - = in it)
				LowestH = AnimH;
				DbgSoleAt = Sole;
				DbgColZ = ColZ(Sole);
				DbgTrail = Trail;
				bDbgTrailKnown = bKnown;
				DbgSole = float(Foot.Z) - AnkleOverSoleCm * Scale - (DbgColZ + Trail);
			}
		}
		if (Count == 0)
		{
			// no foot bones: the body's middle
			float Trail;
			bool bKnown;
			Best = RestNoDrop - Target(Me, Trail, bKnown);
			DbgSoleAt = Me;
			DbgColZ = ColZ(Me);
			DbgTrail = Trail;
			bDbgTrailKnown = bKnown;
			DbgSole = RestNoDrop - SoleDrop - (DbgColZ + Trail);
		}
		// (2026-11-21) the retargeted walks lift the Meshy bodies with speed (~0.2 cm per cm/s, 40 cm at a brisk walk): a real
		// walk always has one boot near the ground, so the LOWER boot hanging over its rest height is the whole pose lifted -
		// take it down by that much (the floating the user saw while the team followed)
		if (Count >= 2 && MinAnimH > 1.f && MinAnimH < 200.f) { Best += MinAnimH - 1.f; }
		Need = FMath::Clamp(Best, -10.f, 90.f);
	}
	// the deepest need over the last step (two windows of ~0.6 s): a new print appears a moment after its boot lands and is
	// read back a moment later still, so the plain need dipped at every step and the body bobbed
	SoleWindowAge += DeltaSeconds;
	// (2026-11-21 review) off the snow the boots must not stay buried for the window's 1.2 s: forget it at once
	if (!bSoleSnow && FMath::Max(SoleNeedNow, SoleNeedLast) > Need + 2.f) { SoleNeedNow = SoleNeedLast = Need; SoleWindowAge = 0.f; }
	SoleNeedNow = FMath::Max(SoleNeedNow, Need);
	if (SoleWindowAge > 0.6f)
	{
		SoleNeedLast = SoleNeedNow;
		SoleNeedNow = Need;
		SoleWindowAge = 0.f;
	}
	const float Want = bGround ? FMath::Max(SoleNeedNow, SoleNeedLast) : 0.f;
	// down quickly (a boot never hangs over a pit for long), up gently (no pumping)
	SoleDrop = FMath::FInterpTo(SoleDrop, Want, DeltaSeconds, Want > SoleDrop ? 8.f : (bSoleSnow ? 2.f : 9.f));
	ApplyMeshZ();
	OnBodyBonesFinalized();      // the face onto the neck where the body is drawn now
}

static TAutoConsoleVariable<float> CVarStoryFaceYaw(TEXT("ygg.Story.FaceYaw"), -90.f,
	TEXT("Extra yaw (deg) for a MetaHuman face on a foreign (Meshy) body. -90 measured 2026-11-21 (0 looked sideways, +90 backwards)."));
static TAutoConsoleVariable<float> CVarStoryFaceBack(TEXT("ygg.Story.FaceBack"), 3.f,
	TEXT("cm the MetaHuman face sits further back on a Meshy body (its neck piece inside the collar, not in front of it)."));
static TAutoConsoleVariable<float> CVarStoryFaceDrop(TEXT("ygg.Story.FaceDrop"), 3.f,
	TEXT("cm the MetaHuman face sits lower on a Meshy body (its neck piece tucks into the collar)."));

float AYggStoryNPC::MaxPaceCm() const
{
	// a Meshy body (its own retargeted walk) breaks its pose above ~150 cm/s (the chest swings over the neck, the body
	// lifts): a brisk trekking pace is the top; MetaHumans (the mannequin's own walk) may go to the walk's end
	return FaceFollow.IsValid() || !bMetaHumanSkeleton ? 150.f : 195.f;
}

void AYggStoryNPC::OnBodyBonesFinalized()
{
	USkeletalMeshComponent* F = FaceFollow.Get();
	USkeletalMeshComponent* M = GetMesh();
	if (!F || !M || M->GetBoneIndex(FaceNeckBone) == INDEX_NONE) { return; }
	// the neck's world point as drawn this frame (bones x the mesh component, sole drop included), upright with the body
	const FQuat Q = M->GetComponentQuat();
	F->SetWorldLocationAndRotation(M->GetSocketLocation(FaceNeckBone) + Q.RotateVector(FaceNeckOffset * M->GetComponentScale()) - FVector(0.f, 0.f, CVarStoryFaceDrop.GetValueOnGameThread())
		- GetActorForwardVector() * CVarStoryFaceBack.GetValueOnGameThread(), Q * FaceRot * FQuat(FVector::UpVector, FMath::DegreesToRadians(CVarStoryFaceYaw.GetValueOnGameThread())), false, nullptr, ETeleportType::TeleportPhysics);
}

bool AYggStoryNPC::DressAs(const FString& Look, float Scale)
{
	const TSharedPtr<FJsonObject> Cast = StoryCast();
	const TSharedPtr<FJsonObject>* Person = nullptr;
	if (!Cast.IsValid() || !Cast->TryGetObjectField(Look, Person))
	{
		return false;
	}
	USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *StoryObjectPath((*Person)->GetStringField(TEXT("body"))));
	// a look may bring its own walk (the AI-made characters each have their own skeleton); MetaHumans share one
	FString AbpPath = TEXT("/Game/Story/MetaHumans/Common/Anims/ABP_Unarmed_MH.ABP_Unarmed_MH_C");
	(*Person)->TryGetStringField(TEXT("abp"), AbpPath);
	if (!AbpPath.EndsWith(TEXT("_C"))) { AbpPath = StoryObjectPath(AbpPath) + TEXT("_C"); }
	UClass* Walk = LoadClass<UAnimInstance>(nullptr, *AbpPath);
	bMetaHumanSkeleton = AbpPath.Contains(TEXT("ABP_Unarmed_MH"));
	// or a single held animation (the father sitting cross-legged in the snow): "anim" in the look
	FString PosePath;
	UAnimationAsset* Pose = ((*Person)->TryGetStringField(TEXT("anim"), PosePath) && !PosePath.IsEmpty())
		? LoadObject<UAnimationAsset>(nullptr, *StoryObjectPath(PosePath)) : nullptr;
	if (Pose) { Walk = nullptr; }
	// the walk's own blend space next to its ABP (ABP_Unarmed_X -> BS_Idle_Walk_Run_X), played directly when the body
	// has no ik_foot bones for the ABP's foot IK to read (see bBlendWalk)
	UBlendSpace* WalkBS = nullptr;
	if (!Pose && Body && CVarStoryNPCWalkBS.GetValueOnGameThread() != 0 && Body->GetRefSkeleton().FindBoneIndex(FName(TEXT("ik_foot_l"))) == INDEX_NONE)
	{
		FString BsPath = AbpPath.LeftChop(2).Replace(TEXT("ABP_Unarmed_"), TEXT("BS_Idle_Walk_Run_"));     // drop "_C"
		WalkBS = LoadObject<UBlendSpace>(nullptr, *BsPath);
		if (!WalkBS) { UE_LOG(LogTemp, Warning, TEXT("Story: no walk blend space %s for '%s' - its ABP walks (feet may glide)"), *BsPath, *Look); }
	}
	if (!Body || (!Walk && !Pose))
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: no MetaHuman body/walk for '%s' - using the climber look"), *Look);
		return false;
	}
	for (USceneComponent* C : LookParts)
	{
		if (C) { C->DestroyComponent(); }
	}
	LookParts.Reset();

	// the body runs the walk itself (its own post-process ABP adds the correctives), so everything that follows it
	// with the leader pose gets the real MetaHuman bones - garments share the body's skeleton exactly
	USkeletalMeshComponent* M = GetMesh();
	M->EmptyOverrideMaterials();
	FaceFollow.Reset();
	if (FaceFollowHandle.IsValid()) { M->UnregisterOnBoneTransformsFinalizedDelegate(FaceFollowHandle); FaceFollowHandle.Reset(); }
	M->bComponentUseFixedSkelBounds = false;   // a foreign-body look sets these below
	M->SetBoundsScale(1.f);
	M->SetSkeletalMesh(Body);
	if (Pose)
	{
		M->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		M->PlayAnimation(Pose, true);
	}
	else if (WalkBS)
	{
		M->PlayAnimation(WalkBS, true);
		bBlendWalk = true;
		BlendSpeed = 0.f;
	}
	else
	{
		M->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		M->SetAnimInstanceClass(Walk);
	}
	M->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	auto AddMesh = [this](const FString& Path, USceneComponent* Parent, const TCHAR* Name) -> USkeletalMeshComponent*
	{
		USkeletalMesh* Mesh = Path.IsEmpty() ? nullptr : LoadObject<USkeletalMesh>(nullptr, *StoryObjectPath(Path));
		if (!Mesh) { return nullptr; }
		USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(this, Name);
		C->SetupAttachment(Parent);
		C->SetSkeletalMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->RegisterComponent();
		LookParts.Add(C);
		return C;
	};
	USkeletalMeshComponent* Face = AddMesh((*Person)->GetStringField(TEXT("face")), M, TEXT("Face"));
	if (Face)
	{
		// ABP_Face copies the body pose from its attach parent, then runs the facial rig
		if (UClass* FaceAnim = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Story/MetaHumans/Common/Face/ABP_Face.ABP_Face_C")))
		{
			Face->SetAnimInstanceClass(FaceAnim);
		}
		Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
	if (USkeletalMeshComponent* Garment = AddMesh((*Person)->GetStringField(TEXT("garment")), M, TEXT("Garment")))
	{
		Garment->SetLeaderPoseComponent(M);
	}
	// hair cards, eyebrows, lashes, beard: GroomComponents set up by reflection (no HairStrandsCore link dependency)
	UClass* GroomClass = FindObject<UClass>(nullptr, TEXT("/Script/HairStrandsCore.GroomComponent"));
	const TArray<TSharedPtr<FJsonValue>>* Grooms = nullptr;
	if (GroomClass && Face && (*Person)->TryGetArrayField(TEXT("grooms"), Grooms))
	{
		for (const TSharedPtr<FJsonValue>& V : *Grooms)
		{
			const TSharedPtr<FJsonObject> G = V->AsObject();
			UObject* Asset = G ? LoadObject<UObject>(nullptr, *StoryObjectPath(G->GetStringField(TEXT("groom")))) : nullptr;
			if (!Asset) { continue; }
			UObject* Binding = LoadObject<UObject>(nullptr, *StoryObjectPath(G->GetStringField(TEXT("binding"))));
			USceneComponent* C = NewObject<USceneComponent>(this, GroomClass, *(TEXT("Groom_") + G->GetStringField(TEXT("slot"))));
			if (FObjectPropertyBase* P = FindFProperty<FObjectPropertyBase>(GroomClass, TEXT("GroomAsset"))) { P->SetObjectPropertyValue_InContainer(C, Asset); }
			if (Binding)
			{
				if (FObjectPropertyBase* P = FindFProperty<FObjectPropertyBase>(GroomClass, TEXT("BindingAsset"))) { P->SetObjectPropertyValue_InContainer(C, Binding); }
			}
			C->SetupAttachment(Face);
			C->RegisterComponent();
			LookParts.Add(C);
		}
	}
	// (2026-11-21) a MetaHuman face on another body (a headless Meshy body on the UE5-mannequin skeleton): ABP_Face copies
	// the body's pose by bone NAME, so the face follows only the bones both skeletons have - "head" above all
	if (Face && Face->GetSkeletalMeshAsset())
	{
		const FReferenceSkeleton& BodyRef = Body->GetRefSkeleton();
		if (BodyRef.FindBoneIndex(FName(TEXT("head"))) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Story: look '%s' - the body %s has no 'head' bone: its MetaHuman face (ABP_Face copies the pose by bone name) cannot follow it"), *Look, *GetNameSafe(Body));
		}
		else
		{
			FString Missing;
			for (const TCHAR* Bone : { TEXT("neck_01"), TEXT("neck_02"), TEXT("spine_05"), TEXT("spine_04") })
			{
				if (Face->GetSkeletalMeshAsset()->GetRefSkeleton().FindBoneIndex(FName(Bone)) != INDEX_NONE && BodyRef.FindBoneIndex(FName(Bone)) == INDEX_NONE) { Missing += FString(TEXT(" ")) + Bone; }
			}
			if (!Missing.IsEmpty())
			{
				UE_LOG(LogTemp, Display, TEXT("Story: look '%s' - the face follows the body's head; the body lacks%s, so those face bones keep their reference pose under the head's parent"), *Look, *Missing);
			}
			// the copy takes the head's component-space rotation as it is: a skeleton with other bone axes turns the face
			// with it (a pose difference is a few degrees, other axes are 90 or 180)
			auto RefCS = [](const FReferenceSkeleton& Ref, int32 Bone)
			{
				FTransform Xf = Ref.GetRefBonePose()[Bone];
				for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent)) { Xf = Xf * Ref.GetRefBonePose()[Parent]; }
				return Xf;
			};
			const FReferenceSkeleton& FaceRef = Face->GetSkeletalMeshAsset()->GetRefSkeleton();
			const int32 FaceHead = FaceRef.FindBoneIndex(FName(TEXT("head")));
			// (2026-11-21) a face on a foreign body (headless Meshy, UE5-mannequin skeleton: other head-bone axes, 90 deg) rides
			// RIGIDLY on the body's head bone - copying the rotation turned the face mesh about the head and it hovered over the
			// shoulders. Placed so its head bone sits on the body's, upright and facing the same way as in both reference
			// poses; no ABP_Face (the face keeps its neutral reference pose).
			if (FaceHead != INDEX_NONE && BodyRef.FindBoneIndex(FName(TEXT("neck_02"))) == INDEX_NONE)
			{
				const FTransform BodyHeadCS = RefCS(BodyRef, BodyRef.FindBoneIndex(FName(TEXT("head"))));
				const FTransform FaceHeadCS = RefCS(FaceRef, FaceHead);
				// which way each points, in its own component space: the face by its nose (in front of the head bone), the body
				// by its "headfront" socket bone (Meshy rigs) - the MetaHuman face mesh faces another axis than the Meshy body
				// (the faces looked 90 deg sideways)
				FVector FaceFwd = FVector::ZeroVector, BodyFwd(0.f, 1.f, 0.f);
				for (const TCHAR* Nose : { TEXT("FACIAL_C_NoseTip"), TEXT("FACIAL_C_Nose"), TEXT("FACIAL_C_LipUpper"), TEXT("FACIAL_C_Jaw") })
				{
					const int32 NI = FaceRef.FindBoneIndex(FName(Nose));
					if (NI != INDEX_NONE) { FaceFwd = (RefCS(FaceRef, NI).GetLocation() - FaceHeadCS.GetLocation()).GetSafeNormal2D(); if (!FaceFwd.IsNearlyZero()) { break; } }
				}
				const int32 HF = BodyRef.FindBoneIndex(FName(TEXT("headfront")));
				if (HF != INDEX_NONE)
				{
					const FVector D = (RefCS(BodyRef, HF).GetLocation() - BodyHeadCS.GetLocation()).GetSafeNormal2D();
					if (!D.IsNearlyZero()) { BodyFwd = D; }
				}
				FaceRot = FaceFwd.IsNearlyZero() ? FQuat::Identity : FQuat::FindBetweenNormals(FaceFwd, BodyFwd);
				const FTransform FaceInBody(FaceRot, BodyHeadCS.GetLocation() - FaceRot.RotateVector(FaceHeadCS.GetLocation()));
				UE_LOG(LogTemp, Display, TEXT("Story: look '%s' - face forward %s, body forward %s -> face turned %.0f deg"), *Look, *FaceFwd.ToCompactString(), *BodyFwd.ToCompactString(),
					FMath::RadiansToDegrees(float(FaceRot.GetAngle())));
				// carried by the NECK (the retargeted walks move the Meshy "head" bone oddly at times: the face flew off), the
				// neck bone named by "face_bone" in the look, default neck_01
				// the face follows the body's NECK POSITION every frame (OnBodyBonesFinalized), upright with the body: carried by a
				// bone (head, neck, chest) it took that bone's retargeted rotation, and the Meshy walks swing those wildly
				const FName Carrier(*JStr(*Person, TEXT("face_bone"), TEXT("neck_01")));   // the neck: the Meshy head bone jumps ~1 m in the retargeted walks
				const int32 CarrierIdx = BodyRef.FindBoneIndex(Carrier) != INDEX_NONE ? BodyRef.FindBoneIndex(Carrier) : BodyRef.FindBoneIndex(FName(TEXT("head")));
				Face->SetAnimInstanceClass(nullptr);
				Face->AttachToComponent(M, FAttachmentTransformRules::KeepRelativeTransform);
				Face->SetRelativeTransform(FaceInBody);
				FaceFollow = Face;
				FaceNeckBone = BodyRef.GetBoneName(CarrierIdx);
				FaceNeckOffset = FaceInBody.GetLocation() - RefCS(BodyRef, CarrierIdx).GetLocation();
				FaceFollowHandle = M->RegisterOnBoneTransformsFinalizedDelegate(FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &AYggStoryNPC::OnBodyBonesFinalized));
				// a headless Meshy body's auto physics asset can give it bounds away from the mesh: it got culled while its face
				// (good bounds) still drew, floating. Fixed bounds from the mesh itself, a little larger for the stride.
				M->bComponentUseFixedSkelBounds = true;
				M->SetBoundsScale(3.f);
				// face, grooms and hat cull WITH the body (a face still drew floating over a culled body)
				Face->bUseAttachParentBound = true;
				for (USceneComponent* C : LookParts) { if (C && C->GetAttachParent() == Face) { C->bUseAttachParentBound = true; } }
				UE_LOG(LogTemp, Display, TEXT("Story: look '%s' - the MetaHuman face rides on the body's %s (rigid, reference pose)"), *Look, *BodyRef.GetBoneName(CarrierIdx).ToString());
			}
			else if (FaceHead != INDEX_NONE)
			{
				const FQuat A = RefCS(BodyRef, BodyRef.FindBoneIndex(FName(TEXT("head")))).GetRotation();
				const FQuat B = RefCS(FaceRef, FaceHead).GetRotation();
				const float Deg = FMath::RadiansToDegrees(float(A.AngularDistance(B)));
				if (Deg > 30.f)
				{
					UE_LOG(LogTemp, Warning, TEXT("Story: look '%s' - the body's head bone is turned %.0f deg from the face's in the reference pose (other bone axes?): the face will sit crooked on it"), *Look, Deg);
				}
			}
		}
	}
	// "hat" (2026-11-21): a static mesh on the face's head bone ("hat_socket"), placed by hat_loc / hat_rot / hat_scale,
	// coloured by hat_tint where its material has a Tint / BaseColorTint. Older looks carry "hat": true / false (the
	// garment's own hat): no mesh then.
	{
		const TSharedPtr<FJsonValue> HatField = (*Person)->TryGetField(TEXT("hat"));
		const FString HatPath = HatField.IsValid() && HatField->Type == EJson::String ? HatField->AsString() : FString();
		if (!HatPath.IsEmpty())
		{
			UStaticMesh* HatMesh = LoadObject<UStaticMesh>(nullptr, *StoryObjectPath(HatPath));
			USkinnedMeshComponent* HatParent = Face ? static_cast<USkinnedMeshComponent*>(Face) : static_cast<USkinnedMeshComponent*>(M);
			const FName Socket(*JStr(*Person, TEXT("hat_socket"), TEXT("head")));
			if (!HatMesh)
			{
				UE_LOG(LogTemp, Warning, TEXT("Story: look '%s' - hat %s not found"), *Look, *HatPath);
			}
			else if (!HatParent->DoesSocketExist(Socket))
			{
				UE_LOG(LogTemp, Warning, TEXT("Story: look '%s' - no bone or socket '%s' on %s for the hat"), *Look, *Socket.ToString(), *GetNameSafe(HatParent->GetSkinnedAsset()));
			}
			else
			{
				auto Vec3 = [&](const TCHAR* Key, const FVector& Def)
				{
					const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
					if ((*Person)->TryGetArrayField(Key, Arr) && Arr->Num() >= 3) { return FVector((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber()); }
					return Def;
				};
				const FVector Loc = Vec3(TEXT("hat_loc"), FVector::ZeroVector);
				const FVector Rot = Vec3(TEXT("hat_rot"), FVector::ZeroVector);
				double Uniform = 1.0;
				const FVector HatScale = (*Person)->TryGetNumberField(TEXT("hat_scale"), Uniform) ? FVector(Uniform) : Vec3(TEXT("hat_scale"), FVector::OneVector);
				UStaticMeshComponent* Hat = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("Hat")));
				Hat->SetStaticMesh(HatMesh);
				Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Hat->SetGenerateOverlapEvents(false);
				Hat->SetCanEverAffectNavigation(false);
				Hat->SetCastShadow(true);
				Hat->SetupAttachment(HatParent, Socket);
				Hat->SetRelativeLocationAndRotation(Loc, FRotator(Rot.X, Rot.Y, Rot.Z));
				Hat->SetRelativeScale3D(HatScale);
				Hat->bUseAttachParentBound = true;     // culls with the head it sits on
				Hat->RegisterComponent();
				LookParts.Add(Hat);
				const FVector Tint = Vec3(TEXT("hat_tint"), FVector(-1.f));
				if (Tint.X >= 0.f)
				{
					for (int32 i = 0; i < Hat->GetNumMaterials(); ++i)
					{
						UMaterialInterface* Mat = Hat->GetMaterial(i);
						if (!Mat) { continue; }
						FName Param;
						FLinearColor Old;
						for (const TCHAR* Name : { TEXT("Tint"), TEXT("BaseColorTint") })
						{
							if (Param.IsNone() && Mat->GetVectorParameterValue(FName(Name), Old)) { Param = FName(Name); }
						}
						if (Param.IsNone()) { continue; }   // no tint parameter: the hat keeps its own colour
						if (UMaterialInstanceDynamic* D = Hat->CreateAndSetMaterialInstanceDynamic(i)) { D->SetVectorParameterValue(Param, FLinearColor(Tint.X, Tint.Y, Tint.Z)); }
					}
				}
			}
		}
	}
	SetActorScale3D(FVector(Scale));
	GetCharacterMovement()->MaxWalkSpeed = 190.f * FMath::Lerp(0.85f, 1.f, Scale);
	bMetaHuman = true;
	bFeetBones = M->DoesSocketExist(TEXT("foot_l")) && M->DoesSocketExist(TEXT("foot_r"));
	bFeetInit = false;
	bTickFar = false;
	LodCheck = 0.f;
	// (#84) soles on the drawn snow; a held pose (sitting cross-legged) keeps the old offset its "sink" was tuned against
	if (Pose)
	{
		bSoleFit = false;
		MeshBaseZ = -82.f;
		SoleDrop = 0.f;
		AppliedMeshZ = 1e9f;
		ApplyMeshZ();
	}
	else
	{
		FitSoles();
	}
	return true;
}

bool AYggStoryNPC::ReachedGoal(float Radius) const
{
	return Path.Num() == 0 && FVector::Dist2D(GetActorLocation(), Goal) < Radius;
}

void AYggStoryNPC::FaceTo(const FVector& At, AActor* Actor, float Seconds)
{
	FaceAt = At;
	FaceActor = Actor;
	FaceLeft = Seconds < 0.f ? -1.f : FMath::Max(0.01f, Seconds);
}

bool AYggStoryNPC::HopAside(const FVector& Me, const FVector& Dir, bool bAnySurface)
{
	UWorld* World = GetWorld();
	const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FCollisionShape Cap = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), Half);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryNPCHop), false, this);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir) * 140.f;
	for (const float S : { 1.f, -1.f })
	{
		const FVector To = Me + Side * S + Dir * 120.f;
		FVector G;
		bool bGround = StoryGround(World, To, 600.f, 600.f, G, this);
		if (!bGround && bAnySurface)
		{
			// a person spawned a little inside a coarse collision surface could never move (the porters stood frozen):
			// after a few failed hops any surface will do, still only where the capsule fits
			FHitResult H;
			FCollisionQueryParams TQ(SCENE_QUERY_STAT(StoryNPCHopAny), true, this);
			if (World->LineTraceSingleByChannel(H, To + FVector(0.f, 0.f, 600.f), To - FVector(0.f, 0.f, 600.f), ECC_Visibility, TQ) && !Cast<APawn>(H.GetActor()))
			{
				G = H.ImpactPoint;
				bGround = true;
			}
		}
		if (!bGround) { continue; }
		const FVector Spot = G + FVector(0.f, 0.f, Half + 5.f);
		// a tent roof, a barrel, a chorten: the capsule does not fit there, so it is not a place to stand (audit)
		if (World->OverlapAnyTestByChannel(Spot, FQuat::Identity, ECC_Pawn, Cap, Q)) { continue; }
		TeleportTo(Spot, Dir.Rotation(), false, true);
		return true;
	}
	return false;
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
	const FVector PLoc = P->GetActorLocation();
	if (Mode != LastMode)
	{
		LastMode = Mode;
		bPresentGoal = false;
		SteerYaw = 0.f;
		SteerHold = 0.f;
		StuckTime = 0.f;
		HopFails = 0;
		TrailCursor = -1;   // (2026-11-20) a stale crumb from before a goto made her walk back; -1 forces the nearest-crumb search
		TrailSkipCheck = 0.f;
		SwitchAt = -1.0;
		bFollowYield = false;
		bFollowTurn = false;
		FollowSpeed = float(GetVelocity().Size2D());   // a follow after a walk goes on at the pace she has, not a stale one
	}
	TalkLeft = FMath::Max(0.f, TalkLeft - DeltaSeconds);
	if (FaceLeft > 0.f) { FaceLeft = FMath::Max(0.f, FaceLeft - DeltaSeconds); if (FaceLeft <= 0.f) { FaceActor = nullptr; } }
	// far away: the MetaHuman body, face and garment pose only while drawn (once a second, with a margin so it cannot flap)
	LodCheck -= DeltaSeconds;
	if (bMetaHuman && LodCheck <= 0.f)
	{
		LodCheck = 1.f;
		const float D = FVector::Dist(Me, PLoc);
		const bool bFar = bTickFar ? D > 4500.f : D > 5000.f;
		if (bFar != bTickFar)
		{
			bTickFar = bFar;
			const EVisibilityBasedAnimTickOption Opt = bFar ? EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered : EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			GetMesh()->VisibilityBasedAnimTickOption = Opt;
			for (USceneComponent* C : LookParts) { if (USkeletalMeshComponent* S = Cast<USkeletalMeshComponent>(C)) { S->VisibilityBasedAnimTickOption = Opt; } }
		}
	}
	// never stand under the drawn ground (a tile whose collision was still cooking when this person landed), and never
	// fall through the world: off the trail's collision corridor (or before a tile has cooked) there is nothing under
	// a person - the porters dropped 120 m out of sight (2026-11-15). Falling > 0.4 s: look far; no ground within
	// 30 m below -> hover (no gravity) until it appears; ground above the feet -> stand on it.
	{
		UCharacterMovementComponent* CM = GetCharacterMovement();
		const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FallTime = CM && CM->IsFalling() ? FallTime + DeltaSeconds : 0.f;
		GroundCheck -= DeltaSeconds;
		const bool bLost = FallTime > 0.4f || bHover;
		if (GroundCheck <= 0.f || FallTime > 0.4f)
		{
			GroundCheck = 0.5f;
			const FVector Feet = Me - FVector(0.f, 0.f, Half);
			FVector G;
			bool bGround = StoryGround(GetWorld(), Feet, bLost ? 6000.f : 220.f, bLost ? 3000.f : 10.f, G, this);
			if (!bGround && bLost)      // any solid surface will do once a person is lost (not only terrain tiles)
			{
				FHitResult H;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryNPCGround), true, this);
				if (GetWorld()->LineTraceSingleByChannel(H, Feet + FVector(0.f, 0.f, 300.f), Feet - FVector(0.f, 0.f, 3000.f), ECC_Visibility, Q)
					&& !Cast<APawn>(H.GetActor())) { G = H.ImpactPoint; bGround = true; }
			}
			if (bGround && (G.Z > Feet.Z + 15.f || bHover))
			{
				SetActorLocation(G + FVector(0.f, 0.f, Half + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
				if (CM) { CM->Velocity = FVector::ZeroVector; CM->GravityScale = 1.f; if (bHover) { CM->SetMovementMode(MOVE_Walking); } }
				bHover = false;
			}
			else if (CM && FallTime > 0.4f && !bGround && !bHover)
			{
				bHover = true; HoverFrom = Me;
				CM->SetMovementMode(MOVE_Flying); CM->Velocity = FVector::ZeroVector; CM->GravityScale = 1.f;
			}
		}
		if (bHover && CM)
		{
			float Z = HoverFrom.Z;
			const float Span = FVector::Dist2D(HoverFrom, Goal);
			if ((Mode == EMode::GoTo || Mode == EMode::Lead || Mode == EMode::Present) && Span > 1.f)
			{
				const float T = FMath::Clamp(1.f - FVector::Dist2D(Me, Goal) / Span, 0.f, 1.f);
				Z = FMath::Lerp(HoverFrom.Z, Goal.Z + Half + 2.f, T);     // places are surface heights
			}
			SetActorLocation(FVector(Me.X, Me.Y, Z), false, nullptr, ETeleportType::TeleportPhysics);
			CM->Velocity.Z = 0.f;
		}
	}
	FVector Target = Me;
	bool bMove = false;
	float Scale = Speed;
	WantSpeed = -1.f;
	switch (Mode)
	{
	case EMode::Follow:
		TickFollow(DeltaSeconds, P, Me, PLoc, Target, bMove);
		break;
	case EMode::GoTo:
	case EMode::Lead:
		// waypoints: the next one once this one is close (a straight line between far places leaves the trail)
		if (Path.Num() > 0 && FVector::Dist2D(Me, Goal) < 200.f)
		{
			Goal = Path[0];
			Path.RemoveAt(0);
		}
		Target = Goal;
		bMove = !ReachedGoal(110.f) && (Mode == EMode::GoTo || (bMayWalk && FVector::Dist2D(Me, PLoc) < 900.f));
		break;
	case EMode::Present:
	{
		// in front of the view at PresentDist, on walkable ground; the goal moves only when the view turns > 25 degrees or the
		// player walks > 1 m, so a glance around does not make the person dance
		const float ViewYaw = P->GetControlRotation().Yaw;
		if (!bPresentGoal || FVector::Dist2D(PLoc, PresentFrom) > 100.f || FMath::Abs(FRotator::NormalizeAxis(ViewYaw - PresentYaw)) > 25.f)
		{
			FVector G = PLoc + FRotator(0.f, ViewYaw, 0.f).Vector() * PresentDist;
			FVector Ground;
			if (StoryGround(GetWorld(), G, 300.f, 600.f, Ground, this)) { G = Ground; }
			else
			{
				FHitResult H;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryNPCPresent), true, this);
				Q.AddIgnoredActor(P);
				if (GetWorld()->LineTraceSingleByChannel(H, G + FVector(0.f, 0.f, 300.f), G - FVector(0.f, 0.f, 600.f), ECC_Visibility, Q)) { G = H.ImpactPoint; }
			}
			Goal = G;
			PresentFrom = PLoc;
			PresentYaw = ViewYaw;
			bPresentGoal = true;
		}
		Target = Goal;
		const float D = FVector::Dist2D(Me, Goal);
		bMove = D > 60.f;
		Scale = FMath::Max(Scale, D > 500.f ? 1.3f : 1.f);
		break;
	}
	default:
		break;
	}
	bArrived = Mode == EMode::GoTo || Mode == EMode::Lead ? ReachedGoal(150.f) : false;
	if (bMove)
	{
		bFollowTurn = false;
		FVector Dir = (Target - Me).GetSafeNormal2D();
		// steering, 4x a second: a capsule sweep 1.2 m ahead; blocked by a tent, a barrel, a chorten -> walk 60 degrees to the
		// free side for a moment (followers stuck on the camp tents for minutes, audit)
		SteerCheck -= DeltaSeconds;
		SteerHold = FMath::Max(0.f, SteerHold - DeltaSeconds);
		if (SteerCheck <= 0.f)
		{
			SteerCheck = 0.25f;
			const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const FCollisionShape Cap = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), Half * 0.75f);
			FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryNPCSteer), false, this);
			const FVector From = Me + FVector(0.f, 0.f, Half * 0.25f + 10.f);   // a little up: the slope ahead is not in the way
			const bool bFollowing = Mode == EMode::Follow;
			auto Blocked = [&](const FVector& D)
			{
				FHitResult H;
				if (!GetWorld()->SweepSingleByChannel(H, From, From + D * 120.f, FQuat::Identity, ECC_Pawn, Cap, Q)) { return false; }
				// people: the hop sorts that out - except a follower overtaking another one (the file reorders after the
				// player turns back through it): walk round the standing person, never a sideways hop through him
				// (only a STANDING one: the person ahead in the file walks our way, and steering round him made the man
				// behind zig-zag 60 degrees every second, follow test 2026-11-20)
				if (Cast<APawn>(H.GetActor()))
				{
					return bFollowing && !H.bStartPenetrating && H.GetActor()->IsA<AYggStoryNPC>()
						&& H.GetActor()->GetVelocity().SizeSquared2D() < FMath::Square(40.f);
				}
				return !IsStoryTerrain(H) && H.ImpactNormal.Z < 0.7f;          // ground and walkable slopes are not obstacles
			};
			if (SteerYaw != 0.f && SteerHold > 0.f)
			{
				// (2026-11-20 #8) round an obstacle on one side, held at least 0.6 s: re-testing the straight way 4x a second
				// flipped straight / aside / straight - a zig-zag walk with the body swinging every quarter second
				if (Blocked(Dir.RotateAngleAxis(SteerYaw, FVector::UpVector)))
				{
					SteerYaw = Blocked(Dir.RotateAngleAxis(-SteerYaw, FVector::UpVector)) ? 0.f : -SteerYaw;
					SteerHold = 0.6f;
				}
			}
			else
			{
				SteerYaw = 0.f;
				if (Blocked(Dir))
				{
					// the side taken last time first (no left-right flip round the same tent)
					const float First = 60.f * SteerSide;
					if (!Blocked(Dir.RotateAngleAxis(First, FVector::UpVector))) { SteerYaw = First; }
					else if (!Blocked(Dir.RotateAngleAxis(-First, FVector::UpVector))) { SteerYaw = -First; }
					SteerHold = 0.6f;
				}
			}
			if (SteerYaw != 0.f) { SteerSide = SteerYaw > 0.f ? 1.f : -1.f; }
		}
		if (SteerYaw != 0.f) { Dir = Dir.RotateAngleAxis(SteerYaw, FVector::UpVector); }
		if (WantSpeed >= 0.f)
		{
			// follow: the pace is chosen by TickFollow (matched to the player's, see there; 40 = its floor behind a trudge)
			AddMovementInput(Dir, 1.f);
			// (2026-11-21) never into the run: the retargeted jog lifts the Meshy bodies ~55 cm off the snow (the floating the
			// user saw while they caught up), and nobody runs at 4,400 m
			GetCharacterMovement()->MaxWalkSpeed = FMath::Clamp(WantSpeed, 40.f, MaxPaceCm());
		}
		else
		{
			AddMovementInput(Dir, FMath::Clamp(Scale, 0.2f, 1.f));
			GetCharacterMovement()->MaxWalkSpeed = FMath::Min(190.f * FMath::Max(1.f, Scale) * GetActorScale3D().X, MaxPaceCm());   // no run (see above)
		}
		GetCharacterMovement()->MaxFlySpeed = GetCharacterMovement()->MaxWalkSpeed;     // hovering (no ground) walks at the same pace
		// stuck against a rock: hop round it (stuck = under a third of the pace asked for; a slow follower is not stuck)
		const float Pace = FMath::Min(GetCharacterMovement()->MaxWalkSpeed, 190.f);
		StuckTime = FVector::Dist2D(Me, LastPos) < FMath::Max(Pace * 0.3f, 25.f) * DeltaSeconds ? StuckTime + DeltaSeconds : 0.f;
		if (StuckTime > 1.5f)
		{
			{	// say what blocks the way (diagnostics for people who cannot walk somewhere)
				FHitResult B;
				FCollisionQueryParams BQ(SCENE_QUERY_STAT(StoryNPCBlock), false, this);
				const FCollisionShape Cap = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				const bool bHit = GetWorld()->SweepSingleByChannel(B, Me, Me + Dir * 60.f, FQuat::Identity, ECC_Pawn, Cap, BQ);
				UE_LOG(LogTemp, Display, TEXT("Story NPC %s stuck at %s: %s %s start-penetrating=%d normal=%s"), *Id.ToString(), *Me.ToCompactString(),
					bHit && B.GetActor() ? *B.GetActor()->GetName() : TEXT("-"), bHit && B.GetComponent() ? *B.GetComponent()->GetName() : TEXT("-"),
					bHit ? int32(B.bStartPenetrating) : -1, bHit ? *B.ImpactNormal.ToCompactString() : TEXT("-"));
			}
			if (HopAside(Me, Dir, HopFails >= 2)) { HopFails = 0; }
			else if (++HopFails >= 4)
			{
				// nothing fits anywhere near: the old unchecked hop, so a person is never frozen for good
				FVector To = Me + FVector::CrossProduct(FVector::UpVector, Dir) * 140.f + Dir * 120.f + FVector(0.f, 0.f, 60.f);
				UE_LOG(LogTemp, Warning, TEXT("Story NPC %s: no free spot to hop to - unchecked hop"), *Id.ToString());
				TeleportTo(To, Dir.Rotation(), false, true);
				HopFails = 0;
			}
			StuckTime = 0.f;
		}
	}
	else
	{
		SteerYaw = 0.f;     // a side step held from before a stop must not send the next start 60 degrees off
		SteerHold = 0.f;
		const bool bFace = FaceLeft != 0.f && (FaceActor.IsValid() || !FaceAt.IsZero());
		if (bFace)
		{
			// npc_face: head and torso round in about 0.5 s, held while standing
			const FVector To = FaceActor.IsValid() ? FaceActor->GetActorLocation() : FaceAt;
			const FVector D = (To - Me).GetSafeNormal2D();
			if (!D.IsNearlyZero()) { SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, D.Rotation().Yaw, 0.f), DeltaSeconds, 9.f)); }
		}
		else if (Mode == EMode::Follow)
		{
			// (2026-11-20 #8) following, standing: turn to the player only once well off (> 35 degrees), at a steady 90 deg/s
			// until roughly there, and not while still sliding to the stop. The old exponential turn tracked every step he
			// took like a turret and swung the whole body fast on big angles.
			const FVector D = (PLoc - Me).GetSafeNormal2D();
			const float Off = D.IsNearlyZero() ? 0.f : float(FRotator::NormalizeAxis(D.Rotation().Yaw - GetActorRotation().Yaw));
			if (!bFacePlayer || FVector::Dist2D(Me, PLoc) > 700.f || FMath::Abs(Off) < 6.f) { bFollowTurn = false; }
			else if (FMath::Abs(Off) > 35.f) { bFollowTurn = true; }
			if (bFollowTurn && GetVelocity().SizeSquared2D() < FMath::Square(40.f))
			{
				SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), FRotator(0.f, D.Rotation().Yaw, 0.f), DeltaSeconds, 90.f));
			}
		}
		else if ((bFacePlayer || Mode == EMode::Present) && FVector::Dist2D(Me, PLoc) < 700.f)
		{
			const FRotator Want = (PLoc - Me).GetSafeNormal2D().Rotation();
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Want.Yaw, 0.f), DeltaSeconds, 3.f));
		}
	}
	LastPos = Me;
	TickWalkBlend(DeltaSeconds);
	TickFeet(DeltaSeconds, P);
	TickSoles(DeltaSeconds, P);
	DebugLog(DeltaSeconds);
}

void AYggStoryNPC::TickWalkBlend(float DeltaSeconds)
{
	if (!bBlendWalk) { return; }
	UAnimSingleNodeInstance* SI = GetMesh() ? GetMesh()->GetSingleNodeInstance() : nullptr;
	if (!SI) { return; }
	// the blend space's speed axis is the clip's own pace (walk 300, jog 600 cm/s at scale 1): a smaller person takes
	// smaller steps, so the speed is read at body scale 1
	const FVector V = GetVelocity();
	const float Spd = float(V.Size2D()) / FMath::Max(float(GetActorScale3D().X), 0.5f);
	BlendSpeed = FMath::FInterpTo(BlendSpeed, Spd, DeltaSeconds, 8.f);
	float Dir = 0.f;
	if (Spd > 10.f) { Dir = FRotator::NormalizeAxis(V.Rotation().Yaw - GetActorRotation().Yaw); }
	SI->SetBlendSpacePosition(FVector(Dir, BlendSpeed < 4.f ? 0.f : BlendSpeed, 0.f));
}

void AYggStoryNPC::TickFollow(float DeltaSeconds, APawn* P, const FVector& Me, const FVector& PLoc, FVector& Target, bool& bMove)
{
	// only where the player walks counts, never where he looks: his velocity and his trail, no view, no actor facing
	const FVector V = P->GetVelocity();
	const float Vp = float(V.Size2D());
	if (Vp > 50.f)
	{
		// his walking direction, eased (a strafe tap or a step on a stone swung the talk_side spot across his back)
		const FVector Vd = V.GetSafeNormal2D();
		const FVector F = FollowFwd.IsNearlyZero() ? Vd : (FollowFwd + (Vd - FollowFwd) * FMath::Min(1.f, DeltaSeconds * 4.f)).GetSafeNormal2D();
		FollowFwd = F.IsNearlyZero() ? Vd : F;
	}
	// walking or standing, with a margin (a pace near one threshold flipped the decision every frame); his pace eased
	if (Vp > 45.f) { bLeaderWalks = true; } else if (Vp < 25.f) { bLeaderWalks = false; }
	LeaderPace = FMath::FInterpTo(LeaderPace, Vp, DeltaSeconds, 4.f);
	const float Base = 190.f * Speed * GetActorScale3D().X;      // this person's own walking pace
	const float Straight = float(FVector::Dist2D(Me, PLoc));
	const double Now = GetWorld()->GetTimeSeconds();
	UYggStorySubsystem* S = GetWorld()->GetSubsystem<UYggStorySubsystem>();
	const bool bTalk = TalkSide > 0.f && TalkLeft > 0.f;
	float Gap = Straight;
	float Behind = FollowBehind;
	FVector Aim = PLoc;
	bFollowYield = false;
	bool bOnTrail = false;
	if (bTalk || !S || S->Crumbs.Num() == 0)
	{
		// talk_side: while this person speaks he walks at your shoulder, so a 30 degree glance finds the face (spec 6.5)
		const FVector F = !FollowFwd.IsNearlyZero() ? FollowFwd : (PLoc - Me).GetSafeNormal2D();
		const FVector R = FVector::CrossProduct(FVector::UpVector, F);
		const float Side = bTalk ? TalkSide * (FollowSide < 0.f ? -1.f : 1.f) : FollowSide;
		Aim = PLoc - F * (bTalk ? 30.f : FollowBehind) + R * Side;
		Behind = 0.f;
		if (bLeaderWalks)
		{
			// (2026-11-20 #8) walking: how far the spot lies ahead of us along his way (< 0 = we are past it: slow down,
			// never turn round to it), and the walk aims on past it. The plain distance to it stopped her at the spot and
			// sent her after it again a second later, all through a line.
			Gap = float(FVector::DotProduct(Aim - Me, F));
			Aim += F * 150.f;
		}
		else { Gap = float(FVector::Dist2D(Me, Aim)); }
		// off the trail now: once the line ends, look for our place on it again (the cursor from before the line lay
		// 10 m back and she walked back to it)
		TrailCursor = -1;
	}
	else
	{
		bOnTrail = true;
		const TArray<FVector>& C = S->Crumbs;
		const TArray<float>& CL = S->CrumbLen;
		const int32 N = C.Num();
		// the trail started over, or this person has no place on it yet: the nearest point (the later one on a tie)
		if (TrailEpochSeen != S->TrailEpoch || TrailCursor < S->TrailFirst || TrailCursor >= S->TrailFirst + N)
		{
			int32 Best = N - 1;
			double BestD = TNumericLimits<double>::Max();
			for (int32 i = 0; i < N; ++i)
			{
				const double D = FVector::DistSquared2D(Me, C[i]);
				if (D <= BestD) { BestD = D; Best = i; }
			}
			TrailCursor = S->TrailFirst + Best;
			TrailEpochSeen = S->TrailEpoch;
			TrailSkipCheck = 0.f;
		}
		int32 Ci = TrailCursor - S->TrailFirst;
		// past a point (reached it, or it lies behind on the way to the next one): the next
		while (Ci < N - 1)
		{
			const FVector A = C[Ci];
			const float DA = float(FVector::Dist2D(Me, A));
			const bool bPast = DA < 400.f && FVector::DotProduct((C[Ci + 1] - A).GetSafeNormal2D(), Me - A) > 0.f;
			if (DA < 90.f || bPast) { ++Ci; } else { break; }
		}
		// (2026-11-20 #8) the trail folds back past us (he turned round and walked back by us, or walked a loop): a later
		// point close by (4 m, about level), reached in a clear line, saves the walk out to the turn and back - she walked
		// away from him to his old turning point first. Worth it only when the trail skipped beats twice the line off it
		// by 2 m (a bend is never cut, a fold is). Twice a second, from the cursor on.
		TrailSkipCheck -= DeltaSeconds;
		if (TrailSkipCheck <= 0.f && Ci < N - 1)
		{
			TrailSkipCheck = 0.5f;
			const float SMe0 = CL[Ci] - float(FVector::Dist2D(Me, C[Ci]));
			int32 BestK = INDEX_NONE;
			float BestSave = 200.f;
			for (int32 k = Ci + 1; k < N; ++k)
			{
				const double D2 = FVector::DistSquared2D(Me, C[k]);
				if (D2 > 160000.0 || FMath::Abs(C[k].Z - Me.Z) > 120.0) { continue; }
				const float Save = (CL[k] - SMe0) - 2.f * float(FMath::Sqrt(D2));
				if (Save > BestSave) { BestSave = Save; BestK = k; }
			}
			if (BestK != INDEX_NONE)
			{
				FHitResult H;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryNPCTrailSkip), false, this);
				Q.AddIgnoredActor(P);
				if (!GetWorld()->LineTraceSingleByChannel(H, Me, C[BestK], ECC_Visibility, Q) || Cast<APawn>(H.GetActor()))
				{
					if (CVarStoryNPCDebug.GetValueOnGameThread() != 0)
					{
						UE_LOG(LogTemp, Display, TEXT("NPCDbg %s trail skip %d -> %d (saves %.0f cm)"), *Id.ToString(), S->TrailFirst + Ci, S->TrailFirst + BestK, BestSave);
					}
					Ci = BestK;
				}
			}
		}
		TrailCursor = S->TrailFirst + Ci;
		// (2026-11-20 #8) how far behind ALONG the walked way. It was min(trail, straight across), and straight across is
		// always the shorter: the band ran on the straight line, so on every bend and switchback the rear people found
		// themselves "too close", stopped, and went on once the line opened again. Straight x3 stays as a guard only.
		const float SPlayer = CL[N - 1] + float(FVector::Dist2D(C[N - 1], PLoc));
		const float SMe = CL[Ci] - float(FVector::Dist2D(Me, C[Ci]));
		Gap = FMath::Min(SPlayer - SMe, Straight * 3.f);
		// he turned back toward us: never walk into him (nor toward him as he comes by). Walking at us (his line passes
		// within 2.2 m) and nearer than our place -> stand and let him come by; the skip above puts us behind him again
		if (bLeaderWalks)
		{
			const FVector Vd = V.GetSafeNormal2D();
			const float RX = float(Me.X - PLoc.X), RY = float(Me.Y - PLoc.Y);
			const float Ahead = float(Vd.X) * RX + float(Vd.Y) * RY;
			const float Lateral = FMath::Abs(float(Vd.X) * RY - float(Vd.Y) * RX);
			// (ahead more than half the side offset: not when a talk_side shoulder walk ends a step in front of him)
			bFollowYield = Ahead > Lateral * 0.5f && Lateral < 220.f && Straight < Behind + 150.f;
		}
		// aim a little ahead on the trail (a walk, not a hop from point to point); the side keeps the file loose
		int32 J = Ci;
		while (J < N - 1 && FVector::Dist2D(Me, C[J]) < 150.f) { ++J; }
		Aim = C[J];
		// (2026-11-20 #8) at the head of the trail (beside him after a talk_side line, or he just came by us) the last point
		// can lie at our side or behind: walk on his way instead of turning round to it
		if (J == N - 1 && !FollowFwd.IsNearlyZero() && FVector::Dist2D(Me, C[J]) < 150.f) { Aim = Me + FollowFwd * 150.f; }
		const FVector Along = (Aim - Me).GetSafeNormal2D();
		Aim += FVector::CrossProduct(FVector::UpVector, Along) * FMath::Clamp(FollowSide * 0.3f, -60.f, 60.f);
		// far behind and out of sight: catch up quietly onto the trail (a person walking with you never gets lost)
		if (Straight > 4500.f)
		{
			int32 K = N - 1;
			while (K > 0 && SPlayer - CL[K] < FollowBehind) { --K; }
			const FVector View = P->GetControlRotation().Vector().GetSafeNormal2D();
			// (2026-11-20) a trail too short to hold a point FollowBehind back (C[K] on top of the player) skips the catch-up
			const FVector ToK = C[K] - PLoc;
			if (ToK.SizeSquared2D() > FMath::Square(FollowBehind * 0.5f) && FVector::DotProduct(View, ToK.GetSafeNormal2D()) < 0.2f)
			{
				TeleportTo(C[K] + FVector(0.f, 0.f, 120.f), GetActorRotation());
				TrailCursor = S->TrailFirst + K;
				bFollowStopped = true;
				SwitchAt = -1.0;
				FollowSpeed = 0.f;
			}
		}
	}
	// pace: a distance band round "behind". (2026-11-20 #8) While he walks we never stop for being close, we only slow
	// down (a floor under his pace: stop-and-go behind a slow player); we stop when he stops and we are near our place,
	// or when he walks straight at us. Every stop and restart comes a moment after his (people do not all react on his
	// frame); restarts ripple down the file, the nearest first. Walking: his pace, a little faster when behind, slower
	// when close, a jog to catch up from far.
	const float Err = Gap - Behind;
	FollowErr = Err;
	if (bFollowStopped)
	{
		const bool bGo = !bFollowYield && (bTalk ? (bLeaderWalks || Err > 90.f) : (Err > 110.f || (bLeaderWalks && Err > -60.f)));
		if (bGo)
		{
			if (SwitchAt < 0.0) { SwitchAt = Now + FMath::FRandRange(0.3f, 0.6f) + FMath::Min(FollowBehind, 1500.f) * 0.0004f; }
			if (Now >= SwitchAt) { bFollowStopped = false; SwitchAt = -1.0; }
		}
		else { SwitchAt = -1.0; }
	}
	else
	{
		// he stopped: stop with him (the settle slides ~0.5 m); or about to touch him (a creep under our slowest pace)
		const bool bStop = bFollowYield || (!bLeaderWalks && Err < (bTalk ? 50.f : 90.f)) || (bOnTrail && Gap < 80.f);
		if (bStop)
		{
			if (SwitchAt < 0.0) { SwitchAt = Now + (bFollowYield ? FMath::FRandRange(0.05f, 0.15f) : FMath::FRandRange(0.1f, 0.35f)); }
			if (Now >= SwitchAt) { bFollowStopped = true; SwitchAt = -1.0; }
		}
		else { SwitchAt = -1.0; }
	}
	float Want = 0.f;
	if (!bFollowStopped)
	{
		// the cap: a jog of this person's own, or a little over the player's pace (he may walk faster than we jog)
		const float Cap = FMath::Min(FMath::Max(Base * 1.6f, LeaderPace * 1.3f), 520.f);
		// the floor: under a slow player's own pace (70 over a 50 cm/s trudge closed in, stopped, waited, went again)
		const float Floor = FMath::Clamp(LeaderPace * 0.6f, 40.f, 70.f);
		// (2026-11-20 #8) inside the band (0.6 m closer .. 0.8 m further than our place) just his pace: chasing the exact
		// spot made every start a hurry at 1.3x his pace and the speed breathe in and out. A talk_side walk keeps it tight
		// (the face has to stay in a 30 degree glance)
		const float E = FMath::Clamp(Err, -200.f, 500.f);
		Want = LeaderPace + (E - FMath::Clamp(E, bTalk ? -30.f : -60.f, bTalk ? 30.f : 80.f)) * 0.45f;
		if (Err > 900.f) { Want = FMath::Max(Want, Cap); }
		Want = FMath::Clamp(Want, Floor, Cap);
	}
	FollowSpeed = FMath::FInterpTo(FollowSpeed, Want, DeltaSeconds, 2.5f);
	Target = Aim;
	bMove = !bFollowStopped;
	WantSpeed = bMove ? FMath::Max(FollowSpeed, 40.f) : -1.f;
}

void AYggStoryNPC::TickFeet(float DeltaSeconds, APawn* P)
{
	if (!bFeetBones || CVarStoryNPCPrints.GetValueOnGameThread() == 0 || DeltaSeconds <= 0.f) { return; }
	const UCharacterMovementComponent* CM = GetCharacterMovement();
	const float Spd = float(GetVelocity().Size2D());
	if (Spd < 30.f || !CM || !CM->IsMovingOnGround() || FVector::DistSquared2D(GetActorLocation(), P->GetActorLocation()) > FMath::Square(6000.f))
	{
		bFootPlanted[0] = bFootPlanted[1] = true;
		bFeetInit = false;
		SinceStamp = 0.f;
		return;
	}
	USnowTrailComponent* Snow = PlayerSnow.Get();
	if (!Snow) { Snow = P->FindComponentByClass<USnowTrailComponent>(); PlayerSnow = Snow; }
	if (!Snow) { return; }
	USkeletalMeshComponent* M = GetMesh();
	const FVector Fwd = GetVelocity().GetSafeNormal2D();
	static const FName Bones[2] = { FName(TEXT("foot_l")), FName(TEXT("foot_r")) };
	SinceStamp += DeltaSeconds;
	FVector Loc[2];
	for (int32 i = 0; i < 2; ++i)
	{
		Loc[i] = M->GetSocketLocation(Bones[i]);
		if (!bFeetInit) { FootLast[i] = Loc[i]; FootRatio[i] = 0.f; continue; }
		// a planted boot stands still in the world while the body walks on; the swinging one moves at ~2x the body
		const float Ratio = float(FVector::Dist2D(Loc[i], FootLast[i])) / DeltaSeconds / Spd;
		FootLast[i] = Loc[i];
		FootRatio[i] = FMath::Lerp(FootRatio[i], Ratio, 0.5f);
		if (!bFootPlanted[i] && FootRatio[i] < 0.45f)
		{
			bFootPlanted[i] = true;
			Stamps += Snow->StampOtherFoot(Loc[i], Fwd, i == 0, this) ? 1 : 0;
			SinceStamp = 0.f;
		}
		else if (bFootPlanted[i] && FootRatio[i] > 1.1f)
		{
			bFootPlanted[i] = false;
		}
	}
	// safety net: no plant seen for a while (a blend that slides) - the lower foot prints
	if (bFeetInit && SinceStamp > 0.9f)
	{
		const int32 Low = Loc[0].Z <= Loc[1].Z ? 0 : 1;
		Stamps += Snow->StampOtherFoot(Loc[Low], Fwd, Low == 0, this) ? 1 : 0;
		SinceStamp = 0.f;
	}
	bFeetInit = true;
}

void AYggStoryNPC::DebugLog(float DeltaSeconds)
{
	if (CVarStoryNPCDebug.GetValueOnGameThread() == 0) { return; }
	DebugTime -= DeltaSeconds;
	if (DebugTime > 0.f) { return; }
	DebugTime = 0.5f;
	const UCharacterMovementComponent* CM = GetCharacterMovement();
	USkeletalMeshComponent* M = GetMesh();
	UAnimInstance* A = M ? M->GetAnimInstance() : nullptr;
	auto Num = [A](const TCHAR* Name) -> FString
	{
		const FProperty* Pr = A ? A->GetClass()->FindPropertyByName(FName(Name)) : nullptr;
		if (const FDoubleProperty* D = CastField<FDoubleProperty>(Pr)) { return FString::Printf(TEXT("%.0f"), D->GetPropertyValue_InContainer(A)); }
		if (const FFloatProperty* F = CastField<FFloatProperty>(Pr)) { return FString::Printf(TEXT("%.0f"), F->GetPropertyValue_InContainer(A)); }
		if (const FBoolProperty* B = CastField<FBoolProperty>(Pr)) { return B->GetPropertyValue_InContainer(A) ? TEXT("1") : TEXT("0"); }
		return TEXT("?");
	};
	const FVector Base = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float FL = M && M->DoesSocketExist(TEXT("foot_l")) ? float(M->GetSocketLocation(TEXT("foot_l")).Z - Base.Z) : -999.f;
	const float FR = M && M->DoesSocketExist(TEXT("foot_r")) ? float(M->GetSocketLocation(TEXT("foot_r")).Z - Base.Z) : -999.f;
	const float Stride = M && M->DoesSocketExist(TEXT("foot_l")) ? float(FVector::Dist2D(M->GetSocketLocation(TEXT("foot_l")), M->GetSocketLocation(TEXT("foot_r")))) : -1.f;
	// (#84) the lower boot: sole = its sole over the drawn snow under it (collision + the read-back trench, cm, - = in it),
	// sink = the player's sink rule there, drop = how far the body is lowered for it, trail = the trench (- ) or bank (+)
	// drawn there (? = not read back yet), snow = the ground is snow, base = the mesh's fitted offset. NPCDebug 2 adds a
	// depth capture of the real drawn surface (render flush, testing only): solev = the sole over it, visgap = it minus
	// (collision + trail), the part of the drawn snow the fit does not know about (the tiles' own displacement).
	FString Soles = FString::Printf(TEXT(" sole=%.1f sink=%.1f drop=%.1f trail=%.1f%s snow=%d base=%.1f fit=%d"), DbgSole, SoleSink, SoleDrop, DbgTrail,
		bDbgTrailKnown ? TEXT("") : TEXT("?"), int32(bSoleSnow), MeshBaseZ, int32(bSoleFit && CVarStoryNPCSoles.GetValueOnGameThread() != 0));
	// (2026-11-21) a MetaHuman face on a foreign body: how far its head bone is from where the body's neck puts it, and the
	// face's own rendering (a face drew floating over a body that did not)
	for (USceneComponent* C : LookParts)
	{
		USkeletalMeshComponent* F = Cast<USkeletalMeshComponent>(C);
		if (!F || !F->GetName().StartsWith(TEXT("Face")) || !M) { continue; }
		const FVector FHead = F->GetSocketLocation(TEXT("head"));
		const FVector BNeck = M->DoesSocketExist(TEXT("neck_01")) ? M->GetSocketLocation(TEXT("neck_01")) : M->GetComponentLocation();
		const FVector Off = FHead - GetActorLocation();
		Soles += FString::Printf(TEXT(" faceoff=%.0f,%.0f,%.0f facegap=%.0f facez=%.0f neckz=%.0f actz=%.0f facerend=%d attach=%s"), float(Off.X), float(Off.Y), float(Off.Z), float(FVector::Dist(FHead, BNeck)), float(FHead.Z - GetActorLocation().Z),
			float(BNeck.Z - GetActorLocation().Z), float(GetActorLocation().Z), int32(F->WasRecentlyRendered(0.5f)), *F->GetAttachSocketName().ToString());
		break;
	}
	APawn* DbgPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
	if (CVarStoryNPCDebug.GetValueOnGameThread() >= 2 && bSoleFit && DbgPlayer && FVector::Dist2D(GetActorLocation(), DbgPlayer->GetActorLocation()) < 3000.f)
	{
		const double Vis = StoryNPCVisibleZ(this, DbgPlayer, DbgSoleAt, DbgColZ + 400.0);
		if (!FMath::IsNaN(Vis))
		{
			const double SoleZ = DbgColZ + DbgTrail + DbgSole;
			Soles += FString::Printf(TEXT(" solev=%.1f visgap=%.1f"), float(SoleZ - Vis), float(Vis - (DbgColZ + DbgTrail)));
		}
	}
	UE_LOG(LogTemp, Display, TEXT("NPCDbg %s mode=%d move=%d hover=%d vel=%.0f acc=%.0f maxwalk=%.0f anim=%s mode=%d GroundSpeed=%s ShouldMove=%s IsFalling=%s footL=%.1f footR=%.1f stride=%.0f tickopt=%d rendered=%d blend=%d stamps=%d stopped=%d cursor=%d dist=%.0f want=%.0f err=%.0f yield=%d steer=%.0f yaw=%.0f pace=%.0f%s"),
		*Id.ToString(), int32(Mode), CM ? int32(CM->MovementMode) : -1, int32(bHover), GetVelocity().Size2D(), CM ? CM->GetCurrentAcceleration().Size2D() : -1.f,
		CM ? CM->MaxWalkSpeed : -1.f, A ? *A->GetClass()->GetName() : TEXT("none"), M ? int32(M->GetAnimationMode()) : -1,
		*Num(TEXT("GroundSpeed")), *Num(TEXT("ShouldMove")), *Num(TEXT("IsFalling")), FL, FR, Stride,
		M ? int32(M->VisibilityBasedAnimTickOption) : -1, M ? int32(M->WasRecentlyRendered(0.5f)) : -1, int32(bBlendWalk), Stamps, int32(bFollowStopped), TrailCursor,
		UGameplayStatics::GetPlayerPawn(this, 0) ? float(FVector::Dist2D(GetActorLocation(), UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation())) : -1.f, FollowSpeed,
		FollowErr, int32(bFollowYield), SteerYaw, float(GetActorRotation().Yaw), LeaderPace, *Soles);
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
	// AlignX 0.5 = centred on Anchor, 0 = starts at Anchor
	auto TextAt = [&](int32 Layer, const FString& T, const FSlateFontInfo& F, FLinearColor C, FVector2f Anchor, float AlignX)
	{
		const FVector2f Sz = FVector2f(Measure->Measure(T, F));
		const FVector2f Pos = Anchor - FVector2f(Sz.X * AlignX, Sz.Y * 0.5f);
		FLinearColor Sh = YggUI::Ink; Sh.A = C.A * 0.5f;
		FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 2.f) * U)), T, F, ESlateDrawEffect::None, Sh);
		FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)), T, F, ESlateDrawEffect::None, C);
		return Sz;
	};
	auto Text = [&](int32 Layer, const FString& T, const FSlateFontInfo& F, FLinearColor C, FVector2f Centre)
	{
		return TextAt(Layer, T, F, C, Centre, 0.5f);
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
	auto Gradient = [&](int32 Layer, float X0, float W, float Top, float Bottom, float A)
	{
		// the lower-third gradient behind captions: clear at the top, darkest at the bottom edge (spec 5.9: no card veil)
		const int32 N = 12;
		const float H = (Bottom - Top) / N;
		for (int32 i = 0; i < N; ++i)
		{
			const float K = float(i + 1) / N;
			Box(Layer, FVector2f(X0, Top + H * i), FVector2f(W, H + 1.f), YggUI::WithA(YggUI::Ink, A * 0.42f * K * K));
		}
	};
	auto Lines = [](const FString& T)
	{
		TArray<FString> Rows;
		T.Replace(TEXT("|"), TEXT("\n")).ParseIntoArray(Rows, TEXT("\n"), false);
		return Rows;
	};

	// eyelids (spec 6.6): two soft curved lids closing over the view, full black once almost shut
	if (Lid > 0.001f)
	{
		const FLinearColor LidC = YggUI::WithA(BlackColor, 1.f);
		if (Lid >= 0.98f)
		{
			Box(L, FVector2f(0.f, 0.f), S, LidC);
		}
		else
		{
			const float Edge = 0.12f * S.Y;
			const float UpH = Lid * 0.55f * S.Y;
			const float LoH = Lid * 0.50f * S.Y;
			auto SoftEdge = [&](float Y, bool bLower)
			{
				if (LidTex)
				{
					FSlateBrush Brush;
					Brush.SetResourceObject(LidTex);
					Brush.ImageSize = FVector2D(S.X, Edge);
					Brush.DrawAs = ESlateBrushDrawType::Image;
					Brush.Mirroring = bLower ? ESlateBrushMirrorType::Vertical : ESlateBrushMirrorType::NoMirror;
					FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(S.X, Edge), FSlateLayoutTransform(FVector2f(0.f, Y))), &Brush, ESlateDrawEffect::None, LidC);
					return;
				}
				// no T_Eyelid yet: a straight soft edge from stacked strips
				const int32 N = 10;
				for (int32 i = 0; i < N; ++i)
				{
					const float K = (i + 0.5f) / N;
					const float A = bLower ? K : 1.f - K;
					Box(L, FVector2f(0.f, Y + Edge * i / N), FVector2f(S.X, Edge / N + 1.f), YggUI::WithA(BlackColor, A * A));
				}
			};
			if (UpH - Edge > 0.f) { Box(L, FVector2f(0.f, 0.f), FVector2f(S.X, UpH - Edge), LidC); }
			SoftEdge(UpH - Edge, false);
			if (LoH - Edge > 0.f) { Box(L, FVector2f(0.f, S.Y - LoH + Edge), FVector2f(S.X, LoH - Edge), LidC); }
			SoftEdge(S.Y - LoH, true);
		}
	}
	if (Black > 0.001f)
	{
		Box(L, FVector2f(0.f, 0.f), S, YggUI::WithA(BlackColor, Black));
	}
	// inspect: the flat card only without the view rig (the rig holds the real object in the hands); its words always
	if (InspectA > 0.01f)
	{
		const bool bBack = bInspectBack;
		if (bInspectDarken)
		{
			Box(L + 1, FVector2f(0.f, 0.f), S, YggUI::WithA(YggUI::Ink, InspectA * 0.25f));      // the background darkens by a quarter
		}
		if (bInspectCard)
		{
			// the photo held up: turned by the mouse (the rig's turn foreshortens it), its back once turned past 90 degrees
			UTexture2D* Img = bBack ? InspectBackImage.Get() : InspectFront.Get();
			const UTexture2D* SizeOf = Img ? Img : InspectFront.Get();
			const float Aspect = SizeOf ? float(SizeOf->GetSizeX()) / FMath::Max(1.f, float(SizeOf->GetSizeY())) : 1.5f;
			const float H = S.Y * 0.46f * InspectTurnY;
			const float W = S.Y * 0.46f * Aspect * InspectTurnX;
			const FVector2f Pos(S.X * 0.5f - W * 0.5f, S.Y * 0.40f - H * 0.5f);
			if (Img)
			{
				FSlateBrush Brush;
				Brush.SetResourceObject(Img);
				Brush.ImageSize = FVector2D(W, H);
				FSlateDrawElement::MakeBox(Out, L + 1, G.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(Pos)), &Brush, ESlateDrawEffect::None, YggUI::WithA(FLinearColor::White, InspectA));
			}
			else
			{
				Box(L + 1, Pos, FVector2f(W, H), YggUI::WithA(FLinearColor(0.86f, 0.83f, 0.76f), InspectA));      // the plain paper back of a photo
			}
		}
		const FString& Words = bBack && !InspectBackText.IsEmpty() ? InspectBackText : InspectText;
		const TArray<FString> Rows = Lines(Words);
		if (Rows.Num() > 0)
		{
			Gradient(L + 1, 0.f, S.X, S.Y * 0.64f, S.Y, InspectA);
			float Y = S.Y * 0.80f - (Rows.Num() - 1) * 19.f * U;
			for (const FString& R : Rows)
			{
				Text(L + 2, R, StoryFont(1, int32(28 * U)), YggUI::WithA(YggUI::Snow, InspectA), FVector2f(S.X * 0.5f, Y));
				Y += 38.f * U;
			}
		}
		const bool bHasBack = InspectBackImage || !InspectBackText.IsEmpty();
		const TCHAR* Keys = bInspectNeedBack ? (bInspectFlipKey ? TEXT("[F]  turn it over") : TEXT("turn it over with the mouse"))
			: (bInspectFlipKey && bHasBack ? TEXT("[F]  turn it over      [E]  put it away") : TEXT("[E]  put it away"));
		Text(L + 2, Keys, YggUI::SerifInfo(int32(13 * U)), YggUI::WithA(YggUI::Shade, InspectA), FVector2f(S.X * 0.5f, S.Y * 0.95f));
	}
	if (TitleA > 0.01f)
	{
		Text(L + 2, Title, StoryFont(0, int32(46 * U), int32(260 * U)), YggUI::WithA(YggUI::Snow, TitleA), FVector2f(S.X * 0.5f, S.Y * 0.44f));
		if (!Sub.IsEmpty())
		{
			Text(L + 2, Sub, YggUI::SerifInfo(int32(17 * U)), YggUI::WithA(YggUI::Mist, TitleA), FVector2f(S.X * 0.5f, S.Y * 0.44f + 58.f * U));
		}
	}
	// captions (spec 6.14): lower third on a soft gradient, or small in the lower left
	if (CaptionA > 0.01f && !Caption.IsEmpty())
	{
		const TArray<FString> Rows = Lines(Caption);
		if (bCaptionLowLeft)
		{
			Gradient(L + 1, 0.f, S.X * 0.5f, S.Y * 0.80f, S.Y, CaptionA * 0.8f);
			float Y = S.Y * 0.92f - (Rows.Num() - 1) * 24.f * U;
			for (const FString& R : Rows)
			{
				TextAt(L + 2, R, YggUI::SerifInfo(int32(16 * U)), YggUI::WithA(YggUI::Snow, CaptionA), FVector2f(S.X * 0.05f, Y), 0.f);
				Y += 24.f * U;
			}
		}
		else
		{
			Gradient(L + 1, 0.f, S.X, S.Y * 0.60f, S.Y, CaptionA);
			float Y = S.Y * 0.74f - (Rows.Num() - 1) * 16.f * U;
			for (const FString& R : Rows)
			{
				Text(L + 2, R, YggUI::SerifInfo(int32(21 * U)), YggUI::WithA(YggUI::Snow, CaptionA), FVector2f(S.X * 0.5f, Y));
				Y += 32.f * U;
			}
		}
	}
	if (LineA > 0.01f && !Line.IsEmpty())
	{
		const float Y = S.Y * 0.84f;
		Veil(L + 1, Y, 92.f * U, LineA);
		if (!Who.IsEmpty())
		{
			Text(L + 2, Who.ToUpper(), StoryFont(0, int32(11 * U), int32(220 * U)), YggUI::WithA(YggUI::Sun, LineA * 0.9f), FVector2f(S.X * 0.5f, Y - 26.f * U));
		}
		Text(L + 2, Line, YggUI::SerifInfo(int32(22 * U)), YggUI::WithA(YggUI::Snow, LineA), FVector2f(S.X * 0.5f, Y + 4.f * U));
	}
	if (HintA > 0.01f && !Hint.IsEmpty())
	{
		const FLinearColor HintColour = FMath::Lerp(YggUI::Mist, YggUI::Snow, FMath::Clamp(HintGlow, 0.f, 1.f));
		Text(L + 2, Hint, YggUI::SerifInfo(int32(15 * U)), YggUI::WithA(HintColour, HintA), FVector2f(S.X * 0.5f, S.Y * 0.93f));
		if (HoldProgress >= 0.f)
		{
			const float W = 160.f * U;
			Box(L + 2, FVector2f(S.X * 0.5f - W * 0.5f, S.Y * 0.93f + 16.f * U), FVector2f(W, 1.f * U), YggUI::WithA(YggUI::Shade, HintA * 0.5f));
			Box(L + 3, FVector2f(S.X * 0.5f - W * 0.5f, S.Y * 0.93f + 16.f * U), FVector2f(W * FMath::Clamp(HoldProgress, 0.f, 1.f), 1.5f * U), YggUI::WithA(YggUI::Sun, HintA));
		}
	}
	if (PressA > 0.01f && !PressPrompt.IsEmpty())
	{
		Text(L + 2, PressPrompt, YggUI::SerifInfo(int32(16 * U)), YggUI::WithA(YggUI::Snow, PressA), FVector2f(S.X * 0.5f, S.Y * 0.60f));
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
			Text(L + 2, T, YggUI::SerifInfo(int32(20 * U)), YggUI::WithA(YggUI::Snow, ChoiceA), FVector2f(S.X * 0.5f, Y0 + Gap * i));
		}
		if (ChoiceTimeLeft >= 0.f)
		{
			const float W = 220.f * U * FMath::Clamp(ChoiceTimeLeft, 0.f, 1.f);
			Box(L + 3, FVector2f(S.X * 0.5f - W * 0.5f, Y0 + Gap * Options.Num()), FVector2f(W, 1.2f * U), YggUI::WithA(YggUI::Mist, ChoiceA * 0.6f));
		}
	}
	if (PageA > 0.01f)
	{
		Box(L + 4, FVector2f(0.f, 0.f), S, YggUI::WithA(YggUI::Ink, PageA * 0.86f));
		float Y = S.Y * 0.16f;
		Text(L + 5, PageHead, StoryFont(0, int32(16 * U), int32(260 * U)), YggUI::WithA(YggUI::Sun, PageA), FVector2f(S.X * 0.5f, Y));
		Y += 34.f * U;
		Box(L + 5, FVector2f(S.X * 0.5f - 60.f * U, Y), FVector2f(120.f * U, 1.f * U), YggUI::WithA(YggUI::Mist, PageA * 0.5f));
		Y += 30.f * U;
		for (const FString& R : Page)
		{
			Text(L + 5, R, StoryFont(1, int32(29 * U)), YggUI::WithA(YggUI::Snow, PageA), FVector2f(S.X * 0.5f, Y + 18.f * U));
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
	if (Overlay) { Overlay->RemoveFromParent(); Overlay = nullptr; }
	bEndFade = false;
	CamSteps.Reset();
	CamStep = INDEX_NONE;
	FrameTime = -1.f;
	Super::Deinitialize();
}

APawn* UYggStorySubsystem::Player() const { return UGameplayStatics::GetPlayerPawn(GetWorld(), 0); }
APlayerController* UYggStorySubsystem::PC() const { return UGameplayStatics::GetPlayerController(GetWorld(), 0); }

UYggViewRigComponent* UYggStorySubsystem::Rig()
{
	UYggViewRigComponent* R = RigCache.Get();
	APawn* P = Player();
	if (!R || !P || R->GetOwner() != P)
	{
		R = P ? UYggViewRigComponent::Find(P) : nullptr;
		RigCache = R;
	}
	return R;
}

void UYggStorySubsystem::EnsureOverlay()
{
	if (Overlay || !PC()) { return; }
	Overlay = CreateWidget<UYggStoryOverlay>(PC(), UYggStoryOverlay::StaticClass());
	if (Overlay)
	{
		Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		Overlay->AddToViewport(60);
		Overlay->LidTex = YggAssets::Kept<UTexture2D>(TEXT("/Game/UI/T_Eyelid.T_Eyelid"), LOAD_NoWarn | LOAD_Quiet);
	}
}

bool UYggStorySubsystem::LoadPlaces(const TSharedPtr<FJsonObject>& Root)
{
	// [x, y, z, yaw, (pitch)]: places are surface heights; the rotation is the facing a teleport there gets
	auto Read = [this](const TSharedPtr<FJsonObject>& Obj)
	{
		int32 N = 0;
		if (!Obj.IsValid()) { return N; }
		for (const auto& KV : Obj->Values)
		{
			if (!KV.Value.IsValid() || KV.Value->Type != EJson::Array) { continue; }
			const TArray<TSharedPtr<FJsonValue>>& A = KV.Value->AsArray();
			if (A.Num() >= 3)
			{
				const float Yaw = A.Num() > 3 ? float(A[3]->AsNumber()) : 0.f;
				const float Pitch = A.Num() > 4 ? float(A[4]->AsNumber()) : 0.f;
				Places.Add(FName(*KV.Key), FTransform(FRotator(Pitch, Yaw, 0.f), FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber())));
				++N;
			}
		}
		return N;
	};
	Places.Reset();
	const TSharedPtr<FJsonObject>* P;
	if (Root.IsValid() && Root->TryGetObjectField(TEXT("places"), P)) { Read(*P); }
	// "places_file": the world builder's own file wins over the copy embedded in the story, which went stale each time the
	// builder moved a place (the tent was being rebuilt, 2026-11-19 audit)
	const FString File = JStr(Root, TEXT("places_file"));
	if (!File.IsEmpty())
	{
		FString Raw;
		TSharedPtr<FJsonObject> PF;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Story") / File;
		if (FFileHelper::LoadFileToString(Raw, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), PF) && PF.IsValid())
		{
			const TSharedPtr<FJsonObject>* Inner;
			const int32 N = Read(PF->TryGetObjectField(TEXT("places"), Inner) ? *Inner : PF);
			UE_LOG(LogTemp, Display, TEXT("Story: %d places from %s"), N, *File);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Story: places_file %s not readable - using the places in the story file"), *Path);
		}
	}
	return Places.Num() > 0;
}

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
	LoadPlaces(Root);
	bPreloadCast = JBool(Root, TEXT("preload_cast"), true);
	Beats.Reset();
	const TArray<TSharedPtr<FJsonValue>>* B;
	if (Root->TryGetArrayField(TEXT("beats"), B))
	{
		for (const TSharedPtr<FJsonValue>& V : *B)
		{
			const TSharedPtr<FJsonObject> O = V.IsValid() && V->Type == EJson::Object ? V->AsObject() : nullptr;
			if (!O.IsValid()) { continue; }
			FBeat Beat;
			Beat.Id = JStr(O, TEXT("id"));
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
	LastInputAt = 0.0;
	bRunning = true;
	// a new chapter, a fresh nudge budget (spec 6.2: 5 hard per chapter, enforced in the rig)
	if (UYggViewRigComponent* R = Rig()) { R->ResetChapterBudget(); }
	// every look of the cast loads in the background now, so a person's first appearance finds it in memory (DressAs
	// froze the game 2.7 s at the team's spawn, spec 6.16)
	if (bPreloadCast) { Preload({ TEXT("cast") }); }
	// a grab never swaps to the climbing arm while the chapter plays (ClimbingComponent's switch, when it has one)
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.Climb.Camera")))
	{
		ClimbCameraBefore = V->GetString();
		V->Set(TEXT("0"), ECVF_SetByCode);
		bClimbCameraSet = true;
	}
	return true;
}

void UYggStorySubsystem::Stop()
{
	const bool bWasRunning = bRunning;
	bRunning = false;
	bTimeHold = false;
	Runs.Reset();
	Tweens.Reset();
	EndHand();
	ClearPendingTeleport();   // a climber held in the air for the ground must not stay flying once the story is gone
	ReleasePosedPin();        // nor pinned with his movement off (the chapter ends sitting in the night tent)
	// the chapter end, Stop, the end of the world: every story hold goes and the view is plain walking again (spec 4.4)
	if (bWasRunning || StoryLeases.Num() > 0) { ResetCamera(0.4f); }
	UWorld* World = GetWorld();
	const bool bTearDown = !World || World->bIsTearingDown;
	for (auto& KV : People) { if (AYggStoryNPC* N = KV.Value.Get()) { N->Destroy(); } }
	People.Reset();
	// the chapter's loops fade out instead of cutting off (spawned sounds destroy themselves when they finish)
	for (UAudioComponent* A : Sounds) { if (A) { if (bTearDown) { A->Stop(); } else { A->FadeOut(1.5f, 0.f); } } }
	Sounds.Reset();
	if (Headlamp) { Headlamp->DestroyComponent(); Headlamp = nullptr; }
	for (AActor* A : SpawnedLights) { if (A) { A->Destroy(); } }
	SpawnedLights.Reset();
	// (2026-11-20) a light the story faded to 0 / hidden goes back to how the level had it, or the next run reads 0 as its base
	for (const TPair<TWeakObjectPtr<ULightComponent>, TPair<float, bool>>& KV : LightOrig)
	{
		if (ULightComponent* L = KV.Key.Get()) { L->SetIntensity(KV.Value.Key); L->SetVisibility(KV.Value.Value); }
	}
	LightOrig.Reset();
	for (int32 i = 0; i < 4; ++i) { LightComps[i].Reset(); LightName[i] = NAME_None; LightK[i] = 0.f; }
	if (WornKhata) { WornKhata->DestroyComponent(); WornKhata = nullptr; }
	PreloadHandle.Reset();
	if (bClimbCameraSet)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.Climb.Camera"))) { V->Set(*ClimbCameraBefore, ECVF_SetByCode); }
		bClimbCameraSet = false;
	}
	// a chapter that ends in the dark (the sleep blink, the title) keeps its black a moment and fades it out, so the world
	// comes back gently with the people already gone; anything else drops the overlay at once
	const bool bFadeOut = bWasRunning && !bTearDown && Overlay && (Overlay->Black > 0.01f || CurrentLid() > 0.5f);
	if (Overlay && bFadeOut)
	{
		Overlay->Black = FMath::Max(Overlay->Black, CurrentLid());
		Overlay->Lid = 0.f;
		for (float* V : { &Overlay->TitleA, &Overlay->LineA, &Overlay->HintA, &Overlay->ChoiceA, &Overlay->PageA, &Overlay->CaptionA, &Overlay->PressA, &Overlay->InspectA })
		{
			*V = 0.f;
		}
		Overlay->HoldProgress = -1.f;
		bEndFade = true;
	}
	else
	{
		if (Overlay) { Overlay->RemoveFromParent(); Overlay = nullptr; }
		bEndFade = false;
	}
	bChoiceOpen = false;
	IdleHint.Reset();
	Durings.Reset();
	PromptBeat = INDEX_NONE;
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
		FBeat& B = Beats[i];
		B.bFired = i <= Index;
		B.bDone = i < Index;
		B.DoneAt = Clock;
		B.CondSince = -1.0;
		B.LookHeld = B.SeenHeld = 0.f;
		B.bCompanion = B.bLure = B.bNudge = B.bPromptShown = B.bPromptHint = false;
	}
	FRun Run;
	Run.Beat = Index;
	Runs.Add(Run);
	// the jump drops any open inspect, hold, hand-hold, teleport check and posture from the beats it cut off: the beat
	// that would have released them is already marked done, and the target beat asks for its own
	EndHand();
	ClearPendingTeleport();
	ResetCamera(0.4f);
	if (Overlay)
	{
		for (float* V : { &Overlay->Black, &Overlay->PageA, &Overlay->ChoiceA, &Overlay->InspectA, &Overlay->CaptionA, &Overlay->PressA, &Overlay->HintA })
		{
			CancelTween(*V);
			*V = 0.f;
		}
		Overlay->bInspectCard = false;
		Overlay->HoldProgress = -1.f;
		Overlay->Lid = 0.f;
	}
	bChoiceOpen = false;
	IdleHint.Reset();
	PromptBeat = INDEX_NONE;
	UE_LOG(LogTemp, Display, TEXT("Story: jump to %s"), *BeatId);
}

void UYggStorySubsystem::Choose(int32 Option)
{
	ChoiceResult = Option - 1;
	bCardClose = true;
}

void UYggStorySubsystem::ReleaseAll()
{
	// death / continue: the expedition releases the rig itself (ReleaseAll(0) + HardReset); this drops the story's side
	StoryLeases.Reset();
	Durings.Reset();
	bInDuring = false;
	bLidsShutRequested = false;
	EndHand();
	ClearPendingTeleport();
	// (2026-11-20) forget a posed pin WITHOUT touching the movement: death switched it off and must keep it off (the next
	// UpdatePosedPin would otherwise hand a dead climber MOVE_Walking); Respawn sets walking itself
	bPinned = false;
	bPinSpot = false;
	ReleaseCompass();
	LocalLidTime = -1.f;
	LocalLid = 0.f;
	IdleHint.Reset();
	if (Overlay)
	{
		CancelTween(Overlay->Black);
		Overlay->Black = 0.f;
		Overlay->Lid = 0.f;
	}
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

void UYggStorySubsystem::CancelTween(float& Value)
{
	Tweens.RemoveAll([&](const FTween& T) { return T.Value == &Value; });
}

bool UYggStorySubsystem::IsTweening(const float& Value) const
{
	return Tweens.ContainsByPredicate([&](const FTween& T) { return T.Value == &Value; });
}

bool UYggStorySubsystem::BeatHas(int32 Beat, const TCHAR* Key) const
{
	if (!Beats.IsValidIndex(Beat)) { return false; }
	for (const TSharedPtr<FJsonValue>& V : Beats[Beat].Do)
	{
		if (V.IsValid() && V->Type == EJson::Object && V->AsObject()->HasField(Key)) { return true; }
	}
	return false;
}

// ---- leases the story holds (spec 4.4): tracked here so "hold": false, "pace": false, a teleport and the end of a beat
// can let go of the right ones without knowing which beat asked

void UYggStorySubsystem::TrackLease(int32 Owner, uint8 Kind, bool bKeep)
{
	for (FStoryLease& L : StoryLeases)
	{
		if (L.Owner == Owner && L.Kind == Kind) { L.bKeep |= bKeep; return; }
	}
	FStoryLease L;
	L.Owner = Owner;
	L.Kind = Kind;
	L.bKeep = bKeep;
	StoryLeases.Add(L);
}

void UYggStorySubsystem::ReleaseTracked(uint8 Kind, int32 ExceptOwner)
{
	UYggViewRigComponent* R = Rig();
	for (int32 i = StoryLeases.Num() - 1; i >= 0; --i)
	{
		const FStoryLease& L = StoryLeases[i];
		if (L.Kind != Kind || L.Owner == ExceptOwner) { continue; }
		if (R) { R->ReleaseLease(L.Owner, static_cast<EYggLease>(L.Kind)); }
		StoryLeases.RemoveAt(i);
	}
}

void UYggStorySubsystem::KeepTimed(int32 Owner, uint8 Kind, float Seconds)
{
	// re-taking a lease the owner already holds refreshes it and makes it keep; Seconds caps it (and the rig's own cap)
	if (UYggViewRigComponent* R = Rig()) { R->AcquireLease(Owner, static_cast<EYggLease>(Kind), FMath::Max(0.1f, Seconds), true); }
	TrackLease(Owner, Kind, true);
}

bool UYggStorySubsystem::StoryHolds(uint8 Kind) const
{
	return StoryLeases.ContainsByPredicate([Kind](const FStoryLease& L) { return L.Kind == Kind; });
}

void UYggStorySubsystem::ResetCamera(float Blend)
{
	StoryLeases.Reset();
	Durings.Reset();
	bInDuring = false;
	bLidsShutRequested = false;
	LocalLidTime = -1.f;
	LocalLid = 0.f;
	ReleaseCompass();
	UYggViewRigComponent* R = Rig();
	APlayerController* C = PC();
	if (!R) { return; }
	R->ReleaseAll(Blend);
	const UWorld* W = GetWorld();
	if (C && W && !W->bIsTearingDown)
	{
		const FRotator V = C->GetControlRotation();
		R->HardReset(FRotator(FMath::Clamp(FRotator::NormalizeAxis(V.Pitch), -60.f, 60.f), V.Yaw, 0.f), EYggView::Walk);
	}
}

void UYggStorySubsystem::ClearPendingTeleport()
{
	if (bHeldForGround)
	{
		ACharacter* Ch = Cast<ACharacter>(Player());
		UCharacterMovementComponent* Move = Ch ? Ch->GetCharacterMovement() : nullptr;
		if (Move && Move->MovementMode == MOVE_Flying) { Move->SetMovementMode(MOVE_Falling); }
	}
	bHeldForGround = false;
	HeldForGroundTime = 0.f;
	bPendingReached = false;
	PendingTeleportTime = 0.f;
}

void UYggStorySubsystem::UpdatePosedPin()
{
	ACharacter* Ch = Cast<ACharacter>(Player());
	UCharacterMovementComponent* Move = Ch ? Ch->GetCharacterMovement() : nullptr;
	const UYggViewRigComponent* R = RigCache.Get();
	if (!Move || !R || !bPinSpot) { return; }
	const EYggView V = R->GetView();
	const bool bPosed = V == EYggView::Lie || V == EYggView::Sit || V == EYggView::Kneel1 || V == EYggView::Kneel2;
	if (bPosed && CVarStoryPinPosed.GetValueOnGameThread() != 0)
	{
		const float Off = float(FVector::Dist(Ch->GetActorLocation(), PinSpot));
		if (Off > 15.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Story: posed climber %.0f cm off the cut spot (now %s) - put back and held"), Off, *Ch->GetActorLocation().ToCompactString());
			Ch->TeleportTo(PinSpot, Ch->GetActorRotation(), false, true);
		}
		// every frame, not once: a teleport can switch a held climber to falling (UCharacterMovementComponent::OnTeleported)
		if (Move->MovementMode != MOVE_None)
		{
			Move->StopMovementImmediately();
			Move->DisableMovement();
		}
		bPinned = true;
		return;
	}
	// walking or crawling: he moves himself again. The cut's own frame is a standing one (the rig's hard reset) before the
	// posture that follows it, so the spot is kept for the first second after the cut
	if (bPinned)
	{
		if (Move->MovementMode == MOVE_None) { Move->SetMovementMode(MOVE_Walking); }   // only undo our own hold (2026-11-20)
		bPinned = false;
	}
	if (Clock - PinCutClock > 1.0) { bPinSpot = false; }
}

void UYggStorySubsystem::ReleasePosedPin()
{
	if (bPinned)
	{
		ACharacter* Ch = Cast<ACharacter>(Player());
		UCharacterMovementComponent* Move = Ch ? Ch->GetCharacterMovement() : nullptr;
		if (Move && Move->MovementMode == MOVE_None) { Move->SetMovementMode(MOVE_Walking); }
	}
	bPinned = false;
	bPinSpot = false;
}

void UYggStorySubsystem::LogCutOverlaps(const FString& PlaceName, const FVector& Where) const
{
	ACharacter* Ch = Cast<ACharacter>(Player());
	UCapsuleComponent* Cap = Ch ? Ch->GetCapsuleComponent() : nullptr;
	UWorld* W = GetWorld();
	if (!Cap || !W) { return; }
	TArray<FOverlapResult> Hits;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryCutOverlap), false, Ch);
	FCollisionResponseParams RP;
	Cap->InitSweepCollisionParams(Q, RP);
	W->OverlapMultiByChannel(Hits, Where, Cap->GetComponentQuat(), Cap->GetCollisionObjectType(), Cap->GetCollisionShape(), Q, RP);
	for (const FOverlapResult& O : Hits)
	{
		if (!O.bBlockingHit) { continue; }
		const AActor* A = O.GetActor();
		const UPrimitiveComponent* C = O.GetComponent();
		FString Tags;
		if (A) { for (const FName& T : A->Tags) { Tags += T.ToString() + TEXT(" "); } }
		UE_LOG(LogTemp, Warning, TEXT("Story: cut %s: the climber's capsule (r %.0f, half %.0f cm) overlaps %s / %s [%s] - the physics will push him off the spot"),
			*PlaceName, Cap->GetScaledCapsuleRadius(), Cap->GetScaledCapsuleHalfHeight(), A ? *A->GetName() : TEXT("?"), C ? *C->GetName() : TEXT("?"),
			*Tags.TrimEnd());
	}
}

void UYggStorySubsystem::UpdatePendingTeleport(float Dt)
{
	ACharacter* Ch = Cast<ACharacter>(Player());
	if (!Ch || PendingTeleportTime <= 0.f) { return; }
	UCharacterMovementComponent* Move = Ch->GetCharacterMovement();
	const FVector Here = Ch->GetActorLocation();
	const float Away = float(FVector::Dist2D(Here, PendingTeleport));
	const float Jump = float(FVector::Dist2D(Here, PendingTeleportLast));
	if (Away <= 300.f)
	{
		bPendingReached = true;
	}
	else if (!bPendingReached || StoryHolds(static_cast<uint8>(EYggLease::Move)) || Jump > 250.f)
	{
		// put back only what the player did not do himself: the jump was refused (he never got there), something else
		// moved him (metres in one frame), or he moved while a story Move lease holds him still. Walking off after a
		// free teleport is his own choice and stays (the 3 m yank-back caught players walking to Dolma, audit / spec 6.7).
		UE_LOG(LogTemp, Warning, TEXT("Story: climber moved away from the teleport spot (now %s) - putting him back"), *Here.ToCompactString());
		Ch->TeleportTo(PendingTeleport, FRotator(0.f, PendingTeleportYaw, 0.f), false, true);
		Move->StopMovementImmediately();
	}
	PendingTeleportLast = Ch->GetActorLocation();
	// ground under the spot? (after a long jump the tiles there may still be loading or cooking their collision)
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryTeleportGround), false, Ch);
	const FVector From = Ch->GetActorLocation();
	const bool bGround = GetWorld()->LineTraceSingleByChannel(Hit, From, From - FVector(0.f, 0.f, 600.f), ECC_Visibility, Q);
	if (!bGround)
	{
		if (!bHeldForGround) { UE_LOG(LogTemp, Display, TEXT("Story: no ground under the teleport spot yet - holding the climber")); }
		bHeldForGround = true;
		// (2026-11-20) the hold has a way out: after 30 s (collision streaming on a laptop can be slow) put him on ground found
		// far below and let go; with nothing below at all, keep holding (letting go there would drop him into the void)
		HeldForGroundTime += Dt;
		if (HeldForGroundTime > 30.f)
		{
			FHitResult Far;
			if (GetWorld()->LineTraceSingleByChannel(Far, From, From - FVector(0.f, 0.f, 10000.f), ECC_Visibility, Q))
			{
				Ch->TeleportTo(Far.ImpactPoint + FVector(0.f, 0.f, Ch->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), Ch->GetActorRotation(), false, true);
				if (bPinSpot) { PinSpot = Ch->GetActorLocation(); }   // a posed pin holds him here, not back up in the air
				UE_LOG(LogTemp, Warning, TEXT("Story: no ground under the teleport spot after 30 s - put on the ground %.0f cm below and released"),
					From.Z - Far.ImpactPoint.Z);
				ClearPendingTeleport();
				return;
			}
			UE_LOG(LogTemp, Warning, TEXT("Story: still no ground under the teleport spot after 30 s (nothing within 100 m) - holding on"));
			HeldForGroundTime = 0.f;
		}
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Flying);
		PendingTeleportTime = FMath::Max(PendingTeleportTime, 0.5f);   // keep holding until the ground is there
		return;
	}
	HeldForGroundTime = 0.f;
	if (bHeldForGround)
	{
		bHeldForGround = false;
		Move->SetMovementMode(MOVE_Walking);
		UE_LOG(LogTemp, Display, TEXT("Story: ground loaded under the teleport spot - released"));
	}
	PendingTeleportTime -= Dt;
}

FString UYggStorySubsystem::Expand(const FString& Text) const
{
	FString Out = Text.Replace(TEXT("{day}"), *FString::FromInt(Day));
	// (#77) the keys a chapter writes as the defaults show the player's own: [W] the key that walks forward (Enhanced
	// Input), [E] read / use, [Q] the compass (the rebindable ones in the settings)
	const ESearchCase::Type Cs = ESearchCase::CaseSensitive;
	if (Out.Contains(TEXT("[W]"), Cs)) { Out.ReplaceInline(TEXT("[W]"), *FString::Printf(TEXT("[%s]"), *YggSettings::KeyName(StoryForwardKey(PC()))), Cs); }
	if (Out.Contains(TEXT("[E]"), Cs)) { Out.ReplaceInline(TEXT("[E]"), *YggSettings::KeyLabel(TEXT("Interact"), EKeys::E), Cs); }
	if (Out.Contains(TEXT("[Q]"), Cs)) { Out.ReplaceInline(TEXT("[Q]"), *YggSettings::KeyLabel(TEXT("Compass"), EKeys::Q), Cs); }
	return Out;
}

float UYggStorySubsystem::StoryDt(float DeltaTime) const
{
	// the slip slows the world to 0.35 for 0.7 s; waits, tweens, eyelids and lines keep real time (spec 6.8)
	const UWorld* W = GetWorld();
	const AWorldSettings* WS = W ? W->GetWorldSettings() : nullptr;
	if (!WS || FMath::IsNearlyEqual(WS->GetEffectiveTimeDilation(), 1.f, 0.001f)) { return DeltaTime; }
	return FMath::Clamp(W->DeltaRealTimeSeconds, 0.f, 0.1f);
}

void UYggStorySubsystem::RecordTrail()
{
	APawn* P = Player();
	if (!P) { return; }
	const FVector L = P->GetActorLocation();
	const float D = Crumbs.Num() ? float(FVector::Dist2D(L, Crumbs.Last())) : 0.f;
	if (Crumbs.Num() == 0 || D > 1500.f)
	{
		// first point, or a teleport / cut (15 m in one frame): the trail starts over here
		Crumbs.Reset();
		CrumbLen.Reset();
		Crumbs.Add(L);
		CrumbLen.Add(0.f);
		++TrailEpoch;
		return;
	}
	if (D >= 60.f)
	{
		CrumbLen.Add(CrumbLen.Last() + D);
		Crumbs.Add(L);
		if (Crumbs.Num() > 700)     // ~420 m kept; the oldest 200 points go
		{
			Crumbs.RemoveAt(0, 200);
			CrumbLen.RemoveAt(0, 200);
			TrailFirst += 200;
		}
	}
}

void UYggStorySubsystem::Tick(float DeltaTime)
{
	const float Dt = StoryDt(DeltaTime);
	TickTools(Dt);
	if (bRunning) { RecordTrail(); }
	if (bEndFade && !bRunning)
	{
		// after the chapter: its last black fades out over 2 s, then the overlay goes
		if (Overlay) { Overlay->Black = FMath::Max(0.f, Overlay->Black - Dt * 0.5f); }
		if (!Overlay || Overlay->Black <= 0.f)
		{
			if (Overlay) { Overlay->RemoveFromParent(); Overlay = nullptr; }
			bEndFade = false;
		}
	}
	if (!bRunning || !Ready())
	{
		return;
	}
	EnsureOverlay();
	if (!Overlay)
	{
		return;
	}
	Rig();
	Clock += Dt;
	TickLocalLids(Dt);
	RunDurings();
	if (!bRunning) { return; }
	TickTriggers(Dt);
	TickRuns(Dt);
	if (!bRunning) { return; }
	TickTweens(Dt);
	TickHand(Dt);
	TickHints(Dt);
	TickPoseHint(Dt);
	TickLights(Dt);
	TickInspect(Dt);
	UpdatePendingTeleport(Dt);
	UpdatePosedPin();
	// eyelids: the rig's value (it ticks before the story, so this is this frame's), drawn by the overlay
	Overlay->Lid = CurrentLid();
	// lids the story shut: the black veil takes over the dark, so a page or a title can outlast the rig's 8 s lid lease
	if (bLidsShutRequested && Overlay->Lid >= 0.99f && Overlay->Black < 1.f && !IsTweening(Overlay->Black))
	{
		Overlay->Black = 1.f;
	}
	// while the veil holds the dark, the story's lid leases are kept alive (else the rig opens the lids under the black and logs a repair)
	if (bLidsShutRequested && Overlay->Black >= 0.99f)
	{
		if (UYggViewRigComponent* R = RigCache.Get())
		{
			for (const FStoryLease& L : StoryLeases)
			{
				if (L.Kind == static_cast<uint8>(EYggLease::Lids)) { R->AcquireLease(L.Owner, EYggLease::Lids, 8.f, true); }
			}
		}
	}
	if (bTimeHold)
	{
		if (!IsTweening(TimeValue)) { TimeValue += TimeRate * Dt * 100.f / 60.f; }
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

// ------------------------------------------------------------------------------------------------ triggers

bool UYggStorySubsystem::BasicConditions(const TSharedPtr<FJsonObject>& W, double ParentDone, const FString& BeatId) const
{
	if (!W.IsValid()) { return true; }
	if (JBool(W, TEXT("never"), false)) { return false; }
	double LastDone = ParentDone;
	const TArray<FString> Afters = JStrs(W, TEXT("after"));
	if (Afters.Num() > 0)
	{
		LastDone = 0.0;
		for (const FString& After : Afters)
		{
			const FBeat* Other = Beats.FindByPredicate([&](const FBeat& B) { return B.Id == After; });
			if (!Other || !Other->bDone) { return false; }
			LastDone = FMath::Max(LastDone, Other->DoneAt);
		}
	}
	if (Clock - LastDone < JNum(W, TEXT("delay"), 0.f)) { return false; }
	for (const FString& F : JStrs(W, TEXT("flag"))) { if (!Flags.Contains(FName(*F))) { return false; } }
	for (const FString& F : JStrs(W, TEXT("noflag"))) { if (Flags.Contains(FName(*F))) { return false; } }
	const APawn* P = Player();
	if (!P) { return false; }
	FVector At;
	const FString Near = JStr(W, TEXT("near"));
	if (!Near.IsEmpty() && (!Where(Near, At) || FVector::Dist(P->GetActorLocation(), At) > JNum(W, TEXT("r"), 600.f))) { return false; }
	const FString Far = JStr(W, TEXT("far"));
	if (!Far.IsEmpty() && Where(Far, At) && FVector::Dist(P->GetActorLocation(), At) < JNum(W, TEXT("r"), 600.f)) { return false; }
	// the body's posture (without a view rig there are no postures: the condition holds, so a chapter never stalls)
	const FString Pose = JStr(W, TEXT("posture"));
	if (!Pose.IsEmpty())
	{
		EYggView Want;
		const UYggViewRigComponent* R = RigCache.Get();
		if (R && StoryPose(Pose, Want) && R->GetView() != Want) { return false; }
	}
	// any: true when one of the sub-conditions holds (a sub-condition without "after" counts its delay from this one's)
	const TArray<TSharedPtr<FJsonValue>>* Any;
	if (W->TryGetArrayField(TEXT("any"), Any) && Any->Num() > 0)
	{
		bool bAny = false;
		for (const TSharedPtr<FJsonValue>& V : *Any)
		{
			if (V.IsValid() && V->Type == EJson::Object && BasicConditions(V->AsObject(), LastDone, BeatId)) { bAny = true; break; }
		}
		if (!bAny) { return false; }
	}
	return true;
}

void UYggStorySubsystem::ResetLook(FBeat& Beat, int32 Index)
{
	if (Beat.bPromptShown && PromptBeat == Index && Overlay) { Tween(Overlay->PressA, 0.f, 0.3f); PromptBeat = INDEX_NONE; }
	if (Beat.bPromptHint && Overlay) { Tween(Overlay->HintA, 0.f, 0.5f); }
	Beat.CondSince = -1.0;
	Beat.LookHeld = Beat.SeenHeld = 0.f;
	Beat.bCompanion = Beat.bLure = Beat.bNudge = Beat.bPromptShown = Beat.bPromptHint = false;
}

bool UYggStorySubsystem::Conditions(FBeat& Beat, int32 Index, float Dt, bool& bLookPart)
{
	bLookPart = false;
	const TSharedPtr<FJsonObject>& W = Beat.When;
	if (!W.IsValid()) { return true; }
	if (!BasicConditions(W, 0.0, Beat.Id)) { return false; }
	const FString Look = JStr(W, TEXT("look"));
	const FString Press = JStr(W, TEXT("press"));
	if (Look.IsEmpty() && Press.IsEmpty()) { return true; }
	// the look trigger with escalation (spec 6.1): T0 = now every other condition holds
	bLookPart = true;
	if (Beat.CondSince < 0.0)
	{
		Beat.CondSince = Clock;
		Beat.LookHeld = Beat.SeenHeld = 0.f;
		Beat.bCompanion = Beat.bLure = Beat.bNudge = Beat.bPromptShown = Beat.bPromptHint = false;
	}
	const float Since = float(Clock - Beat.CondSince);
	const bool bAuto = CVarStoryAuto.GetValueOnGameThread() != 0;
	APlayerController* C = PC();
	APawn* P = Player();
	FVector At = FVector::ZeroVector;
	const bool bTarget = !Look.IsEmpty() && Where(Look, At);
	if (!Look.IsEmpty() && !bTarget && Press.IsEmpty()) { return true; }   // nothing to look at: the old rule, it fires
	At.Z += JNum(W, TEXT("up"), 0.f);
	// looking: inside the cone for "dwell" seconds, or (seen) inside the inner 60% of the frame for 1 s - for things
	// below the eye line that a strict cone misses
	bool bInView = Look.IsEmpty();
	bool bLooked = false;
	if (bTarget && C && C->PlayerCameraManager)
	{
		const FVector Eye = C->PlayerCameraManager->GetCameraLocation();
		const FVector View = C->PlayerCameraManager->GetCameraRotation().Vector();
		const float Cone = FMath::Cos(FMath::DegreesToRadians(JNum(W, TEXT("cone"), 14.f)));
		const bool bCone = FVector::DotProduct(View, (At - Eye).GetSafeNormal()) > Cone;
		Beat.LookHeld = bCone ? Beat.LookHeld + Dt : 0.f;
		bLooked = Beat.LookHeld >= JNum(W, TEXT("dwell"), 0.35f);
		bInView = bCone;
		if (JBool(W, TEXT("seen"), false))
		{
			int32 VX = 0, VY = 0;
			C->GetViewportSize(VX, VY);
			FVector2D Scr;
			const bool bIn = VX > 0 && VY > 0 && C->ProjectWorldLocationToScreen(At, Scr, true)
				&& FMath::Abs(Scr.X - VX * 0.5) <= VX * 0.3 && FMath::Abs(Scr.Y - VY * 0.5) <= VY * 0.3;
			Beat.SeenHeld = bIn ? Beat.SeenHeld + Dt : 0.f;
			bLooked |= Beat.SeenHeld >= 1.f;
			bInView |= bIn;
		}
	}
	bool bFire = false;
	if (!Press.IsEmpty())
	{
		// press: the look (and near) only shows "[E] Open"; the beat fires on the key. After 20 s the prompt grows to a hint
		if (bInView && !Beat.bPromptShown)
		{
			Overlay->PressPrompt = Expand(JStr(W, TEXT("prompt"), TEXT("[E]")));
			Tween(Overlay->PressA, 1.f, 0.25f);
			Beat.bPromptShown = true;
			PromptBeat = Index;
		}
		else if (!bInView && Beat.bPromptShown)
		{
			if (PromptBeat == Index) { Tween(Overlay->PressA, 0.f, 0.3f); PromptBeat = INDEX_NONE; }
			Beat.bPromptShown = false;
		}
		if (Since > 20.f && !Beat.bPromptHint)
		{
			Beat.bPromptHint = true;
			CancelTween(Overlay->HintA);
			Overlay->Hint = Expand(JStr(W, TEXT("prompt"), TEXT("[E]")));
			Tween(Overlay->HintA, 1.f, 1.f);
		}
		if ((bInView || Beat.bPromptHint) && C && C->WasInputKeyJustPressed(StoryKey(Press))) { bFire = true; }
		if (bAuto && Since > 3.f) { bFire = true; }
	}
	else
	{
		bFire = bLooked;
	}
	if (!bFire && bTarget)
	{
		// T0 + 3 s: a companion turns his head to it
		const FString Companion = JStr(W, TEXT("companion"));
		if (!Companion.IsEmpty() && !Beat.bCompanion && Since >= 3.f)
		{
			Beat.bCompanion = true;
			if (AYggStoryNPC* N = Person(Companion)) { N->FaceTo(At, nullptr, 12.f); }
		}
		// T0 + lure_at (7; the old "timeout" maps here, at most 8): a sound at the thing, from within the view
		float LureAt = JNum(W, TEXT("lure_at"), -1.f);
		if (LureAt < 0.f) { LureAt = W->HasField(TEXT("timeout")) ? FMath::Min(JNum(W, TEXT("timeout"), 7.f), 8.f) : 7.f; }
		TSharedPtr<FJsonObject> Lure;
		const TSharedPtr<FJsonObject>* LureObj;
		if (W->TryGetObjectField(TEXT("lure"), LureObj)) { Lure = *LureObj; }
		else if (W->HasField(TEXT("lure")) && !JStr(W, TEXT("lure")).IsEmpty()) { Lure = MakeShared<FJsonObject>(); Lure->SetStringField(TEXT("sound"), JStr(W, TEXT("lure"))); }
		if (!Beat.bLure && Since >= LureAt && Lure.IsValid())
		{
			Beat.bLure = true;
			PlayLure(Lure, At, Beat.Id);
		}
		// T0 + 12 s: the nudge, last resort, only when standing or slow, and only while the rig's budget allows (spec 6.2)
		const FString NudgeMode = JField(W, TEXT("nudge"));
		// only when the data asks for it ("nudge": true or {pitchmax}); absent means no nudge (spec 6.1 "only if set")
		if (!Beat.bNudge && Since >= 12.f && W->HasField(TEXT("nudge")) && NudgeMode != TEXT("false"))
		{
			Beat.bNudge = true;
			const TSharedPtr<FJsonObject>* NudgeObj;
			const float PitchMax = W->TryGetObjectField(TEXT("nudge"), NudgeObj) ? JNum(*NudgeObj, TEXT("pitchmax"), 8.f) : 8.f;
			UYggViewRigComponent* R = Rig();
			const bool bSlow = P && P->GetVelocity().Size2D() < 150.f;
			// the rig owns the budget, the speed rule and Story camera "Gentle" (settings or ygg.View.Gentle): it says no, we lure
			if (R && bSlow && R->Nudge(Index, At, 45.f, 20.f, PitchMax))
			{
				TrackLease(Index, static_cast<uint8>(EYggLease::Nudge), false);
				UE_LOG(LogTemp, Display, TEXT("Story: nudge toward %s (beat %s)"), *Look, *Beat.Id);
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("Story: nudge toward %s refused (%s) - lure instead (beat %s)"), *Look,
					!R ? TEXT("no view rig") : (!bSlow ? TEXT("walking") : TEXT("budget or Gentle")), *Beat.Id);
				if (Lure.IsValid()) { PlayLure(Lure, At, Beat.Id); }
			}
		}
		// T0 + fire (15 s): the beat goes on anyway. A press beat waits for its key unless it gives "fire" itself
		float FireAt = Press.IsEmpty() ? 15.f : -1.f;
		if (W->HasField(TEXT("fire"))) { FireAt = JField(W, TEXT("fire")) == TEXT("false") ? -1.f : JNum(W, TEXT("fire"), FireAt); }
		if (FireAt > 0.f && Since >= FireAt)
		{
			bFire = true;
			UE_LOG(LogTemp, Display, TEXT("Story: beat %s fires without the look (%.0f s)"), *Beat.Id, FireAt);
		}
	}
	if (bFire)
	{
		if (Beat.bPromptShown && PromptBeat == Index) { Tween(Overlay->PressA, 0.f, 0.25f); PromptBeat = INDEX_NONE; }
		if (Beat.bPromptHint) { Tween(Overlay->HintA, 0.f, 0.4f); }
	}
	return bFire;
}

void UYggStorySubsystem::TickTriggers(float Dt)
{
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		FBeat& B = Beats[i];
		if (B.bFired) { continue; }
		bool bLook = false;
		if (Conditions(B, i, Dt, bLook))
		{
			B.bFired = true;
			B.FiredAt = Clock;
			FRun Run;
			Run.Beat = i;
			Runs.Add(Run);
			UE_LOG(LogTemp, Display, TEXT("Story: beat %s"), *B.Id);
		}
		else if (!bLook && B.CondSince >= 0.0)
		{
			ResetLook(B, i);
		}
	}
}

// ------------------------------------------------------------------------------------------------ runs

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
			const TSharedPtr<FJsonValue>& V = Beats[Run.Beat].Do[Run.Step++];
			const TSharedPtr<FJsonObject> A = V.IsValid() && V->Type == EJson::Object ? V->AsObject() : nullptr;
			const float W = A.IsValid() ? DoAction(Run, A) : 0.f;
			if (!bRunning) { return; }
			if (W < 0.f) { Run.bBlocked = true; Run.BlockAction = A; Run.BlockedAt = Clock; }
			else { Run.Wait += W; }
		}
		if (!Run.bBlocked && Run.Wait <= 0.f && Beats.IsValidIndex(Run.Beat) && Run.Step >= Beats[Run.Beat].Do.Num())
		{
			const int32 Beat = Run.Beat;
			Runs.RemoveAt(r--);
			EndRun(Beat);
		}
	}
}

void UYggStorySubsystem::EndRun(int32 Beat)
{
	if (Beats.IsValidIndex(Beat))
	{
		Beats[Beat].bDone = true;
		Beats[Beat].DoneAt = Clock;
	}
	// every lease of the beat goes with it, except those it asked to keep (spec 4.4)
	if (UYggViewRigComponent* R = Rig())
	{
		R->ReleaseOwner(Beat);
		if (CVarStoryDebug.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Story: beat %s done - rig %s"), Beats.IsValidIndex(Beat) ? *Beats[Beat].Id : TEXT("?"), *R->Audit());
		}
	}
	StoryLeases.RemoveAll([Beat](const FStoryLease& L) { return L.Owner == Beat && !L.bKeep; });
}

bool UYggStorySubsystem::LegacyAction(FRun& Run, const TSharedPtr<FJsonObject>& A, float& OutWait)
{
	OutWait = 0.f;
	const FString BeatId = Beats.IsValidIndex(Run.Beat) ? Beats[Run.Beat].Id : TEXT("?");
	auto Removed = [&](const TCHAR* Key) { UE_LOG(LogTemp, Error, TEXT("Story: removed key %s in beat %s"), Key, *BeatId); };
	// the remapped action keeps the old one's wait (and conditions, already met)
	auto Keep = [&](const TSharedPtr<FJsonObject>& O)
	{
		if (A->HasField(TEXT("wait")) && !O->HasField(TEXT("wait"))) { O->SetField(TEXT("wait"), A->TryGetField(TEXT("wait"))); }
		return O;
	};
	// First the old actions that carry another action's name as an option: {"lock": true, "look": false} and
	// {"card": .., "title": ..} were caught by the "look" and "title" branches once (2026-11-19 play-test)
	if (A->HasField(TEXT("lock")))
	{
		Removed(TEXT("lock"));
		const bool bLook = JBool(A, TEXT("look"), false);
		TSharedPtr<FJsonObject> H = MakeShared<FJsonObject>();
		H->SetBoolField(TEXT("hold"), JBool(A, TEXT("lock"), true));
		DoAction(Run, H);
		if (bLook && (BeatHas(Run.Beat, TEXT("fall")) || BeatHas(Run.Beat, TEXT("flyout")) || BeatHas(Run.Beat, TEXT("tumble"))))
		{
			TSharedPtr<FJsonObject> L = MakeShared<FJsonObject>();
			L->SetBoolField(TEXT("lookhold"), true);
			DoAction(Run, L);
		}
		else if (!bLook)
		{
			ReleaseTracked(static_cast<uint8>(EYggLease::Look));
		}
		return true;
	}
	if (A->HasField(TEXT("card")))
	{
		Removed(TEXT("card"));
		const FString Words = Expand(JStr(A, TEXT("card"))).Replace(TEXT("|"), TEXT("\n"));
		const FString Title = JStr(A, TEXT("title"));
		TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
		if (!JStr(A, TEXT("image")).IsEmpty())
		{
			O->SetStringField(TEXT("inspect"), Title.IsEmpty() ? TEXT("card") : Title);
			O->SetStringField(TEXT("front"), JStr(A, TEXT("image")));
			O->SetStringField(TEXT("text"), Words);
		}
		else
		{
			O->SetStringField(TEXT("caption"), Title.IsEmpty() ? Words : Title + TEXT("\n") + Words);
		}
		OutWait = DoAction(Run, O);
		return true;
	}
	if (A->HasField(TEXT("kneel")))
	{
		Removed(TEXT("kneel"));
		const float Down = JNum(A, TEXT("kneel"), 55.f);
		const TCHAR* Pose = Down >= 90.f ? TEXT("sit") : (Down >= 50.f ? TEXT("kneel2") : (Down >= 1.f ? TEXT("kneel1") : TEXT("stand")));
		TSharedPtr<FJsonObject> O = Keep(MakeShared<FJsonObject>());
		O->SetStringField(TEXT("pose"), Pose);
		if (A->HasField(TEXT("t"))) { O->SetNumberField(TEXT("t"), FMath::Max(0.f, JNum(A, TEXT("t"), 0.9f))); }
		OutWait = DoAction(Run, O);
		return true;
	}
	if (A->HasField(TEXT("tumble")))
	{
		Removed(TEXT("tumble"));
		const float Len = JNum(A, TEXT("tumble"), 1.6f);
		TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
		if (Len <= 0.5f) { O->SetStringField(TEXT("knock"), TEXT("left")); }
		else { O->SetNumberField(TEXT("fall"), Len); O->SetStringField(TEXT("side"), TEXT("left")); }
		OutWait = DoAction(Run, O);
		return true;
	}
	if (A->HasField(TEXT("view")))
	{
		Removed(TEXT("view"));      // the rig owns the view target (fly-out end, watchdog)
		return true;
	}
	if (A->HasField(TEXT("look")) && !A->HasField(TEXT("npc")) && !A->HasField(TEXT("npc_mode")) && !A->HasField(TEXT("flyout")))
	{
		// a gaze grab -> a lure: never a pull (spec 7.3)
		Removed(TEXT("look"));
		if (A->HasField(TEXT("sound")))
		{
			TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("lure"), JStr(A, TEXT("look")));
			O->SetStringField(TEXT("sound"), JStr(A, TEXT("sound")));
			O->SetNumberField(TEXT("vol"), JNum(A, TEXT("vol"), 0.9f));
			DoAction(Run, O);
		}
		return true;
	}
	return false;
}

float UYggStorySubsystem::DoAction(FRun& Run, const TSharedPtr<FJsonObject>& A)
{
	for (const FString& F : JStrs(A, TEXT("if"))) { if (!Flags.Contains(FName(*F))) { return 0.f; } }
	for (const FString& F : JStrs(A, TEXT("ifnot"))) { if (Flags.Contains(FName(*F))) { return 0.f; } }
	UWorld* World = GetWorld();
	APawn* P = Player();
	APlayerController* C = PC();
	UYggViewRigComponent* R = Rig();
	const int32 Owner = Run.Beat;
	const FString BeatId = Beats.IsValidIndex(Run.Beat) ? Beats[Run.Beat].Id : TEXT("?");
	const float T = JNum(A, TEXT("t"), -1.f);
	const bool bWait = JBool(A, TEXT("wait"), true);
	FVector At = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;
	auto NoRig = [&](const TCHAR* What)
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: %s in beat %s needs the view rig - skipped"), What, *BeatId);
	};
	// left / right, or the side of the view a place or person is on
	auto SideOf = [&](const FString& S) -> float
	{
		if (S.Equals(TEXT("left"), ESearchCase::IgnoreCase)) { return -1.f; }
		if (S.Equals(TEXT("right"), ESearchCase::IgnoreCase)) { return 1.f; }
		FVector W;
		if (C && C->PlayerCameraManager && Where(S, W))
		{
			const FVector F = C->PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal2D();
			const FVector D = (W - C->PlayerCameraManager->GetCameraLocation()).GetSafeNormal2D();
			return FVector::CrossProduct(F, D).Z >= 0.f ? 1.f : -1.f;
		}
		if (!S.IsEmpty()) { UE_LOG(LogTemp, Warning, TEXT("Story: side '%s' in beat %s is not left, right or a place - right"), *S, *BeatId); }
		return 1.f;
	};
	// a Move lease (spec 4.4): true -> until the beat ends; keep -> past it, until "hold": false; end -> to the chapter end
	auto HoldMove = [&](const FString& Mode)
	{
		if (!R) { NoRig(TEXT("hold")); return; }
		const bool bKeep = Mode == TEXT("keep") || Mode == TEXT("end");
		R->AcquireLease(Owner, EYggLease::Move, Mode == TEXT("end") ? 36000.f : (bKeep ? 600.f : 120.f), bKeep);   // keep: through a slow hold-[F] and the plume
		TrackLease(Owner, static_cast<uint8>(EYggLease::Move), bKeep);
	};

	float LegacyWait = 0.f;
	if (LegacyAction(Run, A, LegacyWait)) { return LegacyWait; }

	// First the actions that carry another action's name as an option: hand (hint, pace), pose (hold, face), blink (hold),
	// exposure (hold), flyout (look, fov, path), lure (sound), npc (look, at, path), inspect (text), say (npc). Checked after them, the
	// option's own branch would swallow the action (the dawn lock never held, 2026-11-19 play-test).
	if (A->HasField(TEXT("hand")))
	{
		HandNPC = Person(JStr(A, TEXT("hand")));
		if (HandNPC.IsValid() && Where(JStr(A, TEXT("to")), At))
		{
			HandNPC->Mode = AYggStoryNPC::EMode::Lead;
			HandNPC->Goal = At;
			HandNPC->Path.Reset();
			HandNPC->bMayWalk = false;
			HandOwner = Owner;
			HandPull = JNum(A, TEXT("pull"), 8.f);
			bHandHeld = false;
			bHandActive = true;
			CancelTween(Overlay->HintA);
			Overlay->Hint = Expand(JStr(A, TEXT("hint"), TEXT("Hold  [F]  to hold her hand")));
			// hand in hand you walk at her pace (spec 6.4): a Pace lease for the whole hold
			if (R)
			{
				R->SetPace(Owner, JNum(A, TEXT("pace"), 0.8f), JNum(A, TEXT("max"), 90.f) + 5.f, false);
				TrackLease(Owner, static_cast<uint8>(EYggLease::Pace), false);
			}
			Run.BlockKind = TEXT("hand");
			return -1.f;
		}
		HandNPC = nullptr;
		return 0.f;
	}
	if (A->HasField(TEXT("pose")))
	{
		EYggView V;
		if (!StoryPose(JStr(A, TEXT("pose")), V))
		{
			UE_LOG(LogTemp, Warning, TEXT("Story: unknown pose '%s' in beat %s"), *JStr(A, TEXT("pose")), *BeatId);
			return 0.f;
		}
		if (!R) { NoRig(TEXT("pose")); return 0.f; }
		FYggPoseReq Q = FYggPoseReq();
		Q.Pose = V;
		const float Def = V == EYggView::Walk ? StoryPoseUp(R->GetView()) : StoryPoseDown(V);
		Q.Seconds = A->HasField(TEXT("t")) ? FMath::Max(0.f, T) : Def;
		bool bSnap = JBool(A, TEXT("snap"), false);
		if (bSnap && !EyesShut())
		{
			// a snap is a cut: only behind closed eyelids (spec 4.2)
			UE_LOG(LogTemp, Warning, TEXT("Story: pose %s snap in beat %s with the eyes open - played as a %.1f s transition"), StoryPoseName(V), *BeatId, Def);
			bSnap = false;
			if (Q.Seconds <= 0.f) { Q.Seconds = Def; }
		}
		Q.bSnap = bSnap;
		if (P && Where(JStr(A, TEXT("face")), At)) { Q.FaceYaw = float((At - P->GetActorLocation()).GetSafeNormal2D().Rotation().Yaw); }
		Q.bAfterInput = JBool(A, TEXT("after_input"), false);
		Q.Auto = JNum(A, TEXT("auto"), 0.f);
		if (Where(JStr(A, TEXT("exit")), At)) { Q.Exit = At; }
		Q.ExitEv = JNum(A, TEXT("exit_ev"), 0.f);
		Q.Prop = FName(*JStr(A, TEXT("prop")));
		R->RequestPose(Owner, Q);
		const FString Hold = JField(A, TEXT("hold"));
		if (Hold != TEXT("false")) { HoldMove(Hold); }
		UE_LOG(LogTemp, Display, TEXT("Story: pose %s (%.1f s%s%s) in beat %s"), StoryPoseName(V), Q.Seconds, bSnap ? TEXT(", snap") : TEXT(""),
			Hold != TEXT("false") ? TEXT(", held") : TEXT(""), *BeatId);
		return (bWait && !bSnap && !Q.bAfterInput) ? Q.Seconds : 0.f;
	}
	if (A->HasField(TEXT("blink")))
	{
		const TSharedPtr<FJsonValue> V = A->TryGetField(TEXT("blink"));
		FString Mode = TEXT("blink");
		float Close = T;
		float Hold = JNum(A, TEXT("hold"), 0.f);
		if (V.IsValid() && V->Type == EJson::Number)
		{
			// the old {"blink": 0.7}, a black pulse of that length: a blink holding the middle of it
			Close = 0.15f;
			Hold = FMath::Max(0.f, float(V->AsNumber()) - 0.75f);
		}
		else if (V.IsValid() && V->Type == EJson::String)
		{
			Mode = V->AsString().ToLower();
		}
		TArray<TSharedPtr<FJsonValue>> During;
		const TArray<TSharedPtr<FJsonValue>>* D;
		if (A->TryGetArrayField(TEXT("during"), D)) { During = *D; }
		const float Len = StartLids(Owner, Mode, Close, Hold, During);
		if (!bRunning) { return 0.f; }
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("exposure")))
	{
		if (!R) { NoRig(TEXT("exposure")); return 0.f; }
		const float Rise = T < 0.f ? 1.5f : T;
		const float HoldSec = JNum(A, TEXT("hold"), 0.f);
		R->StoryExposure(Owner, JNum(A, TEXT("exposure"), 0.3f), Rise, HoldSec);
		KeepTimed(Owner, static_cast<uint8>(EYggLease::Exposure), Rise + HoldSec + 0.25f);
		return 0.f;
	}
	if (A->HasField(TEXT("flyout")))
	{
		// the one cinematic (spec 6.10): a path and a look target through places; the rig rebuilds the rotation from them
		// every frame with roll 0, so it can never loop or roll (the old Euler blend flipped the view, 2026-11-19)
		const TSharedPtr<FJsonValue> FV = A->TryGetField(TEXT("flyout"));
		const float Len = T > 0.f ? T : (FV.IsValid() && FV->Type == EJson::Number ? float(FV->AsNumber()) : 13.f);
		TArray<FVector> Path, LookAt;
		for (const FString& S : JStrs(A, TEXT("path")))
		{
			if (Where(S, At)) { Path.Add(At); }
			else { UE_LOG(LogTemp, Warning, TEXT("Story: flyout path place %s missing (beat %s)"), *S, *BeatId); }
		}
		for (const FString& S : JStrs(A, TEXT("look"))) { if (Where(S, At)) { LookAt.Add(At); } }
		const bool bOldKeys = A->HasField(TEXT("rise")) || A->HasField(TEXT("back")) || A->HasField(TEXT("face"));
		if (bOldKeys)
		{
			if (A->HasField(TEXT("rise"))) { UE_LOG(LogTemp, Error, TEXT("Story: removed key rise in beat %s"), *BeatId); }
			if (A->HasField(TEXT("back"))) { UE_LOG(LogTemp, Error, TEXT("Story: removed key back in beat %s"), *BeatId); }
			if (A->HasField(TEXT("face"))) { UE_LOG(LogTemp, Error, TEXT("Story: removed key face in beat %s"), *BeatId); }
		}
		if (Path.Num() == 0 && C && C->PlayerCameraManager)
		{
			// the old straight rise-and-back from the eye, as a path the rig can fly (and a look at "face" or level ahead)
			const FVector From = C->PlayerCameraManager->GetCameraLocation();
			const float Yaw = C->GetControlRotation().Yaw;
			const FVector Back = -FRotator(0.f, Yaw, 0.f).Vector();
			const FVector To = From + FVector(0.f, 0.f, JNum(A, TEXT("rise"), 2500.f)) + Back * JNum(A, TEXT("back"), 3000.f);
			Path = { From, FMath::Lerp(From, To, 0.35f) + FVector(0.f, 0.f, 200.f), To };
			if (LookAt.Num() == 0)
			{
				if (Where(JStr(A, TEXT("face")), At)) { LookAt.Add(At); }
				else { LookAt.Add(To - Back * 100000.f); }
			}
		}
		if (LookAt.Num() == 0 && Path.Num() > 0) { LookAt.Add(Path.Last() + FRotator(0.f, C ? C->GetControlRotation().Yaw : 0.f, 0.f).Vector() * 100000.f); }
		FVector2D Fov(70.f, 50.f);
		const TArray<TSharedPtr<FJsonValue>>* FovArr;
		if (A->TryGetArrayField(TEXT("fov"), FovArr) && FovArr->Num() >= 2) { Fov = FVector2D((*FovArr)[0]->AsNumber(), (*FovArr)[1]->AsNumber()); }
		else if (A->HasField(TEXT("fovdeg"))) { Fov = FVector2D(JNum(A, TEXT("fovdeg"), 70.f), JNum(A, TEXT("fovdeg"), 70.f)); }
		if (!EyesShut()) { UE_LOG(LogTemp, Warning, TEXT("Story: flyout in beat %s starts with the eyes open (spec 6.10 wants them closed)"), *BeatId); }
		if (R && Path.Num() > 0)
		{
			R->FlyOut(Owner, Path, LookAt, Len, Fov, JNum(A, TEXT("open"), 1.5f));
			// the shot outlives the slip beat by its closing lids: the beat's end must not cut it (the rig ends it itself)
			KeepTimed(Owner, static_cast<uint8>(EYggLease::Fly), Len + 3.f);
			UE_LOG(LogTemp, Display, TEXT("Story: flyout %s, %d points, %.1f s (beat %s)"), *JStr(A, TEXT("flyout")), Path.Num(), Len, *BeatId);
		}
		else if (!R) { NoRig(TEXT("flyout")); }
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("lure")))
	{
		if (Where(JStr(A, TEXT("lure")), At)) { PlayLure(A, At, BeatId); }
		else { UE_LOG(LogTemp, Warning, TEXT("Story: lure target %s missing (beat %s)"), *JStr(A, TEXT("lure")), *BeatId); }
		return 0.f;
	}
	if ((A->HasField(TEXT("npc")) || A->HasField(TEXT("npc_mode"))) && !A->HasField(TEXT("say")))   // (2026-11-20) a say may carry an npc (who talks)
	{
		const FString Id = A->HasField(TEXT("npc")) ? JStr(A, TEXT("npc")) : JStr(A, TEXT("npc_mode"));
		AYggStoryNPC* N = Person(Id);
		// where a person stands at a place: on the drawn ground (tiles), else whatever collision is there, else the place
		auto GroundSpot = [&](const FVector& Place)
		{
			FVector G;
			if (StoryGround(World, Place, 400.f, 400.f, G, nullptr)) { return G + FVector(0.f, 0.f, 95.f); }
			FHitResult H;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(StorySpawnGround), true);
			if (World->LineTraceSingleByChannel(H, Place + FVector(0.f, 0.f, 400.f), Place - FVector(0.f, 0.f, 400.f), ECC_Visibility, Q) && !Cast<APawn>(H.GetActor()))
			{
				return H.ImpactPoint + FVector(0.f, 0.f, 95.f);
			}
			return Place + FVector(0.f, 0.f, 100.f);
		};
		if (A->HasField(TEXT("npc")) && Where(JStr(A, TEXT("at")), At, &Rot))
		{
			const FRotator Face(0.f, JNum(A, TEXT("yaw"), Rot.Yaw), 0.f);
			FVector Spot = GroundSpot(At);
			// two people sent to the same spot (porter1 + porter2 at porter_start) spawned inside each other and
			// locked: queue the newcomer a step behind whoever already stands there
			for (const TPair<FName, TWeakObjectPtr<AYggStoryNPC>>& Other : People)
			{
				if (Other.Value.IsValid() && Other.Value.Get() != N && FVector::Dist2D(Other.Value->GetActorLocation(), Spot) < 100.f)
				{
					Spot -= Face.Vector().GetSafeNormal2D() * 110.f;
				}
			}
			if (!N)
			{
				// people appear only in the dark (spec 6.16): the spawn's construction hitch is hidden there
				if (!EyesShut()) { UE_LOG(LogTemp, Warning, TEXT("Story: %s appears with the eyes open in beat %s (spawn behind closed eyelids)"), *Id, *BeatId); }
				FActorSpawnParameters SP;
				SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				N = World->SpawnActor<AYggStoryNPC>(AYggStoryNPC::StaticClass(), Spot, Face, SP);
				// the spawn's overlap fix-up can shove a person far sideways (the porters landed 42 m off the trail,
				// off the collision corridor, and fell out of the world): put them back on their spot
				if (N && FVector::Dist2D(N->GetActorLocation(), Spot) > 150.f) { N->TeleportTo(Spot, Face, false, true); }
				if (N)
				{
					// story people never block each other (a pair spawned close together could not walk); the player still collides
					for (const TPair<FName, TWeakObjectPtr<AYggStoryNPC>>& Other : People)
					{
						if (AYggStoryNPC* O = Other.Value.Get())
						{
							N->GetCapsuleComponent()->IgnoreActorWhenMoving(O, true);
							O->GetCapsuleComponent()->IgnoreActorWhenMoving(N, true);
						}
					}
					People.Add(FName(*Id), N); N->Id = FName(*Id);
				}
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
				const FString Look = JStr(A, TEXT("look"), TEXT(""));
				if (Look.IsEmpty() || !N->DressAs(Look, JNum(A, TEXT("scale"), 1.f)))
				{
					N->Dress(Col, JNum(A, TEXT("scale"), 1.f), JBool(A, TEXT("hood"), false));
				}
				// set, not added (a second spawn of the same person went deeper each time); the soles fit (#84) keeps it
				N->SetStorySink(FMath::Max(JNum(A, TEXT("sink"), 0.f), 0.f));
			}
		}
		else if (N && Where(JStr(A, TEXT("at")), At, &Rot))
		{
			// npc_mode with "at": the person is put there (inside a blink: Nima sat opposite you at night)
			N->TeleportTo(GroundSpot(At), FRotator(0.f, JNum(A, TEXT("yaw"), Rot.Yaw), 0.f), false, true);
		}
		if (N)
		{
			const FString Mode = JStr(A, TEXT("mode"), TEXT(""));
			if (!Mode.IsEmpty()) { N->ClearFace(); }
			if (Mode == TEXT("follow")) { N->Mode = AYggStoryNPC::EMode::Follow; }
			else if (Mode == TEXT("goto")) { N->Mode = AYggStoryNPC::EMode::GoTo; }
			else if (Mode == TEXT("lead")) { N->Mode = AYggStoryNPC::EMode::Lead; }
			else if (Mode == TEXT("sit")) { N->Mode = AYggStoryNPC::EMode::Sit; }
			else if (Mode == TEXT("idle")) { N->Mode = AYggStoryNPC::EMode::Idle; }
			else if (Mode == TEXT("present")) { N->Mode = AYggStoryNPC::EMode::Present; }
			// "path": waypoints before "to" (the porters walked straight off the trail, audit)
			TArray<FVector> Points;
			for (const FString& S : JStrs(A, TEXT("path")))
			{
				if (Where(S, At)) { Points.Add(At); }
				else { UE_LOG(LogTemp, Warning, TEXT("Story: path place %s missing (beat %s)"), *S, *BeatId); }
			}
			if (Where(JStr(A, TEXT("to")), At)) { Points.Add(At); }
			if (Points.Num() > 0)
			{
				N->Goal = Points[0];
				Points.RemoveAt(0);
				N->Path = Points;
				N->bArrived = false;
			}
			N->FollowBehind = JNum(A, TEXT("behind"), N->FollowBehind);
			N->FollowSide = JNum(A, TEXT("side"), N->FollowSide);
			N->TalkSide = JNum(A, TEXT("talk_side"), N->TalkSide);
			N->PresentDist = JNum(A, TEXT("dist"), N->PresentDist);
			N->Speed = JNum(A, TEXT("speed"), N->Speed);
			N->bFacePlayer = JBool(A, TEXT("face"), N->bFacePlayer);
			N->bMayWalk = true;
			if ((A->HasField(TEXT("to")) || A->HasField(TEXT("path"))) && JBool(A, TEXT("arrive"), false)) { Run.BlockKind = TEXT("arrive"); return -1.f; }
		}
		return 0.f;
	}
	if (A->HasField(TEXT("npc_face")))
	{
		// head and torso to a place or to the player over about 0.5 s (spec 6.5)
		if (AYggStoryNPC* N = Person(JStr(A, TEXT("npc_face"))))
		{
			const FString To = JStr(A, TEXT("to"), TEXT("player"));
			if (To == TEXT("player")) { N->FaceTo(FVector::ZeroVector, P, -1.f); }
			else if (Where(To, At)) { N->FaceTo(At, nullptr, -1.f); }
		}
		return 0.f;
	}
	if (A->HasField(TEXT("inspect")))
	{
		UTexture2D* Front = JStr(A, TEXT("front")).IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *StoryObjectPath(JStr(A, TEXT("front"))));
		UTexture2D* Back = JStr(A, TEXT("back")).IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *StoryObjectPath(JStr(A, TEXT("back"))));
		UStaticMesh* Mesh = JStr(A, TEXT("mesh")).IsEmpty() ? nullptr : LoadObject<UStaticMesh>(nullptr, *StoryObjectPath(JStr(A, TEXT("mesh"))));
		const FString Words = Expand(JStr(A, TEXT("text"))).Replace(TEXT("|"), TEXT("\n"));
		const FString BackWords = Expand(JStr(A, TEXT("back_text"))).Replace(TEXT("|"), TEXT("\n"));
		const bool bNeedBack = JBool(A, TEXT("need_back"), false);
		if (!Front && !Back && !Mesh)
		{
			// nothing to hold (a drawing not made yet): its words as a caption (spec 8.7)
			const FString All = BackWords.IsEmpty() ? Words : (Words.IsEmpty() ? BackWords : Words + TEXT("\n") + BackWords);
			if (All.IsEmpty()) { return 0.f; }
			const float Len = StoryReadTime(All);
			ShowCaption(All, Len, false);
			return bWait ? Len : 0.f;
		}
		Overlay->InspectText = Words;
		Overlay->InspectBackText = BackWords;
		Overlay->bInspectBack = false;
		Overlay->InspectTurnX = Overlay->InspectTurnY = 1.f;
		// a photo is drawn here as a card (turned by the rig's mouse capture); a mesh (the compass) the rig holds in 3D
		Overlay->bInspectCard = !Mesh || !R;
		Overlay->InspectFront = Front ? Front : Back;
		Overlay->InspectBackImage = Back;
		Overlay->bInspectDarken = !R;
		Overlay->bInspectFlipKey = !R;
		Overlay->bInspectNeedBack = bNeedBack;
		if (R)
		{
			FYggInspect I = FYggInspect();
			I.Id = FName(*JStr(A, TEXT("inspect")));
			I.Front = Front;
			I.Back = Back;
			I.Text = FText::FromString(Words);
			I.BackText = FText::FromString(BackWords);
			I.bNeedBack = bNeedBack;
			I.Mesh = Mesh;
			R->BeginInspect(Owner, I);
			TrackLease(Owner, static_cast<uint8>(EYggLease::Inspect), false);
		}
		Tween(Overlay->InspectA, 1.f, 0.3f);
		UE_LOG(LogTemp, Display, TEXT("Story: inspect %s (beat %s)"), *JStr(A, TEXT("inspect")), *BeatId);
		Run.BlockKind = TEXT("inspect");
		Run.BlockValue = bNeedBack ? 1.f : 0.f;     // 1 = the back must be seen first (flat card; the rig checks its own)
		return -1.f;
	}
	if (A->HasField(TEXT("holdkey")))
	{
		CancelTween(Overlay->HintA);
		Overlay->Hint = Expand(JStr(A, TEXT("prompt"), FString::Printf(TEXT("Hold  [%s]"), *JStr(A, TEXT("holdkey")))));
		Overlay->HintA = 1.f;
		Overlay->HoldProgress = 0.f;
		Run.BlockKind = TEXT("holdkey");
		Run.BlockValue = 0.f;
		return -1.f;
	}

	// the rest: their options never carry another action's name
	if (A->HasField(TEXT("hold")))
	{
		const FString Mode = JField(A, TEXT("hold"));
		if (Mode == TEXT("false")) { ReleaseTracked(static_cast<uint8>(EYggLease::Move)); }
		else { HoldMove(Mode); }
		return 0.f;
	}
	if (A->HasField(TEXT("lookhold")))
	{
		if (JField(A, TEXT("lookhold")) == TEXT("false")) { ReleaseTracked(static_cast<uint8>(EYggLease::Look)); return 0.f; }
		// look locks only in the fall and the fly-out (spec R8)
		if (!BeatHas(Run.Beat, TEXT("fall")) && !BeatHas(Run.Beat, TEXT("flyout")))
		{
			UE_LOG(LogTemp, Error, TEXT("Story: lookhold in beat %s, which has no fall or flyout - ignored (spec R8)"), *BeatId);
			return 0.f;
		}
		if (!R) { NoRig(TEXT("lookhold")); return 0.f; }
		R->AcquireLease(Owner, EYggLease::Look, 20.f, false);
		TrackLease(Owner, static_cast<uint8>(EYggLease::Look), false);
		return 0.f;
	}
	if (A->HasField(TEXT("nudge")))
	{
		if (Where(JStr(A, TEXT("nudge")), At))
		{
			At.Z += JNum(A, TEXT("up"), 0.f);
			const bool bOk = R && P && P->GetVelocity().Size2D() < 150.f
				&& R->Nudge(Owner, At, JNum(A, TEXT("max"), 45.f), JNum(A, TEXT("rate"), 20.f), JNum(A, TEXT("pitchmax"), 8.f));
			if (bOk) { TrackLease(Owner, static_cast<uint8>(EYggLease::Nudge), false); }
			else
			{
				UE_LOG(LogTemp, Display, TEXT("Story: nudge %s refused in beat %s - lure instead"), *JStr(A, TEXT("nudge")), *BeatId);
				if (A->HasField(TEXT("sound"))) { PlayLure(A, At, BeatId); }
			}
		}
		return 0.f;
	}
	if (A->HasField(TEXT("reflex")))
	{
		// the orienting reflex to a loud sound: the rig turns 10 degrees toward it if it is 30-120 degrees off (spec 6.3);
		// it skips it itself under Story camera "Gentle" and in its 20 s cooldown
		if (R && Where(JStr(A, TEXT("reflex")), At)) { R->Reflex(At, JNum(A, TEXT("deg"), 10.f)); }
		return 0.f;
	}
	if (A->HasField(TEXT("fall")))
	{
		const float Len = FMath::Clamp(JNum(A, TEXT("fall"), 1.5f), 0.3f, 2.5f);
		if (R)
		{
			R->Fall(Owner, Len, SideOf(JStr(A, TEXT("side"))));
			TrackLease(Owner, static_cast<uint8>(EYggLease::Fall), false);
		}
		else { NoRig(TEXT("fall")); }
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("knock")))
	{
		if (R)
		{
			R->Knock(Owner, SideOf(JStr(A, TEXT("knock"))));
			TrackLease(Owner, static_cast<uint8>(EYggLease::Knock), false);
		}
		else { NoRig(TEXT("knock")); }
		return bWait ? 0.14f : 0.f;
	}
	if (A->HasField(TEXT("fovbeat")))
	{
		if (R)
		{
			const float HoldSec = T < 0.f ? 7.f : T;
			R->FovBeat(Owner, JNum(A, TEXT("fovbeat"), 2.5f), JNum(A, TEXT("bpm"), 96.f), HoldSec);
			KeepTimed(Owner, static_cast<uint8>(EYggLease::FovBeat), 1.5f + HoldSec + 2.f + 0.3f);     // in 1.5, hold, out 2
		}
		return 0.f;
	}
	if (A->HasField(TEXT("attention")))
	{
		if (R)
		{
			const float Len = T < 0.f ? 4.f : T;
			R->Attention(Owner, JNum(A, TEXT("attention"), -3.f), Len);
			KeepTimed(Owner, static_cast<uint8>(EYggLease::Attention), Len + 1.2f);     // + the 1.2 s release
		}
		return 0.f;
	}
	if (A->HasField(TEXT("fov")))
	{
		float Deg = JNum(A, TEXT("fov"), 0.f);
		if (FMath::Abs(Deg) > 3.f)
		{
			UE_LOG(LogTemp, Error, TEXT("Story: removed key fov > 3 in beat %s (%.1f clamped to 3)"), *BeatId, Deg);
			Deg = FMath::Clamp(Deg, -3.f, 3.f);
		}
		if (R)
		{
			// one story FOV at a time: a new one takes over from a kept one ("together" undoes "alone")
			ReleaseTracked(static_cast<uint8>(EYggLease::StoryFov), Owner);
			const bool bKeep = JBool(A, TEXT("keep"), false);
			R->SetStoryFov(Owner, Deg, FMath::Max(1.5f, T < 0.f ? 2.5f : T));
			if (bKeep) { R->AcquireLease(Owner, EYggLease::StoryFov, 1200.f, true); }   // the alone beat spans ~950 m of walking
			TrackLease(Owner, static_cast<uint8>(EYggLease::StoryFov), bKeep);
		}
		return 0.f;
	}
	if (A->HasField(TEXT("breath")))
	{
		if (R)
		{
			const float Len = T < 0.f ? 6.f : T;
			R->StoryBreath(Owner, JNum(A, TEXT("breath"), 0.4f), Len);
			KeepTimed(Owner, static_cast<uint8>(EYggLease::Breath), Len);
		}
		return 0.f;
	}
	if (A->HasField(TEXT("hands")))
	{
		const FString Item = JField(A, TEXT("hands"));
		if (!R) { return 0.f; }
		// the hands lower for whoever raised them
		for (const FStoryLease& L : StoryLeases) { if (L.Kind == static_cast<uint8>(EYggLease::Hands)) { R->SetHands(L.Owner, NAME_None); } }
		ReleaseTracked(static_cast<uint8>(EYggLease::Hands));
		if (Item != TEXT("false") && !Item.IsEmpty())
		{
			// kept past the beat: the compass raised at the stone is still in the hand at the plume
			R->SetHands(Owner, FName(*Item));
			R->AcquireLease(Owner, EYggLease::Hands, 60.f, true);
			TrackLease(Owner, static_cast<uint8>(EYggLease::Hands), true);
		}
		return 0.f;
	}
	if (A->HasField(TEXT("pace")))
	{
		if (JField(A, TEXT("pace")) == TEXT("false")) { ReleaseTracked(static_cast<uint8>(EYggLease::Pace)); return 0.f; }
		if (R)
		{
			const bool bKeep = JBool(A, TEXT("keep"), false);
			R->SetPace(Owner, JNum(A, TEXT("pace"), 1.f), T > 0.f ? T : 600.f, bKeep);
			TrackLease(Owner, static_cast<uint8>(EYggLease::Pace), bKeep);
		}
		return 0.f;
	}
	if (A->HasField(TEXT("light")))
	{
		SetLight(JStr(A, TEXT("light")), JBool(A, TEXT("on"), true), FMath::Max(0.f, T));
		return 0.f;
	}
	if (A->HasField(TEXT("caption")))
	{
		const FString Words = Expand(JStr(A, TEXT("caption")));
		const float Len = T < 0.f ? StoryReadTime(Words) : T;
		ShowCaption(Words, Len, JStr(A, TEXT("pos")).Equals(TEXT("lowleft"), ESearchCase::IgnoreCase));
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("preload")))
	{
		Preload(JStrs(A, TEXT("preload")));
		return 0.f;
	}
	if (A->HasField(TEXT("compass")))
	{
		const TSharedPtr<FJsonObject>* Obj;
		if (A->TryGetObjectField(TEXT("compass"), Obj)) { Compass(*Obj); }
		return 0.f;
	}
	if (A->HasField(TEXT("wear")))
	{
		Wear(JStr(A, TEXT("wear")), JBool(A, TEXT("on"), true));
		return 0.f;
	}
	if (A->HasField(TEXT("lens")))
	{
		UE_LOG(LogTemp, Display, TEXT("Story: lens (depth of field) is not built yet (P2) - skipped in beat %s"), *BeatId);
		return 0.f;
	}
	if (A->HasField(TEXT("teleport")))
	{
		const FString Name = JStr(A, TEXT("teleport"));
		if (!P || !Where(Name, At, &Rot))
		{
			UE_LOG(LogTemp, Warning, TEXT("Story: teleport place %s missing (beat %s)"), *Name, *BeatId);
			return 0.f;
		}
		const FRotator Facing(JNum(A, TEXT("pitch"), Rot.Pitch), Rot.Yaw, 0.f);
		EYggView V;
		const int32 Pose = StoryPose(JStr(A, TEXT("pose")), V) ? int32(V) : -1;
		// cuts only with the eyes shut (spec R7, 6.7); "cut" passes when the story already asked for closed eyelids (they
		// shut at the rig's next tick) or in the chapter's start beat
		const bool bStartBeat = Beats.IsValidIndex(Run.Beat) && JBool(Beats[Run.Beat].When, TEXT("start"), false);
		if (EyesShut() || (JBool(A, TEXT("cut"), false) && (bLidsShutRequested || bStartBeat)))
		{
			CutTo(Name, At, Facing, JNum(A, TEXT("up"), 100.f), Pose);
			return 0.f;
		}
		UE_LOG(LogTemp, Warning, TEXT("Story: teleport %s in beat %s with the eyes open - wrapped in a 0.15 / 0.5 s blink (spec 6.7)"), *Name, *BeatId);
		TArray<TSharedPtr<FJsonValue>> During;
		During.Add(MakeShared<FJsonValueObject>(A));
		StartLids(Owner, TEXT("blink"), 0.15f, 0.1f, During);
		return 0.3f;    // what follows a jump (a pose, a line) comes once the lids are shut
	}
	if (A->HasField(TEXT("fade")))
	{
		// the title card and the end page only; in the present the eyelids do this (spec 6.6)
		const float Len = T < 0.f ? 1.5f : T;
		Tween(Overlay->Black, JNum(A, TEXT("fade"), 1.f), Len);
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("title")))
	{
		Overlay->Title = JStr(A, TEXT("title"));
		Overlay->Sub = Expand(JStr(A, TEXT("sub")));
		const float Len = T < 0.f ? 6.f : T;
		Tween(Overlay->TitleA, 1.f, Len, true);
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("say")))
	{
		Overlay->Line = Expand(JStr(A, TEXT("say")));
		Overlay->Who = JStr(A, TEXT("who"));
		const float Len = T < 0.f ? StoryReadTime(Overlay->Line) : T;
		Tween(Overlay->LineA, 1.f, Len, true);
		TalkBy(A->HasField(TEXT("npc")) ? JStr(A, TEXT("npc")) : Overlay->Who, Len);
		UE_LOG(LogTemp, Display, TEXT("Story [VO] %s: %s"), *Overlay->Who, *Overlay->Line);
		return bWait ? Len : 0.f;
	}
	if (A->HasField(TEXT("hint")))
	{
		const FString H = Expand(JStr(A, TEXT("hint")));
		const float Len = T < 0.f ? 5.f : T;
		const float Idle = JNum(A, TEXT("idle"), 0.f);
		if (Idle > 0.f)
		{
			// only after that long without input (a hint for a player who is lost, not for one who is looking)
			IdleHint = H;
			IdleHintAfter = Idle;
			IdleHintShow = Len;
			IdleHintSince = Clock;
			return 0.f;
		}
		IdleHint.Reset();
		Overlay->Hint = H;
		Overlay->HoldProgress = -1.f;
		Tween(Overlay->HintA, 1.f, Len, true);
		return 0.f;
	}
	if (A->HasField(TEXT("choice")))
	{
		Overlay->Options = JStrs(A, TEXT("choice"));
		Overlay->Prompt = JStr(A, TEXT("prompt"));
		Overlay->ChoiceTimeLeft = JNum(A, TEXT("timeout"), 0.f) > 0.f ? 1.f : -1.f;
		Tween(Overlay->ChoiceA, 1.f, 0.5f);
		ChoiceResult = -1;
		bCardClose = false;
		bChoiceOpen = true;     // keys 1-4 answer it now, they do not eat or drink (ClimberControls asks)
		Run.BlockKind = TEXT("choice");
		return -1.f;
	}
	if (A->HasField(TEXT("time")))
	{
		TimeValue = JNum(A, TEXT("time"), 900.f);
		YggTime::Set(World, TimeValue);
		return 0.f;
	}
	if (A->HasField(TEXT("timehold")))
	{
		// the story keeps the clock: from now it moves at "rate" game minutes per real second (0 = stands still)
		bTimeHold = JBool(A, TEXT("timehold"), true);
		TimeValue = YggTime::Get(World);
		TimeRate = JNum(A, TEXT("rate"), 0.f);
		return 0.f;
	}
	if (A->HasField(TEXT("timeto")))
	{
		// glide the sun to a time over t seconds (no light pop) - never backwards: a slow player already past the target
		// keeps his sun (the memorial dusk replayed the dawn in 35 s, audit)
		const float Now = YggTime::Get(World);
		float To = JNum(A, TEXT("timeto"), 1700.f);
		if (To < Now && Now - To < 1200.f) { To = Now; }
		const float Len = T < 0.f ? 20.f : T;
		bTimeHold = true;
		TimeValue = Now;
		Tween(TimeValue, To, Len);
		return JBool(A, TEXT("wait"), false) ? Len : 0.f;
	}
	if (A->HasField(TEXT("cmd")))
	{
		for (const FString& Cmd : JStrs(A, TEXT("cmd"))) { if (C) { C->ConsoleCommand(Cmd, false); } }
		return 0.f;
	}
	if (A->HasField(TEXT("npc_remove")))
	{
		if (AYggStoryNPC* N = Person(JStr(A, TEXT("npc_remove"))))
		{
			if (HandNPC.Get() == N) { EndHand(); }
			N->Destroy();
		}
		People.Remove(FName(*JStr(A, TEXT("npc_remove"))));
		return 0.f;
	}
	if (A->HasField(TEXT("page")))
	{
		Overlay->PageHead = Expand(JStr(A, TEXT("head"), TEXT("Day {day} / 49")));
		Overlay->Page.Reset();
		const TArray<TSharedPtr<FJsonValue>>* PageLines;
		if (A->TryGetArrayField(TEXT("page"), PageLines))
		{
			for (const TSharedPtr<FJsonValue>& V : *PageLines)
			{
				if (!V.IsValid()) { continue; }
				if (V->Type == EJson::String) { Overlay->Page.Add(Expand(V->AsString())); continue; }
				if (V->Type != EJson::Object) { continue; }
				const TSharedPtr<FJsonObject> O = V->AsObject();
				bool bOk = true;
				for (const FString& F : JStrs(O, TEXT("if"))) { bOk &= Flags.Contains(FName(*F)); }
				for (const FString& F : JStrs(O, TEXT("ifnot"))) { bOk &= !Flags.Contains(FName(*F)); }
				if (bOk) { Overlay->Page.Add(Expand(JStr(O, TEXT("text")))); }
			}
		}
		Tween(Overlay->PageA, 1.f, 1.2f);
		bCardClose = false;
		Run.BlockKind = TEXT("page");
		Run.BlockValue = 0.f;
		// you do not walk off while reading: a Move lease (a menu cannot drop it), unless the story already holds one
		if (R && !StoryHolds(static_cast<uint8>(EYggLease::Move)))
		{
			R->AcquireLease(Owner, EYggLease::Move, 600.f, false);
			TrackLease(Owner, static_cast<uint8>(EYggLease::Move), false);
			Run.BlockValue = 1.f;
		}
		return -1.f;
	}
	if (A->HasField(TEXT("day"))) { Day = int32(JNum(A, TEXT("day"), 1.f)); return 0.f; }
	if (A->HasField(TEXT("sound")))
	{
		const float Vol = JNum(A, TEXT("vol"), 1.f);
		const bool bAt = Where(JStr(A, TEXT("at")), At);
		if (UAudioComponent* Comp = PlaySound(JStr(A, TEXT("sound")), bAt ? &At : nullptr, Vol, JNum(A, TEXT("falloff"), 3000.f)))
		{
			const float Fade = JNum(A, TEXT("fadein"), 0.f);
			if (Fade > 0.f) { Comp->FadeIn(Fade, Vol); }
			Comp->ComponentTags.Add(FName(*JStr(A, TEXT("id"), TEXT("sound"))));
		}
		return 0.f;
	}
	if (A->HasField(TEXT("stopsound")))
	{
		const FName Id(*JStr(A, TEXT("stopsound")));
		for (UAudioComponent* S : Sounds) { if (S && S->ComponentHasTag(Id)) { S->FadeOut(JNum(A, TEXT("fadeout"), 1.5f), 0.f); } }
		return 0.f;
	}
	if (A->HasField(TEXT("param")))
	{
		const FName Tag(*JStr(A, TEXT("tag"))), Name(*JStr(A, TEXT("param")));
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
			Tween(ParamValue[Slot], JNum(A, TEXT("to"), 1.f), T < 0.f ? 2.f : T, JBool(A, TEXT("pulse"), false));
		}
		return 0.f;
	}
	if (A->HasField(TEXT("show")) || A->HasField(TEXT("hide")))
	{
		const bool bShow = A->HasField(TEXT("show"));
		const FName Tag(*JStr(A, bShow ? TEXT("show") : TEXT("hide")));
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->ActorHasTag(Tag)) { continue; }
			It->SetActorHiddenInGame(!bShow);
			It->SetActorEnableCollision(bShow);
			if (bShow)
			{
				// the world builder also hides the meshes themselves (the tied khata never appeared on the stone, audit)
				TInlineComponentArray<UMeshComponent*> Meshes(*It);
				for (UMeshComponent* M : Meshes)
				{
					if (M && !M->IsEditorOnly()) { M->SetVisibility(true); M->SetHiddenInGame(false); }
				}
			}
		}
		return 0.f;
	}
	if (A->HasField(TEXT("flag"))) { for (const FString& F : JStrs(A, TEXT("flag"))) { Flags.Add(FName(*F)); } return 0.f; }
	if (A->HasField(TEXT("unflag"))) { for (const FString& F : JStrs(A, TEXT("unflag"))) { Flags.Remove(FName(*F)); } return 0.f; }
	if (A->HasField(TEXT("waitnear")))
	{
		Run.BlockKind = TEXT("near");
		return -1.f;
	}
	if (A->HasField(TEXT("end")))
	{
		// the end clears the people, lights and sounds: never with the eyes open (Nima and Elena vanished on screen after
		// the last fade-in, audit). Eyes open: fade to black first, then this action again (inside a during list it is
		// dark already)
		if (!EyesShut() && Overlay && !bInDuring)
		{
			if (!IsTweening(Overlay->Black)) { Tween(Overlay->Black, 1.f, 1.f); }
			--Run.Step;
			return 0.1f;
		}
		UE_LOG(LogTemp, Display, TEXT("Story: %s finished"), *ChapterName);
		Stop();
		return 0.f;
	}
	// a plain pause last: other actions carry "wait" as an option ({"flyout": .., "wait": false} is not a pause)
	if (A->HasField(TEXT("wait"))) { return JNum(A, TEXT("wait"), 1.f); }
	UE_LOG(LogTemp, Warning, TEXT("Story: unknown action in beat %s"), *BeatId);
	return 0.f;
}

bool UYggStorySubsystem::Unblocked(FRun& Run, float Dt)
{
	APlayerController* C = PC();
	UYggViewRigComponent* R = Rig();
	const TSharedPtr<FJsonObject>& A = Run.BlockAction;
	const bool bAuto = CVarStoryAuto.GetValueOnGameThread() != 0;
	const double Open = Clock - Run.BlockedAt;
	if (Run.BlockKind == TEXT("choice"))
	{
		const float Timeout = JNum(A, TEXT("timeout"), 0.f);
		if (Timeout > 0.f) { Overlay->ChoiceTimeLeft = 1.f - float(Open) / Timeout; }
		static const FKey Keys[4] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
		const TArray<FString> Opts = JStrs(A, TEXT("choice"));
		for (int32 i = 0; i < FMath::Min(4, Opts.Num()) && C; ++i) { if (C->WasInputKeyJustPressed(Keys[i])) { ChoiceResult = i; } }   // (2026-11-20) only keys that have an option
		if (bAuto && Open > 1.5) { ChoiceResult = 0; }
		if (ChoiceResult < 0 && Timeout > 0.f && Open > Timeout) { ChoiceResult = int32(JNum(A, TEXT("default"), -1.f)); if (ChoiceResult < 0) { ChoiceResult = 99; } }
		if (ChoiceResult < 0) { return false; }
		const TArray<FString> Fl = JStrs(A, TEXT("flags"));
		if (Fl.IsValidIndex(ChoiceResult) && !Fl[ChoiceResult].IsEmpty()) { Flags.Add(FName(*Fl[ChoiceResult])); }
		if (Opts.IsValidIndex(ChoiceResult) && JBool(A, TEXT("echo"), false))
		{
			Overlay->Who = TEXT("Sangye");
			Overlay->Line = Opts[ChoiceResult];
			Tween(Overlay->LineA, 1.f, 2.8f, true);
		}
		Tween(Overlay->ChoiceA, 0.f, 0.4f);
		ChoiceResult = -1;
		bChoiceOpen = false;
		return true;
	}
	if (Run.BlockKind == TEXT("inspect"))
	{
		bool bDone = false;
		if (R)
		{
			// the rig holds it, turns it with the mouse and reads its own keys (E, right mouse, Esc; with need_back only once
			// the back was seen); the story waits for it to be put away
			if ((bAuto && Open > 2.0) || Open > 300.0) { R->ReleaseLease(Run.Beat, EYggLease::Inspect); }
			bDone = Open > 0.25 && !R->IsInspecting();
		}
		else
		{
			// no rig, the flat card: F turns it over; E, right mouse or Esc put it down - never the left mouse (spec V17)
			const bool bHasBack = Overlay->InspectBackImage || !Overlay->InspectBackText.IsEmpty();
			if (bHasBack && C && C->WasInputKeyJustPressed(EKeys::F)) { Overlay->bInspectBack = !Overlay->bInspectBack; }
			if (Overlay->bInspectBack) { Run.BlockValue = 0.f; Overlay->bInspectNeedBack = false; }
			const bool bKey = C && (C->WasInputKeyJustPressed(EKeys::E) || C->WasInputKeyJustPressed(EKeys::RightMouseButton) || C->WasInputKeyJustPressed(EKeys::Escape));
			const bool bMay = Run.BlockValue < 0.5f || bAuto;
			bDone = Open > 0.4 && bMay && (bKey || bCardClose || (bAuto && Open > 2.0));
		}
		if (!bDone) { return false; }
		if (R)
		{
			R->ReleaseLease(Run.Beat, EYggLease::Inspect);
			StoryLeases.RemoveAll([&](const FStoryLease& L) { return L.Owner == Run.Beat && L.Kind == static_cast<uint8>(EYggLease::Inspect); });
		}
		Tween(Overlay->InspectA, 0.f, 0.25f);
		bCardClose = false;
		return true;
	}
	if (Run.BlockKind == TEXT("page"))
	{
		const bool bKey = C && (C->WasInputKeyJustPressed(EKeys::F) || C->WasInputKeyJustPressed(EKeys::E) || C->WasInputKeyJustPressed(EKeys::Enter)
			|| C->WasInputKeyJustPressed(EKeys::SpaceBar) || C->WasInputKeyJustPressed(EKeys::LeftMouseButton));
		if (!(Open > 0.8 && (bKey || bCardClose)) && !(bAuto && Open > 2.0)) { return false; }
		Tween(Overlay->PageA, 0.f, 0.6f);
		if (Run.BlockValue > 0.5f && R)
		{
			R->ReleaseLease(Run.Beat, EYggLease::Move);
			StoryLeases.RemoveAll([&](const FStoryLease& L) { return L.Owner == Run.Beat && L.Kind == static_cast<uint8>(EYggLease::Move) && !L.bKeep; });
		}
		bCardClose = false;
		return true;
	}
	if (Run.BlockKind == TEXT("hand"))
	{
		// "max" seconds (90) so a person who cannot get through never holds the chapter forever
		if (!bHandActive || !HandNPC.IsValid() || HandNPC->ReachedGoal(160.f) || Open > JNum(A, TEXT("max"), 90.f))
		{
			EndHand();
			return true;
		}
		return false;
	}
	if (Run.BlockKind == TEXT("holdkey"))
	{
		const FKey Key = StoryKey(JStr(A, TEXT("holdkey")));
		const float Need = FMath::Max(0.1f, JNum(A, TEXT("t"), 2.f));
		const bool bDown = (C && C->IsInputKeyDown(Key)) || bAuto;
		Run.BlockValue = bDown ? Run.BlockValue + Dt : FMath::Max(0.f, Run.BlockValue - Dt * 2.f);
		Overlay->HoldProgress = FMath::Clamp(Run.BlockValue / Need, 0.f, 1.f);
		if (Run.BlockValue < Need) { return false; }
		Overlay->HoldProgress = -1.f;
		Tween(Overlay->HintA, 0.f, 0.4f);
		return true;
	}
	if (Run.BlockKind == TEXT("arrive"))
	{
		AYggStoryNPC* N = Person(JStr(A, TEXT("npc_mode")).IsEmpty() ? JStr(A, TEXT("npc")) : JStr(A, TEXT("npc_mode")));
		return !N || N->ReachedGoal(160.f) || Open > JNum(A, TEXT("max"), 60.f);
	}
	if (Run.BlockKind == TEXT("near"))
	{
		FVector At;
		return !Player() || !Where(JStr(A, TEXT("waitnear")), At) || FVector::Dist(Player()->GetActorLocation(), At) < JNum(A, TEXT("r"), 500.f);
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
		*T.Value = T.bPulse ? T.To * StoryPulse(K) : FMath::Lerp(T.From, T.To, StoryEase(K));
		if (K >= 1.f) { *T.Value = T.bPulse ? 0.f : T.To; Tweens.RemoveAt(i); }
	}
}

// ------------------------------------------------------------------------------------------------ eyelids and cuts

float UYggStorySubsystem::StartLids(int32 Owner, const FString& ModeName, float T, float Hold, const TArray<TSharedPtr<FJsonValue>>& During)
{
	EYggLidMode M = EYggLidMode::Blink;
	float Def = 0.15f;
	if (ModeName == TEXT("closed")) { M = EYggLidMode::Closed; Def = 0.6f; }
	else if (ModeName == TEXT("open")) { M = EYggLidMode::Open; Def = 0.6f; }
	else if (ModeName == TEXT("wake")) { M = EYggLidMode::Wake; Def = 2.8f; }
	else if (ModeName == TEXT("sleep")) { M = EYggLidMode::Sleep; Def = 2.5f; }
	else if (ModeName != TEXT("blink")) { UE_LOG(LogTemp, Warning, TEXT("Story: blink mode '%s' unknown - a blink"), *ModeName); }
	if (T < 0.f) { T = Def; }
	if (M == EYggLidMode::Wake) { T = FMath::Max(T, 1.2f); }
	const bool bShut = M == EYggLidMode::Closed || M == EYggLidMode::Sleep;
	if (!bShut)
	{
		// opening (a blink ends open too): the kept lid lease of an earlier close lets go first, so this request is the only
		// one; the black that held the dark goes while the lids still cover
		ReleaseTracked(static_cast<uint8>(EYggLease::Lids));
		bLidsShutRequested = false;
		if (Overlay && Overlay->Black > 0.f && !IsTweening(Overlay->Black)) { Tween(Overlay->Black, 0.f, FMath::Min(0.25f, FMath::Max(T, 0.05f))); }
	}
	if (During.Num() > 0)
	{
		FDuring D;
		D.Serial = ++DuringSerial;
		D.Beat = Owner;
		D.Actions = During;
		D.Since = Clock;
		D.MaxWait = T + 0.5f;
		Durings.Add(MoveTemp(D));
	}
	if (UYggViewRigComponent* R = Rig())
	{
		// the rig says when the lids are fully shut; the "during" list then runs on the next story tick (the same frame:
		// the story ticks after the rig), not inside the rig's own tick
		TFunction<void()> OnClosed;
		if (During.Num() > 0)
		{
			TWeakObjectPtr<UYggStorySubsystem> Self(this);
			const int32 Serial = DuringSerial;
			OnClosed = [Self, Serial]()
			{
				if (UYggStorySubsystem* Me = Self.Get())
				{
					for (FDuring& D : Me->Durings) { if (D.Serial == Serial) { D.bReady = true; } }
				}
			};
		}
		R->SetLids(Owner, M, T, Hold, MoveTemp(OnClosed));
		if (bShut)
		{
			// held past the end of the beat (the slip closes them, the dawn opens them); the rig caps it at 8 s
			R->AcquireLease(Owner, EYggLease::Lids, 8.f, true);
			TrackLease(Owner, static_cast<uint8>(EYggLease::Lids), true);
		}
	}
	else
	{
		LocalLidMode = static_cast<uint8>(M);
		LocalLidTime = 0.f;
		LocalLidT = T;
		LocalLidHold = Hold;
		LocalLidFrom = LocalLid;
	}
	if (bShut)
	{
		bLidsShutRequested = true;
		if (T <= 0.01f && Overlay) { CancelTween(Overlay->Black); Overlay->Black = 1.f; }     // shut at once: dark this very frame
	}
	UE_LOG(LogTemp, Display, TEXT("Story: eyelids %s (%.2f s, hold %.2f)%s"), *ModeName, T, Hold, During.Num() ? TEXT(" with a during list") : TEXT(""));
	// already shut (a blink requested in the dark): the during list runs now
	if (During.Num() > 0 && EyesShut()) { RunDurings(); }
	return M == EYggLidMode::Blink ? T + Hold + 0.6f : T;
}

void UYggStorySubsystem::RunDurings()
{
	if (Durings.Num() == 0) { return; }
	const bool bShut = CurrentLid() >= 0.99f || (Overlay && Overlay->Black >= 0.99f);
	for (int32 i = 0; i < Durings.Num();)
	{
		const bool bLate = Clock - Durings[i].Since > Durings[i].MaxWait;
		if (!Durings[i].bReady && !bShut && !bLate) { ++i; continue; }
		FDuring D = MoveTemp(Durings[i]);
		Durings.RemoveAt(i);
		if (bLate && !D.bReady && !bShut)
		{
			UE_LOG(LogTemp, Warning, TEXT("Story: the eyelids never closed for the during list of beat %s - running it anyway"),
				Beats.IsValidIndex(D.Beat) ? *Beats[D.Beat].Id : TEXT("?"));
		}
		FRun Tmp;
		Tmp.Beat = D.Beat;
		const bool bWas = bInDuring;
		bInDuring = true;
		for (const TSharedPtr<FJsonValue>& V : D.Actions)
		{
			if (!V.IsValid() || V->Type != EJson::Object) { continue; }
			if (DoAction(Tmp, V->AsObject()) < 0.f)
			{
				UE_LOG(LogTemp, Warning, TEXT("Story: a blocking action inside a during list (beat %s) does not block"), Beats.IsValidIndex(D.Beat) ? *Beats[D.Beat].Id : TEXT("?"));
			}
			if (!bRunning) { break; }
		}
		bInDuring = bWas;
		if (!bRunning) { return; }
	}
}

void UYggStorySubsystem::TickLocalLids(float Dt)
{
	if (RigCache.IsValid()) { LocalLid = 0.f; LocalLidTime = -1.f; return; }
	if (LocalLidTime < 0.f) { return; }
	LocalLidTime += Dt;
	bool bDone = false;
	LocalLid = StoryLidCurve(static_cast<EYggLidMode>(LocalLidMode), LocalLidTime, LocalLidT, LocalLidHold, LocalLidFrom, bDone);
	if (bDone) { LocalLidTime = -1.f; }
}

float UYggStorySubsystem::CurrentLid() const
{
	if (const UYggViewRigComponent* R = RigCache.Get()) { return R->GetLid(); }
	return LocalLid;
}

bool UYggStorySubsystem::EyesShut() const
{
	return bInDuring || CurrentLid() >= 0.99f || (Overlay && Overlay->Black >= 0.99f);
}

void UYggStorySubsystem::CutTo(const FString& PlaceName, const FVector& At, const FRotator& Facing, float Up, int32 Pose)
{
	APawn* P = Player();
	APlayerController* C = PC();
	if (!P) { return; }
	// the cut frame (spec 6.7): no motion blur or temporal smear across the jump
	if (C && C->PlayerCameraManager) { C->PlayerCameraManager->SetGameCameraCutThisFrame(); }
	// no encroachment check: the story decides where the climber is. With the check the engine refused the jump from the
	// Balcony to the tent in a play-test and the player stayed at 8,400 m (2026-11-19)
	// (2026-11-20) the capsule's bottom never starts inside the ground: the default "up" (100) fits a half height of 96 cm;
	// the climber's capsule now measures ~110, started 10 cm under the tent floor and the depenetration threw it 1.2-1.7 m aside
	const ACharacter* PCh = Cast<ACharacter>(P);
	const float Half = PCh && PCh->GetCapsuleComponent() ? PCh->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	const FVector Dest = At + FVector(0.f, 0.f, FMath::Max(Up, Half + 2.f));
	const bool bMoved = P->TeleportTo(Dest, FRotator(0.f, Facing.Yaw, 0.f), false, true);
	if (ACharacter* Ch = Cast<ACharacter>(P))
	{
		Ch->GetCharacterMovement()->StopMovementImmediately();
		Ch->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	ClearPendingTeleport();   // a hold from an earlier jump ends here (this jump sets the walking mode itself)
	PendingTeleport = Dest; PendingTeleportLast = Dest; PendingTeleportYaw = Facing.Yaw; PendingTeleportTime = 2.5f;
	// what blocks the capsule here is what the physics will push him out of (the tent wake, 2026-11-20): say it by name
	LogCutOverlaps(PlaceName, Dest);
	// a posture that follows this cut (the tent wake, the night tent) is held on this spot until he walks or crawls
	bPinned = false;   // SetMovementMode(MOVE_Walking) above already ended an earlier pin
	PinSpot = Dest; PinCutClock = Clock; bPinSpot = true;
	const FRotator Face(FMath::Clamp(FRotator::NormalizeAxis(Facing.Pitch), -60.f, 60.f), Facing.Yaw, 0.f);
	// the one absolute view write the story keeps (spec R2): a teleport's facing, behind closed eyelids
	if (C) { C->SetControlRotation(Face); }
	if (UYggViewRigComponent* R = Rig()) { R->HardReset(Face, Pose >= 0 ? static_cast<EYggView>(Pose) : EYggView::Walk); }
	// a teleport ends a nudge, an attention zoom and a knock (spec 4.4)
	ReleaseTracked(static_cast<uint8>(EYggLease::Nudge));
	ReleaseTracked(static_cast<uint8>(EYggLease::Attention));
	ReleaseTracked(static_cast<uint8>(EYggLease::Knock));
	UE_LOG(LogTemp, Display, TEXT("Story: camera cut %s (lid %.2f, veil %.2f) - teleport %s, now at %s"), *PlaceName, CurrentLid(),
		Overlay ? Overlay->Black : 0.f, bMoved ? TEXT("ok") : TEXT("REFUSED"), *P->GetActorLocation().ToCompactString());
}

// ------------------------------------------------------------------------------------------------ hands, hints, lights, inspect

void UYggStorySubsystem::EndHand()
{
	if (bHandActive)
	{
		if (UYggViewRigComponent* R = Rig())
		{
			R->SetHandPull(TWeakObjectPtr<AActor>(), 0.f);
			if (bHandHeld) { R->SetHands(HandOwner, NAME_None); }
			R->ReleaseLease(HandOwner, EYggLease::Pace);
		}
		StoryLeases.RemoveAll([this](const FStoryLease& L) { return L.Owner == HandOwner && L.Kind == static_cast<uint8>(EYggLease::Pace) && !L.bKeep; });
		// the hint must not stay up after the hold (the player let go just before Dolma arrived, audit)
		if (Overlay) { Tween(Overlay->HintA, 0.f, 0.5f); }
	}
	if (AYggStoryNPC* N = HandNPC.Get()) { if (N->Mode == AYggStoryNPC::EMode::Lead) { N->Mode = AYggStoryNPC::EMode::Idle; } }
	HandNPC = nullptr;
	HandOwner = INDEX_NONE;
	bHandActive = false;
	bHandHeld = false;
}

void UYggStorySubsystem::TickHand(float Dt)
{
	if (!bHandActive) { return; }
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
	const bool bNear = bHeld && D < 600.f;
	if (bNear != bHandHeld)
	{
		// her hand in yours: the view leans toward where she goes (at most HandPull deg/s, yields to the mouse), the left
		// hand reaches down to hers (spec 6.4)
		bHandHeld = bNear;
		if (UYggViewRigComponent* R = Rig())
		{
			R->SetHandPull(bNear ? TWeakObjectPtr<AActor>(N) : TWeakObjectPtr<AActor>(), bNear ? HandPull : 0.f);
			R->SetHands(HandOwner, bNear ? N->Id : NAME_None);
		}
	}
	if (bNear)
	{
		// the hand walk can outlast the rig's 60 s Hands lease: keep it alive, and raise the hand again if it was dropped
		if (UYggViewRigComponent* R = Rig())
		{
			if (R->GetHandsItem().IsNone()) { R->SetHands(HandOwner, N->Id); }
			else { R->AcquireLease(HandOwner, EYggLease::Hands, 0.f); }
		}
	}
	CancelTween(Overlay->HintA);
	if (bNear)
	{
		// she pulls you along beside her (the walk is capped by the hand's Pace lease)
		const FVector Pull = (N->GetActorLocation() + N->GetActorForwardVector() * 60.f - P->GetActorLocation()).GetSafeNormal2D();
		P->AddMovementInput(Pull, D > 160.f ? 0.8f : 0.45f);
		Overlay->HintA = FMath::FInterpTo(Overlay->HintA, 0.f, Dt, 3.f);
	}
	else
	{
		Overlay->HintA = FMath::FInterpTo(Overlay->HintA, 1.f, Dt, 3.f);
	}
}

void UYggStorySubsystem::TickHints(float Dt)
{
	APawn* P = Player();
	APlayerController* C = PC();
	if (!P || !C) { return; }
	// input = the view turned or the body moved
	const FRotator V = C->GetControlRotation();
	const bool bTurned = FMath::Abs(FRotator::NormalizeAxis(V.Yaw - LastView.Yaw)) > 0.3f || FMath::Abs(FRotator::NormalizeAxis(V.Pitch - LastView.Pitch)) > 0.3f;
	LastView = V;
	const float Speed = P->GetVelocity().Size2D();
	if (bTurned || Speed > 20.f || !P->GetLastMovementInputVector().IsNearlyZero()) { LastInputAt = Clock; }
	if (IdleHint.IsEmpty()) { return; }
	// the player found his way (he walks) or it is long past: the hint is no longer his
	if (Speed > 50.f || Clock - IdleHintSince > 120.0) { IdleHint.Reset(); return; }
	if (Clock - FMath::Max(LastInputAt, IdleHintSince) >= IdleHintAfter)
	{
		CancelTween(Overlay->HintA);
		Overlay->Hint = IdleHint;
		Overlay->HoldProgress = -1.f;
		Tween(Overlay->HintA, 1.f, IdleHintShow, true);
		IdleHint.Reset();
	}
}

void UYggStorySubsystem::TickPoseHint(float Dt)
{
	if (!Overlay) { return; }
	UYggViewRigComponent* R = RigCache.Get();
	APawn* P = Player();
	// the line is someone else's now (a story hint, a hold), or ours has faded out: not ours any more
	if (bPoseHintOwned && (Overlay->Hint != PoseHintText || (!IsTweening(Overlay->HintA) && Overlay->HintA <= 0.01f))) { bPoseHintOwned = false; }
	if (Overlay->HintGlow > 0.f && (Overlay->Hint != PoseHintText || (!IsTweening(Overlay->HintA) && Overlay->HintA <= 0.01f))) { Overlay->HintGlow = 0.f; }
	// what a held move key does now; nothing to say behind closed eyelids or the black
	const bool bEyesOpen = CurrentLid() < 0.05f && Overlay->Black < 0.05f;
	if (!bEyesOpen) { PoseEyesOpenSince = -1.0; }
	else if (PoseEyesOpenSince < 0.0) { PoseEyesOpenSince = Clock; }
	const uint8 Step = R && P && bEyesOpen ? R->GetMoveHintStep() : 0;
	if (Step != PoseHintStep)
	{
		// sat up, crawling, standing: the old step's words go at once
		if (bPoseHintOwned) { Tween(Overlay->HintA, 0.f, 0.4f); bPoseHintOwned = false; }
		PoseHintStep = Step;
		PoseHintSince = Clock;
		PoseHintShows = 0;
		PoseHintShownAt = -100.0;
		CrawlStillSince = -1.0;
	}
	if (Step == 0) { return; }
	// the key is held: it works, so the words step aside
	if (R->IsMoveHeld())
	{
		if (bPoseHintOwned) { Tween(Overlay->HintA, 0.f, 0.4f); bPoseHintOwned = false; }
		CrawlStillSince = -1.0;
		return;
	}
	// crawling: only once the crawl has stopped (the key let go) for 3 s
	if (Step == 3)
	{
		if (P->GetVelocity().SizeSquared2D() > FMath::Square(5.f)) { CrawlStillSince = -1.0; return; }
		if (CrawlStillSince < 0.0) { CrawlStillSince = Clock; }
	}
	// out of the way of everything else on screen: the story's own hint, a hand to hold, a choice, a prompt, a card, a title
	if (bPoseHintOwned || Overlay->HintA > 0.05f || IsTweening(Overlay->HintA) || bHandActive || bChoiceOpen || Overlay->PressA > 0.05f
		|| Overlay->InspectA > 0.05f || Overlay->PageA > 0.05f || Overlay->TitleA > 0.05f || R->IsInspecting())
	{
		return;
	}
	const double Settled = FMath::Max(PoseHintSince, PoseEyesOpenSince);
	bool bShow = false, bStrong = false;
	if (PoseHintShows == 0)
	{
		// the first time: lying 2 s with the eyes open (the wake-up lands first), sitting 2.5 s after the sit-up holds,
		// crawling 3 s after it stopped
		bShow = Step == 3 ? Clock - CrawlStillSince >= 3.0 : Clock - Settled >= (Step == 1 ? 2.0 : 2.5);
	}
	else if (PoseHintShows < 6)
	{
		// again: brighter after 25 s without any input (asleep at the keys), plain every 30 s for one who looks round
		// and does not find the way out
		bStrong = Clock - FMath::Max(LastInputAt, PoseHintShownAt) >= 25.0;
		bShow = bStrong || Clock - PoseHintShownAt >= 30.0;
	}
	if (!bShow) { return; }
	const FString Key = YggSettings::KeyName(StoryForwardKey(PC()));
	PoseHintText = Step == 1 ? FString::Printf(TEXT("Hold  [%s]  to sit up"), *Key) : FString::Printf(TEXT("Hold  [%s]  to crawl out"), *Key);
	CancelTween(Overlay->HintA);
	Overlay->Hint = PoseHintText;
	Overlay->HoldProgress = -1.f;
	Overlay->HintGlow = bStrong ? 1.f : 0.f;
	// a pulse like the story's hints: in over a fifth, held, out over the last fifth
	Tween(Overlay->HintA, 1.f, bStrong ? 9.f : 6.f, true);
	bPoseHintOwned = true;
	++PoseHintShows;
	PoseHintShownAt = Clock;
	UE_LOG(LogTemp, Display, TEXT("Story: posture hint '%s' (%s, show %d)"), *PoseHintText, Step == 1 ? TEXT("lying") : Step == 2 ? TEXT("sitting, exit") : TEXT("crawl stopped"), PoseHintShows);
}

void UYggStorySubsystem::SetLight(const FString& Name, bool bOn, float Seconds)
{
	const FName Key(*Name);
	int32 Slot = INDEX_NONE;
	for (int32 i = 0; i < 4 && Slot == INDEX_NONE; ++i) { if (LightName[i] == Key) { Slot = i; } }
	for (int32 i = 0; i < 4 && Slot == INDEX_NONE; ++i) { if (LightName[i].IsNone()) { Slot = i; } }
	if (Slot == INDEX_NONE) { UE_LOG(LogTemp, Warning, TEXT("Story: more than 4 story lights - %s skipped"), *Name); return; }
	LightComps[Slot].RemoveAll([](const TWeakObjectPtr<ULightComponent>& L) { return !L.IsValid(); });
	if (LightComps[Slot].Num() == 0)
	{
		APawn* P = Player();
		if (Name == TEXT("headlamp"))
		{
			// the player's headlamp (spec 6.15): a spot on the eye, 25 degrees, 25 m, and no shadows (a shadowed light costs
			// 0.5-1 ms of virtual shadow maps on the laptop)
			if (!Headlamp && P)
			{
				UCameraComponent* Cam = StoryCamera(P);
				Headlamp = NewObject<USpotLightComponent>(P, TEXT("StoryHeadlamp"));
				Headlamp->SetupAttachment(Cam ? static_cast<USceneComponent*>(Cam) : P->GetRootComponent());
				Headlamp->SetRelativeLocation(FVector(4.f, 0.f, 6.f));
				Headlamp->SetIntensityUnits(ELightUnits::Lumens);
				Headlamp->SetIntensity(CVarStoryHeadlampLm.GetValueOnGameThread());
				Headlamp->SetOuterConeAngle(38.f);     // a soft centre fading out wide, like a real LED headlamp on snow;
				Headlamp->SetInnerConeAngle(16.f);     // 4 deg made a hot spot that lit whatever was held up (the compass) like a lamp
				Headlamp->SetSpecularScale(CVarStoryHeadlampSpec.GetValueOnGameThread());
				Headlamp->SetSourceRadius(3.f);
				Headlamp->SetAttenuationRadius(2500.f);
				Headlamp->SetCastShadows(false);
				Headlamp->SetLightColor(FLinearColor(1.f, 0.94f, 0.86f));
				Headlamp->SetVisibility(false);
				Headlamp->RegisterComponent();
			}
			if (Headlamp) { LightComps[Slot].Add(Headlamp); }
		}
		else
		{
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				if (!It->ActorHasTag(Key)) { continue; }
				TInlineComponentArray<ULightComponent*> Lights(*It);
				for (ULightComponent* L : Lights) { LightComps[Slot].Add(L); }
			}
			FVector At;
			FRotator Rot;
			if (LightComps[Slot].Num() == 0 && Where(Name, At, &Rot))
			{
				// no placed light: a spot at the place, hanging from a tent loop and aimed down (15 degrees, no shadows)
				FActorSpawnParameters SP;
				SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (ASpotLight* S = GetWorld()->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), At + FVector(0.f, 0.f, 170.f), FRotator(-75.f, Rot.Yaw, 0.f), SP))
				{
					if (USpotLightComponent* L = S->FindComponentByClass<USpotLightComponent>())
					{
						L->SetMobility(EComponentMobility::Movable);
						L->SetIntensityUnits(ELightUnits::Lumens);
						L->SetIntensity(300.f);
						L->SetOuterConeAngle(15.f);
						L->SetInnerConeAngle(8.f);
						L->SetAttenuationRadius(800.f);
						L->SetCastShadows(false);
						L->SetLightColor(FLinearColor(1.f, 0.9f, 0.78f));
						L->SetVisibility(false);
						LightComps[Slot].Add(L);
					}
					SpawnedLights.Add(S);
				}
			}
		}
		if (LightComps[Slot].Num() == 0) { UE_LOG(LogTemp, Warning, TEXT("Story: no light %s (no tagged actor, no place)"), *Name); return; }
		for (const TWeakObjectPtr<ULightComponent>& L : LightComps[Slot])
		{
			if (L.IsValid() && !LightOrig.Contains(L)) { LightOrig.Add(L, TPair<float, bool>(L->Intensity, L->IsVisible())); }
		}
		LightBase[Slot] = LightComps[Slot][0]->Intensity;
		LightK[Slot] = LightComps[Slot][0]->IsVisible() ? 1.f : 0.f;
	}
	LightName[Slot] = Key;
	Tween(LightK[Slot], bOn ? 1.f : 0.f, Seconds);
	UE_LOG(LogTemp, Display, TEXT("Story: light %s %s"), *Name, bOn ? TEXT("on") : TEXT("off"));
}

void UYggStorySubsystem::TickLights(float Dt)
{
	for (int32 i = 0; i < 4; ++i)
	{
		if (LightName[i].IsNone()) { continue; }
		const bool bVisible = LightK[i] > 0.001f;
		for (const TWeakObjectPtr<ULightComponent>& W : LightComps[i])
		{
			ULightComponent* L = W.Get();
			if (!L) { continue; }
			const float I = LightBase[i] * LightK[i];
			if (!FMath::IsNearlyEqual(L->Intensity, I, 0.01f)) { L->SetIntensity(I); }
			if (L->IsVisible() != bVisible) { L->SetVisibility(bVisible); }
		}
	}
}

void UYggStorySubsystem::TickInspect(float Dt)
{
	// the rig holds the inspect state (raised, turned by the mouse, back seen); the overlay draws the card and its words
	// from it each frame
	if (!Overlay || Overlay->InspectA <= 0.001f) { return; }
	const UYggViewRigComponent* R = RigCache.Get();
	if (!R || !R->IsInspecting()) { return; }
	Overlay->bInspectBack = R->IsInspectBackFacing();
	Overlay->bInspectNeedBack = !R->CanCloseInspect();
	// the card foreshortens as it turns (yaw free, pitch +-60): edge-on at 90 degrees, where the back takes over
	const FRotator Turn = R->GetInspectRotation();
	Overlay->InspectTurnX = FMath::Max(0.04f, FMath::Abs(FMath::Cos(FMath::DegreesToRadians(Turn.Yaw))));
	Overlay->InspectTurnY = FMath::Max(0.3f, FMath::Abs(FMath::Cos(FMath::DegreesToRadians(Turn.Pitch))));
}

// ------------------------------------------------------------------------------------------------ sound, preload, props

USoundAttenuation* UYggStorySubsystem::Attenuation(float Falloff)
{
	const int32 Key = FMath::Max(100, FMath::RoundToInt(Falloff / 100.f) * 100);
	if (TObjectPtr<USoundAttenuation>* Found = Attenuations.Find(Key)) { return Found->Get(); }
	// the sound waves carry no attenuation of their own (MountainAudioComponent builds its own for the same reason):
	// full volume within 3 m, fading to nothing at the falloff, placed in space
	USoundAttenuation* A = NewObject<USoundAttenuation>(this);
	A->Attenuation.bAttenuate = true;
	A->Attenuation.bSpatialize = true;
	A->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	A->Attenuation.AttenuationShapeExtents = FVector(300.f, 0.f, 0.f);
	A->Attenuation.FalloffDistance = float(Key);
	Attenuations.Add(Key, A);
	return A;
}

UAudioComponent* UYggStorySubsystem::PlaySound(const FString& Path, const FVector* At, float Vol, float Falloff)
{
	if (Path.IsEmpty() || Path.StartsWith(TEXT("<"))) { return nullptr; }     // "<by ear>": a sound not chosen yet
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *StoryObjectPath(Path));
	if (!Sound) { UE_LOG(LogTemp, Warning, TEXT("Story: no sound %s"), *Path); return nullptr; }
	UAudioComponent* Comp = At
		? UGameplayStatics::SpawnSoundAtLocation(GetWorld(), Sound, *At, FRotator::ZeroRotator, Vol, 1.f, 0.f, Attenuation(Falloff))
		: UGameplayStatics::SpawnSound2D(GetWorld(), Sound, Vol);
	if (Comp)
	{
		Sounds.RemoveAll([](const TObjectPtr<UAudioComponent>& S) { return !IsValid(S); });
		Sounds.Add(Comp);
	}
	return Comp;
}

void UYggStorySubsystem::PlayLure(const TSharedPtr<FJsonObject>& Lure, const FVector& Target, const FString& BeatId)
{
	// a sound at the thing, heard from within the view: a thing behind the player is heard 60 degrees off-centre on its
	// side, so turning toward the sound turns toward it, and the player makes the turn himself (spec 6.1)
	const FString Path = JStr(Lure, TEXT("sound"));
	APlayerController* C = PC();
	if (Path.IsEmpty() || !C || !C->PlayerCameraManager) { return; }
	FVector To = Target;
	Where(JStr(Lure, TEXT("at")), To);
	const FVector Eye = C->PlayerCameraManager->GetCameraLocation();
	const FVector Fwd = C->PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal2D();
	const FVector Dir = (To - Eye).GetSafeNormal2D();
	FVector At = To;
	if (FVector::DotProduct(Fwd, Dir) < FMath::Cos(FMath::DegreesToRadians(60.f)))
	{
		const float Side = FVector::CrossProduct(Fwd, Dir).Z >= 0.f ? 1.f : -1.f;
		const float Dist = FMath::Clamp(float(FVector::Dist2D(To, Eye)), 400.f, 2000.f);
		At = Eye + Fwd.RotateAngleAxis(60.f * Side, FVector::UpVector) * Dist;
	}
	if (PlaySound(Path, &At, JNum(Lure, TEXT("vol"), 0.9f), JNum(Lure, TEXT("falloff"), 3000.f)))
	{
		UE_LOG(LogTemp, Display, TEXT("Story: lure %s (beat %s)"), *FPaths::GetBaseFilename(Path), *BeatId);
	}
}

void UYggStorySubsystem::Preload(const TArray<FString>& Looks)
{
	// once the whole cast is loading, later requests have nothing to add
	if (PreloadHandle.IsValid() || !UAssetManager::IsInitialized()) { return; }
	const TSharedPtr<FJsonObject> Cast = StoryCast();
	if (!Cast.IsValid()) { return; }
	const bool bAll = Looks.Num() == 0 || Looks.ContainsByPredicate([](const FString& S) { return S.Equals(TEXT("cast"), ESearchCase::IgnoreCase); });
	TArray<FSoftObjectPath> Paths;
	Paths.Add(FSoftObjectPath(TEXT("/Game/Story/MetaHumans/Common/Anims/ABP_Unarmed_MH.ABP_Unarmed_MH_C")));
	Paths.Add(FSoftObjectPath(TEXT("/Game/Story/MetaHumans/Common/Face/ABP_Face.ABP_Face_C")));
	TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> Collect;
	Collect = [&Paths, &Collect](const FString& Key, const TSharedPtr<FJsonValue>& V)
	{
		if (!V.IsValid()) { return; }
		if (V->Type == EJson::String)
		{
			const FString S = V->AsString();
			if (!S.StartsWith(TEXT("/Game/"))) { return; }
			FString P = StoryObjectPath(S);
			if (Key == TEXT("abp") && !P.EndsWith(TEXT("_C"))) { P += TEXT("_C"); }     // an anim blueprint: its class
			Paths.AddUnique(FSoftObjectPath(P));
		}
		else if (V->Type == EJson::Object)
		{
			for (const auto& KV : V->AsObject()->Values) { Collect(FString(*KV.Key), KV.Value); }
		}
		else if (V->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& E : V->AsArray()) { Collect(Key, E); }
		}
	};
	for (const auto& KV : Cast->Values)
	{
		if (bAll || Looks.Contains(FString(*KV.Key))) { Collect(FString(*KV.Key), KV.Value); }
	}
	PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Paths), FStreamableDelegateWithHandle(),
		FStreamableManager::DefaultAsyncLoadPriority, false, false, TEXT("StoryCast"));
	UE_LOG(LogTemp, Display, TEXT("Story: preloading %s in the background"), bAll ? TEXT("the whole cast") : *FString::Join(Looks, TEXT(", ")));
}

void UYggStorySubsystem::Wear(const FString& Item, bool bOn)
{
	if (!Item.Equals(TEXT("khata"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: wear %s - only the khata is known"), *Item);
		return;
	}
	if (!bOn)
	{
		if (WornKhata) { WornKhata->DestroyComponent(); WornKhata = nullptr; }
		return;
	}
	ACharacter* Ch = Cast<ACharacter>(Player());
	if (!Ch || WornKhata) { return; }
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, KhataMeshPath);
	if (!Mesh)
	{
		ShowCaption(TEXT("The white khata hangs from your neck."), 4.f, false);
		return;
	}
	// on the chest of the body you see of yourself (FirstPersonFeet: looking down at -55 degrees shows white on the red
	// jacket); without a chest bone there, at chest height on the capsule
	USceneComponent* Parent = nullptr;
	FName Bone = NAME_None;
	TInlineComponentArray<USkinnedMeshComponent*> Skins(Ch);
	for (USkinnedMeshComponent* S : Skins)
	{
		if (!S || !S->GetName().Contains(TEXT("FirstPersonFeet"))) { continue; }
		for (const TCHAR* B : { TEXT("spine_05"), TEXT("spine_04"), TEXT("spine_03"), TEXT("Spine2"), TEXT("mixamorig:Spine2"), TEXT("Spine1"), TEXT("mixamorig:Spine1") })
		{
			if (S->GetBoneIndex(FName(B)) != INDEX_NONE) { Parent = S; Bone = FName(B); break; }
		}
	}
	WornKhata = NewObject<UStaticMeshComponent>(Ch, TEXT("StoryKhata"));
	WornKhata->SetStaticMesh(Mesh);
	WornKhata->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WornKhata->SetCastShadow(false);
	if (Parent)
	{
		WornKhata->SetupAttachment(Parent, Bone);
	}
	else
	{
		WornKhata->SetupAttachment(Ch->GetCapsuleComponent());
		WornKhata->SetRelativeLocation(FVector(16.f, 0.f, 36.f));
	}
	WornKhata->SetRelativeScale3D(FVector(0.5f));
	WornKhata->RegisterComponent();
	UE_LOG(LogTemp, Display, TEXT("Story: khata worn on %s"), Parent ? *Bone.ToString() : TEXT("the capsule (no chest bone found)"));
}

void UYggStorySubsystem::Compass(const TSharedPtr<FJsonObject>& Params)
{
	APawn* P = Player();
	UCompassComponent* Comp = P ? P->FindComponentByClass<UCompassComponent>() : nullptr;
	if (!Comp || !Params.IsValid()) { return; }
	const float Tremble = JNum(Params, TEXT("tremble"), 0.f);
	FVector Toward = FVector::ZeroVector;
	Where(JStr(Params, TEXT("toward")), Toward);
	// the needle trembles and points up the ridge (spec 6.12 / 8.11): a lure held in your own hand; tremble 0 gives the
	// needle back. Without a "toward" place it trembles where it points (a jolt to start it).
	if (Tremble > 0.f && Toward.IsZero())
	{
		Comp->Kick(FMath::Clamp(Tremble * 0.5f, 0.f, 1.f), false);
		return;
	}
	Comp->SetStoryTarget(Toward, Tremble);
}

void UYggStorySubsystem::ReleaseCompass()
{
	// a story target holds the compass up until released: Stop, JumpTo, the chapter end and death give the needle back
	APawn* P = Player();
	UCompassComponent* Comp = P ? P->FindComponentByClass<UCompassComponent>() : nullptr;
	if (Comp && Comp->HasStoryTarget()) { Comp->SetStoryTarget(FVector::ZeroVector, 0.f); }
}

void UYggStorySubsystem::TalkBy(const FString& Who, float Seconds)
{
	// the speaker: "npc" in the line, or a person whose id is the name or one of its words ("Aunt Pema" -> aunt)
	TArray<FString> Words;
	Who.ToLower().ParseIntoArrayWS(Words);
	const FString Low = Who.ToLower();
	for (const TPair<FName, TWeakObjectPtr<AYggStoryNPC>>& KV : People)
	{
		AYggStoryNPC* N = KV.Value.Get();
		if (!N) { continue; }
		const FString Id = KV.Key.ToString().ToLower();
		if (Id == Low || Words.Contains(Id)) { N->TalkLeft = Seconds; }
	}
}

void UYggStorySubsystem::ShowCaption(const FString& Text, float Seconds, bool bLowLeft)
{
	if (!Overlay) { return; }
	Overlay->Caption = Text.Replace(TEXT("|"), TEXT("\n"));
	Overlay->bCaptionLowLeft = bLowLeft;
	Tween(Overlay->CaptionA, 1.f, Seconds, true);
	UE_LOG(LogTemp, Display, TEXT("Story [caption] %s"), *Overlay->Caption.Replace(TEXT("\n"), TEXT(" / ")));
}

// ------------------------------------------------------------------------------------------------ tools

void UYggStorySubsystem::TickTools(float Dt)
{
	if (CamStep != INDEX_NONE) { TickCamTest(Dt); }
	if (FrameTime >= 0.f) { TickFrameShot(Dt); }
	// a tool's jump (ygg.Cam.Frame up at a fly-out point) is held in the air like the story's own
	if (!bRunning && PendingTeleportTime > 0.f) { UpdatePendingTeleport(Dt); }
}

void UYggStorySubsystem::StartFrameShot(const FString& Place, const FString& Pose)
{
	if (bRunning) { UE_LOG(LogTemp, Warning, TEXT("Cam.Frame: stop the story first (ygg.Story.Stop)")); return; }
	if (Places.Num() == 0)
	{
		FString Raw;
		TSharedPtr<FJsonObject> Root;
		if (FFileHelper::LoadFileToString(Raw, *(FPaths::ProjectContentDir() / TEXT("Story/Chapter1.ygs"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Root))
		{
			LoadPlaces(Root);
		}
	}
	FVector At;
	FRotator Rot;
	if (!Where(Place, At, &Rot)) { UE_LOG(LogTemp, Warning, TEXT("Cam.Frame: no place %s"), *Place); return; }
	EYggView V = EYggView::Walk;
	if (!Pose.IsEmpty() && !StoryPose(Pose, V)) { UE_LOG(LogTemp, Warning, TEXT("Cam.Frame: unknown pose %s - standing"), *Pose); }
	CutTo(Place, At, Rot, 100.f, int32(V));
	if (V != EYggView::Walk)
	{
		if (UYggViewRigComponent* R = Rig())
		{
			FYggPoseReq Q = FYggPoseReq();
			Q.Pose = V;
			Q.Seconds = 0.f;
			Q.bSnap = true;
			R->RequestPose(-2, Q);
		}
	}
	FramePlace = Place;
	FramePose = V == EYggView::Walk ? FString() : FString(StoryPoseName(V));
	FrameTime = 0.f;
	bFrameShot = false;
}

void UYggStorySubsystem::TickFrameShot(float Dt)
{
	FrameTime += Dt;
	// 2.5 s for the ground, the streaming and the posture to settle, then the scene without the HUD
	if (!bFrameShot && FrameTime >= 2.5f)
	{
		bFrameShot = true;
		const FString Name = FramePose.IsEmpty() ? FramePlace : FramePlace + TEXT("_") + FramePose;
		const FString Path = FPaths::ProjectSavedDir() / TEXT("Review/frames") / (Name + TEXT(".png"));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
		FScreenshotRequest::RequestScreenshot(Path, false, false);
		UE_LOG(LogTemp, Display, TEXT("Cam.Frame: %s -> %s"), *Name, *Path);
	}
	if (FrameTime >= 3.5f) { FrameTime = -1.f; }
}

void UYggStorySubsystem::StartCamTest()
{
	if (bRunning) { UE_LOG(LogTemp, Warning, TEXT("CamTest: stop the story first (ygg.Story.Stop)")); return; }
	if (!Rig() || !PC() || !PC()->PlayerCameraManager || !Player()) { UE_LOG(LogTemp, Warning, TEXT("CamTest: no view rig on the player")); return; }
	CamSteps.Reset();
	CamResults.Reset();
	CamBaseFov = PC()->PlayerCameraManager->GetFOVAngle();
	const int32 O = CamTestOwner;

	auto Add = [this](const FString& Name, float Wait, TFunction<void()> Do, TFunction<FString()> Check = TFunction<FString()>(), TFunction<void(float)> OnTick = TFunction<void(float)>())
	{
		FCamStep S;
		S.Name = Name;
		S.Wait = Wait;
		S.Do = MoveTemp(Do);
		S.Check = MoveTemp(Check);
		S.OnTick = MoveTemp(OnTick);
		CamSteps.Add(MoveTemp(S));
	};
	auto Pose = [this, O](EYggView V, float Seconds, bool bExit)
	{
		UYggViewRigComponent* R = Rig();
		APawn* P = Player();
		if (!R || !P || !PC()) { return; }
		FYggPoseReq Q = FYggPoseReq();
		Q.Pose = V;
		Q.Seconds = Seconds;
		if (bExit) { Q.Exit = P->GetActorLocation() + FRotator(0.f, PC()->GetControlRotation().Yaw, 0.f).Vector() * 150.f; }
		R->RequestPose(O, Q);
	};
	auto Hold = [this, O]() { if (UYggViewRigComponent* R = Rig()) { R->AcquireLease(O, EYggLease::Move, 120.f, false); } };
	auto Release = [this, O]() { if (UYggViewRigComponent* R = Rig()) { R->ReleaseOwner(O); } };
	auto Yaw = [this]() { return PC() ? float(PC()->GetControlRotation().Yaw) : 0.f; };
	auto Eye = [this]() { return PC() && PC()->PlayerCameraManager ? PC()->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector; };
	auto Level = [this]() { return FRotator(0.f, PC() ? PC()->GetControlRotation().Yaw : 0.f, 0.f); };
	// the checks every test ends with (spec V3): eye, pitch, roll, FOV, view target, pawn shown, dilation, plane, lids, leases
	auto Common = [this](bool bWalk) -> FString
	{
		UYggViewRigComponent* R = Rig();
		APawn* P = Player();
		APlayerController* C = PC();
		if (!R || !P || !C || !C->PlayerCameraManager) { return TEXT("no pawn, controller or rig"); }
		TArray<FString> Bad;
		if (C->GetViewTarget() != P) { Bad.Add(FString::Printf(TEXT("view target %s"), *GetNameSafe(C->GetViewTarget()))); }
		if (P->IsHidden()) { Bad.Add(TEXT("pawn hidden")); }
		const AWorldSettings* WS = GetWorld()->GetWorldSettings();
		if (WS && !FMath::IsNearlyEqual(WS->GetEffectiveTimeDilation(), 1.f, 0.01f)) { Bad.Add(FString::Printf(TEXT("time dilation %.2f"), WS->GetEffectiveTimeDilation())); }
		if (FMath::Abs(FRotator::NormalizeAxis(C->GetControlRotation().Roll)) > 2.f) { Bad.Add(TEXT("control roll")); }
		const FRotator Cam = C->PlayerCameraManager->GetCameraRotation();
		const float Pitch = FRotator::NormalizeAxis(Cam.Pitch);
		if (Pitch < -80.f || Pitch > 82.f) { Bad.Add(FString::Printf(TEXT("pitch %.0f"), Pitch)); }
		if (FMath::Abs(FRotator::NormalizeAxis(Cam.Roll)) > 2.f) { Bad.Add(FString::Printf(TEXT("roll %.1f"), FRotator::NormalizeAxis(Cam.Roll))); }
		if (FMath::Abs(C->PlayerCameraManager->GetFOVAngle() - CamBaseFov) > 5.f) { Bad.Add(FString::Printf(TEXT("FOV %.1f (was %.1f)"), C->PlayerCameraManager->GetFOVAngle(), CamBaseFov)); }
		const ACharacter* Ch = Cast<ACharacter>(P);
		if (Ch && Ch->GetCharacterMovement() && Ch->GetCharacterMovement()->bConstrainToPlane && R->GetView() != EYggView::Crawl) { Bad.Add(TEXT("plane constraint on")); }
		if (R->GetLid() > 0.05f) { Bad.Add(FString::Printf(TEXT("eyelids %.2f"), R->GetLid())); }
		if (bWalk)
		{
			if (R->GetView() != EYggView::Walk) { Bad.Add(FString::Printf(TEXT("posture %s"), StoryPoseName(R->GetView()))); }
			if (R->GetEyeAboveFloor() < 135.f) { Bad.Add(FString::Printf(TEXT("eye %.0f cm above the floor"), R->GetEyeAboveFloor())); }
			if (R->ShapeWalkSpeed(100.f) < 99.f) { Bad.Add(FString::Printf(TEXT("walk speed shaped to %.0f%% (a lease left)"), R->ShapeWalkSpeed(100.f))); }
		}
		return FString::Join(Bad, TEXT(", "));
	};
	auto Band = [this](EYggView V) -> FString
	{
		UYggViewRigComponent* R = Rig();
		if (!R) { return TEXT("no rig"); }
		if (R->GetView() != V) { return FString::Printf(TEXT("posture %s, not %s"), StoryPoseName(R->GetView()), StoryPoseName(V)); }
		const float E = R->GetEyeAboveFloor();
		if (FMath::Abs(E - StoryPoseEye(V)) > StoryPoseBand(V) + 15.f) { return FString::Printf(TEXT("eye %.0f cm (want %.0f)"), E, StoryPoseEye(V)); }
		return FString();
	};

	// postures
	Add(TEXT("lie"), 3.2f, [=, this]() { Pose(EYggView::Lie, 2.5f, false); }, [=, this]() { return Band(EYggView::Lie); });
	Add(TEXT("lie -> sit"), 2.8f, [=, this]() { Pose(EYggView::Sit, 2.2f, false); }, [=, this]() { return Band(EYggView::Sit); });
	Add(TEXT("sit -> stand"), 2.4f, [=, this]() { Pose(EYggView::Walk, 1.6f, false); Release(); }, [=, this]() { return Common(true); });
	TSharedRef<bool> bCrawled = MakeShared<bool>(false);
	Add(TEXT("sit (exit ahead)"), 2.4f, [=, this]() { Pose(EYggView::Sit, 2.0f, true); *bCrawled = false; });
	Add(TEXT("sit -> crawl -> stand"), 9.f, [=, this]() { Pose(EYggView::Crawl, 0.8f, true); },
		[=, this]() { return Common(true); },
		[=, this](float)
		{
			// crawl forward to the exit; stop pushing once he stands
			UYggViewRigComponent* R = Rig();
			APawn* P = Player();
			if (!R || !P) { return; }
			if (R->GetView() == EYggView::Crawl) { *bCrawled = true; }
			if (!(*bCrawled && R->GetView() == EYggView::Walk)) { P->AddMovementInput(FRotator(0.f, Yaw(), 0.f).Vector(), 1.f); }
		});
	Add(TEXT("kneel1"), 1.4f, [=, this]() { Pose(EYggView::Kneel1, 0.9f, false); }, [=, this]() { return Band(EYggView::Kneel1); });
	Add(TEXT("kneel1 -> stand"), 1.6f, [=, this]() { Pose(EYggView::Walk, 1.0f, false); Release(); }, [=, this]() { return Common(true); });
	Add(TEXT("kneel2"), 1.5f, [=, this]() { Pose(EYggView::Kneel2, 1.0f, false); }, [=, this]() { return Band(EYggView::Kneel2); });
	Add(TEXT("kneel2 -> stand"), 1.8f, [=, this]() { Pose(EYggView::Walk, 1.2f, false); Release(); }, [=, this]() { return Common(true); });
	// guidance
	TSharedRef<FVector> NudgeAt = MakeShared<FVector>(FVector::ZeroVector);
	Add(TEXT("nudge 40 degrees away"), 3.6f,
		[=, this]()
		{
			*NudgeAt = Eye() + FRotator(0.f, Yaw() + 40.f, 0.f).Vector() * 1000.f;
			UYggViewRigComponent* R = Rig();
			if (R && !R->Nudge(O, *NudgeAt, 45.f, 20.f, 8.f)) { UE_LOG(LogTemp, Warning, TEXT("CamTest: the rig refused the nudge (budget or Gentle)")); }
		},
		[=, this]()
		{
			const float Err = FMath::Abs(FRotator::NormalizeAxis(float((*NudgeAt - Eye()).Rotation().Yaw) - Yaw()));
			Release();
			return Err < 16.f ? FString() : FString::Printf(TEXT("still %.0f degrees off"), Err);
		});
	Add(TEXT("nudge cancelled by the mouse"), 0.f, [=, this]() {}, [=, this]() { return FString(TEXT("SKIP (needs a hand on the mouse: move it during a nudge)")); });
	TSharedRef<float> Yaw0 = MakeShared<float>(0.f);
	Add(TEXT("reflex"), 0.9f,
		[=, this]()
		{
			*Yaw0 = Yaw();
			if (UYggViewRigComponent* R = Rig()) { R->Reflex(Eye() + FRotator(0.f, *Yaw0 - 60.f, 0.f).Vector() * 800.f, 10.f); }
		},
		[=, this]()
		{
			const float Turned = FRotator::NormalizeAxis(*Yaw0 - Yaw());
			return Turned > 5.f && Turned < 15.f ? FString() : FString::Printf(TEXT("turned %.1f degrees (want about 10 toward the sound)"), Turned);
		});
	// takeovers
	Add(TEXT("knock"), 1.4f, [=, this]() { if (UYggViewRigComponent* R = Rig()) { R->Knock(O, 1.f); } }, [=, this]() { Release(); return Common(true); });
	Add(TEXT("fall"), 2.0f,
		[=, this]()
		{
			if (UYggViewRigComponent* R = Rig())
			{
				R->AcquireLease(O, EYggLease::Look, 20.f, false);
				R->AcquireLease(O, EYggLease::Move, 120.f, false);
				R->Fall(O, 1.5f, -1.f);
			}
		});
	Add(TEXT("fall + HardReset"), 1.2f, [=, this]() { if (UYggViewRigComponent* R = Rig()) { R->ReleaseAll(0.f); R->HardReset(Level(), EYggView::Walk); } }, [=, this]() { return Common(true); });
	TSharedRef<bool> bLidsShut = MakeShared<bool>(false);
	TSharedRef<bool> bCut = MakeShared<bool>(false);
	Add(TEXT("blink + self-teleport"), 1.6f,
		[=, this]()
		{
			*bLidsShut = false;
			*bCut = false;
			if (UYggViewRigComponent* R = Rig()) { R->SetLids(O, EYggLidMode::Blink, 0.15f, 0.3f, [bLidsShut]() { *bLidsShut = true; }); }
		},
		[=, this]()
		{
			Release();
			FString W = Common(true);
			if (!*bCut) { W += W.IsEmpty() ? TEXT("the lids never reported closed") : TEXT(", the lids never reported closed"); }
			return W;
		},
		[=, this](float)
		{
			// the cut on the story's tick after the rig said shut, as the story does it
			APawn* P = Player();
			if (*bLidsShut && !*bCut && P)
			{
				*bCut = true;
				const float Half = P->GetSimpleCollisionHalfHeight();
				CutTo(TEXT("camtest"), P->GetActorLocation() - FVector(0.f, 0.f, Half), Level(), Half + 4.f, -1);
			}
		});
	Add(TEXT("fly-out: lids shut"), 0.4f, [=, this]() { if (UYggViewRigComponent* R = Rig()) { R->SetLids(O, EYggLidMode::Closed, 0.15f, 0.f, nullptr); } });
	Add(TEXT("fly-out 3 s"), 1.2f,
		[=, this]()
		{
			UYggViewRigComponent* R = Rig();
			APawn* P = Player();
			if (!R || !P) { return; }
			const FVector E = Eye();
			const FVector Back = -FRotator(0.f, Yaw(), 0.f).Vector();
			R->FlyOut(O, { E + FVector(0.f, 0.f, 300.f), E + Back * 1500.f + FVector(0.f, 0.f, 1500.f), E + Back * 4000.f + FVector(0.f, 0.f, 4000.f) },
				{ P->GetActorLocation() }, 3.f, FVector2D(70.f, 50.f), 0.5f);
		},
		[=, this]() { return PC() && PC()->GetViewTarget() != Player() ? FString() : FString(TEXT("the view never left the climber")); });
	Add(TEXT("fly-out end"), 4.5f, [=, this]() {});
	Add(TEXT("after the fly-out"), 1.2f, [=, this]() { Release(); }, [=, this]() { return Common(true); });
	// inspect
	Add(TEXT("inspect open"), 0.7f,
		[=, this]()
		{
			if (UYggViewRigComponent* R = Rig())
			{
				FYggInspect I = FYggInspect();
				I.Id = TEXT("camtest");
				I.Text = FText::FromString(TEXT("CamTest"));
				I.Mesh = LoadObject<UStaticMesh>(nullptr, KhataMeshPath);
				R->BeginInspect(O, I);
			}
		},
		[=, this]()
		{
			UYggViewRigComponent* R = Rig();
			if (!R) { return FString(TEXT("no rig")); }
			FString W;
			if (!R->IsInspecting()) { W = TEXT("not inspecting"); }
			if (R->ShapeWalkSpeed(100.f) > 1.f) { W += W.IsEmpty() ? TEXT("walk speed not 0") : TEXT(", walk speed not 0"); }
			return W;
		});
	Add(TEXT("inspect close"), 0.8f, [=, this]() { if (UYggViewRigComponent* R = Rig()) { R->ReleaseLease(O, EYggLease::Inspect); } },
		[=, this]()
		{
			UYggViewRigComponent* R = Rig();
			Release();
			return R && R->IsInspecting() ? FString(TEXT("still inspecting")) : Common(true);
		});
	// resets
	Add(TEXT("JumpTo mid-pose: kneel2"), 0.6f, [=, this]() { Pose(EYggView::Kneel2, 1.0f, false); Hold(); });
	Add(TEXT("JumpTo mid-pose"), 2.0f, [=, this]() { ResetCamera(0.4f); }, [=, this]() { return Common(true); });
	Add(TEXT("Stop mid-fly-out: lids shut"), 0.4f, [=, this]() { if (UYggViewRigComponent* R = Rig()) { R->SetLids(O, EYggLidMode::Closed, 0.15f, 0.f, nullptr); } });
	Add(TEXT("Stop mid-fly-out: fly"), 1.5f,
		[=, this]()
		{
			UYggViewRigComponent* R = Rig();
			APawn* P = Player();
			if (!R || !P) { return; }
			const FVector E = Eye();
			R->FlyOut(O, { E + FVector(0.f, 0.f, 300.f), E + FVector(0.f, 0.f, 2000.f) }, { P->GetActorLocation() }, 6.f, FVector2D(70.f, 50.f), 0.5f);
		});
	Add(TEXT("Stop mid-fly-out"), 2.0f, [=, this]() { ResetCamera(0.4f); }, [=, this]() { return Common(true); });
	Add(TEXT("menu mid-pose: kneel1"), 1.2f, [=, this]() { Pose(EYggView::Kneel1, 0.9f, false); Hold(); });
	Add(TEXT("menu open / close mid-pose"), 0.4f,
		[=, this]()
		{
			// what menus do to the controller (YggMenuBase, SettingsWidget, BackpackWidget): leases must not care
			if (APlayerController* C = PC()) { C->SetIgnoreMoveInput(true); C->ResetIgnoreInputFlags(); }
		},
		[=, this]()
		{
			UYggViewRigComponent* R = Rig();
			if (!R) { return FString(TEXT("no rig")); }
			FString W;
			if (R->GetView() != EYggView::Kneel1) { W = TEXT("the posture was lost"); }
			if (R->ShapeWalkSpeed(100.f) > 1.f) { W += W.IsEmpty() ? TEXT("the Move lease was lost") : TEXT(", the Move lease was lost"); }
			return W;
		});
	Add(TEXT("menu mid-pose: stand"), 1.6f, [=, this]() { Release(); Pose(EYggView::Walk, 1.0f, false); }, [=, this]() { return Common(true); });
	Add(TEXT("death mid-pose: sit"), 2.2f, [=, this]() { Pose(EYggView::Sit, 2.0f, false); Hold(); });
	Add(TEXT("death mid-pose"), 2.4f,
		[=, this]()
		{
			// ExpeditionComponent's death path: Story->ReleaseAll(), Rig->ReleaseAll(0), HardReset
			ReleaseAll();
			if (UYggViewRigComponent* R = Rig()) { R->ReleaseAll(0.f); R->HardReset(Level(), EYggView::Walk); }
		},
		[=, this]() { return Common(true); });

	CamStep = 0;
	CamStepTime = -1.f;
	UE_LOG(LogTemp, Display, TEXT("CamTest: %d steps, about 75 s - keep the mouse and keys still"), CamSteps.Num());
}

void UYggStorySubsystem::TickCamTest(float Dt)
{
	if (!CamSteps.IsValidIndex(CamStep))
	{
		int32 Pass = 0, Fail = 0, Skip = 0;
		for (const FString& R : CamResults)
		{
			if (R.Contains(TEXT(" PASS"))) { ++Pass; } else if (R.Contains(TEXT(" SKIP"))) { ++Skip; } else { ++Fail; }
		}
		const FString Summary = FString::Printf(TEXT("CamTest: %d PASS, %d FAIL, %d SKIP"), Pass, Fail, Skip);
		UE_LOG(LogTemp, Display, TEXT("%s"), *Summary);
		if (UYggViewRigComponent* R = Rig()) { CamResults.Add(TEXT("rig: ") + R->Audit()); }
		CamResults.Insert(Summary, 0);
		const FString Path = FPaths::ProjectSavedDir() / TEXT("Review/camtest.txt");
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
		FFileHelper::SaveStringToFile(FString::Join(CamResults, TEXT("\n")) + TEXT("\n"), *Path);
		CamSteps.Reset();
		CamStep = INDEX_NONE;
		return;
	}
	FCamStep& S = CamSteps[CamStep];
	if (CamStepTime < 0.f)
	{
		CamStepTime = 0.f;
		UE_LOG(LogTemp, Display, TEXT("CamTest: %s"), *S.Name);
		if (S.Do) { S.Do(); }
		if (CamStep == INDEX_NONE || !CamSteps.IsValidIndex(CamStep)) { return; }
	}
	else
	{
		if (S.OnTick) { S.OnTick(Dt); }
		CamStepTime += Dt;
	}
	if (CamStepTime < S.Wait) { return; }
	if (S.Check)
	{
		const FString Why = S.Check();
		const bool bSkip = Why.StartsWith(TEXT("SKIP"));
		const FString Line = FString::Printf(TEXT("%-32s %s%s"), *S.Name, bSkip ? TEXT("SKIP") : (Why.IsEmpty() ? TEXT("PASS") : TEXT("FAIL")),
			Why.IsEmpty() ? TEXT("") : *(TEXT(" - ") + Why));
		UE_LOG(LogTemp, Display, TEXT("CamTest: %s"), *Line);
		CamResults.Add(Line);
	}
	++CamStep;
	CamStepTime = -1.f;
}
