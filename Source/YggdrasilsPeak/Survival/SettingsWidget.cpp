// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SettingsWidget.h"

#include "Survival/SnowTrailComponent.h"
#include "Survival/YggUIStyle.h"
#include "Survival/YggMenuWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	const TCHAR* const Section = TEXT("YggdrasilsPeak.Display");
}

static FAutoConsoleCommandWithWorld CmdSettings(TEXT("ygg.Settings"), TEXT("Opens or closes the display settings (same as F2)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (World) { USettingsWidget::Toggle(World->GetFirstPlayerController()); }
	}));

int32 USettingsWidget::GetFootprints()
{
	int32 V = 1;
	GConfig->GetInt(Section, TEXT("Footprints"), V, GGameUserSettingsIni);
	return FMath::Clamp(V, 0, 2);
}

int32 USettingsWidget::GetGraphics()
{
	int32 V = -1;
	GConfig->GetInt(Section, TEXT("Graphics"), V, GGameUserSettingsIni);
	return FMath::Clamp(V, -1, 3);
}

void USettingsWidget::SetFootprints(UWorld* World, int32 Value)
{
	GConfig->SetInt(Section, TEXT("Footprints"), Value, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	USnowTrailComponent::SetFootprintQuality(Value);
}

void USettingsWidget::SetGraphics(UWorld* World, int32 Value)
{
	GConfig->SetInt(Section, TEXT("Graphics"), Value, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	if (GEngine && World)
	{
		GEngine->Exec(World, *(Value < 0 ? FString(TEXT("ygg.Quality auto")) : FString::Printf(TEXT("ygg.Quality %d"), Value)));
	}
}

int32 USettingsWidget::GetUpscaling() { int32 V = 0; GConfig->GetInt(Section, TEXT("Upscaling"), V, GGameUserSettingsIni); return FMath::Clamp(V, 0, 4); }
int32 USettingsWidget::GetFrameLimit() { int32 V = 2; GConfig->GetInt(Section, TEXT("FrameLimit"), V, GGameUserSettingsIni); return FMath::Clamp(V, 0, 2); }
int32 USettingsWidget::GetRayTracing() { int32 V = 1; GConfig->GetInt(Section, TEXT("RayTracing"), V, GGameUserSettingsIni); return FMath::Clamp(V, 0, 1); }

void USettingsWidget::SetInt(const TCHAR* Key, int32 Value)
{
	GConfig->SetInt(Section, Key, Value, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void USettingsWidget::ApplyGraphicsOptions(UWorld* World)
{
	IConsoleManager& CM = IConsoleManager::Get();
	auto Set = [&CM](const TCHAR* Name, float V) { if (IConsoleVariable* C = CM.FindConsoleVariable(Name)) { C->Set(V, ECVF_SetByConsole); } };
	// Upscaling: render fewer pixels and let DLSS (when the NVIDIA plugin is installed) or TSR rebuild the full image.
	// Auto in the packaged game = YggQualitySubsystem dynamic resolution; in the editor that system is off, so Auto
	// means Quality there (Play used to render every pixel natively: the GPU sat at 100%).
	const int32 Up = GetUpscaling();
	static const float Percent[] = { 67.f, 100.f, 67.f, 58.f, 50.f };
	const bool bDLSS = CM.FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")) != nullptr;
	if (Up != 0 || GIsEditor)
	{
		Set(TEXT("r.ScreenPercentage"), Percent[Up]);
	}
	if (bDLSS)
	{
		Set(TEXT("r.NGX.DLSS.Enable"), Up == 1 ? 0.f : 1.f);
	}
	Set(TEXT("r.AntiAliasingMethod"), 4.f);
	static const float Fps[] = { 60.f, 120.f, 0.f };
	Set(TEXT("t.MaxFPS"), Fps[GetFrameLimit()]);
	Set(TEXT("r.Lumen.HardwareRayTracing"), float(GetRayTracing()));
	UE_LOG(LogTemp, Display, TEXT("Settings: upscaling %d (%s, %.0f%%), frame limit %.0f, hardware ray tracing %d"), Up, bDLSS ? TEXT("DLSS") : TEXT("TSR"), Percent[Up], Fps[GetFrameLimit()], GetRayTracing());
}

void USettingsWidget::ApplySaved(UWorld* World)
{
	USnowTrailComponent::SetFootprintQuality(GetFootprints());
	ApplyGraphicsOptions(World);
	if (GetGraphics() >= 0 && GEngine && World)
	{
		GEngine->Exec(World, *FString::Printf(TEXT("ygg.Quality %d"), GetGraphics()));
	}
}

void USettingsWidget::Toggle(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	TArray<UUserWidget*> Open;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, USettingsWidget::StaticClass(), true);
	for (UUserWidget* W : Open)
	{
		Cast<USettingsWidget>(W)->Close();
		return;
	}
	UYggMenuWidget::CloseAll(PC);   // one menu at a time
	if (USettingsWidget* S = CreateWidget<USettingsWidget>(PC, USettingsWidget::StaticClass()))
	{
		S->SetIsFocusable(true);
		S->AddToViewport(60);
		S->SetLoose(true);
	}
}

void USettingsWidget::CloseIfOpen(APlayerController* PC)
{
	if (!PC) { return; }
	TArray<UUserWidget*> Open;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, USettingsWidget::StaticClass(), true);
	for (UUserWidget* W : Open) { Cast<USettingsWidget>(W)->Close(); }
}

TSharedRef<SWidget> USettingsWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Build();
	}
	return Super::RebuildWidget();
}

