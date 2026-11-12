// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/BackpackWidget.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/InventoryComponent.h"
#include "Survival/StoryClue.h"
#include "Survival/SurvivalComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	const TCHAR* ArtDir = TEXT("/Game/UI/Backpack/");
	const FLinearColor BoneWhite(0.93f, 0.9f, 0.82f, 1.f);
	const FLinearColor Ink(0.13f, 0.11f, 0.1f, 1.f);
	const FLinearColor Sepia(0.42f, 0.3f, 0.2f, 1.f);
	const FLinearColor Ember(0.78f, 0.3f, 0.08f, 1.f);

	// Layout on a 1920 x 1080 canvas, positions relative to the centre of the screen.
	const FVector2D PackCentre(-70.f, -10.f);
	const float PackSize = 820.f;
	// the compartment opening in the pack art (Saved/ItemIcons/render/Pack_layout.json), 0..1 of the image
	const FBox2D Opening(FVector2D(0.358f, 0.362f), FVector2D(0.642f, 0.789f));
	const FVector2D CardCentre(600.f, -10.f);
	const FVector2D CardSize(430.f, 597.f);
	const FVector2D ScaleCentre(-740.f, 0.f);
	const FVector2D ScaleSize(146.f, 595.f);
	// 0 kg and 30 kg on the scale art (Art/UI/Backpack/T_Scale_layout.txt), 0..1 of its height
	const float ScaleZero = 0.1847f, ScaleFull = 0.8266f;

	float EaseOutBack(float T)
	{
		const float C1 = 1.70158f, C3 = C1 + 1.f;
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f + C3 * FMath::Pow(T - 1.f, 3.f) + C1 * FMath::Pow(T - 1.f, 2.f);
	}

	float EaseOut(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f - FMath::Pow(1.f - T, 3.f);
	}

	struct FPackThingInfo
	{
		const TCHAR* Texture;
		float Size;
		FText Title;
		FText Body;
		FText Stats;
	};

	const FPackThingInfo* Info(FName Item)
	{
		static TMap<FName, FPackThingInfo> Infos;
		if (Infos.Num() == 0)
		{
			Infos.Add(TEXT("Ration"), { TEXT("T_Item_Ration"), 110.f, NSLOCTEXT("Pack", "RationT", "Rations"),
				NSLOCTEXT("Pack", "RationB", "Tins from the shop in Namche, a twist of tsampa, dried yak cheese. Mother packed food like this for every trek. It tastes like home and freezes like stone."),
				NSLOCTEXT("Pack", "RationS", "+35 food  ·  0.6 kg each") });
			Infos.Add(TEXT("Water"), { TEXT("T_Item_Water"), 125.f, NSLOCTEXT("Pack", "WaterT", "Thermos"),
				NSLOCTEXT("Pack", "WaterB", "Snow melted on the stove at the last camp, still a little warm. Up here the air takes water out of you with every breath."),
				NSLOCTEXT("Pack", "WaterS", "+40 water  ·  1 kg each") });
			Infos.Add(TEXT("Oxygen"), { TEXT("T_Item_Oxygen"), 150.f, NSLOCTEXT("Pack", "O2T", "Oxygen bottle"),
				NSLOCTEXT("Pack", "O2B", "Four litres at three hundred bar. Above the South Col, a bottle is not air - it is time."),
				NSLOCTEXT("Pack", "O2S", "3 kg each  ·  put on by itself when you run low") });
			Infos.Add(TEXT("Stove"), { TEXT("T_Item_Stove"), 95.f, NSLOCTEXT("Pack", "StoveT", "Stove"),
				NSLOCTEXT("Pack", "StoveB", "Melts snow into water at the camps. Without it there is nothing to drink above the last stream."),
				NSLOCTEXT("Pack", "StoveS", "gear  ·  part of the 10 kg you always carry") });
			Infos.Add(TEXT("Gas"), { TEXT("T_Item_Gas"), 75.f, NSLOCTEXT("Pack", "GasT", "Gas cartridge"),
				NSLOCTEXT("Pack", "GasB", "The cold makes it weak. Sleep with it inside the bag."),
				NSLOCTEXT("Pack", "GasS", "gear") });
			Infos.Add(TEXT("Compass"), { TEXT("T_Item_Compass"), 90.f, NSLOCTEXT("Pack", "CompassT", "Father's compass"),
				NSLOCTEXT("Pack", "CompassB", "Found in the box of their things. The runes round the dial belong to no language I know. An old compass, nothing more - that is what I keep telling myself."),
				NSLOCTEXT("Pack", "CompassS", "Q - hold it up") });
			Infos.Add(TEXT("Journal"), { TEXT("T_Item_Journal"), 110.f, NSLOCTEXT("Pack", "JournalT", "Mother's journal"),
				FText::GetEmpty(), FText::GetEmpty() });
			Infos.Add(TEXT("IceAxe"), { TEXT("T_Item_IceAxe"), 0.f, NSLOCTEXT("Pack", "AxeT", "Ice axe"),
				NSLOCTEXT("Pack", "AxeB", "Balance on the ice, a step cut where the crampons will not hold, the last thing between a slip and a fall."),
				NSLOCTEXT("Pack", "AxeS", "gear") });
			Infos.Add(TEXT("Rope"), { TEXT("T_Item_Rope"), 0.f, NSLOCTEXT("Pack", "RopeT", "Rope and harness"),
				NSLOCTEXT("Pack", "RopeB", "Clip into the fixed lines and they hold you. Steeper ground can be climbed while you are on a rope."),
				NSLOCTEXT("Pack", "RopeS", "gear") });
		}
		return Infos.Find(Item);
	}

	UFont* MakeFont(const TCHAR* Face)
	{
		UFontFace* FontFace = LoadObject<UFontFace>(nullptr, Face);
		if (!FontFace)
		{
			return nullptr;
		}
		UFont* Font = NewObject<UFont>(GetTransientPackage());
		Font->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry& Entry = Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = TEXT("Regular");
		Entry.Font = FFontData(FontFace);
		return Font;
	}
}

