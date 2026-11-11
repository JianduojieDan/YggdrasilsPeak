// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SoundTourComponent.h"

#include "Survival/MountainAudioComponent.h"
#include "Survival/SnowTrailComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Environment/RealmSky.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Dom/JsonObject.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

static FAutoConsoleCommandWithWorldAndArgs CmdSoundTour(TEXT("snd"),
	TEXT("In-game sound tour: snd (start) | snd new (only new / changed / not yet good) | snd <group number or word> | ")
	TEXT("snd note <words> | snd bad <words> | snd good | snd unsure | snd next | snd back | snd again | snd 0 (end)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		USoundTourComponent::Command(World, Args);
	}));

namespace
{
	float SS(float A, float B, float X) { return FMath::SmoothStep(A, B, X); }

	FString Pretty(const FString& Name)
	{
		return Name.Replace(TEXT("_"), TEXT(" "));
	}

	void SetStepSurface(int32 Surface)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.Snow.StepSurface")))
		{
			V->Set(Surface, ECVF_SetByCode);
		}
	}

	FString Field(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name)
	{
		FString Value;
		if (Object.IsValid()) { Object->TryGetStringField(Name, Value); }
		return Value;
	}

	const TCHAR* const DangerName[] = { TEXT("harmless"), TEXT("risky"), TEXT("deadly") };
}

// ================================================================ panel

TSharedRef<SWidget> USoundTourWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Build();
	}
	return Super::RebuildWidget();
}

void USoundTourWidget::Build()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;

	if (UFontFace* Face = LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Fonts/FF_Alegreya.FF_Alegreya")))
	{
		Serif = NewObject<UFont>(this);
		Serif->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry& Entry = Serif->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = TEXT("Regular");
		Entry.Font = FFontData(Face);
	}

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	Card->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.72f));
	Card->SetPadding(FMargin(26.f, 20.f));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Card->SetContent(Box);

	auto MakeText = [this, Box](const TCHAR* Name, int32 Size, const FLinearColor& Colour, bool bSerif, float Gap)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = Text->GetFont();
		if (bSerif && Serif)
		{
			Font.FontObject = Serif;
			Font.TypefaceFontName = TEXT("Regular");
		}
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Colour));
		Text->SetAutoWrapText(true);
		Text->SetWrapTextAt(400.f);
		Box->AddChildToVerticalBox(Text)->SetPadding(FMargin(0.f, 0.f, 0.f, Gap));
		return Text;
	};
	Header = MakeText(TEXT("Header"), 10, FLinearColor(0.72f, 0.78f, 0.84f, 0.85f), false, 8.f);
	Title = MakeText(TEXT("Title"), 24, FLinearColor(1.f, 0.88f, 0.66f, 1.f), true, 6.f);
	Where = MakeText(TEXT("Where"), 12, FLinearColor(0.88f, 0.9f, 0.93f, 0.95f), false, 12.f);
	Status = MakeText(TEXT("Status"), 13, FLinearColor(0.62f, 0.86f, 1.f, 1.f), false, 6.f);
	Verdict = MakeText(TEXT("Verdict"), 12, FLinearColor(0.95f, 0.95f, 0.85f, 0.95f), false, 16.f);
	UTextBlock* Keys = MakeText(TEXT("Keys"), 11, FLinearColor(0.72f, 0.76f, 0.8f, 0.9f), false, 0.f);
	Keys->SetText(FText::FromString(TEXT("[G] good    [B] bad    [U] unsure    [Down] hear again\n")
		TEXT("[Left] back    [Right] next    [Up] next group\n")
		TEXT("console:  snd note <words>  (why)    snd 0  (end the tour)")));

	UCanvasPanelSlot* CardSlot = Canvas->AddChildToCanvas(Card);
	CardSlot->SetAnchors(FAnchors(0.f, 0.5f));
	CardSlot->SetAlignment(FVector2D(0.f, 0.5f));
	CardSlot->SetPosition(FVector2D(36.f, 0.f));
	CardSlot->SetSize(FVector2D(452.f, 0.f));
	CardSlot->SetAutoSize(true);
}

void USoundTourWidget::Put(UTextBlock* Text, const FString& Value)
{
	if (Text && Text->GetText().ToString() != Value)
	{
		Text->SetText(FText::FromString(Value));
		Text->SetVisibility(Value.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void USoundTourWidget::SetLines(const FString& InHeader, const FString& InTitle, const FString& InWhere, const FString& InStatus, const FString& InVerdict)
{
	Put(Header, InHeader);
	Put(Title, InTitle);
	Put(Where, InWhere);
	Put(Status, InStatus);
	Put(Verdict, InVerdict);
}

// ================================================================ console

void USoundTourComponent::Command(UWorld* World, const TArray<FString>& Args)
{
	APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("snd: start Play first (the tour plays inside the game)."));
		return;
	}
	USoundTourComponent* Tour = Pawn->FindComponentByClass<USoundTourComponent>();
	const FString A = Args.Num() > 0 ? Args[0].ToLower() : FString();
	FString Rest;
	for (int32 i = 1; i < Args.Num(); ++i)
	{
		Rest += (i > 1 ? TEXT(" ") : TEXT("")) + Args[i];
	}
	if (A == TEXT("0") || A == TEXT("off") || A == TEXT("stop") || A == TEXT("end"))
	{
		if (Tour) { Tour->Finish(); }
		return;
	}
	if (!Tour)
	{
		Tour = NewObject<USoundTourComponent>(Pawn, TEXT("SoundTour"));
		Tour->RegisterComponent();
	}
	const bool bWasRunning = Tour->bRunning;
	if (A == TEXT("new") || A == TEXT("all") || !bWasRunning)
	{
		Tour->Begin(A == TEXT("new"));
		if (!Tour->bRunning || A == TEXT("new") || A == TEXT("all") || A.IsEmpty())
		{
			return;
		}
	}
	if (A.IsEmpty()) { return; }
	if (A == TEXT("note")) { Tour->AddNote(Rest); return; }
	if (A == TEXT("good")) { Tour->Rate(TEXT("good"), Rest); return; }
	if (A == TEXT("bad")) { Tour->Rate(TEXT("bad"), Rest); return; }
	if (A == TEXT("unsure")) { Tour->Rate(TEXT("unsure"), Rest); return; }
	if (A == TEXT("next")) { Tour->Go(1); return; }
	if (A == TEXT("back")) { Tour->Go(-1); return; }
	if (A == TEXT("again")) { Tour->Enter(Tour->Current); return; }
	Tour->Jump(A);
}

// ================================================================ the tour

USoundTourComponent::USoundTourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
}

void USoundTourComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Finish();
	Super::EndPlay(EndPlayReason);
}

USoundBase* USoundTourComponent::Load(const TCHAR* Folder, const FString& Name, FString* OutFile)
{
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *(FString(TEXT("/Game/Audio/")) + Folder + TEXT("/") + Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Sound)
	{
		Keep.Add(Sound);
		if (OutFile)
		{
			*OutFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Audio") / Folder / Name + TEXT(".uasset"));
		}
	}
	return Sound;
}

void USoundTourComponent::AddGroup(const FString& Title, const FString& Where)
{
	Groups.Add({ Title, Where });
}

USoundTourComponent::FStep& USoundTourComponent::AddStep(EKind Kind, const FString& Key, const FString& Title, const FString& Hint)
{
	FStep& S = All.AddDefaulted_GetRef();
	S.Group = Groups.Num() - 1;
	S.Kind = Kind;
	S.Key = Key;
	S.Title = Title;
	S.Hint = Hint;
	return S;
}

void USoundTourComponent::AddSet(EKind Kind, const TCHAR* Folder, const TCHAR* Prefix, int32 Max, float Volume, float Distance, const FString& Hint)
{
	for (int32 i = 1; i <= Max; ++i)
	{
		const FString Name = FString::Printf(TEXT("%s_%02d"), Prefix, i);
		FString File;
		if (USoundBase* Sound = Load(Folder, Name, &File))
		{
			FStep& S = AddStep(Kind, Name, Pretty(Name), Hint);
			S.Sound = Sound;
			S.AssetFile = File;
			S.Volume = Volume;
			S.Distance = Distance;
			S.Seconds = Sound->GetDuration() + (Kind == EKind::Thunder ? 3.5f : 0.f);
		}
	}
}

