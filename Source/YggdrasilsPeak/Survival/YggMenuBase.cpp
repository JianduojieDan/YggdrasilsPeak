// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggMenuBase.h"

#include "Survival/YggFateFx.h"
#include "Survival/SurvivalHUDWidget.h"
#include "Survival/YggUIProxy.h"
#include "Survival/YggUIStyle.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** "8,849|m" -> "8,849 m", "2|h  14|m" -> "2 h 14 m" (v2's numeral markup, read as plain words). */
	FString MenuPlain(const FString& S)
	{
		FString R = S.Replace(TEXT("|"), TEXT(" "));
		while (R.Contains(TEXT("  "))) { R.ReplaceInline(TEXT("  "), TEXT(" ")); }
		return R.TrimStartAndEnd();
	}
}

FLinearColor UYggMenuBase::Accent() const
{
	if (Page.AccentOverride.A > 0.f) { return Page.AccentOverride; }
	return Page.Tone == ETone::Death ? YggUI::EmberC : YggUI::Sun;
}

void UYggMenuBase::SetPage(const FPage& InPage)
{
	Page = InPage;
	Selected = FMath::Clamp(Page.Selected, 0, FMath::Max(Page.Options.Num() - 1, 0));
	PageAge = 0.f;
	if (bBuilt)
	{
		BuildColumn();
	}
}

TSharedRef<SWidget> UYggMenuBase::RebuildWidget()
{
	if (!bBuilt)
	{
		BuildFrame();
		BuildColumn();
		bBuilt = true;
	}
	return Super::RebuildWidget();
}

void UYggMenuBase::BuildFrame()
{
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;
	Serif = YggUI::SerifFont();
	Hand = YggUI::MakeHand(this);
	BuildBackground(Canvas);
	// on the mountain the world behind the menu goes soft (the bright snow fought with the words); the start menu's
	// own pictures stay sharp. A death page's world is changed by M_PP_Fate instead (no blur over it).
	if (!bUIOnlyInput)
	{
		Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass());
		Blur->SetBlurStrength(0.f);
		Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(Blur);
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}
	UMaterialInstanceDynamic* MID = nullptr;
	YggUI::CloudVeil(WidgetTree, Canvas, this, VeilAlpha, 0.2f, MID);
	VeilMID = MID;
	// the summit's warm light from the upper right
	ToneGlow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	ToneGlow->SetBrush(YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f)));
	ToneGlow->SetColorAndOpacity(YggUI::WithA(YggUI::Hex(255, 176, 110), 0.42f));
	ToneGlow->SetVisibility(ESlateVisibility::Collapsed);
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(ToneGlow);
		S->SetAnchors(FAnchors(0.8f, 0.1f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetSize(FVector2D(2000.f, 1500.f));
	}
	// a darkening behind a question
	{
		DimLayer = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::Box;
		B.TintColor = FSlateColor(FLinearColor::White);
		DimLayer->SetBrush(B);
		DimLayer->SetColorAndOpacity(YggUI::InkC);
		DimLayer->SetRenderOpacity(0.f);
		DimLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(DimLayer);
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

	Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	ColumnSlot = Canvas->AddChildToCanvas(Column);
	ColumnSlot->SetAutoSize(true);

	SidePanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Side"));
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(SidePanel);
		S->SetAnchors(FAnchors(1.f - ColumnX, 0.5f));
		S->SetAlignment(FVector2D(1.f, 0.5f));
		S->SetAutoSize(true);
	}
	Roll = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Roll"));
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Roll);
		S->SetAnchors(FAnchors(0.66f, 0.f));
		S->SetAlignment(FVector2D(0.5f, 0.f));
		S->SetAutoSize(true);
	}
	for (int32 k = 0; k < 2; ++k)
	{
		UImage* M = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		M->SetBrush(YggUI::TextureBrush(YggUI::FadeTexture(), FVector2D(4.f, 64.f)));
		M->SetColorAndOpacity(YggUI::WithA(YggUI::InkC, 0.85f));
		M->SetVisibility(ESlateVisibility::Collapsed);
		if (k == 1) { M->SetRenderTransformAngle(180.f); }
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(M);
		S->SetAnchors(k == 0 ? FAnchors(0.f, 0.f, 1.f, 0.f) : FAnchors(0.f, 1.f, 1.f, 1.f));
		S->SetOffsets(k == 0 ? FMargin(0.f, 0.f, 0.f, 320.f) : FMargin(0.f, -320.f, 0.f, 320.f));
		RollMasks.Add(M);
	}
	SetRenderOpacity(0.f);
}