void USettingsWidget::AddRow(UVerticalBox* Box, const FString& Label, const TArray<TPair<FString, FName>>& Options, int32 Group, const FString& Hint)
{
	using namespace YggUI;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* L = Text(WidgetTree, Label, 9, Faint, 320, TEXT("Regular"));
	L->SetMinDesiredWidth(210.f);
	Row->AddChildToHorizontalBox(L)->SetVerticalAlignment(VAlign_Center);
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		UTextBlock* T = nullptr; UImage* B = nullptr;
		UButton* Btn = Option(WidgetTree, Options[i].Key, 11, T, B);
		FScriptDelegate D;
		D.BindUFunction(this, Options[i].Value);
		Btn->OnClicked.Add(D);
		UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Btn);
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 26.f, 0.f));
		OptButtons.Add(Btn); OptTexts.Add(T); OptBeads.Add(B); OptGroup.Add(Group); OptIndex.Add(i);
	}
	Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, Hint.IsEmpty() ? 20.f : 4.f));
	if (!Hint.IsEmpty())
	{
		Box->AddChildToVerticalBox(Text(WidgetTree, Hint, 11, Faint, 20))->SetPadding(FMargin(210.f, 0.f, 0.f, 20.f));
	}
}

void USettingsWidget::Build()
{
	using namespace YggUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;
	Serif = MakeSerif(this);
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Veil"));
	Veil->SetBrushColor(FLinearColor(0.004f, 0.006f, 0.012f, 0.55f));
	UCanvasPanelSlot* VS = Canvas->AddChildToCanvas(Veil);
	VS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	VS->SetOffsets(FMargin(0.f));

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("DISPLAY"), 10, Dim, 420, TEXT("Regular")))->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("Settings"), 44, Ivory, 0, TEXT("Regular"), Serif))->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	UHorizontalBox* Rule = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Rule->AddChildToHorizontalBox(Line(WidgetTree, 560.f))->SetVerticalAlignment(VAlign_Center);
	Rule->AddChildToHorizontalBox(Bead(WidgetTree, 5.f))->SetVerticalAlignment(VAlign_Center);
	Box->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, 0.f, 0.f, 30.f));

	const bool bDLSS = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")) != nullptr;
	AddRow(Box, TEXT("UPSCALING"), { { TEXT("Auto"), TEXT("OnUpAuto") }, { TEXT("Native"), TEXT("OnUpNative") }, { TEXT("Quality"), TEXT("OnUpQuality") },
		{ TEXT("Balanced"), TEXT("OnUpBalanced") }, { TEXT("Performance"), TEXT("OnUpPerformance") } }, 2,
		bDLSS ? TEXT("NVIDIA DLSS renders fewer pixels and rebuilds the image.") : TEXT("TSR renders fewer pixels and rebuilds the image: the biggest saving."));
	AddRow(Box, TEXT("GRAPHICS"), { { TEXT("Auto"), TEXT("OnGfxAuto") }, { TEXT("Low"), TEXT("OnGfxLow") }, { TEXT("Medium"), TEXT("OnGfxMedium") },
		{ TEXT("High"), TEXT("OnGfxHigh") }, { TEXT("Epic"), TEXT("OnGfxEpic") } }, 1, TEXT("Auto keeps the frame rate steady on this machine."));
	AddRow(Box, TEXT("RAY-TRACED LIGHT"), { { TEXT("Off"), TEXT("OnRtOff") }, { TEXT("On"), TEXT("OnRtOn") } }, 4, TEXT("Off is a little flatter and much lighter on the GPU."));
	AddRow(Box, TEXT("FRAME LIMIT"), { { TEXT("60"), TEXT("OnFps60") }, { TEXT("120"), TEXT("OnFps120") }, { TEXT("Off"), TEXT("OnFpsOff") } }, 3, FString());
	AddRow(Box, TEXT("FOOTPRINTS"), { { TEXT("Off"), TEXT("OnFootOff") }, { TEXT("Normal"), TEXT("OnFootNormal") }, { TEXT("High"), TEXT("OnFootHigh") } }, 0,
		TEXT("Off saves the most. High draws the prints twice as sharp."));

	UTextBlock* CT = nullptr; UImage* CB = nullptr;
	UButton* CloseBtn = Option(WidgetTree, TEXT("Close   ·   F2"), 11, CT, CB);
	CloseBtn->OnClicked.AddDynamic(this, &USettingsWidget::OnCloseClicked);
	OptButtons.Add(CloseBtn); OptTexts.Add(CT); OptBeads.Add(CB); OptGroup.Add(-1); OptIndex.Add(0);
	Box->AddChildToVerticalBox(CloseBtn)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Box);
	S->SetAnchors(FAnchors(0.5f, 0.5f));
	S->SetAlignment(FVector2D(0.5f, 0.5f));
	S->SetAutoSize(true);
}