void USoundTourComponent::BuildSteps()
{
	Groups.Reset();
	All.Reset();
	Keep.Reset();
	const FString Walk = TEXT("Walk now (W; Shift to run). Wherever you are, every step sounds like this surface, at the game's loudness for your speed.");
	const FString AtFeet = TEXT("one step at your feet, at walking loudness");

	// ---- feet
	static const TCHAR* const Surfaces[][3] = {
		{ TEXT("Walking on snow"), TEXT("Step_Snow"), TEXT("every footstep on snow, and the steps that follow you") },
		{ TEXT("Walking on ice and hard crust"), TEXT("Step_Ice"), TEXT("bare ice, hard crust, the sides of seracs") },
		{ TEXT("Walking on rubble and rock"), TEXT("Step_Rubble"), TEXT("moraine rubble and rock, low on the glacier") },
		{ TEXT("Crossing a ladder"), TEXT("Step_Ladder"), TEXT("crampons on the aluminium ladders over crevasses") },
	};
	for (int32 i = 0; i < 4; ++i)
	{
		AddGroup(Surfaces[i][0], Surfaces[i][2]);
		AddStep(EKind::WalkScene, FString(TEXT("scene:")) + Surfaces[i][1], Surfaces[i][0], Walk).Index = i + 1;
		AddSet(EKind::Step, TEXT("Footsteps"), Surfaces[i][1], 16, FootstepVolume, 0.f, AtFeet);
	}

	// ---- body
	AddGroup(TEXT("Breathing"), TEXT("whole phrases of real breathing back to back; harder and louder with effort"));
	AddStep(EKind::BreathScene, TEXT("scene:Breath_Phrase"), TEXT("Breathing gets harder"),
		TEXT("Effort rises from rest to exhausted over 45 s; the game picks each phrase for the effort at that moment.")).Seconds = 50.f;
	{
		TArray<USoundBase*> Found;
		TArray<FString> Files;
		for (int32 i = 1; i <= 12; ++i)
		{
			FString File;
			if (USoundBase* S = Load(TEXT("Body"), FString::Printf(TEXT("Breath_Phrase_%02d"), i), &File)) { Found.Add(S); Files.Add(File); }
		}
		for (int32 i = 0; i < Found.Num(); ++i)
		{
			const float Effort = FMath::Lerp(0.15f, 1.f, Found.Num() > 1 ? float(i) / (Found.Num() - 1) : 1.f);
			FStep& S = AddStep(EKind::Flat, FString::Printf(TEXT("Breath_Phrase_%02d"), i + 1), FString::Printf(TEXT("Breath Phrase %02d"), i + 1),
				FString::Printf(TEXT("the phrase the game picks at about %d%% effort, at that loudness"), FMath::RoundToInt(Effort * 100.f)));
			S.Sound = Found[i];
			S.AssetFile = Files[i];
			S.Volume = BreathVolume * FMath::Lerp(0.35f, 1.f, SS(0.15f, 0.9f, Effort));
			S.Seconds = Found[i]->GetDuration();
		}
	}

	// ---- wind
	AddGroup(TEXT("Wind"), TEXT("four loops blended by the weather's wind: calm air, gusts, blizzard, and a thin whistle high up"));
	AddStep(EKind::WindScene, TEXT("scene:Wind"), TEXT("The wind rises to a storm"),
		TEXT("The game's own wind mix while the wind goes from 2 to 10 over 40 s, as if at 6,000 m. Sound only: the sky does not change.")).Seconds = 45.f;
	{
		const float E = GetOwner()->FindComponentByClass<USurvivalComponent>() ? GetOwner()->FindComponentByClass<USurvivalComponent>()->GetElevationMeters() : 5000.f;
		const float Altitude = FMath::Lerp(0.55f, 1.f, SS(4000.f, 7000.f, E));
		static const TCHAR* const Beds[][3] = {
			{ TEXT("Wind_Calm"), TEXT("0.6"), TEXT("always under everything, strongest in calm weather") },
			{ TEXT("Wind_Gusty"), TEXT("0.8"), TEXT("moderate to strong wind (at its strongest, wind 5-7)") },
			{ TEXT("Wind_Blizzard"), TEXT("0.95"), TEXT("gales and storms") },
			{ TEXT("Wind_Whistle"), TEXT("0.7"), TEXT("only high up (5,600 m and above) in strong wind; here at its strongest") },
		};
		for (const auto& Bed : Beds)
		{
			FString File;
			if (USoundBase* Sound = Load(TEXT("Weather"), Bed[0], &File))
			{
				FStep& S = AddStep(EKind::WindBed, Bed[0], Pretty(Bed[0]) + TEXT(" (loop, alone)"), FString(Bed[2]) + TEXT(". The other wind is silent while it plays."));
				S.Sound = Sound;
				S.AssetFile = File;
				S.Volume = WindVolume * FCString::Atof(Bed[1]) * (FString(Bed[0]) == TEXT("Wind_Whistle") ? 1.f : Altitude);
			}
		}
	}
	AddGroup(TEXT("A single gust"), TEXT("now and then on top of strong wind (above 6 of 10)"));
	AddSet(EKind::Flat, TEXT("Weather"), TEXT("Wind_Gust"), 4, WindVolume * 0.6f, 0.f, TEXT("one gust at its full loudness, over the live wind"));

	// ---- sky and mountain
	AddGroup(TEXT("Thunder"), TEXT("a few seconds after a lightning flash in the Jotunheim storm, from the storm's side"));
	AddStep(EKind::ThunderScene, TEXT("scene:Thunder"), TEXT("The storm"), TEXT("The Jotunheim storm: bursts of strikes at once, near and far. The nearer, the louder and brighter its thunder; the further, the duller and later.")).Seconds = 55.f;
	AddSet(EKind::Thunder, TEXT("Weather"), TEXT("Thunder"), 8, 1.f, 3000.f, TEXT("a bolt 1.2 km away, then 3.5 s later this thunder"));

	AddGroup(TEXT("Distant avalanche"), TEXT("every 4-10 minutes above 4,900 m, far across the valley"));
	AddSet(EKind::Avalanche, TEXT("Ice"), TEXT("Avalanche"), 8, AvalancheVolume, 0.f, TEXT("far away, from a random direction (turn your head to place it)"));

	AddGroup(TEXT("Boom inside the mountain"), TEXT("the Western Cwm: a deep boom under you, felt more than heard"));
	AddSet(EKind::Below, TEXT("Ice"), TEXT("Mountain_Boom"), 4, 1.6f, 1500.f, TEXT("deep below you and a little to one side"));

	AddGroup(TEXT("Serac zone"), TEXT("ice towers over the route: groaning, warning cracks, then a collapse"));
	AddStep(EKind::SeracScene, TEXT("scene:Serac"), TEXT("A serac zone breaks"), TEXT("The strain grows: a groan far off, cracks coming closer, the last warning, then the collapse.")).Seconds = 22.f;
	AddSet(EKind::Around, TEXT("Ice"), TEXT("Ice_Groan"), 6, 0.8f, 1700.f, TEXT("the zone straining, halfway to a collapse: around you, 17 m away"));
	AddSet(EKind::Around, TEXT("Ice"), TEXT("Ice_Crack"), 12, 0.75f, 1700.f, TEXT("a crack while the strain builds: around you, 17 m away"));
	AddSet(EKind::Around, TEXT("Ice"), TEXT("Serac_Collapse"), 4, 1.6f, 300.f, TEXT("a tower of ice falls, 3 m from you"));

	AddGroup(TEXT("Whispers"), TEXT("when the mind grows cold: a whisper just behind one shoulder"));
	AddSet(EKind::Behind, TEXT("Mind"), TEXT("Whisper"), 8, 0.55f, 80.f, TEXT("behind your left or right shoulder"));

	// ---- gear
	AddGroup(TEXT("Fixed rope"), TEXT("clipping in, moving clipped, unclipping"));
	AddStep(EKind::RopeScene, TEXT("scene:Rope"), TEXT("Clip in, move, unclip"), TEXT("A clip-in gesture, the rope running through the carabiner while you walk, then the unclip.")).Seconds = 8.f;
	AddSet(EKind::Flat, TEXT("Gear"), TEXT("Carabiner_ClipIn"), 8, RopeVolume * 0.8f, 0.f, TEXT("clipping into the fixed rope"));
	AddSet(EKind::Flat, TEXT("Gear"), TEXT("Carabiner_Click"), 8, RopeVolume * 0.8f, 0.f, TEXT("unclipping from the fixed rope"));
	AddSet(EKind::Flat, TEXT("Gear"), TEXT("Rope_Tighten"), 8, RopeVolume * 0.7f, 0.f, TEXT("the rope pulling tight (approved, but not used in the game now)"));
	{
		FString File;
		if (USoundBase* Sound = Load(TEXT("Gear"), TEXT("Rope_Slide_Loop"), &File))
		{
			FStep& S = AddStep(EKind::Loop, TEXT("Rope_Slide_Loop"), TEXT("Rope Slide (loop)"), TEXT("the rope running through the carabiner while you walk clipped"));
			S.Sound = Sound; S.AssetFile = File; S.Volume = RopeVolume * 0.35f; S.Pitch = 1.04f;
		}
		for (const TCHAR* Name : { TEXT("Rope_Slide_Fast"), TEXT("Rope_Slide_Slow") })
		{
			if (USoundBase* Sound = Load(TEXT("Gear"), Name, &File))
			{
				FStep& S = AddStep(EKind::Flat, Name, Pretty(Name), TEXT("a single slide (not used in the game yet)"));
				S.Sound = Sound; S.AssetFile = File; S.Volume = RopeVolume * 0.35f; S.Seconds = Sound->GetDuration();
			}
		}
	}

	// ---- the weather wheel
	AddGroup(TEXT("Weather wheel"), TEXT("the HUD wheel: a tick per divider, then the landing sound of that weather"));
	AddStep(EKind::WheelScene, TEXT("scene:Wheel"), TEXT("The wheel spins"), TEXT("The real wheel on your HUD spins and lands on the coming weather.")).Seconds = 13.f;
	AddSet(EKind::Flat, TEXT("UI"), TEXT("Wheel_Tick"), 8, 0.4f, 0.f, TEXT("one tick, as a divider passes the pointer"));
	{
		TArray<int32> Order;
		for (int32 i = 0; i < ARealmSky::NumWeathers(); ++i) { Order.Add(i); }
		Order.StableSort([](int32 L, int32 R) { return ARealmSky::GetWeatherInfo(L).Danger < ARealmSky::GetWeatherInfo(R).Danger; });
		for (int32 i : Order)
		{
			const FRealmWeatherInfo& Info = ARealmSky::GetWeatherInfo(i);
			const FString& SoundName = ARealmSky::GetLandingSound(i);
			FString File;
			USoundBase* Sound = Load(TEXT("UI"), SoundName, &File);
			const FString Weather = FString(Info.Name).Replace(TEXT("’"), TEXT("'"));
			FStep& S = AddStep(EKind::Landing, TEXT("Landing:") + Weather,
				FString::Printf(TEXT("%s (%s)"), *FString(Weather).ToLower(), DangerName[FMath::Clamp(Info.Danger, 0, 2)]),
				FString::Printf(TEXT("The wheel lands on %s: landing sound %s. The more dangerous the weather, the more intense."), *Weather, *SoundName.Replace(TEXT("UI_Chime_"), TEXT(""))));
			S.Title[0] = FChar::ToUpper(S.Title[0]);
			S.Sound = Sound;
			S.AssetFile = File;
			S.Index = i;
			S.Seconds = 1.85f + (Sound ? Sound->GetDuration() : 3.f) + 0.3f;
		}
	}
}