void UYggMenuBase::BuildColumn()
{
	Column->ClearChildren();
	OptButtons.Reset(); OptTexts.Reset(); OptSubs.Reset(); OptGlows.Reset(); Proxies.Reset(); OptHot.Reset();
	SplashPrompt = nullptr;
	const bool bCentre = Page.bModal || Page.bSplash;
	if (ColumnSlot)
	{
		ColumnSlot->SetAnchors(bCentre ? FAnchors(0.5f, 0.5f) : FAnchors(ColumnX, 0.5f));
		ColumnSlot->SetAlignment(bCentre ? FVector2D(0.5f, 0.5f) : FVector2D(0.f, 0.5f));
		ColumnSlot->SetPosition(FVector2D(0.f, bCentre ? -20.f : 30.f));
	}
	const EHorizontalAlignment HA = bCentre ? HAlign_Center : HAlign_Left;
	auto Add = [this, HA](UWidget* W, float Gap)
	{
		UVerticalBoxSlot* S = Column->AddChildToVerticalBox(W);
		S->SetHorizontalAlignment(HA);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, Gap));
	};
	auto Centre = [bCentre](UTextBlock* T) { if (bCentre) { T->SetJustification(ETextJustify::Center); } return T; };

	if (!Page.Eyebrow.IsEmpty()) { Add(Centre(YggUI::Words(WidgetTree, Page.Eyebrow, 20, YggUI::Mist)), 2.f); }
	TitleLetters.Reset();
	TitleBox = nullptr;
	const float TitleGap = Page.bSplash ? 10.f : 16.f;
	if (Page.Fate != YggFate::None)
	{
		// the title letter by letter, so each can shiver, fall, sink or breathe
		TitleBox = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const bool bHand = Page.Fate == YggFate::Lost && Hand;
		for (int32 c = 0; c < Page.Title.Len(); ++c)
		{
			UTextBlock* L = YggUI::Text(WidgetTree, FString::Chr(Page.Title[c]), int32(Page.TitleSize * (bHand ? 1.2f : 1.f)), YggUI::Snow, 0, TEXT("Regular"), bHand ? Hand.Get() : Serif.Get());
			L->SetRenderTransformPivot(FVector2D(0.5f, 0.75f));
			L->SetRenderOpacity(0.f);
			Row->AddChildToHorizontalBox(L)->SetVerticalAlignment(VAlign_Bottom);
			TitleLetters.Add(L);
		}
		TitleBox->AddChildToOverlay(Row);
		Add(TitleBox, TitleGap);
	}
	else
	{
		Add(Centre(YggUI::Words(WidgetTree, Page.Title, int32(Page.TitleSize), YggUI::Snow)), TitleGap);
	}
	if (!Page.Line.IsEmpty())
	{
		UTextBlock* L = Centre(YggUI::Words(WidgetTree, Page.Line, 24, YggUI::Mist));
		L->SetWrapTextAt(700.f);   // a fixed measure: auto wrap followed the narrow column and broke the line early
		Add(L, Page.Facts.IsEmpty() ? 46.f : 8.f);
	}
	if (!Page.Facts.IsEmpty())
	{
		Add(Centre(YggUI::Words(WidgetTree, Page.Facts, 19, YggUI::Shade)), 46.f);
	}
	if (Page.Body.Num() > 0 && !Page.bRoll)
	{
		for (int32 i = 0; i < Page.Body.Num(); ++i)
		{
			const FString& L = Page.Body[i];
			const bool bHead = L.StartsWith(TEXT("#"));
			UTextBlock* T = bHead ? YggUI::Words(WidgetTree, L.Mid(1), 16, YggUI::Shade) : YggUI::Words(WidgetTree, L, 19, YggUI::Snow);
			const bool bNextHead = i + 1 < Page.Body.Num() && Page.Body[i + 1].StartsWith(TEXT("#"));
			Add(T, bHead ? 2.f : (bNextHead ? 18.f : 1.f));
		}
		Add(YggUI::Line(WidgetTree, 1.f, FLinearColor::Transparent, 20.f), 0.f);
	}

	// the options: words; the chosen one is lit by the sun (a glow behind it), the rest stand in shadow
	UHorizontalBox* Across = nullptr;
	if (Page.bModal)
	{
		Across = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Add(Across, 0.f);
	}
	const FLinearColor Acc = Accent();
	for (int32 i = 0; i < Page.Options.Num(); ++i)
	{
		UButton* B = YggUI::Bare(WidgetTree);
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SB->SetWidthOverride(Page.bModal ? 260.f : 560.f);
		UOverlay* O = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Glow = YggUI::Glow(WidgetTree, YggUI::WithA(FMath::Lerp(YggUI::Sun, Acc, 0.5f), Page.bModal ? 0.38f : 0.34f), Page.bModal ? FVector2D(340.f, 120.f) : FVector2D(540.f, 130.f));
		Glow->SetRenderOpacity(0.f);
		UOverlaySlot* GS = O->AddChildToOverlay(Glow);
		GS->SetHorizontalAlignment(Page.bModal ? HAlign_Center : HAlign_Left);
		GS->SetVerticalAlignment(VAlign_Center);
		UTextBlock* T = YggUI::Words(WidgetTree, Page.Options[i].Label, Page.bModal ? 27 : 31, YggUI::Mist);
		UTextBlock* Sub = Page.Options[i].Sub.IsEmpty() ? nullptr : YggUI::Words(WidgetTree, Page.Options[i].Sub, 16, YggUI::Shade);
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		V->AddChildToVerticalBox(T)->SetHorizontalAlignment(Page.bModal ? HAlign_Center : HAlign_Left);
		if (Sub) { V->AddChildToVerticalBox(Sub)->SetPadding(FMargin(1.f, 0.f, 0.f, 0.f)); }
		UOverlaySlot* VS = O->AddChildToOverlay(V);
		VS->SetHorizontalAlignment(Page.bModal ? HAlign_Center : HAlign_Left);
		VS->SetVerticalAlignment(VAlign_Center);
		VS->SetPadding(FMargin(Page.bModal ? 0.f : 30.f, Sub ? 8.f : 10.f, 0.f, Sub ? 8.f : 10.f));
		SB->SetContent(O);
		B->SetContent(SB);
		UYggUIProxy* P = NewObject<UYggUIProxy>(this);
		P->OnClick = [this, i]() { Activate(i); };
		FScriptDelegate D;
		D.BindUFunction(P, TEXT("Click"));
		B->OnClicked.Add(D);
		Proxies.Add(P);
		OptButtons.Add(B); OptTexts.Add(T); OptSubs.Add(Sub); OptGlows.Add(Glow); OptHot.Add(i == Selected ? 1.f : 0.f);
		if (Across)
		{
			Across->AddChildToHorizontalBox(B)->SetPadding(FMargin(i == 0 ? 0.f : 24.f, 0.f, 0.f, 0.f));
		}
		else
		{
			// the column's left edge is the words' edge: the option's own inset moves back out
			UVerticalBoxSlot* S = Column->AddChildToVerticalBox(B);
			S->SetHorizontalAlignment(HAlign_Left);
			S->SetPadding(FMargin(-30.f, 0.f, 0.f, 0.f));
		}
	}
	if (Page.bSplash)
	{
		SplashPrompt = YggUI::Words(WidgetTree, TEXT("Press any key"), 20, YggUI::Mist);
		SplashPrompt->SetRenderOpacity(0.f);
		UVerticalBoxSlot* S = Column->AddChildToVerticalBox(SplashPrompt);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 84.f, 0.f, 0.f));
	}
	Column->SetRenderOpacity(0.f);
	// the far side's shadow deepens under a side panel or the credits (the world is brightest there)
	if (VeilMID) { VeilMID->SetScalarParameterValue(TEXT("Right"), (Page.Side.Num() > 0 || Page.bRoll) ? 0.55f : (bCentre ? 0.45f : 0.18f)); }
	if (ToneGlow) { ToneGlow->SetVisibility(Page.Tone == ETone::Summit ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	BuildSide();
	BuildRoll();
	BuildPrompts();
	UpdateFateWorld();
}

void UYggMenuBase::BuildSide()
{
	SidePanel->ClearChildren();
	SideItems.Reset();
	const FLinearColor Acc = Accent();
	auto AddRight = [this](UWidget* W, float Gap, float RightPad = 0.f)
	{
		UVerticalBoxSlot* S = SidePanel->AddChildToVerticalBox(W);
		S->SetHorizontalAlignment(HAlign_Right);
		S->SetPadding(FMargin(0.f, 0.f, RightPad, Gap));
		SideItems.Add(W);
	};
	for (int32 i = 0; i < Page.Side.Num(); ++i)
	{
		const FString& L = Page.Side[i];
		if (L.StartsWith(TEXT("#"))) { AddRight(YggUI::Words(WidgetTree, L.Mid(1), 17, YggUI::Shade), 12.f); }
		else if (L.StartsWith(TEXT("="))) { AddRight(YggUI::Words(WidgetTree, MenuPlain(L.Mid(1)), 46, YggUI::Snow), 0.f); }
		else if (L.StartsWith(TEXT("~"))) { AddRight(YggUI::Words(WidgetTree, L.Mid(1), 17, YggUI::Mist), 30.f); }
		else if (L == TEXT("-")) { AddRight(YggUI::Line(WidgetTree, 1.f, FLinearColor::Transparent, 22.f), 0.f); }
		else if (L.StartsWith(TEXT("@")))
		{
			// a camp on the thread of the route: name and height, a point of light where you are
			TArray<FString> F;
			L.Mid(1).ParseIntoArray(F, TEXT("|"), false);
			const int32 State = F.Num() > 2 ? FCString::Atoi(*F[2]) : 0;
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			const FLinearColor NameC = State == 2 ? YggUI::Snow : State == 1 ? YggUI::Mist : YggUI::Shade;
			UHorizontalBoxSlot* NS = Row->AddChildToHorizontalBox(YggUI::Words(WidgetTree, F.Num() > 0 ? F[0] : FString(), State == 2 ? 21 : 18, NameC));
			NS->SetVerticalAlignment(VAlign_Center);
			NS->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
			UHorizontalBoxSlot* AS = Row->AddChildToHorizontalBox(YggUI::Words(WidgetTree, F.Num() > 1 ? F[1] : FString(), 15, State == 2 ? YggUI::Mist : YggUI::Shade));
			AS->SetVerticalAlignment(VAlign_Center);
			AS->SetPadding(FMargin(0.f, 2.f, 16.f, 0.f));
			USizeBox* BB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			BB->SetWidthOverride(14.f);
			BB->SetHeightOverride(14.f);
			UOverlay* Dot = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			if (State == 2)
			{
				UImage* G = YggUI::Glow(WidgetTree, YggUI::WithA(Acc, 0.55f), FVector2D(34.f, 34.f));
				UOverlaySlot* GS = Dot->AddChildToOverlay(G);
				GS->SetHorizontalAlignment(HAlign_Center);
				GS->SetVerticalAlignment(VAlign_Center);
			}
			UImage* Bd = YggUI::Bead(WidgetTree, State == 2 ? 8.f : 5.f, State == 2 ? Acc : State == 1 ? YggUI::Mist : YggUI::WithA(YggUI::Shade, 0.7f));
			UOverlaySlot* DS = Dot->AddChildToOverlay(Bd);
			DS->SetHorizontalAlignment(HAlign_Center);
			DS->SetVerticalAlignment(VAlign_Center);
			if (USizeBoxSlot* BS = Cast<USizeBoxSlot>(BB->SetContent(Dot))) { BS->SetHorizontalAlignment(HAlign_Center); BS->SetVerticalAlignment(VAlign_Center); }
			Row->AddChildToHorizontalBox(BB)->SetVerticalAlignment(VAlign_Center);
			const bool bNext = i + 1 < Page.Side.Num() && Page.Side[i + 1].StartsWith(TEXT("@"));
			AddRight(Row, 0.f);
			if (bNext)
			{
				// the thread between two camps (lit up to where you are)
				AddRight(YggUI::Line(WidgetTree, 1.f, YggUI::WithA(YggUI::Mist, State >= 1 ? 0.4f : 0.16f), 14.f), 0.f, 6.5f);
			}
			else
			{
				AddRight(YggUI::Line(WidgetTree, 1.f, FLinearColor::Transparent, 28.f), 0.f);
			}
		}
		else
		{
			UTextBlock* T = YggUI::Words(WidgetTree, L, 18, YggUI::Mist);
			T->SetJustification(ETextJustify::Right);
			T->SetWrapTextAt(380.f);
			AddRight(T, 20.f);
		}
	}
	SidePanel->SetRenderOpacity(0.f);
}

void UYggMenuBase::BuildRoll()
{
	Roll->ClearChildren();
	RollY = 0.f;
	bRollStarted = false;
	const bool bOn = Page.bRoll && Page.Body.Num() > 0;
	Roll->SetVisibility(bOn ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	for (UImage* M : RollMasks) { if (M) { M->SetVisibility(bOn ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); } }
	if (!bOn) { return; }
	auto Add = [this](UWidget* W, float Gap)
	{
		UVerticalBoxSlot* S = Roll->AddChildToVerticalBox(W);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, Gap));
	};
	Add(YggUI::Words(WidgetTree, TEXT("Yggdrasil's Peak"), 54, YggUI::Snow), 4.f);
	Add(YggUI::Words(WidgetTree, TEXT("A climb beneath the World Tree"), 19, YggUI::Mist), 100.f);
	for (int32 i = 0; i < Page.Body.Num(); ++i)
	{
		const FString& L = Page.Body[i];
		if (L.StartsWith(TEXT("#")))
		{
			Add(YggUI::Words(WidgetTree, L.Mid(1), 17, YggUI::WithA(YggUI::Sun, 0.9f)), 8.f);
		}
		else
		{
			UTextBlock* T = YggUI::Words(WidgetTree, L, 23, YggUI::Snow);
			T->SetJustification(ETextJustify::Center);
			T->SetWrapTextAt(640.f);
			const bool bNextHead = i + 1 >= Page.Body.Num() || Page.Body[i + 1].StartsWith(TEXT("#"));
			Add(T, bNextHead ? 66.f : 6.f);
		}
	}
	Add(YggUI::Line(WidgetTree, 1.f, FLinearColor::Transparent, 70.f), 0.f);
	Add(YggUI::Words(WidgetTree, TEXT("Thank you for climbing."), 28, YggUI::Snow), 0.f);
}

