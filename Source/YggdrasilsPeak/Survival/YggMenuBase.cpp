// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggMenuBase.h"

#include "Survival/YggEmblem.h"
#include "Survival/SurvivalHUDWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Survival/YggUIProxy.h"
#include "Survival/YggUIStyle.h"
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
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"


namespace
{
	FLinearColor MenuWithA(const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, A); }
}

FLinearColor UYggMenuBase::Accent() const
{
	if (Page.AccentOverride.A > 0.f) { return Page.AccentOverride; }
	return Page.Tone == ETone::Death ? YggUI::Ember : YggUI::Gold;
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
	Serif = YggUI::MakeSerif(this);
	BuildBackground(Canvas);
	// on the mountain the world behind the menu goes soft (the bright snow fought with the words); the start menu's
	// own pictures stay sharp
	if (!bUIOnlyInput)
	{
		Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass());
		Blur->SetBlurStrength(0.f);
		Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(Blur);
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}

	YggUI::Layer(WidgetTree, Canvas, YggUI::VeilTexture(), MenuWithA(YggUI::Ink, VeilAlpha));
	// a second veil from the right, under the side panel (the world is brightest there)
	RightVeil = YggUI::Layer(WidgetTree, Canvas, YggUI::VeilTexture(), MenuWithA(YggUI::Ink, 0.8f));
	RightVeil->SetRenderTransformAngle(180.f);
	RightVeil->SetVisibility(ESlateVisibility::Collapsed);
	YggUI::Layer(WidgetTree, Canvas, YggUI::VignetteTexture(), MenuWithA(YggUI::Ink, 0.6f));
	// the summit's warm light from the upper right
	ToneGlow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	ToneGlow->SetBrush(YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f)));
	ToneGlow->SetColorAndOpacity(FLinearColor(1.f, 0.8f, 0.52f, 0.2f));
	ToneGlow->SetVisibility(ESlateVisibility::Collapsed);
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(ToneGlow);
		S->SetAnchors(FAnchors(0.78f, 0.12f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetSize(FVector2D(1900.f, 1500.f));
	}
	// a darkening behind a question (and under a death)
	{
		DimLayer = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::Box;
		B.TintColor = FSlateColor(FLinearColor::White);
		DimLayer->SetBrush(B);
		DimLayer->SetColorAndOpacity(YggUI::Ink);
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
		S->SetAnchors(FAnchors(0.64f, 0.f));
		S->SetAlignment(FVector2D(0.5f, 0.f));
		S->SetAutoSize(true);
	}
	for (int32 k = 0; k < 2; ++k)
	{
		UImage* M = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		M->SetBrush(YggUI::TextureBrush(YggUI::FadeTexture(), FVector2D(4.f, 64.f)));
		M->SetColorAndOpacity(FLinearColor(YggUI::Ink.R, YggUI::Ink.G, YggUI::Ink.B, 0.9f));
		M->SetVisibility(ESlateVisibility::Collapsed);
		if (k == 1) { M->SetRenderTransformAngle(180.f); }
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(M);
		S->SetAnchors(k == 0 ? FAnchors(0.f, 0.f, 1.f, 0.f) : FAnchors(0.f, 1.f, 1.f, 1.f));
		S->SetOffsets(k == 0 ? FMargin(0.f, 0.f, 0.f, 300.f) : FMargin(0.f, -300.f, 0.f, 300.f));
		RollMasks.Add(M);
	}

	YggUI::FChrome C = YggUI::Chrome(WidgetTree, Canvas, 0.06f, bHeader);
	CrumbText = C.Crumb;
	if (C.Right) { C.Right->SetText(FText::FromString(HeaderRight.ToUpper())); }
	if (C.Foot) { C.Foot->SetText(FText::FromString(FooterLeft.ToUpper())); }
	for (UWidget* W : C.All) { FadeWithWords.Add(W); }

	GrainImg = YggUI::Grain(WidgetTree, Canvas);
	SetRenderOpacity(0.f);
}

