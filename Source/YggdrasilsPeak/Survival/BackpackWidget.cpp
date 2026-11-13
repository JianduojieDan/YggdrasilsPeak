// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/BackpackWidget.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/InventoryComponent.h"
#include "Survival/StoryClue.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/YggUIStyle.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const TCHAR* KitDir = TEXT("/Game/UI/Backpack/Kit/");

	// Layout on a 1920 x 1080 canvas, positions relative to the centre of the screen.
	const FVector2D PageAt(470.f, -260.f);            // the item's page, top left
	const float PageW = 440.f;
	const FVector2D HeadAt(-880.f, -470.f);           // "Pack" and the height, top left
	const FVector2D LoadAt(-880.f, 300.f);            // the load, bottom left
	const float LoadW = 260.f;

	float PackEaseOutBack(float T)
	{
		const float C1 = 1.70158f, C3 = C1 + 1.f;
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f + C3 * FMath::Pow(T - 1.f, 3.f) + C1 * FMath::Pow(T - 1.f, 2.f);
	}

	float PackEaseOut(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f - FMath::Pow(1.f - T, 3.f);
	}

	struct FPackThingInfo
	{
		const TCHAR* Texture;
		FVector2D At;          // where it lies on the cloth (its centre)
		float Size;            // its square render's size on screen
		float Angle;           // how it lies
		float ShadowY;         // where its shadow falls under its centre, share of Size (flat things close, tall ones low)
		FText Title;
		FText Body;
		FText Stats;
	};

	/** The kit, as laid out on the cloth (left to right, row by row) - also the keyboard's order. */
	const TArray<FName>& KitOrder()
	{
		static const TArray<FName> Order = { TEXT("IceAxe"), TEXT("Oxygen"), TEXT("Water"), TEXT("Ration"), TEXT("Rope"), TEXT("Stove"), TEXT("Gas"), TEXT("Compass"), TEXT("Journal") };
		return Order;
	}

	const FPackThingInfo* Info(FName Item)
	{
		static TMap<FName, FPackThingInfo> Infos;
		if (Infos.Num() == 0)
		{
			Infos.Add(TEXT("IceAxe"), { TEXT("T_Kit_IceAxe"), FVector2D(-360.f, -250.f), 430.f, -5.f, 0.04f, NSLOCTEXT("Pack", "AxeT", "Ice axe"),
				NSLOCTEXT("Pack", "AxeB", "Balance on the ice, a step cut where the crampons will not hold, the last thing between a slip and a fall."),
				NSLOCTEXT("Pack", "AxeS", "gear") });
			Infos.Add(TEXT("Oxygen"), { TEXT("T_Kit_Oxygen"), FVector2D(70.f, -250.f), 330.f, 4.f, 0.07f, NSLOCTEXT("Pack", "O2T", "Oxygen bottle"),
				NSLOCTEXT("Pack", "O2B", "Four litres at three hundred bar. Above the South Col, a bottle is not air - it is time."),
				NSLOCTEXT("Pack", "O2S", "3 kg each  ·  put on by itself when you run low") });
			Infos.Add(TEXT("Water"), { TEXT("T_Kit_Water"), FVector2D(-540.f, 40.f), 260.f, -3.f, 0.26f, NSLOCTEXT("Pack", "WaterT", "Thermos"),
				NSLOCTEXT("Pack", "WaterB", "Snow melted on the stove at the last camp, still a little warm. Up here the air takes water out of you with every breath."),
				NSLOCTEXT("Pack", "WaterS", "+40 water  ·  1 kg each") });
			Infos.Add(TEXT("Ration"), { TEXT("T_Kit_Ration"), FVector2D(-270.f, 40.f), 300.f, 2.f, 0.1f, NSLOCTEXT("Pack", "RationT", "Rations"),
				NSLOCTEXT("Pack", "RationB", "Tins from the shop in Namche, a twist of tsampa, dried yak cheese. Mother packed food like this for every trek. It tastes like home and freezes like stone."),
				NSLOCTEXT("Pack", "RationS", "+35 food  ·  0.6 kg each") });
			Infos.Add(TEXT("Rope"), { TEXT("T_Kit_Rope"), FVector2D(20.f, 50.f), 270.f, 0.f, 0.12f, NSLOCTEXT("Pack", "RopeT", "Rope and harness"),
				NSLOCTEXT("Pack", "RopeB", "Clip into the fixed lines and they hold you. Steeper ground can be climbed while you are on a rope."),
				NSLOCTEXT("Pack", "RopeS", "gear") });
			Infos.Add(TEXT("Stove"), { TEXT("T_Kit_Stove"), FVector2D(265.f, 40.f), 230.f, -2.f, 0.24f, NSLOCTEXT("Pack", "StoveT", "Stove"),
				NSLOCTEXT("Pack", "StoveB", "Melts snow into water at the camps. Without it there is nothing to drink above the last stream."),
				NSLOCTEXT("Pack", "StoveS", "gear  ·  part of the 10 kg you always carry") });
			Infos.Add(TEXT("Gas"), { TEXT("T_Kit_Gas"), FVector2D(-540.f, 300.f), 180.f, 0.f, 0.17f, NSLOCTEXT("Pack", "GasT", "Gas cartridge"),
				NSLOCTEXT("Pack", "GasB", "The cold makes it weak. Sleep with it inside the bag."),
				NSLOCTEXT("Pack", "GasS", "gear") });
			Infos.Add(TEXT("Compass"), { TEXT("T_Kit_Compass"), FVector2D(-275.f, 300.f), 220.f, 8.f, 0.14f, NSLOCTEXT("Pack", "CompassT", "Father's compass"),
				NSLOCTEXT("Pack", "CompassB", "Found in the box of their things. The runes round the dial belong to no language I know. An old compass, nothing more - that is what I keep telling myself."),
				NSLOCTEXT("Pack", "CompassS", "Q - hold it up") });
			Infos.Add(TEXT("Journal"), { TEXT("T_Kit_Journal"), FVector2D(-10.f, 305.f), 250.f, -5.f, 0.2f, NSLOCTEXT("Pack", "JournalT", "Mother's journal"),
				FText::GetEmpty(), FText::GetEmpty() });
		}
		return Infos.Find(Item);
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
	const FPackThingInfo* I = Info(InItem);
	ShadowY = I ? I->ShadowY : 0.15f;
	Owner = InOwner;
	Item = InItem;
	Size = InSize;
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Thing"));
		WidgetTree->RootWidget = Root;
		// its shadow on the cloth (the lamp is up and to the left), the light that finds it when chosen, the thing
		Shadow = YggUI::Glow(WidgetTree, YggUI::WithA(YggUI::InkC, 0.75f), FVector2D(Size * 0.72f, Size * 0.2f));
		UOverlaySlot* SS = Root->AddChildToOverlay(Shadow);
		SS->SetHorizontalAlignment(HAlign_Center);
		SS->SetVerticalAlignment(VAlign_Center);
		Shadow->SetRenderTranslation(FVector2D(Size * 0.05f, Size * ShadowY));
		Light = YggUI::Glow(WidgetTree, YggUI::WithA(YggUI::Sun, 0.42f), FVector2D(Size * 1.15f, Size * 0.9f));
		Light->SetRenderOpacity(0.f);
		UOverlaySlot* LS = Root->AddChildToOverlay(Light);
		LS->SetHorizontalAlignment(HAlign_Center);
		LS->SetVerticalAlignment(VAlign_Center);
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Render"));
		UOverlaySlot* IS = Root->AddChildToOverlay(Image);
		IS->SetHorizontalAlignment(HAlign_Fill);
		IS->SetVerticalAlignment(VAlign_Fill);
		CountText = YggUI::Words(WidgetTree, TEXT(""), 24, YggUI::Snow);
		UOverlaySlot* CS = Root->AddChildToOverlay(CountText);
		CS->SetHorizontalAlignment(HAlign_Right);
		CS->SetVerticalAlignment(VAlign_Bottom);
		CS->SetPadding(FMargin(0.f, 0.f, Size * 0.12f, Size * 0.14f));
	}
	if (Texture) { Image->SetBrushFromTexture(Texture, false); }
	Image->SetDesiredSizeOverride(FVector2D(Size, Size));
}