void UYggMenuBase::BuildPrompts()
{
	if (PromptRow) { FadeWithWords.Remove(PromptRow); PromptRow->RemoveFromParent(); PromptRow = nullptr; }
	if (PadRow) { FadeWithWords.Remove(PadRow); PadRow->RemoveFromParent(); PadRow = nullptr; }
	if (!bShowHints || Page.bSplash) { return; }
	TArray<TPair<FString, FString>> Items;
	if (Page.Options.Num() > 1) { Items.Add({ Page.bModal ? TEXT("← →") : TEXT("↑ ↓"), TEXT("Choose") }); }
	if (Page.Options.Num() > 0) { Items.Add({ TEXT("Enter"), TEXT("Confirm") }); }
	if (Page.Back) { Items.Add({ TEXT("Esc"), TEXT("Back") }); }
	// the same prompts as gamepad buttons, shown while a pad is in use
	TArray<TPair<FString, FString>> Pad;
	for (const TPair<FString, FString>& It : Items)
	{
		const FString K = It.Key == TEXT("Enter") ? TEXT("A") : It.Key == TEXT("Esc") ? TEXT("B") : It.Key == TEXT("Q E") ? TEXT("LB RB") : TEXT("D-pad");
		Pad.Add({ K, It.Value });
	}
	for (int32 k = 0; k < 2; ++k)
	{
		UHorizontalBox* Row = YggUI::Prompts(WidgetTree, k == 0 ? Items : Pad);
		Row->SetVisibility(k == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Row);
		S->SetAnchors(FAnchors(0.95f, 1.f));
		S->SetAlignment(FVector2D(1.f, 0.5f));
		S->SetPosition(FVector2D(0.f, -54.f));
		S->SetAutoSize(true);
		S->SetZOrder(5);
		FadeWithWords.Add(Row);
		(k == 0 ? PromptRow : PadRow) = Row;
	}
}