void USoundTourComponent::Begin(bool bOnlyNew)
{
	Finish();
	AActor* Owner = GetOwner();
	if (const USnowTrailComponent* Trail = Owner->FindComponentByClass<USnowTrailComponent>()) { FootstepVolume = Trail->FootstepVolume; }
	if (const UMountainAudioComponent* Audio = Owner->FindComponentByClass<UMountainAudioComponent>())
	{
		WindVolume = Audio->WindVolume;
		BreathVolume = Audio->BreathVolume;
		RopeVolume = Audio->RopeVolume;
		AvalancheVolume = Audio->AvalancheVolume;
	}
	if (!FromDirection)
	{
		// as the game's distant sounds: from a direction, not faded by distance (MountainAudioComponent)
		FromDirection = NewObject<USoundAttenuation>(this);
		FSoundAttenuationSettings& A = FromDirection->Attenuation;
		A.bAttenuate = false;
		A.bSpatialize = true;
		A.StereoSpread = 200000.f;
		A.bAttenuateWithLPF = false;
		A.bEnableOcclusion = false;
	}
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		if (It->GetActorNameOrLabel() == TEXT("Realm_Jotunheim")) { StormPlanet = *It; break; }
	}

	BuildSteps();
	LoadReview();
	Steps = All;
	if (bOnlyNew)
	{
		Steps.RemoveAll([this](const FStep& S) { return IsApproved(S); });
	}
	if (Steps.Num() == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("snd: every sound is already marked good and unchanged. 'snd' plays them all again."));
		return;
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		Panel = CreateWidget<USoundTourWidget>(PC, USoundTourWidget::StaticClass());
		if (Panel) { Panel->AddToViewport(40); }
	}
	bRunning = true;
	UE_LOG(LogTemp, Display, TEXT("snd: sound tour started, %d steps in %d groups%s"), Steps.Num(), Groups.Num(), bOnlyNew ? TEXT(" (only new / changed / not yet good)") : TEXT(""));
	Enter(0);
}

void USoundTourComponent::Finish()
{
	if (!bRunning && !Panel)
	{
		return;
	}
	Leave();
	if (Panel)
	{
		Panel->RemoveFromParent();
		Panel = nullptr;
	}
	if (bRunning)
	{
		SaveReview();
		UE_LOG(LogTemp, Display, TEXT("snd: sound tour ended; verdicts in Docs/Development/Sound_Review_InGame.md"));
	}
	bRunning = false;
	Current = -1;
}