UWidget* UYggMenuBase::Numbers(const FString& Key, int32 Big, int32 Small, bool bRight)
{
	// "8,849|m" -> light numerals with a small unit on their baseline (as the HUD's altitude); "2|h  14|m"
	UHorizontalBox* Num = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TArray<FString> Parts;
	Key.ParseIntoArray(Parts, TEXT("|"), false);
	for (int32 p = 0; p < Parts.Num(); ++p)
	{
		FString Piece = Parts[p];
		FString Unit;
		if (p > 0) { const int32 Sp = Piece.Find(TEXT(" ")); Unit = Piece.Left(Sp >= 0 ? Sp : Piece.Len()); Piece = Piece.Mid(Unit.Len()); }
		if (!Unit.IsEmpty())
		{
			UHorizontalBoxSlot* US = Num->AddChildToHorizontalBox(YggUI::Text(WidgetTree, Unit, Small, YggUI::Dim, 120));
			US->SetVerticalAlignment(VAlign_Bottom);
			US->SetPadding(FMargin(0.f, 0.f, 0.f, Big * 0.12f));
		}
		if (!Piece.TrimStartAndEnd().IsEmpty())
		{
			UHorizontalBoxSlot* S = Num->AddChildToHorizontalBox(YggUI::Text(WidgetTree, Piece.TrimStartAndEnd(), Big, YggUI::Ivory, 10));
			S->SetVerticalAlignment(VAlign_Bottom);
			S->SetPadding(FMargin(p > 0 ? 10.f : 0.f, 0.f, 3.f, 0.f));
		}
	}
	return Num;
}