void UBackpackItemWidget::SetCount(int32 Count, bool bCounted)
{
	bEmpty = bCounted && Count <= 0;
	if (CountText) { CountText->SetText(FText::FromString(bCounted && Count > 1 ? FString::Printf(TEXT("×%d"), Count) : FString())); }
}

TSharedRef<SWidget> UBackpackItemWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Render"));
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
	return LoadObject<UTexture2D>(nullptr, *(FString(KitDir) + Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

UTextBlock* UBackpackWidget::MakeText(UFont* Font, int32 Size, const FLinearColor& Colour, float WrapAt)
{
	UTextBlock* Text = YggUI::Words(WidgetTree, TEXT(""), Size, Colour);
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
	BookFont = YggUI::SerifFont();

	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PackCanvas"));
	WidgetTree->RootWidget = Canvas;

	// the world behind: blurred, then the dark
	Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass());
	UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(Blur);
	BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BS->SetOffsets(FMargin(0.f));
	Blur->SetBlurStrength(0.f);
	{
		Vignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::Box;
		B.TintColor = FSlateColor(FLinearColor::White);
		Vignette->SetBrush(B);
		Vignette->SetColorAndOpacity(YggUI::WithA(YggUI::InkC, 0.62f));
		Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* VS = Canvas->AddChildToCanvas(Vignette);
		VS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		VS->SetOffsets(FMargin(0.f));
	}
	// the canvas tarp in its pool of lamplight
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/Materials/M_UI_Cloth.M_UI_Cloth"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		ClothMID = UMaterialInstanceDynamic::Create(M, this);
		ClothMID->SetScalarParameterValue(TEXT("Light"), 0.f);
		Pack = YggUI::Layer(WidgetTree, Canvas, ClothMID, FLinearColor::White);
	}

	// the page of the chosen thing, on the right
	auto Place = [this](UWidget* W, const FVector2D& Pos)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.f, 0.f));
		S->SetPosition(Pos);
		S->SetAutoSize(true);
		S->SetZOrder(100);
		return W;
	};
	CardTitle = MakeText(nullptr, 42, YggUI::Snow, PageW);
	CardCount = MakeText(nullptr, 21, YggUI::Mist, PageW);
	CardBody = MakeText(nullptr, 22, YggUI::Snow, PageW);
	CardStats = MakeText(nullptr, 18, YggUI::Shade, PageW);
	CardAction = MakeText(nullptr, 21, YggUI::Sun, PageW);
	Place(CardTitle, PageAt);
	Place(CardCount, PageAt + FVector2D(2.f, 62.f));
	Place(CardBody, PageAt + FVector2D(2.f, 110.f));
	Place(CardStats, PageAt + FVector2D(2.f, 380.f));
	Place(CardAction, PageAt + FVector2D(2.f, 414.f));

	// heading, top left
	Header = MakeText(nullptr, 84, YggUI::Snow);
	Header->SetText(NSLOCTEXT("Pack", "Header", "Pack"));
	SubHeader = MakeText(nullptr, 21, YggUI::Mist);
	Place(Header, HeadAt);
	Place(SubHeader, HeadAt + FVector2D(4.f, 118.f));

	// the load, bottom left: the weight in the serif, a line of light from nothing to the most you can carry
	ScaleText = MakeText(nullptr, 46, YggUI::Snow);
	ScaleStatus = MakeText(nullptr, 19, YggUI::Mist, LoadW + 60.f);
	Place(ScaleText, LoadAt);
	Place(ScaleStatus, LoadAt + FVector2D(2.f, 70.f));
	Scale = YggUI::Line(WidgetTree, LoadW, YggUI::WithA(YggUI::Mist, 0.18f), 2.f);
	Place(Scale, LoadAt + FVector2D(2.f, 116.f));
	ScaleGlow = YggUI::Glow(WidgetTree, YggUI::WithA(YggUI::Snow, 0.22f), FVector2D(40.f, 18.f));
	Place(ScaleGlow, LoadAt + FVector2D(2.f, 113.f));
	ScaleMark = YggUI::Line(WidgetTree, 1.f, YggUI::Snow, 2.f);
	Place(ScaleMark, LoadAt + FVector2D(2.f, 116.f));
	ScaleComfort = YggUI::Line(WidgetTree, 1.f, YggUI::WithA(YggUI::Mist, 0.6f), 9.f);
	Place(ScaleComfort, LoadAt + FVector2D(2.f, 112.f));

	// what the keys do, bottom right
	UHorizontalBox* Keys = YggUI::Prompts(WidgetTree, { { TEXT("← →"), TEXT("Choose") }, { TEXT("Enter"), TEXT("Use") }, { TEXT("Tab"), TEXT("Close") } });
	UCanvasPanelSlot* KS = Canvas->AddChildToCanvas(Keys);
	KS->SetAnchors(FAnchors(0.95f, 1.f));
	KS->SetAlignment(FVector2D(1.f, 0.5f));
	KS->SetPosition(FVector2D(0.f, -54.f));
	KS->SetAutoSize(true);
	PromptRow = Keys;
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
	// keyboard / pad: step through the kit in the order it lies, use the chosen thing
	const bool bPrev = Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::A || Key == EKeys::W;
	const bool bNext = Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::D || Key == EKeys::S;
	if (bPrev || bNext)
	{
		const TArray<FName>& Order = KitOrder();
		const int32 Index = FMath::Max(Order.IndexOfByKey(Selected), 0);
		Select(Order[(Index + (bPrev ? -1 : 1) + Order.Num()) % Order.Num()]);
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
	const int32 Rations = Inv ? Inv->GetCount(TEXT("Ration")) : 0;
	const int32 Waters = Inv ? Inv->GetCount(TEXT("Water")) : 0;
	const int32 Bottles = Survival ? Survival->GetCarriedBottles() : 0;

	// laid out once; afterwards only the counts change (the things stay where they lie)
	if (Items.Num() == 0)
	{
		const TArray<FName>& Order = KitOrder();
		for (int32 i = 0; i < Order.Num(); ++i)
		{
			const FPackThingInfo* I = Info(Order[i]);
			UBackpackItemWidget* W = CreateWidget<UBackpackItemWidget>(this, UBackpackItemWidget::StaticClass());
			W->Setup(this, Order[i], Tex(I->Texture), I->Size);
			W->Home = I->At;
			W->BaseAngle = I->Angle;
			W->Delay = bAnimate ? 0.22f + i * 0.045f : -1.f;
			UCanvasPanelSlot* SL = Canvas->AddChildToCanvas(W);
			SL->SetAnchors(FAnchors(0.5f, 0.5f));
			SL->SetAlignment(FVector2D(0.5f, 0.5f));
			SL->SetSize(FVector2D(I->Size));
			SL->SetPosition(I->At);
			SL->SetZOrder(10 + i);
			W->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Items.Add(W);
		}
	}
	for (UBackpackItemWidget* W : Items)
	{
		if (W->Item == TEXT("Ration")) { W->SetCount(Rations, true); }
		else if (W->Item == TEXT("Water")) { W->SetCount(Waters, true); }
		else if (W->Item == TEXT("Oxygen")) { W->SetCount(Bottles, true); }
		else { W->SetCount(1, false); }
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

	CardTitle->SetText(I->Title);
	FText Body = I->Body, Stats = I->Stats, Count = FText::GetEmpty(), Action = FText::GetEmpty();
	if (Selected == TEXT("Ration") || Selected == TEXT("Water"))
	{
		const int32 N = Inv ? Inv->GetCount(Selected) : 0;
		Count = N > 0 ? FText::Format(NSLOCTEXT("Pack", "Left", "{0} left"), FText::AsNumber(N)) : NSLOCTEXT("Pack", "NoneLeft", "none left");
		Action = N > 0 ? (Selected == TEXT("Ration") ? NSLOCTEXT("Pack", "Eat", "Click to eat one  ·  or 1") : NSLOCTEXT("Pack", "Drink", "Click to drink  ·  or 2"))
			: FText::GetEmpty();
	}
	else if (Selected == TEXT("Oxygen"))
	{
		const int32 N = Survival ? Survival->GetCarriedBottles() : 0;
		Count = N > 0 ? FText::Format(NSLOCTEXT("Pack", "Bottles", "{0} spare"), FText::AsNumber(N)) : NSLOCTEXT("Pack", "NoBottles", "no spare bottles");
		Action = Survival && Survival->GetBottleSecondsLeft() > 0.f
			? FText::Format(NSLOCTEXT("Pack", "Flowing", "Breathing one now  ·  {0} min left"), FText::AsNumber(FMath::CeilToInt(Survival->GetBottleSecondsLeft() / 60.f)))
			: (N > 0 ? NSLOCTEXT("Pack", "Valve", "Click to open the valve") : FText::GetEmpty());
	}
	else if (Selected == TEXT("Journal"))
	{
		// the pages found so far, in the order of the route
		TArray<FString> Pages;
		for (TActorIterator<AStoryClue> It(GetWorld()); It; ++It)
		{
			if (Expedition && Expedition->HasFoundClue(It->ClueId))
			{
				Pages.Add(FString(TEXT("—  ")) + It->Title.ToString());
			}
		}
		Count = FText::Format(NSLOCTEXT("Pack", "Pages", "{0} pages found"), FText::AsNumber(Pages.Num()));
		Body = Pages.Num() > 0
			? FText::FromString(FString(TEXT("Her pages are scattered along their route. What I have found:\n\n")) + FString::Join(Pages, TEXT("\n")))
			: NSLOCTEXT("Pack", "NoPages", "Her pages are scattered along their route, somewhere above. Nothing found yet.");
		Stats = NSLOCTEXT("Pack", "JournalS", "read a page where you find it  ·  E");
	}
	CardCount->SetText(Count);
	CardBody->SetText(Body);
	CardStats->SetText(Stats);
	CardAction->SetText(Action);
	// the count follows the title (a long name takes two lines), the body the count, the stats and action the body
	CardTitle->ForceLayoutPrepass();
	const float TitleH = FMath::Max(CardTitle->GetDesiredSize().Y, 50.f);
	if (UCanvasPanelSlot* CS = Cast<UCanvasPanelSlot>(CardCount->Slot)) { CS->SetPosition(PageAt + FVector2D(2.f, TitleH + 4.f)); }
	if (UCanvasPanelSlot* BS0 = Cast<UCanvasPanelSlot>(CardBody->Slot)) { BS0->SetPosition(PageAt + FVector2D(2.f, TitleH + (Count.IsEmpty() ? 18.f : 50.f))); }
	if (UCanvasPanelSlot* BS = Cast<UCanvasPanelSlot>(CardBody->Slot))
	{
		CardBody->ForceLayoutPrepass();
		const float BodyH = FMath::Max(CardBody->GetDesiredSize().Y, 30.f);
		if (UCanvasPanelSlot* SS = Cast<UCanvasPanelSlot>(CardStats->Slot)) { SS->SetPosition(BS->GetPosition() + FVector2D(0.f, BodyH + 28.f)); }
		if (UCanvasPanelSlot* AS = Cast<UCanvasPanelSlot>(CardAction->Slot)) { AS->SetPosition(BS->GetPosition() + FVector2D(0.f, BodyH + 62.f)); }
	}
	PageAge = 0.f;
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
	// the comfortable weight: a small mark on the line
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(ScaleComfort->Slot))
	{
		S->SetPosition(LoadAt + FVector2D(2.f + LoadW * FMath::Clamp(Inv->ComfortableKg / FMath::Max(Inv->MaxKg, 1.f), 0.f, 1.f), 112.f));
	}
}