static FAutoConsoleCommandWithWorld CmdPack(TEXT("ygg.Pack"), TEXT("Opens or closes the pack screen (same as Tab)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (World)
		{
			UBackpackWidget::Toggle(World->GetFirstPlayerController());
		}
	}));

// ---------------------------------------------------------------- item

void UBackpackItemWidget::Setup(UBackpackWidget* InOwner, FName InItem, UTexture2D* Texture, float InSize)
{
	Owner = InOwner;
	Item = InItem;
	Size = InSize;
	if (!WidgetTree->RootWidget)
	{
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Sketch"));
		WidgetTree->RootWidget = Image;
	}
	if (Texture)
	{
		Image->SetBrushFromTexture(Texture, false);
	}
	else
	{
		Image->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));   // an invisible hot spot over the pack art
	}
	Image->SetDesiredSizeOverride(FVector2D(Size, Size));
}

TSharedRef<SWidget> UBackpackItemWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Sketch"));
		WidgetTree->RootWidget = Image;
	}
	return Super::RebuildWidget();
}

void UBackpackItemWidget::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	bHovered = true;
	if (Owner.IsValid())
	{
		Owner->Select(Item);
	}
}

void UBackpackItemWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
	Super::NativeOnMouseLeave(Event);
	bHovered = false;
	if (Owner.IsValid())
	{
		Owner->HoverEnd(Item);
	}
}

FReply UBackpackItemWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Owner.IsValid())
	{
		Owner->UseItem(Item);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}

// ---------------------------------------------------------------- pack screen

void UBackpackWidget::Toggle(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	TArray<UUserWidget*> Open;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, UBackpackWidget::StaticClass(), false);
	for (UUserWidget* W : Open)
	{
		UBackpackWidget* Pack = Cast<UBackpackWidget>(W);
		if (Pack && !Pack->bClosing)
		{
			Pack->Close();
			return;
		}
	}
	if (UBackpackWidget* Pack = CreateWidget<UBackpackWidget>(PC, UBackpackWidget::StaticClass()))
	{
		Pack->AddToViewport(50);
	}
}

