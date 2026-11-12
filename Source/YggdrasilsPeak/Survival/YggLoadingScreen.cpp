// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggLoadingScreen.h"

#include "Survival/YggSaveGame.h"
#include "Survival/YggUIStyle.h"
#include "Survival/YggEmblem.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "MoviePlayer.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"

namespace
{
	/** Small capitals spaced out with thin spaces (FSlateFontInfo::LetterSpacing does not reach MakeText here). */
	FString Spaced(const FString& S)
	{
		FString R;
		for (int32 i = 0; i < S.Len(); ++i)
		{
			if (i > 0) { R.AppendChar(TCHAR(0x2009)); }
			R.AppendChar(S[i]);
			if (S[i] == TEXT(' ')) { R.AppendChar(TCHAR(0x2009)); R.AppendChar(TCHAR(0x2009)); }
		}
		return R;
	}

	class SYggLoading : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SYggLoading) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, UTexture2D* InStill, UFont* InSerif, const FString& InPlace)
		{
			if (InStill) { StillBrush = YggUI::TextureBrush(InStill, FVector2D(InStill->GetSizeX(), InStill->GetSizeY())); bHasStill = true; StillAspect = float(InStill->GetSizeX()) / FMath::Max(1, InStill->GetSizeY()); }
			VeilBrush = YggUI::TextureBrush(YggUI::VeilTexture(), FVector2D(256.f, 64.f));
			GlowBrush = YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f));
			VignetteBrush = YggUI::TextureBrush(YggUI::VignetteTexture(), FVector2D(128.f, 128.f));
			GrainBrush = YggUI::TextureBrush(YggUI::GrainTexture(), FVector2D(128.f, 128.f));
			GrainBrush.Tiling = ESlateBrushTileType::Both;
			Serif = InSerif;
			Place = InPlace;
			static const TCHAR* const Lines[] = {
				TEXT("They went up anyway."),
				TEXT("Something grows above the summit."),
				TEXT("Rest at a camp to warm through. Each camp keeps your climb."),
				TEXT("When the weather wheel turns to danger, find shelter before it lands."),
				TEXT("Above eight thousand metres every breath is borrowed. Carry a spare bottle."),
				TEXT("The mountain does not know you came."),
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
			const float In = FMath::Clamp(T / 1.2f, 0.f, 1.f);
			// black, then the mountain slowly drifting in
			FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor::Black);
			if (bHasStill)
			{
				FVector2f Size(View.X, View.X / StillAspect);
				if (Size.Y < View.Y) { Size = FVector2f(View.Y * StillAspect, View.Y); }
				const float Zoom = 1.06f + 0.004f * T;
				Size *= Zoom;
				const FVector2f Pos = (View - Size) * 0.5f + FVector2f(-6.f * T, 0.f);
				FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), &StillBrush, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, In * 0.85f));
			}
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(), &VeilBrush, ESlateDrawEffect::None, FLinearColor(0.004f, 0.006f, 0.012f, 0.9f));
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(), &VignetteBrush, ESlateDrawEffect::None, FLinearColor(0.004f, 0.006f, 0.012f, 0.6f));
			// the menus' frame: a hairline at the top with the game's name, one at the bottom with the loader
			{
				const FLinearColor Rl(0.93f, 0.91f, 0.84f, 0.16f * In);
				const float L = View.X * 0.06f, Rr = View.X * 0.94f;
				FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(Rr - L, 1.f), FSlateLayoutTransform(FVector2f(L, 76.f))), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Rl);
				FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(Rr - L, 1.f), FSlateLayoutTransform(FVector2f(L, View.Y - 78.f))), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Rl);
				FSlateFontInfo TagF = FCoreStyle::GetDefaultFontStyle("Regular", 9); TagF.LetterSpacing = 420;
				FSlateDrawElement::MakeText(Out, Layer + 3, G.ToPaintGeometry(FVector2f(600.f, 20.f), FSlateLayoutTransform(FVector2f(L, 50.f))), Spaced(TEXT("YGGDRASIL'S PEAK")), TagF, ESlateDrawEffect::None, FLinearColor(0.93f, 0.91f, 0.84f, 0.66f * In));
				// the emblem as the loader: the realms' light runs round the ring while the world loads
				YggEmblem::Paint(Out, Layer + 4, G, FVector2f(Rr - 30.f, View.Y - 128.f), 60.f, T, In, 1.f);
				FSlateFontInfo Ld = FCoreStyle::GetDefaultFontStyle("Regular", 9); Ld.LetterSpacing = 520;
				const int32 Dots = int32(T * 2.f) % 4;
				FSlateDrawElement::MakeText(Out, Layer + 4, G.ToPaintGeometry(FVector2f(300.f, 20.f), FSlateLayoutTransform(FVector2f(Rr - 190.f, View.Y - 56.f))),
					Spaced(TEXT("LOADING")) + FString::ChrN(Dots, TEXT('.')), Ld, ESlateDrawEffect::None, FLinearColor(0.93f, 0.91f, 0.84f, 0.6f * In));
			}
			// the words, bottom left
			const float X = View.X * 0.08f;
			const float Y = View.Y * 0.66f;
			const FLinearColor Iv(0.93f, 0.91f, 0.84f, 0.96f * In), Dm(0.93f, 0.91f, 0.84f, 0.66f * In), Ft(0.93f, 0.91f, 0.84f, 0.5f * In);
			FSlateFontInfo Small = FCoreStyle::GetDefaultFontStyle("Regular", 10); Small.LetterSpacing = 420;
			FSlateFontInfo Big = Serif ? FSlateFontInfo(Serif, 54, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle("Regular", 54);
			FSlateFontInfo Body = Serif ? FSlateFontInfo(Serif, 18, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle("Light", 18);
			auto TextAt = [&](const FString& S, float PX, float PY, const FSlateFontInfo& F, const FLinearColor& C, int32 L)
			{
				FSlateDrawElement::MakeText(Out, Layer + L, G.ToPaintGeometry(FVector2f(1200.f, 120.f), FSlateLayoutTransform(FVector2f(PX, PY))), S, F, ESlateDrawEffect::None, C);
			};
			TextAt(Spaced(TEXT("THE WAY UP")), X, Y, Small, Dm, 3);
			TextAt(Place, X, Y + 18.f, Big, Iv, 3);
			TextAt(Line, X, Y + 112.f, Body, Dm, 3);
			// a hairline with a gold bead travelling along it while the world loads
			const float W = 300.f, LY = Y + 162.f;
			FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(W, 1.f), FSlateLayoutTransform(FVector2f(X, LY))), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(0.93f, 0.91f, 0.84f, 0.3f * In));
			const float K = 0.5f - 0.5f * FMath::Cos(T * 1.4f);
			const float BX = X + W * K;
			FSlateDrawElement::MakeBox(Out, Layer + 4, G.ToPaintGeometry(FVector2f(22.f, 22.f), FSlateLayoutTransform(FVector2f(BX - 11.f, LY - 11.f))), &GlowBrush, ESlateDrawEffect::None, FLinearColor(0.96f, 0.84f, 0.58f, 0.55f * In));
			FSlateDrawElement::MakeBox(Out, Layer + 5, G.ToPaintGeometry(FVector2f(6.f, 6.f), FSlateLayoutTransform(FVector2f(BX - 3.f, LY - 3.f))), &GlowBrush, ESlateDrawEffect::None, FLinearColor(1.f, 0.92f, 0.72f, In));
			TextAt(Spaced(TEXT("EVEREST · SOUTH COL ROUTE")), X, LY + 14.f, Small, Ft, 3);
			return Layer + 6;
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.f, 1080.f); }

	private:
		FSlateBrush StillBrush, VeilBrush, GlowBrush, VignetteBrush, GrainBrush;
		bool bHasStill = false;
		float StillAspect = 16.f / 9.f;
		TObjectPtr<UFont> Serif = nullptr;
		FString Place, Line;
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
	if (!Serif) { Serif = YggUI::MakeSerif(this); }
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