// ---------------------------------------------------------------- moving

void USoundTourComponent::Leave()
{
	StopAll();
	if (UMountainAudioComponent* Audio = GetOwner() ? GetOwner()->FindComponentByClass<UMountainAudioComponent>() : nullptr)
	{
		Audio->TourWind = -1.f;
		Audio->TourElevation = -1.f;
		Audio->TourEffort = -1.f;
		Audio->bTourMuteWind = false;
	}
	SetStepSurface(0);
	Gauge = -1.f;
	SceneLine.Reset();
}

void USoundTourComponent::Enter(int32 Index)
{
	Leave();
	Current = FMath::Clamp(Index, 0, Steps.Num());
	T = PrevT = 0.f;
	if (!Steps.IsValidIndex(Current))
	{
		return;   // past the end
	}
	const FStep& S = Steps[Current];
	UMountainAudioComponent* Audio = GetOwner()->FindComponentByClass<UMountainAudioComponent>();
	ARealmSky* Sky = ARealmSky::Get(GetWorld());
	// the breathing would talk over a single sound: silent, except where it is what you hear or you walk yourself
	if (Audio && S.Kind != EKind::WalkScene && S.Kind != EKind::BreathScene) { Audio->TourEffort = 0.f; }
	switch (S.Kind)
	{
	case EKind::WalkScene: SetStepSurface(S.Index); break;
	case EKind::WindBed: if (Audio) { Audio->bTourMuteWind = true; } Play2D(S.Sound, S.Volume); break;
	case EKind::Loop: Play2D(S.Sound, S.Volume, S.Pitch); break;
	case EKind::Flat: Play2D(S.Sound, S.Volume, S.Pitch); break;
	case EKind::Step: PlayAt(S.Sound, Feet(), S.Volume); break;
	case EKind::Around: PlayAround(S.Sound, S.Volume, S.Distance); break;
	case EKind::Avalanche: PlayAvalanche(S.Sound); break;
	case EKind::Below:
		PlayAt(S.Sound, GetOwner()->GetActorLocation() + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * S.Distance - FVector(0.f, 0.f, 3000.f), S.Volume);
		break;
	case EKind::Behind:
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		const float Yaw = (Pawn ? Pawn->GetControlRotation().Yaw : 0.f) + (FMath::RandBool() ? 90.f : -90.f);
		PlayAt(S.Sound, GetOwner()->GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * S.Distance + FVector(0.f, 0.f, 60.f), S.Volume);
		break;
	}
	case EKind::Thunder: if (Sky) { Sky->Strike(-1, 1.2f, true); } break;   // a bolt 1.2 km away; its thunder 3.5 s later, in TickStep
	case EKind::Landing: if (Sky) { Sky->SpinWheelTo(S.Index); } break;
	default: break;
	}
}

void USoundTourComponent::Go(int32 Delta)
{
	if (!bRunning) { return; }
	Enter(FMath::Clamp(Current + Delta, 0, Steps.Num()));
}

void USoundTourComponent::NextGroup()
{
	if (!Steps.IsValidIndex(Current)) { return; }
	const int32 G = Steps[Current].Group;
	int32 i = Current;
	while (i < Steps.Num() && Steps[i].Group == G) { ++i; }
	Enter(i);
}

void USoundTourComponent::Jump(const FString& Word)
{
	const int32 N = FCString::Atoi(*Word);
	for (int32 i = 0; i < Steps.Num(); ++i)
	{
		const FStep& S = Steps[i];
		if ((N > 0 && S.Group == N - 1) || (N <= 0 && (Groups[S.Group].Title.Contains(Word) || S.Key.Contains(Word))))
		{
			Enter(i);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("snd: no group '%s' in this tour"), *Word);
}

// ---------------------------------------------------------------- playing

UAudioComponent* USoundTourComponent::Play2D(USoundBase* Sound, float Volume, float Pitch)
{
	UAudioComponent* C = Sound ? UGameplayStatics::SpawnSound2D(this, Sound, Volume, Pitch) : nullptr;
	if (C) { Playing.Add(C); }
	return C;
}

UAudioComponent* USoundTourComponent::PlayAt(USoundBase* Sound, const FVector& At, float Volume, float Pitch, USoundAttenuation* Attenuation)
{
	UAudioComponent* C = Sound ? UGameplayStatics::SpawnSoundAtLocation(this, Sound, At, FRotator::ZeroRotator, Volume, Pitch, 0.f, Attenuation) : nullptr;
	if (C) { Playing.Add(C); }
	return C;
}

void USoundTourComponent::PlayAround(USoundBase* Sound, float Volume, float Distance)
{
	// as ASeracCollapseZone::PlayAround: a random side, a little above
	const FVector Where = GetOwner()->GetActorLocation() + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * Distance + FVector(0.f, 0.f, FMath::FRandRange(300.f, 1200.f));
	PlayAt(Sound, Where, Volume);
}

void USoundTourComponent::PlayThunder(USoundBase* Sound)
{
	// as ARealmSky: 30 m towards the storm planet
	const FVector P = GetOwner()->GetActorLocation();
	const FVector Dir = StormPlanet.IsValid() ? (StormPlanet->GetActorLocation() - P).GetSafeNormal() : FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector();
	// as the storm plays a strike 1.2 km away (ARealmSky::ThunderVoice: further = quieter, duller, lower)
	float Volume, Pitch, LowPass;
	ARealmSky::ThunderVoice(1.2f, Volume, Pitch, LowPass);
	if (UAudioComponent* C = PlayAt(Sound, P + Dir * 3000.f, Volume, Pitch))
	{
		C->SetLowPassFilterEnabled(true);
		C->SetLowPassFilterFrequency(LowPass);
	}
}

void USoundTourComponent::PlayAvalanche(USoundBase* Sound)
{
	// as UMountainAudioComponent::Avalanche: across the valley, a little above
	const FVector At = GetOwner()->GetActorLocation() + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * 300000.f + FVector(0.f, 0.f, 50000.f);
	PlayAt(Sound, At, AvalancheVolume, 1.f, FromDirection);
}

void USoundTourComponent::StopAll()
{
	for (const TWeakObjectPtr<UAudioComponent>& C : Playing)
	{
		if (C.IsValid()) { C->FadeOut(0.15f, 0.f); }
	}
	Playing.Reset();
}

USoundBase* USoundTourComponent::PickFrom(const TCHAR* Prefix) const
{
	TArray<USoundBase*> Found;
	for (const FStep& S : All)
	{
		if (S.Sound && S.Kind != EKind::Landing && S.Key.StartsWith(Prefix)) { Found.Add(S.Sound); }
	}
	return Found.Num() > 0 ? Found[FMath::RandRange(0, Found.Num() - 1)] : nullptr;
}

FVector USoundTourComponent::Feet() const
{
	return GetOwner()->GetActorLocation() - FVector(0.f, 0.f, GetOwner()->GetSimpleCollisionHalfHeight());
}

// ---------------------------------------------------------------- tick

void USoundTourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bRunning)
	{
		return;
	}
	Playing.RemoveAll([](const TWeakObjectPtr<UAudioComponent>& C) { return !C.IsValid(); });
	ReadKeys();
	if (!bRunning)
	{
		return;
	}
	TickStep(DeltaTime);
	SavedFlash = FMath::Max(0.f, SavedFlash - DeltaTime);
	UpdatePanel();
}