void UYggMenuBase::UpdateFateWorld()
{
	UWorld* World = GetWorld();
	if (Page.Fate == YggFate::None || !World)
	{
		RemoveFateWorld();
		return;
	}
	if (!FateVolume.IsValid())
	{
		UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/Materials/M_PP_Fate.M_PP_Fate"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!M) { return; }
		FActorSpawnParameters P;
		P.ObjectFlags |= RF_Transient;
		APostProcessVolume* V = World->SpawnActor<APostProcessVolume>(P);
		if (!V) { return; }
		V->bUnbound = true;
		V->Priority = 100000.f;
		V->BlendWeight = 1.f;
		FateMID = UMaterialInstanceDynamic::Create(M, V);
		V->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, FateMID));
		FateVolume = V;
	}
	if (FateMID)
	{
		FateMID->SetScalarParameterValue(TEXT("Fate"), float(Page.Fate));
		FateMID->SetScalarParameterValue(TEXT("T"), 0.f);
		FateMID->SetScalarParameterValue(TEXT("Amount"), 0.f);
	}
}

void UYggMenuBase::RemoveFateWorld()
{
	if (FateVolume.IsValid()) { FateVolume->Destroy(); }
	FateVolume = nullptr;
	FateMID = nullptr;
}

void UYggMenuBase::NativeDestruct()
{
	RemoveFateWorld();
	Super::NativeDestruct();
}

