// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggLoadingScreen.h"

#include "Survival/YggSaveGame.h"
#include "Survival/YggUIStyle.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "MoviePlayer.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"

namespace
{
	/** The loading screen (v3, 2026-11-13): the captured view drifts in under cloud shadow; where you are, large, in the
	 *  serif, its height under it; one line of advice; "Loading" breathing in the corner. No frame, no emblem. (Runs on
	 *  the loading thread: textures only, no materials.) */
	class SYggLoading : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SYggLoading) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, UTexture2D* InStill, UFont* InSerif, const FString& InPlace)
		{
			if (InStill) { StillBrush = YggUI::TextureBrush(InStill, FVector2D(InStill->GetSizeX(), InStill->GetSizeY())); bHasStill = true; StillAspect = float(InStill->GetSizeX()) / FMath::Max(1, InStill->GetSizeY()); }
			VeilBrush = YggUI::TextureBrush(YggUI::VeilTexture(), FVector2D(256.f, 64.f));
			VignetteBrush = YggUI::TextureBrush(YggUI::VignetteTexture(), FVector2D(128.f, 128.f));
			Serif = InSerif;
			// "Dingboche (4410 m)" -> the name, and its height on a line of its own
			Place = InPlace;
			const int32 Br = InPlace.Find(TEXT("("));
			if (Br > 0)
			{
				Place = InPlace.Left(Br).TrimStartAndEnd();
				FString H = InPlace.Mid(Br + 1).Replace(TEXT(")"), TEXT("")).TrimStartAndEnd();
				const int32 Sp = H.Find(TEXT(" "));
				const FString Num = Sp > 0 ? H.Left(Sp) : H;
				if (Num.IsNumeric() && Num.Len() > 3) { H = Num.Left(Num.Len() - 3) + TEXT(",") + Num.Right(3) + (Sp > 0 ? H.Mid(Sp) : FString()); }
				Height = H + TEXT(" above the sea");
			}
			static const TCHAR* const Lines[] = {
				TEXT("They went up anyway."),
				TEXT("Something grows above the summit."),
				TEXT("Rest at a camp to warm through. Each camp keeps your climb."),
				TEXT("When the weather wheel turns to danger, find shelter before it lands."),
				TEXT("Above eight thousand metres every breath is borrowed. Carry a spare bottle."),
				TEXT("The mountain does not know you came."),
				TEXT("Where the ice steepens there are fixed ropes. Clip in, and trust them."),
				TEXT("Eat before you are hungry. Drink before you are thirsty. The cold takes the rest."),
			};
			Line = Lines[FMath::RandRange(0, UE_ARRAY_COUNT(Lines) - 1)];
			Start = FSlateApplication::Get().GetCurrentTime();
		}

		virtual bool ComputeVolatility() const override { return true; }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer,
			const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			const FVector2f View = FVector2f(G.GetLocalSize());
			const float T = float(FSlateApplication::Get().GetCurrentTime() - Start);
			const float In = FMath::Clamp(T / 1.4f, 0.f, 1.f);
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			// night first, then the mountain slowly drifting in
			FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), White, ESlateDrawEffect::None, YggUI::InkC);
			if (bHasStill)
			{
				FVector2f Size(View.X, View.X / StillAspect);
				if (Size.Y < View.Y) { Size = FVector2f(View.Y * StillAspect, View.Y); }
				Size *= 1.06f + 0.004f * T;
				const FVector2f Pos = (View - Size) * 0.5f + FVector2f(-6.f * T, 0.f);
				FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), &StillBrush, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, In * 0.9f));
			}
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(), &VeilBrush, ESlateDrawEffect::None, YggUI::WithA(YggUI::InkC, 0.88f));
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(), &VignetteBrush, ESlateDrawEffect::None, YggUI::WithA(YggUI::InkC, 0.55f));
			auto Font = [this](int32 Sz) { return Serif ? FSlateFontInfo(Serif, Sz, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle("Regular", Sz); };
			auto TextAt = [&](const FString& S, float PX, float PY, const FSlateFontInfo& F, const FLinearColor& C)
			{
				FSlateDrawElement::MakeText(Out, Layer + 3, G.ToPaintGeometry(FVector2f(1400.f, 160.f), FSlateLayoutTransform(FVector2f(PX, PY + 1.f))), S, F, ESlateDrawEffect::None, FLinearColor(0.f, 0.005f, 0.02f, 0.5f * C.A));
				FSlateDrawElement::MakeText(Out, Layer + 4, G.ToPaintGeometry(FVector2f(1400.f, 160.f), FSlateLayoutTransform(FVector2f(PX, PY))), S, F, ESlateDrawEffect::None, C);
			};
			const float X = View.X * 0.085f;
			const float Y = View.Y * 0.62f;
			TextAt(Place, X, Y, Font(96), YggUI::WithA(YggUI::Snow, In));
			if (!Height.IsEmpty()) { TextAt(Height, X + 3.f, Y + 156.f, Font(22), YggUI::WithA(YggUI::Mist, In)); }
			TextAt(Line, X + 3.f, Y + 204.f, Font(22), YggUI::WithA(YggUI::Shade, In * YggUI::Ease((T - 0.6f) * 1.5f)));
			// "Loading", breathing, bottom right
			const FSlateFontInfo LF = Font(19);
			const FString L = TEXT("Loading");
			const float LW = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(L, LF).X;
			TextAt(L, View.X * 0.95f - LW, View.Y - 70.f, LF, YggUI::WithA(YggUI::Mist, In * (0.4f + 0.6f * (0.5f + 0.5f * FMath::Sin(T * 2.f)))));
			return Layer + 5;
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.f, 1080.f); }

	private:
		FSlateBrush StillBrush, VeilBrush, VignetteBrush;
		bool bHasStill = false;
		float StillAspect = 16.f / 9.f;
		TObjectPtr<UFont> Serif = nullptr;
		FString Place, Height, Line;
		double Start = 0.0;
	};
}

void UYggLoadingScreen::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (IsRunningDedicatedServer()) { return; }
	PreHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UYggLoadingScreen::OnPreLoadMap);
	PostHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UYggLoadingScreen::OnPostLoadMap);
}

void UYggLoadingScreen::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostHandle);
	Super::Deinitialize();
}

void UYggLoadingScreen::OnPreLoadMap(const FString& MapName)
{
	if (GIsEditor || !MapName.Contains(TEXT("base_map")) || !IsMoviePlayerEnabled())
	{
		return;
	}
	if (!Still)
	{
		Still = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/StartMenu/Backdrops/T_Backdrop_01.T_Backdrop_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!Serif) { Serif = YggUI::SerifFont(); }
	FString Place = TEXT("Dingboche");
	if (const UYggSaveGame* Save = UGameplayStatics::DoesSaveGameExist(UYggSaveGame::SlotName, 0) ? Cast<UYggSaveGame>(UGameplayStatics::LoadGameFromSlot(UYggSaveGame::SlotName, 0)) : nullptr)
	{
		if (!Save->CampName.IsEmpty()) { Place = Save->CampName; }
	}
	FLoadingScreenAttributes A;
	A.bAutoCompleteWhenLoadingCompletes = true;
	A.bWaitForManualStop = false;
	A.MinimumLoadingScreenDisplayTime = 2.5f;
	A.WidgetLoadingScreen = SNew(SYggLoading, Still.Get(), Serif.Get(), Place);
	GetMoviePlayer()->SetupLoadingScreen(A);
}

void UYggLoadingScreen::OnPostLoadMap(UWorld* World)
{
}