void USoundTourComponent::ReadKeys()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerInput)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::G)) { Rate(TEXT("good"), FString()); }
	else if (PC->WasInputKeyJustPressed(EKeys::B)) { Rate(TEXT("bad"), FString()); }
	else if (PC->WasInputKeyJustPressed(EKeys::U)) { Rate(TEXT("unsure"), FString()); }
	else if (PC->WasInputKeyJustPressed(EKeys::Right)) { Go(1); }
	else if (PC->WasInputKeyJustPressed(EKeys::Left)) { Go(-1); }
	else if (PC->WasInputKeyJustPressed(EKeys::Up)) { NextGroup(); }
	else if (PC->WasInputKeyJustPressed(EKeys::Down)) { Enter(Current); }
}

void USoundTourComponent::TickStep(float DeltaTime)
{
	if (!Steps.IsValidIndex(Current))
	{
		return;
	}
	PrevT = T;
	T += DeltaTime;
	auto Cross = [this](float At) { return PrevT < At && T >= At; };
	const FStep& S = Steps[Current];
	UMountainAudioComponent* Audio = GetOwner()->FindComponentByClass<UMountainAudioComponent>();
	ARealmSky* Sky = ARealmSky::Get(GetWorld());
	switch (S.Kind)
	{
	case EKind::BreathScene:
		Gauge = FMath::Lerp(0.15f, 1.f, FMath::Clamp(T / 45.f, 0.f, 1.f));
		if (Audio) { Audio->TourEffort = Gauge; }
		SceneLine = FString::Printf(TEXT("effort %d%%"), FMath::RoundToInt(Gauge * 100.f));
		break;
	case EKind::WindScene:
		Gauge = 2.f + 8.f * FMath::Clamp(T / 40.f, 0.f, 1.f);
		if (Audio) { Audio->TourWind = Gauge; Audio->TourElevation = 6000.f; }
		SceneLine = FString::Printf(TEXT("wind %.1f of 10%s"), Gauge, Gauge > 6.f ? TEXT("  (single gusts can come now)") : TEXT(""));
		break;
	case EKind::ThunderScene:
		// the storm's own bursts: two, then three strikes at once, then one, each at its own distance
		if (Cross(0.5f) && Sky) { Sky->StormBurst(2); SceneLine = TEXT("two strikes"); }
		if (Cross(14.f) && Sky) { Sky->StormBurst(3); SceneLine = TEXT("three strikes at once"); }
		if (Cross(30.f) && Sky) { Sky->StormBurst(1); SceneLine = TEXT("one strike"); }
		break;
	case EKind::Thunder:
		if (Cross(3.5f)) { PlayThunder(S.Sound); }
		SceneLine = T < 3.5f ? TEXT("flash... (thunder in a moment)") : FString();
		break;
	case EKind::SeracScene:
		if (Cross(0.3f)) { PlayAround(PickFrom(TEXT("Ice_Groan")), 0.68f, 1900.f); SceneLine = TEXT("a groan, far off"); }
		if (Cross(2.5f)) { PlayAround(PickFrom(TEXT("Ice_Crack")), 0.82f, 1500.f); SceneLine = TEXT("cracks, coming closer"); }
		if (Cross(4.0f)) { PlayAround(PickFrom(TEXT("Ice_Crack")), 0.96f, 1200.f); }
		if (Cross(5.2f)) { PlayAround(PickFrom(TEXT("Ice_Crack")), 1.2f, 600.f); SceneLine = TEXT("the last warning"); }
		if (Cross(5.8f)) { PlayAround(PickFrom(TEXT("Ice_Groan")), 1.5f, 400.f); }
		if (Cross(7.5f)) { PlayAround(PickFrom(TEXT("Serac_Collapse")), 1.6f, 300.f); SceneLine = TEXT("collapse"); }
		break;
	case EKind::RopeScene:
		if (Cross(0.3f)) { Play2D(PickFrom(TEXT("Carabiner_ClipIn")), RopeVolume * 0.8f); SceneLine = TEXT("clip in"); }
		if (Cross(1.2f))
		{
			for (const FStep& Other : All)
			{
				if (Other.Key == TEXT("Rope_Slide_Loop")) { Play2D(Other.Sound, RopeVolume * 0.35f, 1.04f); }
			}
			SceneLine = TEXT("walking clipped");
		}
		if (Cross(5.2f))
		{
			for (const TWeakObjectPtr<UAudioComponent>& C : Playing)
			{
				if (C.IsValid() && C->Sound && C->Sound->GetName() == TEXT("Rope_Slide_Loop")) { C->FadeOut(0.25f, 0.f); }
			}
		}
		if (Cross(5.8f)) { Play2D(PickFrom(TEXT("Carabiner_Click")), RopeVolume * 0.8f); SceneLine = TEXT("unclip"); }
		break;
	case EKind::WheelScene:
		if (Cross(0.2f) && Sky)
		{
			const int32 Shown = Sky->GetShownIndex();
			Sky->SpinWheelTo(Sky->HasForecast() ? Sky->GetForecastIndex() : (Shown >= 0 && Shown < ARealmSky::NumWeathers() ? Shown : 0));
		}
		break;
	default:
		break;
	}
}