void UYggMenuBase::SetColumnVisible(bool bVisible)
{
	// the words fade out slowly (and come back the same way); only the picture stays under the settings
	WordsTarget = bVisible ? 1.f : 0.f;
	if (Column && bVisible) { Column->SetVisibility(ESlateVisibility::Visible); }
}

void UYggMenuBase::SetHudVisible(APlayerController* PC, bool bVisible)
{
	static TMap<TWeakObjectPtr<UUserWidget>, ESlateVisibility> Was;
	if (!PC) { return; }
	TArray<UUserWidget*> Huds;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Huds, USurvivalHUDWidget::StaticClass(), true);
	for (UUserWidget* H : Huds)
	{
		if (!bVisible)
		{
			if (H->GetVisibility() != ESlateVisibility::Hidden) { Was.Add(H, H->GetVisibility()); H->SetVisibility(ESlateVisibility::Hidden); }
		}
		else if (H->GetVisibility() == ESlateVisibility::Hidden)
		{
			const ESlateVisibility* V = Was.Find(H);
			H->SetVisibility(V ? *V : ESlateVisibility::SelfHitTestInvisible);
		}
	}
	for (auto It = Was.CreateIterator(); It; ++It) { if (!It.Key().IsValid()) { It.RemoveCurrent(); } }
}