UTexture2D* UBackpackWidget::Tex(const TCHAR* Name) const
{
	return LoadObject<UTexture2D>(nullptr, *(FString(ArtDir) + Name + TEXT(".") + Name));
}

UTextBlock* UBackpackWidget::MakeText(UFont* Font, int32 Size, const FLinearColor& Colour, float WrapAt)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Info = Text->GetFont();
	if (Font)
	{
		Info = FSlateFontInfo(Font, Size, TEXT("Regular"));
	}
	Info.Size = Size;
	Text->SetFont(Info);
	Text->SetColorAndOpacity(FSlateColor(Colour));
	if (WrapAt > 0.f)
	{
		Text->SetAutoWrapText(false);
		Text->SetWrapTextAt(WrapAt);
	}
	return Text;
}

UImage* UBackpackWidget::AddImage(UTexture2D* Texture, const FVector2D& Centre, const FVector2D& Size)
{
	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (Texture)
	{
		Image->SetBrushFromTexture(Texture, false);
	}
	UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Image);
	S->SetAnchors(FAnchors(0.5f, 0.5f));
	S->SetAlignment(FVector2D(0.5f, 0.5f));
	S->SetPosition(Centre);
	S->SetSize(Size);
	return Image;
}

TSharedRef<SWidget> UBackpackWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Build();
	}
	return Super::RebuildWidget();
}