void USoundTourComponent::UpdatePanel()
{
	if (!Panel)
	{
		return;
	}
	if (!Steps.IsValidIndex(Current))
	{
		Panel->SetLines(TEXT("SOUND TOUR"), TEXT("That was every sound"),
			TEXT("Your verdicts are saved in Docs/Development/Sound_Review_InGame.md. Tell Claude when you are done."),
			TEXT("[Left] goes back.  snd 0 closes the tour."), FString());
		return;
	}
	const FStep& S = Steps[Current];
	int32 InGroup = 0, OfGroup = 0;
	for (int32 i = 0; i < Steps.Num(); ++i)
	{
		if (Steps[i].Group == S.Group) { ++OfGroup; if (i <= Current) { ++InGroup; } }
	}
	const bool bScene = S.Key.StartsWith(TEXT("scene:"));
	const FString Header = FString::Printf(TEXT("SOUND TOUR    %d / %d   %s    ·    %s %d / %d"),
		S.Group + 1, Groups.Num(), *Groups[S.Group].Title.ToUpper(), bScene ? TEXT("scene") : TEXT("sound"), InGroup, OfGroup);
	const FString Where = bScene ? S.Hint : FString::Printf(TEXT("%s.\nIn the game: %s."), *S.Hint, *Groups[S.Group].Where);

	FString Status;
	if (S.Kind == EKind::WalkScene)
	{
		Status = TEXT("walk around, then rate the steps");
	}
	else if (S.Kind == EKind::Loop || S.Kind == EKind::WindBed)
	{
		Status = TEXT("looping until you rate it or move on");
	}
	else if (S.Seconds > 0.f && T < S.Seconds)
	{
		Status = FString::Printf(TEXT("playing   %.0f s"), FMath::Max(0.f, S.Seconds - T));
	}
	else
	{
		Status = TEXT("finished: rate it, or [Down] to hear it again");
	}
	if (!SceneLine.IsEmpty() && (S.Seconds <= 0.f || T < S.Seconds))
	{
		Status = SceneLine + TEXT("      ") + Status;
	}

	FString Verdict;
	const TSharedPtr<FJsonObject>* Entry = nullptr;
	if (Review.IsValid() && Review->TryGetObjectField(S.Key, Entry))
	{
		const FString V = Field(*Entry, TEXT("verdict"));
		const FString N = Field(*Entry, TEXT("note"));
		const bool bChanged = !Field(*Entry, TEXT("hash")).IsEmpty() && Field(*Entry, TEXT("hash")) != HashOf(S);
		Verdict = FString::Printf(TEXT("your verdict: %s%s%s"), *V, N.IsEmpty() ? TEXT("") : *(TEXT("  \"") + N + TEXT("\"")), bChanged ? TEXT("  (the sound changed since)") : TEXT(""));
	}
	else
	{
		Verdict = TEXT("not judged yet");
	}
	if (SavedFlash > 0.f && !LastRated.IsEmpty())
	{
		const TSharedPtr<FJsonObject>* Last = nullptr;
		if (Review.IsValid() && Review->TryGetObjectField(LastRated, Last))
		{
			Verdict += FString::Printf(TEXT("\nsaved: %s = %s.  To say why:  snd note <words>"), *LastRatedTitle, *Field(*Last, TEXT("verdict")));
		}
	}
	Panel->SetLines(Header, S.Title, Where, Status, Verdict);
}

// ---------------------------------------------------------------- verdicts

FString USoundTourComponent::HashOf(const FStep& Step) const
{
	auto FileHash = [this](const FString& File) -> FString
	{
		if (File.IsEmpty()) { return FString(); }
		if (const FString* Known = FileHashes.Find(File)) { return *Known; }
		const FString H = LexToString(FMD5Hash::HashFile(*File));
		FileHashes.Add(File, H);
		return H;
	};
	if (Step.Key.StartsWith(TEXT("scene:")))
	{
		// a scene changes when any of its sounds does
		FString Joined;
		for (const FStep& S : All)
		{
			if (S.Group == Step.Group && !S.Key.StartsWith(TEXT("scene:"))) { Joined += FileHash(S.AssetFile); }
		}
		return FMD5::HashAnsiString(*Joined);
	}
	if (Step.Kind == EKind::Landing)
	{
		return FMD5::HashAnsiString(*((Step.Sound ? Step.Sound->GetName() : FString()) + FileHash(Step.AssetFile)));
	}
	return FileHash(Step.AssetFile);
}