void UYggMenuBase::Open(APlayerController* PC, int32 ZOrder, bool bUIOnly)
{
	if (!PC)
	{
		return;
	}
	SetHudVisible(PC, false);
	bUIOnlyInput = bUIOnly;
	SetIsFocusable(true);
	AddToViewport(ZOrder);
	PC->SetShowMouseCursor(true);
	// the controller counts these: lock once, however many menus open on top of each other
	if (!PC->IsLookInputIgnored()) { PC->SetIgnoreLookInput(true); }
	if (!PC->IsMoveInputIgnored()) { PC->SetIgnoreMoveInput(true); }
	if (bUIOnly)
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
	else
	{
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	SetKeyboardFocus();
}

void UYggMenuBase::CloseMenu()
{
	if (bClosing) { return; }
	OnClosed();
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (!bUIOnlyInput)
		{
			PC->SetShowMouseCursor(false);
			PC->ResetIgnoreLookInput();
			PC->ResetIgnoreMoveInput();
			PC->SetInputMode(FInputModeGameOnly());
			SetHudVisible(PC, true);
		}
	}
	// a short fade instead of vanishing; it takes no input meanwhile
	bClosing = true;
	CloseT = 0.f;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UYggMenuBase::Activate(int32 Index)
{
	if (!Page.Options.IsValidIndex(Index) || PageAge < 0.15f)
	{
		return;
	}
	Selected = Index;
	TFunction<void()> Do = Page.Options[Index].Do;   // copy: the action may change the page
	if (Do) { Do(); }
}

void UYggMenuBase::Move(int32 Dir)
{
	if (Page.Options.Num() == 0) { return; }
	Selected = (Selected + Dir + Page.Options.Num()) % Page.Options.Num();
	bMouseMoved = false;
}

void UYggMenuBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	PageAge += InDeltaTime;
	const FVector2D View = MyGeometry.GetLocalSize();
	const bool bFate = Page.Fate != YggFate::None;
	// the menu comes in: the shadow first, then the words (a death slower)
	SetRenderOpacity(YggUI::Ease(Age * (bFate ? 0.9f : 2.4f)));
	YggUI::TickVeil(VeilMID, Age, View);
	Words = FMath::FInterpConstantTo(Words, WordsTarget, InDeltaTime, 1.6f);   // ~0.6 s to fade
	const float W = YggUI::Ease(Words);
	const bool bCentre = Page.bModal || Page.bSplash;
	if (Column)
	{
		const float C = YggUI::Ease((PageAge - (bFate ? 0.5f : 0.f)) * (Page.bSplash ? 0.8f : 2.4f));
		Column->SetRenderOpacity(C * W);
		Column->SetRenderTranslation(bCentre ? FVector2D(0.f, 12.f * (1.f - C)) : FVector2D(-20.f * (1.f - C) - 18.f * (1.f - W), 0.f));
		if (WordsTarget <= 0.f && Words <= 0.001f && Column->GetVisibility() != ESlateVisibility::Hidden) { Column->SetVisibility(ESlateVisibility::Hidden); }
	}
	if (SidePanel)
	{
		const float C = YggUI::Ease((PageAge - 0.3f) * 2.f);
		SidePanel->SetRenderOpacity(C * W);
		SidePanel->SetRenderTranslation(FVector2D(18.f * (1.f - C), 0.f));
	}
	if (DimLayer)
	{
		DimNow = FMath::FInterpTo(DimNow, Page.bModal ? 0.55f : 0.f, InDeltaTime, 6.f);
		DimLayer->SetRenderOpacity(DimNow);
	}
	if (ToneGlow && Page.Tone == ETone::Summit) { ToneGlow->SetRenderOpacity(YggUI::Ease(PageAge * 0.5f) * (0.85f + 0.15f * FMath::Sin(Age * 0.6f))); }
	if (Roll && Page.bRoll)
	{
		const float H = Roll->GetDesiredSize().Y;
		// (a flag, not a negative position: the roll's top is above the screen most of the time)
		if (!bRollStarted && H > 1.f) { RollY = View.Y * 0.55f; bRollStarted = true; }
		RollY -= 34.f * InDeltaTime;
		if (RollY + H < 0.f) { RollY = View.Y; }
		Roll->SetRenderTranslation(FVector2D(0.f, RollY));
		Roll->SetRenderOpacity(YggUI::Ease(PageAge * 1.5f) * W);
	}
	for (UWidget* X : FadeWithWords) { if (X) { X->SetRenderOpacity(W); } }
	if (bMouseMoved)
	{
		for (int32 i = 0; i < OptButtons.Num(); ++i) { if (OptButtons[i] && OptButtons[i]->IsHovered()) { Selected = i; } }
	}
	// the lit word: warm snow-white, a soft glow that breathes and drifts a little, like light through moving cloud
	const float Drift = FMath::Sin(Age * 0.45f) * 10.f;
	const float Breathe = 0.86f + 0.14f * FMath::Sin(Age * 0.8f + 1.3f);
	for (int32 i = 0; i < OptTexts.Num(); ++i)
	{
		OptHot[i] = FMath::FInterpTo(OptHot[i], i == Selected ? 1.f : 0.f, InDeltaTime, 12.f);
		const float H = OptHot[i];
		const float In = YggUI::Ease((PageAge - (bFate ? 0.9f : 0.18f) - 0.07f * i) * 4.f);
		if (OptButtons[i])
		{
			OptButtons[i]->SetRenderOpacity(In);
			OptButtons[i]->SetRenderTranslation(Page.bModal ? FVector2D(0.f, 8.f * (1.f - In)) : FVector2D(8.f * H - 12.f * (1.f - In), 0.f));
		}
		if (OptTexts[i]) { OptTexts[i]->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Mist, YggUI::Snow, H))); }
		if (OptSubs[i]) { OptSubs[i]->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Shade, YggUI::Mist, H))); }
		if (OptGlows[i])
		{
			OptGlows[i]->SetRenderOpacity(H * Breathe);
			OptGlows[i]->SetRenderTranslation(FVector2D(Page.bModal ? 0.f : Drift + 215.f, 0.f));   // the 8 px glow sits at the left: its light centres on the word
		}
	}
	if (TitleLetters.Num() > 0) { AnimateFate(); }
	if (SplashPrompt)
	{
		SplashPrompt->SetRenderOpacity(YggUI::Ease((PageAge - 2.f) * 1.2f) * (0.45f + 0.55f * (0.5f + 0.5f * FMath::Sin(Age * 1.8f))));
	}
	if (PromptRow && PadRow)
	{
		PromptRow->SetVisibility(bPad ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		PadRow->SetVisibility(bPad ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const float BlurTarget = bFate ? 0.f : 7.f;
	if (Blur) { Blur->SetBlurStrength(BlurTarget * YggUI::Ease(Age * 2.5f)); }
	float Fade = 1.f;
	if (bClosing)
	{
		CloseT += InDeltaTime;
		Fade = 1.f - YggUI::Ease(CloseT / 0.22f);
		SetRenderOpacity(Fade);
		if (Blur) { Blur->SetBlurStrength(BlurTarget * Fade); }
	}
	if (FateMID)
	{
		FateMID->SetScalarParameterValue(TEXT("T"), PageAge);
		FateMID->SetScalarParameterValue(TEXT("Amount"), YggUI::Ease(PageAge * 0.8f) * Fade);
		FateMID->SetScalarParameterValue(TEXT("Aspect"), View.Y > 1.f ? float(View.X / View.Y) : 1.7778f);
	}
	if (bClosing && CloseT > 0.24f)
	{
		RemoveFateWorld();
		RemoveFromParent();
	}
}

namespace
{
	float FateBounce(float K)
	{
		K = FMath::Clamp(K, 0.f, 1.f);
		const float N = 7.5625f, D = 2.75f;
		if (K < 1.f / D) { return N * K * K; }
		if (K < 2.f / D) { K -= 1.5f / D; return N * K * K + 0.75f; }
		if (K < 2.5f / D) { K -= 2.25f / D; return N * K * K + 0.9375f; }
		K -= 2.625f / D; return N * K * K + 0.984375f;
	}
}

void UYggMenuBase::AnimateFate()
{
	// the words come in after the world has begun to turn
	const float t = PageAge - 0.5f;
	const int32 N = TitleLetters.Num();
	for (int32 i = 0; i < N; ++i)
	{
		UTextBlock* L = TitleLetters[i];
		if (!L) { continue; }
		FRandomStream R(i * 7919 + 13);
		const float r1 = R.FRandRange(-1.f, 1.f), r2 = R.FRandRange(-1.f, 1.f), r3 = R.FRand();
		const float In = YggUI::Ease((t - 0.06f * i) * 2.5f);
		FVector2D Off(0.f, 0.f);
		float Ang = 0.f, Op = In, Sc = 1.f;
		FLinearColor C = YggUI::Snow;
		switch (Page.Fate)
		{
		case YggFate::Frozen:
		{
			// a shiver that stops as the cold takes each letter, left to right, into the blue of ice
			const float Amp = 2.6f * (1.f - YggUI::Ease((t - 0.3f) / 2.2f));
			Off = FVector2D(FMath::Sin(t * 47.f + i * 3.1f) * Amp, FMath::Cos(t * 53.f + i * 4.7f) * Amp * 0.6f);
			C = FMath::Lerp(YggUI::Snow, YggUI::Ice, YggUI::Ease((t - 0.6f - 0.12f * i) / 1.6f));
			break;
		}
		case YggFate::Breathless:
		{
			const float B = YggFate::Breath(t - 0.6f);
			const float Drop = FMath::Pow(FMath::Max(0.f, FMath::Sin(t * 0.83f + i * 1.7f)), 10.f);
			Op = In * (0.4f + 0.6f * B) * (1.f - 0.7f * Drop);
			Sc = 1.f + 0.035f * B;
			Off.X = (i - (N - 1) * 0.5f) * 4.f * YggUI::Ease(t / 7.f);
			C = FMath::Lerp(YggUI::Snow, YggUI::Hex(176, 186, 236), 0.45f);
			break;
		}
		case YggFate::Fallen:
		{
			// each letter drops from above, bounces, and lands a little crooked
			const float D = 0.2f + 0.075f * i;
			const float K = FMath::Clamp((t - D) / 0.6f, 0.f, 1.f);
			Off.Y = -180.f * (1.f - FateBounce(K)) + (r2 * 0.5f + 0.5f) * 10.f * K;
			Ang = r1 * 9.f * YggUI::Ease(K * 1.4f);
			Op = YggUI::Ease(K * 6.f);
			break;
		}
		case YggFate::Buried:
		{
			// the letters settle and sink a little under the weight, going the colour of packed snow
			const float S = YggUI::Ease((t - 1.0f - 0.05f * i) / 3.2f);
			Off.Y = 20.f * S * (0.7f + 0.3f * r3);
			Ang = r1 * 3.f * S;
			C = FMath::Lerp(YggUI::Snow, YggUI::Hex(214, 224, 238), 0.6f * S);
			break;
		}
		case YggFate::Parched:
			Off = FVector2D(FMath::Sin(t * 1.9f + i * 1.3f) * 1.f, FMath::Sin(t * 2.7f + i * 0.8f) * 2.f);
			C = FMath::Lerp(YggUI::Snow, YggUI::Hex(232, 196, 148), YggUI::Ease((t - 0.4f) / 1.5f));
			break;
		case YggFate::Starved:
		{
			const float Wv = 0.5f + 0.5f * FMath::Sin(t * 0.7f + i * 2.3f);
			Op = In * (0.35f + 0.65f * Wv);
			Off.Y = 6.f * YggUI::Ease(t / 6.f) * (i % 2 ? 1.f : 0.4f);
			C = FMath::Lerp(YggUI::Snow, YggUI::Hex(190, 188, 182), 0.7f);
			break;
		}
		case YggFate::Lost:
		{
			// written by hand, a letter at a time, then the word comes apart in the whiteout
			const float Wr = YggUI::Ease((t - 0.4f - 0.11f * i) * 5.f);
			const float Dr = YggUI::Ease((t - 3.6f) / 9.f);
			Off = FVector2D((i - (N - 1) * 0.5f) * 7.f * Dr, FMath::Sin(i * 1.9f + 0.5f) * 7.f * Dr);
			Ang = r1 * 4.f * Dr;
			Op = Wr * (1.f - (i % 2 ? 0.45f * YggUI::Ease((t - 4.5f) / 8.f) : 0.f));
			break;
		}
		default:
			break;
		}
		L->SetRenderTranslation(Off);
		L->SetRenderTransformAngle(Ang);
		L->SetRenderOpacity(Op);
		L->SetRenderScale(FVector2D(Sc, Sc));
		L->SetColorAndOpacity(FSlateColor(C));
	}
	// the jolt when the fallen word hits
	if (Page.Fate == YggFate::Fallen && Column && t > 0.9f && t < 1.35f)
	{
		const float Amp = 7.f * (1.f - (t - 0.9f) / 0.45f);
		Column->SetRenderTranslation(Column->GetRenderTransform().Translation + FVector2D(FMath::Sin(t * 91.f) * Amp, FMath::Cos(t * 77.f) * Amp));
	}
}

FReply UYggMenuBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bClosing) { return FReply::Unhandled(); }
	const FKey K = InKeyEvent.GetKey();
	bPad = K.IsGamepadKey();
	if (Page.bSplash)
	{
		if (Page.AnyKey && PageAge > 0.6f) { TFunction<void()> F = Page.AnyKey; F(); }
		return FReply::Handled();
	}
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up || K == EKeys::Gamepad_LeftStick_Up
		|| (Page.bModal && (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftStick_Left))) { Move(-1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down || K == EKeys::Gamepad_LeftStick_Down
		|| (Page.bModal && (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_LeftStick_Right))) { Move(1); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { Activate(Selected); return FReply::Handled(); }
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::BackSpace)
	{
		if (Page.Back && PageAge > 0.15f) { TFunction<void()> B = Page.Back; B(); }
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UYggMenuBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Page.bSplash)
	{
		if (Page.AnyKey && PageAge > 0.6f) { TFunction<void()> F = Page.AnyKey; F(); }
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UYggMenuBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.GetCursorDelta().IsNearlyZero()) { bMouseMoved = true; bPad = false; }
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}