void UBackpackWidget::Build()
{
	SetIsFocusable(true);
	HandFont = MakeFont(TEXT("/Game/UI/Fonts/FF_Caveat.FF_Caveat"));
	BookFont = MakeFont(TEXT("/Game/UI/Fonts/FF_Alegreya.FF_Alegreya"));

	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PackCanvas"));
	WidgetTree->RootWidget = Canvas;

	// the world behind: blurred and darkened towards the edges
	Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass());
	UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(Blur);
	BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BS->SetOffsets(FMargin(0.f));
	Blur->SetBlurStrength(0.f);
	Vignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Vignette->SetBrushFromTexture(Tex(TEXT("T_Vignette")), false);
	UCanvasPanelSlot* VS = Canvas->AddChildToCanvas(Vignette);
	VS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	VS->SetOffsets(FMargin(0.f));

	// the pack and its opening frames
	for (int32 i = 0; i < 6; ++i)
	{
		PackFrames.Add(Tex(*FString::Printf(TEXT("T_Pack_f%d"), i)));
	}
	Pack = AddImage(PackFrames[0], PackCentre, FVector2D(PackSize));
	Pack->SetRenderTransformPivot(FVector2D(0.5f, 0.9f));
	const FVector2D TopLeft = PackCentre - FVector2D(PackSize * 0.5f);
	const FVector2D OpenMin = TopLeft + Opening.Min * PackSize, OpenMax = TopLeft + Opening.Max * PackSize;
	Pocket = AddImage(Tex(TEXT("T_Pocket")), (OpenMin + OpenMax) * 0.5f, (OpenMax - OpenMin) - FVector2D(6.f));
	Pocket->SetRenderOpacity(0.f);
	Pocket->SetVisibility(ESlateVisibility::HitTestInvisible);

	// things strapped outside the pack: hot spots over the ice axe (left) and the rope (right) in the pack art
	auto Hotspot = [this, TopLeft](FName Item, const FBox2D& Area)
	{
		UBackpackItemWidget* W = CreateWidget<UBackpackItemWidget>(this, UBackpackItemWidget::StaticClass());
		W->Setup(this, Item, nullptr, 10.f);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetPosition(TopLeft + Area.Min * PackSize);
		S->SetSize((Area.Max - Area.Min) * PackSize);
		Hotspots.Add(W);
	};
	Hotspot(TEXT("IceAxe"), FBox2D(FVector2D(0.235f, 0.24f), FVector2D(0.30f, 0.84f)));
	Hotspot(TEXT("Rope"), FBox2D(FVector2D(0.66f, 0.34f), FVector2D(0.86f, 0.70f)));

	// the item's page on the right
	Card = AddImage(Tex(TEXT("T_Card")), CardCentre, CardSize);
	CardIcon = AddImage(nullptr, CardCentre + FVector2D(10.f, -168.f), FVector2D(190.f));
	auto CardText = [this](UTextBlock* T, const FVector2D& Pos, float Width)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(T);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetPosition(CardCentre + Pos);
		S->SetSize(FVector2D(Width, 300.f));
		S->SetAutoSize(true);
		return T;
	};
	CardTitle = CardText(MakeText(HandFont, 44, Ink), FVector2D(-150.f, -70.f), 330.f);
	CardCount = CardText(MakeText(HandFont, 26, Sepia), FVector2D(-148.f, -14.f), 330.f);
	CardBody = CardText(MakeText(BookFont, 17, Ink, 320.f), FVector2D(-148.f, 26.f), 330.f);
	CardStats = CardText(MakeText(BookFont, 15, Sepia, 320.f), FVector2D(-148.f, 200.f), 330.f);
	CardAction = CardText(MakeText(HandFont, 28, Ember), FVector2D(-148.f, 236.f), 330.f);

	// the porter's spring scale on the left
	Scale = AddImage(Tex(TEXT("T_Scale")), ScaleCentre, ScaleSize);
	ScaleMark = AddImage(Tex(TEXT("T_ScaleMark")), ScaleCentre, FVector2D(60.f, 30.f));
	ScaleText = MakeText(HandFont, 38, BoneWhite);
	ScaleStatus = MakeText(BookFont, 15, FLinearColor(BoneWhite.R, BoneWhite.G, BoneWhite.B, 0.75f), 200.f);
	for (UTextBlock* T : { ScaleText, ScaleStatus })
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(T);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.f));
		S->SetAutoSize(true);
		S->SetPosition(ScaleCentre + FVector2D(0.f, ScaleSize.Y * 0.5f + (T == ScaleText ? 4.f : 62.f)));
		T->SetJustification(ETextJustify::Center);
	}

	// heading and hints
	Header = MakeText(HandFont, 66, BoneWhite);
	Header->SetText(NSLOCTEXT("Pack", "Header", "Pack"));
	SubHeader = MakeText(BookFont, 18, FLinearColor(BoneWhite.R, BoneWhite.G, BoneWhite.B, 0.7f));
	Footer = MakeText(HandFont, 30, FLinearColor(BoneWhite.R, BoneWhite.G, BoneWhite.B, 0.8f));
	Footer->SetText(NSLOCTEXT("Pack", "Footer", "point at a thing to read about it   ·   click to use   ·   Tab to close"));
	for (TPair<UTextBlock*, FVector2D> P : { TPair<UTextBlock*, FVector2D>(Header, FVector2D(-880.f, -500.f)),
		TPair<UTextBlock*, FVector2D>(SubHeader, FVector2D(-874.f, -398.f)), TPair<UTextBlock*, FVector2D>(Footer, FVector2D(-40.f, 470.f)) })
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(P.Key);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAutoSize(true);
		S->SetPosition(P.Value);
		if (P.Key == Footer)
		{
			S->SetAlignment(FVector2D(0.5f, 0.5f));
		}
	}
}

void UBackpackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OpenTime = 0.f;
	APawn* Pawn = GetOwningPlayerPawn();
	if (UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr)
	{
		Inv->OnChanged.AddDynamic(this, &UBackpackWidget::HandleInventoryChanged);
		ShownLoadKg = 0.f;
	}
	if (const USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr)
	{
		SubHeader->SetText(FText::Format(NSLOCTEXT("Pack", "Sub", "{0} m above the sea  ·  what you carry is what you have"),
			FText::AsNumber(FMath::RoundToInt(Survival->GetElevationMeters()))));
	}
	RefreshItems(true);
	Select(TEXT("Ration"));
	SetLooseInput(true);
	SetKeyboardFocus();
}