int32 USettingsWidget::Current(int32 Group)
{
	switch (Group)
	{
	case 0: return GetFootprints();
	case 1: return GetGraphics() + 1;
	case 2: return GetUpscaling();
	case 3: return GetFrameLimit();
	case 4: return GetRayTracing();
	default: return -100;
	}
}

void USettingsWidget::Refresh()
{
}

void USettingsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// the chosen option of each row carries the bead; hovered options brighten
	for (int32 i = 0; i < OptTexts.Num(); ++i)
	{
		const bool bChosen = OptGroup[i] >= 0 && Current(OptGroup[i]) == OptIndex[i];
		const bool bHot = OptButtons[i] && OptButtons[i]->IsHovered();
		if (OptTexts[i]) { OptTexts[i]->SetColorAndOpacity(FSlateColor(bChosen || bHot ? YggUI::Ivory : YggUI::Dim)); }
		if (OptBeads[i]) { OptBeads[i]->SetRenderOpacity(bChosen ? 1.f : (bHot ? 0.45f : 0.f)); }
	}
}

void USettingsWidget::SetLoose(bool bOpen)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	PC->SetShowMouseCursor(bOpen);
	PC->SetIgnoreLookInput(bOpen);
	PC->SetIgnoreMoveInput(bOpen);
	if (bOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		SetKeyboardFocus();
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void USettingsWidget::Close()
{
	SetLoose(false);
	RemoveFromParent();
}

void USettingsWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

FReply USettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::F2)
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USettingsWidget::OnFootOff() { SetFootprints(GetWorld(), 0); Refresh(); }
void USettingsWidget::OnFootNormal() { SetFootprints(GetWorld(), 1); Refresh(); }
void USettingsWidget::OnFootHigh() { SetFootprints(GetWorld(), 2); Refresh(); }
void USettingsWidget::OnGfxAuto() { SetGraphics(GetWorld(), -1); Refresh(); }
void USettingsWidget::OnGfxLow() { SetGraphics(GetWorld(), 0); Refresh(); }
void USettingsWidget::OnGfxMedium() { SetGraphics(GetWorld(), 1); Refresh(); }
void USettingsWidget::OnGfxHigh() { SetGraphics(GetWorld(), 2); Refresh(); }
void USettingsWidget::OnGfxEpic() { SetGraphics(GetWorld(), 3); Refresh(); }
void USettingsWidget::OnCloseClicked() { Close(); }
void USettingsWidget::OnUpAuto() { SetInt(TEXT("Upscaling"), 0); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnUpNative() { SetInt(TEXT("Upscaling"), 1); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnUpQuality() { SetInt(TEXT("Upscaling"), 2); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnUpBalanced() { SetInt(TEXT("Upscaling"), 3); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnUpPerformance() { SetInt(TEXT("Upscaling"), 4); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnFps60() { SetInt(TEXT("FrameLimit"), 0); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnFps120() { SetInt(TEXT("FrameLimit"), 1); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnFpsOff() { SetInt(TEXT("FrameLimit"), 2); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnRtOn() { SetInt(TEXT("RayTracing"), 1); ApplyGraphicsOptions(GetWorld()); Refresh(); }
void USettingsWidget::OnRtOff() { SetInt(TEXT("RayTracing"), 0); ApplyGraphicsOptions(GetWorld()); Refresh(); }