void UYggMenuBase::BuildColumn()
{
	Column->ClearChildren();
	OptButtons.Reset(); OptTexts.Reset(); OptSubs.Reset(); OptNums.Reset(); OptBands.Reset(); OptBars.Reset(); Proxies.Reset(); OptHot.Reset();
	SplashPrompt = nullptr;
	const bool bCentre = Page.bModal || Page.bSplash;
	if (ColumnSlot)
	{
		ColumnSlot->SetAnchors(bCentre ? FAnchors(0.5f, 0.5f) : FAnchors(ColumnX, 0.5f));
		ColumnSlot->SetAlignment(bCentre ? FVector2D(0.5f, 0.5f) : FVector2D(0.f, 0.5f));
		ColumnSlot->SetPosition(FVector2D(0.f, bCentre ? -10.f : 8.f));
	}
	const EHorizontalAlignment HA = bCentre ? HAlign_Center : HAlign_Left;
	const FLinearColor Acc = Accent();
	auto Add = [this, HA](UWidget* W, float Gap)
	{
		UVerticalBoxSlot* S = Column->AddChildToVerticalBox(W);
		S->SetHorizontalAlignment(HA);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, Gap));
	};
	auto Centre = [bCentre](UTextBlock* T) { if (bCentre) { T->SetJustification(ETextJustify::Center); } return T; };

	if (Page.bEmblem)
	{
		UYggEmblem* E = WidgetTree->ConstructWidget<UYggEmblem>(UYggEmblem::StaticClass());
		E->Diameter = Page.bSplash ? 176.f : 92.f;
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SB->SetWidthOverride(E->Diameter);
		SB->SetHeightOverride(E->Diameter);
		SB->SetContent(E);
		Add(SB, Page.bSplash ? 34.f : 22.f);
	}
	if (!Page.Eyebrow.IsEmpty()) { Add(Centre(YggUI::Text(WidgetTree, Page.Eyebrow, 10, YggUI::Dim, 460, TEXT("Regular"))), 6.f); }
	Add(Centre(YggUI::Text(WidgetTree, Page.Title, int32(Page.TitleSize), YggUI::Ivory, Page.bSplash ? 30 : 0, TEXT("Regular"), Serif)), 10.f);
	RealmBeads.Reset();
	{
		UHorizontalBox* RuleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const float LineW = Page.bNineBeads ? 120.f : (bCentre ? 150.f : 300.f);
		RuleRow->AddChildToHorizontalBox(YggUI::Line(WidgetTree, LineW))->SetVerticalAlignment(VAlign_Center);
		// one bead, or nine for the nine realms (a gold light wanders slowly from one to the next)
		for (int32 k = 0; k < (Page.bNineBeads ? 9 : 1); ++k)
		{
			UImage* Bd = YggUI::Bead(WidgetTree, Page.bNineBeads ? 4.f : 5.f, Page.bNineBeads ? YggUI::Faint : Acc);
			UHorizontalBoxSlot* BS = RuleRow->AddChildToHorizontalBox(Bd);
			BS->SetVerticalAlignment(VAlign_Center);
			BS->SetPadding(FMargin(k == 0 ? 8.f : 7.f, 0.f, 0.f, 0.f));
			if (Page.bNineBeads) { RealmBeads.Add(Bd); }
		}
		if (Page.bNineBeads || bCentre)
		{
			UHorizontalBoxSlot* LS = RuleRow->AddChildToHorizontalBox(YggUI::Line(WidgetTree, LineW));
			LS->SetVerticalAlignment(VAlign_Center);
			LS->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}
		Add(RuleRow, Page.Line.IsEmpty() ? 38.f : 20.f);
	}
	if (!Page.Line.IsEmpty())
	{
		UTextBlock* L = Centre(YggUI::Text(WidgetTree, Page.Line, 19, YggUI::Ivory, 10, TEXT("Regular"), Serif));
		L->SetWrapTextAt(640.f);   // a fixed measure: auto wrap followed the narrow column and broke the line early
		Add(L, 34.f);
	}
	if (Page.Stats.Num() > 0)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 i = 0; i < Page.Stats.Num(); ++i)
		{
			UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Cell->AddChildToVerticalBox(Numbers(Page.Stats[i].Key, 34, 14, false));
			UVerticalBoxSlot* LS = Cell->AddChildToVerticalBox(YggUI::Text(WidgetTree, Page.Stats[i].Value, 9, YggUI::Faint, 320, TEXT("Regular")));
			LS->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
			UHorizontalBoxSlot* CSl = Row->AddChildToHorizontalBox(Cell);
			CSl->SetPadding(FMargin(i == 0 ? 0.f : 28.f, 0.f, 28.f, 0.f));
			if (i + 1 < Page.Stats.Num())
			{
				Row->AddChildToHorizontalBox(YggUI::Line(WidgetTree, 1.f, YggUI::Hair, 40.f))->SetVerticalAlignment(VAlign_Center);
			}
		}
		Add(Row, 44.f);
	}
	if (Page.Body.Num() > 0 && !Page.bRoll)
	{
		for (int32 i = 0; i < Page.Body.Num(); ++i)
		{
			const FString& L = Page.Body[i];
			const bool bHead = L.StartsWith(TEXT("#"));
			UTextBlock* T = bHead ? YggUI::Text(WidgetTree, L.Mid(1).ToUpper(), 9, YggUI::Faint, 340, TEXT("Regular"))
				: YggUI::Text(WidgetTree, L, 16, YggUI::Ivory, 10, TEXT("Regular"), Serif);
			const bool bNextHead = i + 1 < Page.Body.Num() && Page.Body[i + 1].StartsWith(TEXT("#"));
			Add(T, bHead ? 3.f : (bNextHead ? 16.f : 1.f));
		}
		Add(YggUI::Line(WidgetTree, 1.f, FLinearColor::Transparent, 18.f), 0.f);
	}

	// the options: numbered rows; the chosen one sits in a band of soft light behind an accent bar
	UHorizontalBox* Across = nullptr;
	if (Page.bModal)
	{
		Across = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Add(Across, 0.f);
	}
	for (int32 i = 0; i < Page.Options.Num(); ++i)
	{
		UButton* B = YggUI::Bare(WidgetTree);
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SB->SetWidthOverride(Page.bModal ? 230.f : 500.f);
		UOverlay* O = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Band = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Band->SetBrush(YggUI::TextureBrush(Page.bModal ? YggUI::GlowTexture() : YggUI::BandTexture(), FVector2D(256.f, 32.f)));
		Band->SetColorAndOpacity(MenuWithA(FMath::Lerp(YggUI::Ivory, Acc, 0.5f), Page.bModal ? 0.18f : 0.17f));
		Band->SetRenderOpacity(0.f);
		UOverlaySlot* BandS = O->AddChildToOverlay(Band);
		BandS->SetHorizontalAlignment(HAlign_Fill);
		BandS->SetVerticalAlignment(VAlign_Fill);
		UTextBlock* T = YggUI::Text(WidgetTree, Page.Options[i].Label.ToUpper(), 15, YggUI::Dim, 280, TEXT("Regular"));
		UTextBlock* Sub = nullptr;
		if (!Page.Options[i].Sub.IsEmpty()) { Sub = YggUI::Text(WidgetTree, Page.Options[i].Sub.ToUpper(), 9, YggUI::Faint, 260, TEXT("Regular")); }
		UTextBlock* Num = nullptr;
		UImage* Bar = nullptr;
		if (Page.bModal)
		{
			UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			V->AddChildToVerticalBox(T)->SetHorizontalAlignment(HAlign_Center);
			Bar = YggUI::Line(WidgetTree, 30.f, Acc, 2.f);
			UVerticalBoxSlot* BarS = V->AddChildToVerticalBox(Bar);
			BarS->SetHorizontalAlignment(HAlign_Center);
			BarS->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
			UOverlaySlot* VS = O->AddChildToOverlay(V);
			VS->SetHorizontalAlignment(HAlign_Center);
			VS->SetVerticalAlignment(VAlign_Center);
			VS->SetPadding(FMargin(0.f, 14.f));
		}
		else
		{
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Bar = YggUI::Line(WidgetTree, 2.f, Acc, 30.f);
			UHorizontalBoxSlot* BarS = Row->AddChildToHorizontalBox(Bar);
			BarS->SetVerticalAlignment(VAlign_Center);
			BarS->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
			if (Page.bNumbered)
			{
				Num = YggUI::Text(WidgetTree, FString::Printf(TEXT("%02d"), i + 1), 10, YggUI::Faint, 200, TEXT("Regular"));
				UHorizontalBoxSlot* NS = Row->AddChildToHorizontalBox(Num);
				NS->SetVerticalAlignment(VAlign_Center);
				NS->SetPadding(FMargin(0.f, 2.f, 18.f, 0.f));
			}
			UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			V->AddChildToVerticalBox(T);
			if (Sub) { V->AddChildToVerticalBox(Sub)->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f)); }
			Row->AddChildToHorizontalBox(V)->SetVerticalAlignment(VAlign_Center);
			UOverlaySlot* RS = O->AddChildToOverlay(Row);
			RS->SetVerticalAlignment(VAlign_Center);
			RS->SetPadding(FMargin(0.f, Sub ? 10.f : 13.f, 12.f, Sub ? 10.f : 13.f));
		}
		Bar->SetRenderOpacity(0.f);
		SB->SetContent(O);
		B->SetContent(SB);
		UYggUIProxy* P = NewObject<UYggUIProxy>(this);
		P->OnClick = [this, i]() { Activate(i); };
		FScriptDelegate D;
		D.BindUFunction(P, TEXT("Click"));
		B->OnClicked.Add(D);
		Proxies.Add(P);
		OptButtons.Add(B); OptTexts.Add(T); OptSubs.Add(Sub); OptNums.Add(Num); OptBands.Add(Band); OptBars.Add(Bar); OptHot.Add(i == Selected ? 1.f : 0.f);
		if (Across)
		{
			UHorizontalBoxSlot* S = Across->AddChildToHorizontalBox(B);
			S->SetPadding(FMargin(i == 0 ? 0.f : 12.f, 0.f, 0.f, 0.f));
		}
		else
		{
			Add(B, 2.f);
		}
	}
	if (Page.bSplash)
	{
		SplashPrompt = YggUI::Text(WidgetTree, TEXT("PRESS ANY KEY"), 11, YggUI::Ivory, 700, TEXT("Regular"));
		SplashPrompt->SetRenderOpacity(0.f);
		UVerticalBoxSlot* S = Column->AddChildToVerticalBox(SplashPrompt);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 70.f, 0.f, 0.f));
	}
	Column->SetRenderOpacity(0.f);
	if (CrumbText)
	{
		CrumbText->SetText(FText::FromString(Page.Context.IsEmpty() ? FString(TEXT("YGGDRASIL'S PEAK")) : FString::Printf(TEXT("YGGDRASIL'S PEAK      /      %s"), *Page.Context.ToUpper())));
	}
	if (RightVeil) { RightVeil->SetVisibility(Page.Side.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (ToneGlow) { ToneGlow->SetVisibility(Page.Tone == ETone::Summit ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	BuildSide();
	BuildRoll();
	BuildPrompts();
}

void UYggMenuBase::BuildSide()
{
	SidePanel->ClearChildren();
	SideItems.Reset();
	const FLinearColor Acc = Accent();
	for (int32 i = 0; i < Page.Side.Num(); ++i)
	{
		const FString& L = Page.Side[i];
		UWidget* W = nullptr;
		float Gap = 0.f;
		if (L.StartsWith(TEXT("#"))) { W = YggUI::Text(WidgetTree, L.Mid(1).ToUpper(), 9, YggUI::Faint, 380, TEXT("Regular")); Gap = 10.f; }
		else if (L.StartsWith(TEXT("="))) { W = Numbers(L.Mid(1), 38, 15, true); Gap = 4.f; }
		else if (L.StartsWith(TEXT("~"))) { W = YggUI::Text(WidgetTree, L.Mid(1).ToUpper(), 9, YggUI::Faint, 320, TEXT("Regular")); Gap = 26.f; }
		else if (L == TEXT("-")) { W = YggUI::Line(WidgetTree, 220.f, YggUI::Rule); Gap = 24.f; }
		else if (L.StartsWith(TEXT("@")))
		{
			TArray<FString> F;
			L.Mid(1).ParseIntoArray(F, TEXT("|"), false);
			const int32 State = F.Num() > 2 ? FCString::Atoi(*F[2]) : 0;
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			const FLinearColor NameC = State == 2 ? YggUI::Ivory : State == 1 ? YggUI::Dim : MenuWithA(YggUI::Ivory, 0.34f);
			UHorizontalBoxSlot* NS = Row->AddChildToHorizontalBox(YggUI::Text(WidgetTree, F.Num() > 0 ? F[0] : FString(), 15, NameC, 10, TEXT("Regular"), Serif));
			NS->SetVerticalAlignment(VAlign_Center);
			NS->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
			UHorizontalBoxSlot* AS = Row->AddChildToHorizontalBox(YggUI::Text(WidgetTree, F.Num() > 1 ? F[1].ToUpper() : FString(), 9, State == 0 ? MenuWithA(YggUI::Ivory, 0.3f) : YggUI::Faint, 200, TEXT("Regular")));
			AS->SetVerticalAlignment(VAlign_Center);
			AS->SetPadding(FMargin(0.f, 1.f, 14.f, 0.f));
			USizeBox* BB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			BB->SetWidthOverride(11.f);
			BB->SetHeightOverride(11.f);
			UImage* Bd = YggUI::Bead(WidgetTree, State == 2 ? 9.f : 5.f, State == 2 ? Acc : State == 1 ? MenuWithA(YggUI::Ivory, 0.75f) : MenuWithA(YggUI::Ivory, 0.25f));
			if (USizeBoxSlot* BS = Cast<USizeBoxSlot>(BB->SetContent(Bd))) { BS->SetHorizontalAlignment(HAlign_Center); BS->SetVerticalAlignment(VAlign_Center); }
			Row->AddChildToHorizontalBox(BB)->SetVerticalAlignment(VAlign_Center);
			W = Row;
			const bool bNext = i + 1 < Page.Side.Num() && Page.Side[i + 1].StartsWith(TEXT("@"));
			if (bNext)
			{
				UVerticalBoxSlot* RS = SidePanel->AddChildToVerticalBox(Row);
				RS->SetHorizontalAlignment(HAlign_Right);
				SideItems.Add(Row);
				// the rope between two camps
				W = YggUI::Line(WidgetTree, 1.f, MenuWithA(YggUI::Ivory, State >= 1 ? 0.3f : 0.12f), 12.f);
				UVerticalBoxSlot* CS = SidePanel->AddChildToVerticalBox(W);
				CS->SetHorizontalAlignment(HAlign_Right);
				CS->SetPadding(FMargin(0.f, 2.f, 5.f, 2.f));
				SideItems.Add(W);
				continue;
			}
			Gap = 26.f;
		}
		else
		{
			UTextBlock* T = YggUI::Text(WidgetTree, L, 16, YggUI::Dim, 10, TEXT("Regular"), Serif);
			T->SetJustification(ETextJustify::Right);
			T->SetWrapTextAt(380.f);
			W = T;
			Gap = 20.f;
		}
		UVerticalBoxSlot* S = SidePanel->AddChildToVerticalBox(W);
		S->SetHorizontalAlignment(HAlign_Right);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, Gap));
		SideItems.Add(W);
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
	UYggEmblem* E = WidgetTree->ConstructWidget<UYggEmblem>(UYggEmblem::StaticClass());
	E->Diameter = 120.f;
	USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SB->SetWidthOverride(120.f);
	SB->SetHeightOverride(120.f);
	SB->SetContent(E);
	Add(SB, 30.f);
	Add(YggUI::Text(WidgetTree, TEXT("Yggdrasil's Peak"), 46, YggUI::Ivory, 0, TEXT("Regular"), Serif), 8.f);
	Add(YggUI::Text(WidgetTree, TEXT("A CLIMB BENEATH THE WORLD TREE"), 9, YggUI::Faint, 460, TEXT("Regular")), 90.f);
	for (int32 i = 0; i < Page.Body.Num(); ++i)
	{
		const FString& L = Page.Body[i];
		if (L.StartsWith(TEXT("#")))
		{
			Add(YggUI::Line(WidgetTree, 24.f, MenuWithA(YggUI::Gold, 0.7f)), 14.f);
			Add(YggUI::Text(WidgetTree, L.Mid(1).ToUpper(), 9, YggUI::Dim, 420, TEXT("Regular")), 12.f);
		}
		else
		{
			UTextBlock* T = YggUI::Text(WidgetTree, L, 20, YggUI::Ivory, 10, TEXT("Regular"), Serif);
			T->SetJustification(ETextJustify::Center);
			T->SetWrapTextAt(620.f);
			const bool bNextHead = i + 1 >= Page.Body.Num() || Page.Body[i + 1].StartsWith(TEXT("#"));
			Add(T, bNextHead ? 64.f : 6.f);
		}
	}
	Add(YggUI::Line(WidgetTree, 1.f, FLinearColor::Transparent, 60.f), 0.f);
	Add(YggUI::Divider(WidgetTree, 60.f), 26.f);
	Add(YggUI::Text(WidgetTree, TEXT("Thank you for climbing."), 24, YggUI::Ivory, 10, TEXT("Regular"), Serif), 0.f);
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
		S->SetAnchors(FAnchors(0.94f, 1.f));
		S->SetAlignment(FVector2D(1.f, 0.5f));
		S->SetPosition(FVector2D(0.f, -50.f));
		S->SetAutoSize(true);
		S->SetZOrder(5);
		FadeWithWords.Add(Row);
		(k == 0 ? PromptRow : PadRow) = Row;
	}
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
	// the menu breathes in: the veil first, then the column slides in, the options one after another (a death slower)
	SetRenderOpacity(YggUI::Ease(Age * (Page.Tone == ETone::Death ? 0.8f : 2.2f)));
	Words = FMath::FInterpConstantTo(Words, WordsTarget, InDeltaTime, 1.6f);   // ~0.6 s to fade
	const float W = YggUI::Ease(Words);
	const bool bCentre = Page.bModal || Page.bSplash;
	if (Column)
	{
		const float C = YggUI::Ease(PageAge * (Page.bSplash ? 0.9f : 2.6f));
		Column->SetRenderOpacity(C * W);
		Column->SetRenderTranslation(bCentre ? FVector2D(0.f, 14.f * (1.f - C)) : FVector2D(-22.f * (1.f - C) - 18.f * (1.f - W), 0.f));
		if (WordsTarget <= 0.f && Words <= 0.001f && Column->GetVisibility() != ESlateVisibility::Hidden) { Column->SetVisibility(ESlateVisibility::Hidden); }
	}
	if (SidePanel)
	{
		const float C = YggUI::Ease((PageAge - 0.25f) * 2.2f);
		SidePanel->SetRenderOpacity(C * W);
		SidePanel->SetRenderTranslation(FVector2D(20.f * (1.f - C), 0.f));
	}
	if (DimLayer)
	{
		const float Target = Page.bModal ? 0.6f : (Page.Tone == ETone::Death ? 0.3f : 0.f);
		DimNow = FMath::FInterpTo(DimNow, Target, InDeltaTime, 6.f);
		DimLayer->SetRenderOpacity(DimNow);
	}
	if (ToneGlow && Page.Tone == ETone::Summit) { ToneGlow->SetRenderOpacity(YggUI::Ease(PageAge * 0.5f) * (0.85f + 0.15f * FMath::Sin(Age * 0.6f))); }
	if (Roll && Page.bRoll)
	{
		const FVector2D View = MyGeometry.GetLocalSize();
		const float H = Roll->GetDesiredSize().Y;
		// (a flag, not a negative position: the roll's top is above the screen most of the time)
		if (!bRollStarted && H > 1.f) { RollY = View.Y * 0.55f; bRollStarted = true; }
		RollY -= 36.f * InDeltaTime;
		if (RollY + H < 0.f) { RollY = View.Y; }
		Roll->SetRenderTranslation(FVector2D(0.f, RollY));
		Roll->SetRenderOpacity(YggUI::Ease(PageAge * 1.5f) * W);
	}
	for (UWidget* X : FadeWithWords) { if (X) { X->SetRenderOpacity(W); } }
	if (bMouseMoved)
	{
		for (int32 i = 0; i < OptButtons.Num(); ++i) { if (OptButtons[i] && OptButtons[i]->IsHovered()) { Selected = i; } }
	}
	const FLinearColor Acc = Accent();
	for (int32 k = 0; k < RealmBeads.Num(); ++k)
	{
		// a light that wanders slowly along the nine realms and back
		const float Pos = 4.f + 4.f * FMath::Sin(Age * 0.35f);
		const float L = FMath::Clamp(1.f - FMath::Abs(Pos - k) * 0.8f, 0.f, 1.f);
		if (RealmBeads[k]) { RealmBeads[k]->SetColorAndOpacity(FMath::Lerp(YggUI::Faint, Acc, L)); }
	}
	for (int32 i = 0; i < OptTexts.Num(); ++i)
	{
		const float Target = i == Selected ? 1.f : 0.f;
		OptHot[i] = FMath::FInterpTo(OptHot[i], Target, InDeltaTime, 14.f);
		const float H = OptHot[i];
		const float In = YggUI::Ease((PageAge - 0.2f - 0.07f * i) * 4.f);
		if (OptButtons[i])
		{
			OptButtons[i]->SetRenderOpacity(In);
			OptButtons[i]->SetRenderTranslation(Page.bModal ? FVector2D(0.f, 8.f * (1.f - In)) : FVector2D(6.f * H - 12.f * (1.f - In), 0.f));
		}
		if (OptTexts[i]) { OptTexts[i]->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Dim, YggUI::Ivory, H))); }
		if (OptSubs[i]) { OptSubs[i]->SetRenderOpacity(FMath::Lerp(0.6f, 1.f, H)); }
		if (OptNums[i]) { OptNums[i]->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Faint, Acc, H))); }
		if (OptBands[i]) { OptBands[i]->SetRenderOpacity(H); }
		if (OptBars[i]) { OptBars[i]->SetRenderOpacity(H); }
	}
	if (SplashPrompt)
	{
		SplashPrompt->SetRenderOpacity(YggUI::Ease((PageAge - 2.f) * 1.2f) * (0.4f + 0.6f * (0.5f + 0.5f * FMath::Sin(Age * 2.1f))));
	}
	YggUI::JitterGrain(GrainImg, GrainClock, InDeltaTime);
	if (PromptRow && PadRow)
	{
		PromptRow->SetVisibility(bPad ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		PadRow->SetVisibility(bPad ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Blur) { Blur->SetBlurStrength(7.f * YggUI::Ease(Age * 2.5f)); }
	if (bClosing)
	{
		CloseT += InDeltaTime;
		const float K = 1.f - YggUI::Ease(CloseT / 0.22f);
		SetRenderOpacity(K);
		if (Blur) { Blur->SetBlurStrength(7.f * K); }
		if (CloseT > 0.24f) { RemoveFromParent(); }
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

void UYggMenuBase::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
}