void UBackpackWidget::NativeDestruct()
{
	APawn* Pawn = GetOwningPlayerPawn();
	if (UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr)
	{
		Inv->OnChanged.RemoveDynamic(this, &UBackpackWidget::HandleInventoryChanged);
	}
	Super::NativeDestruct();
}

void UBackpackWidget::SetLooseInput(bool bPackOpen)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	PC->SetShowMouseCursor(bPackOpen);
	PC->SetIgnoreLookInput(bPackOpen);
	PC->SetIgnoreMoveInput(bPackOpen);
	if (bPackOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		// the cursor starts in the middle of the screen, not wherever it was hidden
		int32 W = 0, H = 0;
		PC->GetViewportSize(W, H);
		PC->SetMouseLocation(W / 2, H / 2);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UBackpackWidget::Close()
{
	if (bClosing)
	{
		return;
	}
	bClosing = true;
	CloseTime = 0.f;
	SetLooseInput(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

FReply UBackpackWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	if (Key == EKeys::Tab || Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Left || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (OpenTime > 0.15f)
		{
			Close();
		}
		return FReply::Handled();
	}
	// keyboard / pad: step through the things in the pack, use the selected one
	if (Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right
		|| Key == EKeys::A || Key == EKeys::D)
	{
		TArray<FName> Order;
		for (const UBackpackItemWidget* W : Items)
		{
			Order.AddUnique(W->Item);
		}
		Order.Add(TEXT("IceAxe")); Order.Add(TEXT("Rope"));
		const int32 Step = (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::A) ? -1 : 1;
		const int32 Index = Order.IndexOfByKey(Selected);
		Select(Order[(Index + Step + Order.Num()) % Order.Num()]);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		UseItem(Selected);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

void UBackpackWidget::HandleInventoryChanged()
{
	RefreshItems(false);
	RefreshCard();
}

void UBackpackWidget::RefreshItems(bool bAnimate)
{
	APawn* Pawn = GetOwningPlayerPawn();
	const UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;

	// what is in the pack, biggest first so the small things settle on top
	TArray<FName> Things;
	for (int32 i = 0; i < (Survival ? Survival->GetCarriedBottles() : 0); ++i) Things.Add(TEXT("Oxygen"));
	for (int32 i = 0; i < (Inv ? Inv->GetCount(TEXT("Water")) : 0); ++i) Things.Add(TEXT("Water"));
	for (int32 i = 0; i < (Inv ? Inv->GetCount(TEXT("Ration")) : 0); ++i) Things.Add(TEXT("Ration"));
	Things.Add(TEXT("Journal"));
	Things.Add(TEXT("Stove"));
	Things.Add(TEXT("Compass"));
	Things.Add(TEXT("Gas"));

	for (UBackpackItemWidget* W : Items)
	{
		W->RemoveFromParent();
	}
	Items.Reset();

	// pile them into the opening from the bottom up, rows with a little overlap and a random tilt (stable per slot)
	const FVector2D TopLeft = PackCentre - FVector2D(PackSize * 0.5f);
	const FVector2D Min = TopLeft + Opening.Min * PackSize, Max = TopLeft + Opening.Max * PackSize;
	const float Width = Max.X - Min.X;
	float Fit = 1.f;
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		float X = 0.f, Y = 0.f, Row = 0.f;
		for (const FName& T : Things)
		{
			const float S = Info(T)->Size * Fit * 0.72f;
			if (X + S > Width + 20.f) { X = 0.f; Y += Row * 0.78f; Row = 0.f; }
			X += S; Row = FMath::Max(Row, S);
		}
		if (Y + Row * 0.95f < (Max.Y - Min.Y) + 10.f) break;
		Fit *= 0.88f;
	}
	FRandomStream Rand(7);
	float X = 0.f, Y = 0.f, Row = 0.f;
	for (int32 i = 0; i < Things.Num(); ++i)
	{
		const FPackThingInfo* I = Info(Things[i]);
		const float S = I->Size * Fit;
		const float Step = S * 0.72f;
		if (X + Step > Width + 20.f) { X = 0.f; Y += Row * 0.78f; Row = 0.f; }
		UBackpackItemWidget* W = CreateWidget<UBackpackItemWidget>(this, UBackpackItemWidget::StaticClass());
		W->Setup(this, Things[i], Tex(I->Texture), S);
		W->Home = FVector2D(Min.X + X + Step * 0.5f + Rand.FRandRange(-8.f, 8.f), Max.Y - Y - S * 0.42f + Rand.FRandRange(-6.f, 6.f));
		W->BaseAngle = Rand.FRandRange(-11.f, 11.f);
		W->Delay = bAnimate ? 0.34f + i * 0.04f : -1.f;
		UCanvasPanelSlot* SL = Canvas->AddChildToCanvas(W);
		SL->SetAnchors(FAnchors(0.5f, 0.5f));
		SL->SetAlignment(FVector2D(0.5f, 0.5f));
		SL->SetSize(FVector2D(S));
		SL->SetPosition(W->Home);
		SL->SetZOrder(10 + i);
		W->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Items.Add(W);
		X += Step; Row = FMath::Max(Row, S);
	}
	// keep the card and the scale above the pile
	for (UWidget* W : TArray<UWidget*>{ Card, CardIcon, CardTitle, CardCount, CardBody, CardStats, CardAction })
	{
		Cast<UCanvasPanelSlot>(W->Slot)->SetZOrder(100);
	}
	RefreshScale();
}

void UBackpackWidget::Select(FName Item)
{
	if (Selected != Item)
	{
		Selected = Item;
		RefreshCard();
	}
}

void UBackpackWidget::HoverEnd(FName Item)
{
}

void UBackpackWidget::RefreshCard()
{
	const FPackThingInfo* I = Info(Selected);
	if (!I)
	{
		return;
	}
	APawn* Pawn = GetOwningPlayerPawn();
	const UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	const UExpeditionComponent* Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr;

	CardIcon->SetBrushFromTexture(Tex(I->Texture), false);
	CardTitle->SetText(I->Title);
	{
		// the hand-written title must stay on the paper (330 px): long names ("Father's compass") get smaller
		FSlateFontInfo F = CardTitle->GetFont();
		const int32 Len = I->Title.ToString().Len();
		F.Size = Len > 13 ? FMath::Max(28, FMath::RoundToInt(44.f * 13.f / Len)) : 44;
		CardTitle->SetFont(F);
	}
	FText Body = I->Body, Stats = I->Stats, Count = FText::GetEmpty(), Action = FText::GetEmpty();
	if (Selected == TEXT("Ration") || Selected == TEXT("Water"))
	{
		const int32 N = Inv ? Inv->GetCount(Selected) : 0;
		Count = FText::Format(NSLOCTEXT("Pack", "Left", "{0} left"), FText::AsNumber(N));
		Action = N > 0 ? (Selected == TEXT("Ration") ? NSLOCTEXT("Pack", "Eat", "click - eat one  (or 1)") : NSLOCTEXT("Pack", "Drink", "click - drink  (or 2)"))
			: NSLOCTEXT("Pack", "None", "none left");
	}
	else if (Selected == TEXT("Oxygen"))
	{
		const int32 N = Survival ? Survival->GetCarriedBottles() : 0;
		Count = FText::Format(NSLOCTEXT("Pack", "Bottles", "{0} spare"), FText::AsNumber(N));
		Action = Survival && Survival->GetBottleSecondsLeft() > 0.f
			? FText::Format(NSLOCTEXT("Pack", "Flowing", "breathing one now - {0} min left"), FText::AsNumber(FMath::CeilToInt(Survival->GetBottleSecondsLeft() / 60.f)))
			: (N > 0 ? NSLOCTEXT("Pack", "Valve", "click - open the valve") : FText::GetEmpty());
	}
	else if (Selected == TEXT("Journal"))
	{
		// the pages found so far, in the order of the route
		TArray<FString> Pages;
		for (TActorIterator<AStoryClue> It(GetWorld()); It; ++It)
		{
			if (Expedition && Expedition->HasFoundClue(It->ClueId))
			{
				Pages.Add(FString(TEXT("-  ")) + It->Title.ToString());
			}
		}
		Count = FText::Format(NSLOCTEXT("Pack", "Pages", "{0} pages found"), FText::AsNumber(Pages.Num()));
		Body = Pages.Num() > 0
			? FText::FromString(FString(TEXT("Her pages are scattered along their route. What I have found:\n\n")) + FString::Join(Pages, TEXT("\n")))
			: NSLOCTEXT("Pack", "NoPages", "Her pages are scattered along their route, somewhere above. Nothing found yet.");
		Stats = NSLOCTEXT("Pack", "JournalS", "read a page where you find it  [E]");
	}
	CardCount->SetText(Count);
	CardBody->SetText(Body);
	CardStats->SetText(Stats);
	CardAction->SetText(Action);
}

void UBackpackWidget::RefreshScale()
{
	APawn* Pawn = GetOwningPlayerPawn();
	const UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!Inv)
	{
		return;
	}
	const float Load = Inv->GetLoadKg();
	ScaleText->SetText(FText::Format(NSLOCTEXT("Pack", "Kg", "{0} kg"), FText::AsNumber(FMath::RoundToFloat(Load * 10.f) / 10.f)));
	ScaleStatus->SetText(Load <= Inv->ComfortableKg ? NSLOCTEXT("Pack", "Light", "a load you can live with")
		: Load < Inv->MaxKg ? NSLOCTEXT("Pack", "Heavy", "heavy - every step costs")
		: NSLOCTEXT("Pack", "Full", "full - nothing more fits"));
}

void UBackpackWidget::UseItem(FName Item)
{
	APawn* Pawn = GetOwningPlayerPawn();
	UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	Select(Item);
	if ((Item == TEXT("Ration") || Item == TEXT("Water")) && Inv)
	{
		Inv->Use(Item);            // OnChanged refreshes the pile and the card
	}
	else if (Item == TEXT("Oxygen") && Survival && Survival->GetCarriedBottles() > 0 && Survival->GetBottleSecondsLeft() <= 0.f)
	{
		Survival->SetCarriedBottles(Survival->GetCarriedBottles() - 1);
		Survival->UseOxygenBottle();
		RefreshItems(false);
		RefreshCard();
	}
}

void UBackpackWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	OpenTime += DeltaTime;
	// closing plays the opening backwards, quicker
	float T = OpenTime;
	if (bClosing)
	{
		CloseTime += DeltaTime;
		T = FMath::Max(0.f, 0.5f - CloseTime * 2.4f);
		if (CloseTime > 0.22f)
		{
			RemoveFromParent();
			return;
		}
	}

	const float Fade = EaseOut(T / 0.25f);
	Blur->SetBlurStrength(6.f * Fade);
	Vignette->SetRenderOpacity(Fade);

	// the pack rises and settles, then its front panel folds open
	const float Rise = EaseOutBack((T - 0.02f) / 0.32f);
	Pack->SetRenderOpacity(EaseOut(T / 0.12f));
	Pack->SetRenderTransform(FWidgetTransform(FVector2D(0.f, (1.f - Rise) * 120.f), FVector2D(0.9f + 0.1f * Rise), FVector2D::ZeroVector, 0.f));
	const float Unzip = FMath::Clamp((T - 0.16f) / 0.26f, 0.f, 1.f);
	const int32 Frame = FMath::Clamp(FMath::FloorToInt(Unzip * (PackFrames.Num() - 1) + 0.5f), 0, PackFrames.Num() - 1);
	Pack->SetBrushFromTexture(PackFrames[Frame], false);
	Pocket->SetRenderOpacity(FMath::Clamp((Unzip - 0.4f) / 0.6f, 0.f, 1.f) * 0.85f);
	Pocket->SetRenderTransform(Pack->GetRenderTransform());

	// things drop into the pack one after another, bounce a little and settle at their tilt
	const float Time = bClosing ? 10.f : OpenTime;
	for (UBackpackItemWidget* W : Items)
	{
		const float D = W->Delay < 0.f ? 1.f : FMath::Clamp((Time - W->Delay) / 0.28f, 0.f, 1.f);
		const float Drop = EaseOutBack(D);
		W->HoverAlpha = FMath::FInterpTo(W->HoverAlpha, (W->bHovered || W->Item == Selected) ? 1.f : 0.f, DeltaTime, 14.f);
		const bool bLit = W->bHovered;
		const float Lift = bLit ? W->HoverAlpha : W->HoverAlpha * 0.35f;
		const float Close = bClosing ? 1.f - Fade : 0.f;
		W->SetRenderOpacity(FMath::Clamp(D * 3.f, 0.f, 1.f) * (1.f - Close));
		W->SetRenderTransform(FWidgetTransform(FVector2D(0.f, (1.f - Drop) * -90.f - Lift * 10.f + Close * 60.f),
			FVector2D(1.f + Lift * 0.1f), FVector2D::ZeroVector, W->BaseAngle * (1.f - Lift * 0.6f) + (1.f - D) * 20.f));
	}
	for (UBackpackItemWidget* W : Hotspots)
	{
		W->SetVisibility(Unzip >= 1.f && !bClosing ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
	}

	// card and scale slide in from the sides
	const float Side = EaseOutBack((T - 0.2f) / 0.32f);
	const float SideOpacity = FMath::Clamp((T - 0.2f) / 0.15f, 0.f, 1.f);
	const FWidgetTransform CardMove(FVector2D((1.f - Side) * 160.f, 0.f), FVector2D(1.f), FVector2D::ZeroVector, (1.f - Side) * 6.f + 1.5f);
	for (UWidget* W : TArray<UWidget*>{ Card, CardIcon, CardTitle, CardCount, CardBody, CardStats, CardAction })
	{
		W->SetRenderOpacity(SideOpacity);
		W->SetRenderTransform(CardMove);
	}
	Card->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	const FWidgetTransform ScaleMove(FVector2D((1.f - Side) * -160.f, 0.f), FVector2D(1.f), FVector2D::ZeroVector, (1.f - Side) * -6.f);
	for (UWidget* W : TArray<UWidget*>{ Scale, ScaleMark, ScaleText, ScaleStatus })
	{
		W->SetRenderOpacity(SideOpacity);
		W->SetRenderTransform(ScaleMove);
	}
	// the scale's pointer swings to the load like a spring (overshoots, settles)
	APawn* Pawn = GetOwningPlayerPawn();
	const UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const float Load = Inv ? Inv->GetLoadKg() : 0.f;
	const float Target = T > 0.3f ? Load : 0.f;
	const float Accel = (Target - ShownLoadKg) * 90.f - ScaleVelocity * 9.f;
	ScaleVelocity += Accel * FMath::Min(DeltaTime, 0.05f);
	ShownLoadKg += ScaleVelocity * FMath::Min(DeltaTime, 0.05f);
	const float Frac = FMath::Lerp(ScaleZero, ScaleFull, FMath::Clamp(ShownLoadKg / 30.f, 0.f, 1.05f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(ScaleMark->Slot))
	{
		S->SetPosition(ScaleCentre + FVector2D(-ScaleSize.X * 0.5f - 8.f, ScaleSize.Y * (Frac - 0.5f)));
	}

	for (UTextBlock* Txt : { Header, SubHeader, Footer })
	{
		Txt->SetRenderOpacity(FMath::Clamp((T - 0.1f) / 0.25f, 0.f, 1.f));
	}
}