void UBackpackWidget::UseItem(FName Item)
{
	APawn* Pawn = GetOwningPlayerPawn();
	UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	Select(Item);
	if ((Item == TEXT("Ration") || Item == TEXT("Water")) && Inv)
	{
		Inv->Use(Item);            // OnChanged refreshes the counts and the page
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
	PageAge += DeltaTime;
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
	const FVector2D View = Geometry.GetLocalSize();
	const float Fade = PackEaseOut(T / 0.25f);
	Blur->SetBlurStrength(7.f * Fade);
	Vignette->SetRenderOpacity(Fade);
	if (ClothMID)
	{
		ClothMID->SetScalarParameterValue(TEXT("Light"), PackEaseOut((T - 0.05f) / 0.4f));
		ClothMID->SetScalarParameterValue(TEXT("Aspect"), View.Y > 1.f ? float(View.X / View.Y) : 1.7778f);
	}

	// the things are laid down one after another; the chosen one lifts a little into the light
	const float Time = bClosing ? 10.f : OpenTime;
	const float Breathe = 0.86f + 0.14f * FMath::Sin(OpenTime * 0.8f + 1.3f);
	for (UBackpackItemWidget* W : Items)
	{
		const float D = W->Delay < 0.f ? 1.f : FMath::Clamp((Time - W->Delay) / 0.32f, 0.f, 1.f);
		const float Down = PackEaseOut(D);
		const bool bChosen = W->Item == Selected;
		W->HoverAlpha = FMath::FInterpTo(W->HoverAlpha, bChosen ? 1.f : 0.f, DeltaTime, 12.f);
		const float H = W->HoverAlpha;
		const float Close = bClosing ? 1.f - Fade : 0.f;
		const float Base = W->bEmpty ? 0.32f : FMath::Lerp(0.82f, 1.f, H);
		W->SetRenderOpacity(FMath::Clamp(D * 2.5f, 0.f, 1.f) * (1.f - Close) * Base);
		W->SetRenderTransform(FWidgetTransform(FVector2D(0.f, (1.f - Down) * -26.f - H * 8.f + Close * 30.f),
			FVector2D(1.f + H * 0.06f), FVector2D::ZeroVector, W->BaseAngle * (1.f - H * 0.5f)));
		if (W->Light) { W->Light->SetRenderOpacity(H * Breathe); }
		if (W->Shadow)
		{
			// lifted, the shadow spreads and softens, and falls a touch further from the lamp
			W->Shadow->SetRenderOpacity(FMath::Lerp(1.f, 0.7f, H));
			W->Shadow->SetRenderTranslation(FVector2D(W->Size * (0.05f + 0.03f * H), W->Size * (W->ShadowY + 0.04f * H)));
		}
	}

	// the page comes in from the right, and again each time the chosen thing changes
	const float Side = PackEaseOutBack((T - 0.2f) / 0.32f);
	const float SideOpacity = FMath::Clamp((T - 0.2f) / 0.15f, 0.f, 1.f);
	const float Page = PackEaseOut(PageAge / 0.25f);
	for (UWidget* W : TArray<UWidget*>{ CardTitle, CardCount, CardBody, CardStats, CardAction })
	{
		W->SetRenderOpacity(SideOpacity * Page);
		W->SetRenderTranslation(FVector2D((1.f - Side) * 140.f + (1.f - Page) * 12.f, 0.f));
	}

	// the load: a line of light that grows to the weight like a spring (overshoots, settles); warm when heavy
	APawn* Pawn = GetOwningPlayerPawn();
	const UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const float Load = Inv ? Inv->GetLoadKg() : 0.f;
	const float MaxKg = Inv ? FMath::Max(Inv->MaxKg, 1.f) : 30.f;
	const float Target = T > 0.3f ? Load : 0.f;
	const float Accel = (Target - ShownLoadKg) * 90.f - ScaleVelocity * 9.f;
	ScaleVelocity += Accel * FMath::Min(DeltaTime, 0.05f);
	ShownLoadKg += ScaleVelocity * FMath::Min(DeltaTime, 0.05f);
	const float Frac = FMath::Clamp(ShownLoadKg / MaxKg, 0.f, 1.02f);
	const FLinearColor LoadC = !Inv || Load <= Inv->ComfortableKg ? YggUI::Snow : Load < Inv->MaxKg ? YggUI::Sun : YggUI::EmberC;
	ScaleMark->SetDesiredSizeOverride(FVector2D(FMath::Max(1.f, LoadW * Frac), 2.f));
	ScaleMark->SetColorAndOpacity(LoadC);
	if (ScaleGlow)
	{
		ScaleGlow->SetColorAndOpacity(YggUI::WithA(LoadC, 0.24f));
		ScaleGlow->SetRenderScale(FVector2D(FMath::Max(LoadW * Frac + 20.f, 8.f) / 8.f, 18.f / 8.f));
		ScaleGlow->SetRenderTranslation(FVector2D(LoadW * Frac * 0.5f, 0.f));
	}
	for (UWidget* W : TArray<UWidget*>{ Header, SubHeader, ScaleText, ScaleStatus, Scale, ScaleMark, ScaleComfort, ScaleGlow, PromptRow })
	{
		if (W) { W->SetRenderOpacity(FMath::Clamp((T - 0.1f) / 0.25f, 0.f, 1.f)); }
	}
}