bool USoundTourComponent::IsApproved(const FStep& Step) const
{
	const TSharedPtr<FJsonObject>* Entry = nullptr;
	return Review.IsValid() && Review->TryGetObjectField(Step.Key, Entry)
		&& Field(*Entry, TEXT("verdict")) == TEXT("good") && Field(*Entry, TEXT("hash")) == HashOf(Step);
}

void USoundTourComponent::Rate(const TCHAR* Verdict, const FString& Note)
{
	if (!bRunning || !Steps.IsValidIndex(Current))
	{
		return;
	}
	const FStep& S = Steps[Current];
	if (!Review.IsValid()) { Review = MakeShared<FJsonObject>(); }
	TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
	const TSharedPtr<FJsonObject>* Old = nullptr;
	FString KeepNote;
	if (Review->TryGetObjectField(S.Key, Old) && FCString::Strcmp(Verdict, TEXT("good")) != 0)
	{
		KeepNote = Field(*Old, TEXT("note"));   // a new verdict keeps your old note, unless the sound is now good
	}
	Entry->SetStringField(TEXT("verdict"), Verdict);
	Entry->SetStringField(TEXT("note"), Note.IsEmpty() ? KeepNote : Note);
	Entry->SetStringField(TEXT("hash"), HashOf(S));
	Entry->SetStringField(TEXT("when"), FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M")));
	Review->SetObjectField(S.Key, Entry);
	LastRated = S.Key;
	LastRatedTitle = S.Title;
	SavedFlash = 8.f;
	SaveReview();
	UE_LOG(LogTemp, Display, TEXT("snd: %s = %s%s"), *S.Key, Verdict, Note.IsEmpty() ? TEXT("") : *(TEXT(" (") + Note + TEXT(")")));
	Go(1);
}

void USoundTourComponent::AddNote(const FString& Note)
{
	if (!bRunning || Note.IsEmpty())
	{
		return;
	}
	// the note goes to the sound you rated last (B moves on at once); before any rating, to the one playing
	FString Key = LastRated;
	FString Title = LastRatedTitle;
	if (Key.IsEmpty() && Steps.IsValidIndex(Current)) { Key = Steps[Current].Key; Title = Steps[Current].Title; }
	if (Key.IsEmpty()) { return; }
	if (!Review.IsValid()) { Review = MakeShared<FJsonObject>(); }
	const TSharedPtr<FJsonObject>* Entry = nullptr;
	TSharedPtr<FJsonObject> E;
	if (Review->TryGetObjectField(Key, Entry)) { E = *Entry; }
	else
	{
		E = MakeShared<FJsonObject>();
		E->SetStringField(TEXT("verdict"), TEXT("note"));
		for (const FStep& S : All) { if (S.Key == Key) { E->SetStringField(TEXT("hash"), HashOf(S)); } }
	}
	E->SetStringField(TEXT("note"), Note);
	Review->SetObjectField(Key, E);
	LastRated = Key;
	LastRatedTitle = Title;
	SavedFlash = 8.f;
	SaveReview();
	UE_LOG(LogTemp, Display, TEXT("snd: note on %s: %s"), *Key, *Note);
}

void USoundTourComponent::LoadReview()
{
	Review = MakeShared<FJsonObject>();
	FString Text;
	if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectSavedDir() / TEXT("AudioWork/review_ingame.json"))))
	{
		TSharedPtr<FJsonObject> Parsed;
		if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Parsed) && Parsed.IsValid())
		{
			Review = Parsed;
		}
	}
}

void USoundTourComponent::SaveReview() const
{
	if (!Review.IsValid())
	{
		return;
	}
	FString Json;
	FJsonSerializer::Serialize(Review.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
	FFileHelper::SaveStringToFile(Json, *(FPaths::ProjectSavedDir() / TEXT("AudioWork/review_ingame.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	FString Md = TEXT("# Sound review in the game (by ear)\n\n")
		TEXT("Written by the in-game sound tour (console `snd`, Source/YggdrasilsPeak/Survival/SoundTourComponent.cpp). ")
		TEXT("good / bad / unsure + your notes, judged inside the game over the live wind.\n\n");
	for (int32 g = 0; g < Groups.Num(); ++g)
	{
		FString Rows;
		for (const FStep& S : All)
		{
			const TSharedPtr<FJsonObject>* Entry = nullptr;
			if (S.Group == g && Review->TryGetObjectField(S.Key, Entry))
			{
				const FString Note = (*Entry)->HasField(TEXT("note")) ? Field(*Entry, TEXT("note")).Replace(TEXT("|"), TEXT("/")) : FString();
				const bool bChanged = Field(*Entry, TEXT("hash")) != HashOf(S);
				Rows += FString::Printf(TEXT("| %s%s | %s%s | %s |\n"), S.Key.StartsWith(TEXT("scene:")) ? TEXT("(scene) ") : TEXT(""), *S.Title,
					*Field(*Entry, TEXT("verdict")), bChanged ? TEXT(" (sound changed since)") : TEXT(""), *Note);
			}
		}
		if (!Rows.IsEmpty())
		{
			Md += FString::Printf(TEXT("## %d. %s\n_%s_\n\n| Sound | Verdict | Note |\n|---|---|---|\n%s\n"), g + 1, *Groups[g].Title, *Groups[g].Where, *Rows);
		}
	}
	FFileHelper::SaveStringToFile(Md, *(FPaths::ProjectDir() / TEXT("Docs/Development/Sound_Review_InGame.md")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
