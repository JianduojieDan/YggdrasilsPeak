// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/BackpackWidget.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/InventoryComponent.h"
#include "Survival/StoryClue.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Survival/YggSettings.h"
#include "Survival/YggUIStyle.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

static TAutoConsoleVariable<int32> CVarPackFont(TEXT("ygg.Pack.Font"), 3,
	TEXT("Pack screen lettering (3 = the author's pick, 2026-11-14): 0 = Roboto, 1 = Alegreya (the menus' serif), 2 = journal (Alegreya headings, Caveat hand-written text), 3 = Cinzel headings, Cormorant Garamond text."));

namespace
{
	// HUD v8's colours (SurvivalHUDWidget, Tools/UI/hud_v8_mockup.py): one white, one red for danger, a blue-black shadow
	const FLinearColor PackWhite = FLinearColor::FromSRGBColor(FColor(244, 246, 248));
	const FLinearColor PackRed = FLinearColor::FromSRGBColor(FColor(238, 70, 58));
	const FLinearColor PackInk = FLinearColor::FromSRGBColor(FColor(0, 6, 16));
	constexpr float PackSoft = 0.72f;           // the second voice: units, labels, hints
	constexpr float PackBlurStrength = 8.f;     // the mockup blurs the game hard so the words read; strength costs GPU
	constexpr int32 PackCapsTrack = 220;        // 2.4 px on 11 px caps, in thousandths of an em

	float PackEaseOut(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f - FMath::Pow(1.f - T, 3.f);
	}

	enum class EPackKit : uint8 { Ration, Water, Oxygen, IceAxe, Rope, Stove, Gas, Journal, Compass };

	struct FPackThingInfo
	{
		EPackKit Kit;
		int32 Group;             // 0 food and water, 1 gear, 2 notes
		float GearKg;            // the weight shown for a thing in the base gear (supplies read theirs from the inventory)
		int32 Effect;            // what it gives, as a HUD icon (PackEffectNames), -1 nothing
		FText Title;
		FText Body;
	};

	/** The HUD's own line icons for what a thing gives (Tools/UI/hud_icons.py -> /Game/UI/HUD/Icons6). */
	const TCHAR* const PackEffectNames[] = { TEXT("T_HUD6_Food"), TEXT("T_HUD6_Drop"), TEXT("T_HUD6_Lungs") };

	/** The list, top to bottom - also the keyboard's order: food and water, the gear, the notes. */
	const TArray<FName>& PackListOrder()
	{
		static const TArray<FName> Order = { TEXT("Ration"), TEXT("Water"), TEXT("Oxygen"), TEXT("IceAxe"), TEXT("Rope"), TEXT("Stove"), TEXT("Gas"), TEXT("Journal"), TEXT("Compass") };
		return Order;
	}

	const FPackThingInfo* PackThingInfo(FName Item)
	{
		static TMap<FName, FPackThingInfo> Infos;
		if (Infos.Num() == 0)
		{
			Infos.Add(TEXT("Ration"), { EPackKit::Ration, 0, 0.f, 0, NSLOCTEXT("Pack", "RationT", "Rations"),
				NSLOCTEXT("Pack", "RationB", "Tins from the shop in Namche, a twist of tsampa, dried yak cheese. Mother packed food like this for every trek. It tastes like home and freezes like stone.") });
			Infos.Add(TEXT("Water"), { EPackKit::Water, 0, 0.f, 1, NSLOCTEXT("Pack", "WaterT", "Thermos"),
				NSLOCTEXT("Pack", "WaterB", "Snow melted on the stove at the last camp, still a little warm. Up here the air takes water out of you with every breath.") });
			Infos.Add(TEXT("Oxygen"), { EPackKit::Oxygen, 0, 0.f, 2, NSLOCTEXT("Pack", "O2T", "Oxygen bottle"),
				NSLOCTEXT("Pack", "O2B", "Four litres at three hundred bar. Above the South Col, a bottle is not air - it is time.") });
			Infos.Add(TEXT("IceAxe"), { EPackKit::IceAxe, 1, 0.5f, -1, NSLOCTEXT("Pack", "AxeT", "Ice axe"),
				NSLOCTEXT("Pack", "AxeB", "Balance on the ice, a step cut where the crampons will not hold, the last thing between a slip and a fall.") });
			Infos.Add(TEXT("Rope"), { EPackKit::Rope, 1, 2.4f, -1, NSLOCTEXT("Pack", "RopeT", "Rope and harness"),
				NSLOCTEXT("Pack", "RopeB", "Clip into the fixed lines and they hold you. Steeper ground can be climbed while you are on a rope.") });
			Infos.Add(TEXT("Stove"), { EPackKit::Stove, 1, 0.4f, -1, NSLOCTEXT("Pack", "StoveT", "Stove"),
				NSLOCTEXT("Pack", "StoveB", "Melts snow into water at the camps. Without it there is nothing to drink above the last stream.") });
			Infos.Add(TEXT("Gas"), { EPackKit::Gas, 1, 0.3f, -1, NSLOCTEXT("Pack", "GasT", "Gas cartridge"),
				NSLOCTEXT("Pack", "GasB", "The cold makes it weak. Sleep with it inside the bag.") });
			Infos.Add(TEXT("Journal"), { EPackKit::Journal, 2, 0.2f, -1, NSLOCTEXT("Pack", "JournalT", "Mother's journal"), FText::GetEmpty() });
			Infos.Add(TEXT("Compass"), { EPackKit::Compass, 2, 0.1f, -1, NSLOCTEXT("Pack", "CompassT", "Father's compass"),
				NSLOCTEXT("Pack", "CompassB", "Found in the box of their things. The runes round the dial belong to no language I know. An old compass, nothing more - that is what I keep telling myself.") });
		}
		return Infos.Find(Item);
	}

	/** Each row's studio render (Saved/ItemIcons/render -> /Game/UI/Backpack/Kit/T_Kit_<Name>, the picture the mockup
	 *  shows), then the effect icons: names made once, so waiting for them costs no strings. */
	const TArray<FName>& PackPictureNames()
	{
		static TArray<FName> Names;
		if (Names.Num() == 0)
		{
			for (const FName Item : PackListOrder())
			{
				Names.Add(FName(*(FString(TEXT("T_Kit_")) + Item.ToString())));
			}
		}
		return Names;
	}

	const TArray<FName>& PackEffectIconNames()
	{
		static TArray<FName> Names;
		if (Names.Num() == 0)
		{
			for (const TCHAR* Name : PackEffectNames)
			{
				Names.Add(FName(Name));
			}
		}
		return Names;
	}

	const FString& PackGroupCaps(int32 Group)
	{
		static const FString Caps[3] = {
			NSLOCTEXT("PackScreen", "GroupFood", "Food and water").ToUpper().ToString(),
			NSLOCTEXT("PackScreen", "GroupGear", "Gear").ToUpper().ToString(),
			NSLOCTEXT("PackScreen", "GroupNotes", "Notes").ToUpper().ToString() };
		return Caps[FMath::Clamp(Group, 0, 2)];
	}

	const USurvivalConfig* PackSurvivalConfig(const USurvivalComponent* Survival)
	{
		return Survival && Survival->Config ? Survival->Config.Get() : GetDefault<USurvivalConfig>();
	}

	/** "0.6" - one decimal, the reader's own separator. */
	FText PackKg(float Kg)
	{
		FNumberFormattingOptions O;
		O.SetMinimumFractionalDigits(1);
		O.SetMaximumFractionalDigits(1);
		return FText::AsNumber(Kg, &O);
	}

	/** "[TAB]" -> "[Tab]": the settings name keys in capitals; the mockup's hints are quieter. */
	FString PackKeyCase(const FString& Label)
	{
		FString Out = Label.ToLower();
		bool bStart = true;
		for (int32 i = 0; i < Out.Len(); ++i)
		{
			TCHAR& Ch = Out[i];
			if (FChar::IsAlpha(Ch))
			{
				if (bStart) { Ch = FChar::ToUpper(Ch); }
				bStart = false;
			}
			else
			{
				bStart = Ch == TEXT(' ') || Ch == TEXT('[');
			}
		}
		return Out;
	}

	// ---------------------------------------------------------------- art, streamed in

	TMap<FName, TWeakObjectPtr<UTexture2D>>& PackArtCache()
	{
		static TMap<FName, TWeakObjectPtr<UTexture2D>> Cache;
		return Cache;
	}

	UTexture2D* PackArt(FName Name)
	{
		const TWeakObjectPtr<UTexture2D>* T = PackArtCache().Find(Name);
		return T ? T->Get() : nullptr;
	}

	/** Streams in the pictures and icons not in memory yet. The asset manager keeps the handle, so they stay for the
	 *  session: opening the pack never waits on the disk (until they arrive the page has no picture). */
	void PackRequestArt(const TArray<FName>& Names)
	{
		static bool bInFlight = false;
		if (bInFlight || !UAssetManager::IsInitialized())
		{
			return;
		}
		TArray<FSoftObjectPath> Paths;
		for (const FName Name : Names)
		{
			if (!PackArt(Name))
			{
				const FString S = Name.ToString();
				const TCHAR* Dir = S.StartsWith(TEXT("T_HUD6_")) ? TEXT("/Game/UI/HUD/Icons6/") : TEXT("/Game/UI/Backpack/Kit/");
				Paths.Emplace(FString::Printf(TEXT("%s%s.%s"), Dir, *S, *S));
			}
		}
		if (Paths.Num() == 0)
		{
			return;
		}
		bInFlight = true;
		FStreamableAsyncLoadParams Params;
		Params.TargetsToStream = Paths;
		Params.bManageActiveHandle = true;
		Params.OnComplete = FStreamableDelegateWithHandle::CreateLambda([Paths](TSharedPtr<FStreamableHandle>)
		{
			bInFlight = false;
			for (const FSoftObjectPath& Path : Paths)
			{
				if (UTexture2D* T = Cast<UTexture2D>(Path.ResolveObject()))
				{
					PackArtCache().Add(Path.GetAssetFName(), T);
				}
			}
		});
		Params.OnCancel = FStreamableDelegateWithHandle::CreateLambda([](TSharedPtr<FStreamableHandle>) { bInFlight = false; });
		const TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Params), FString(TEXT("PackScreenArt")));
		if (!Handle.IsValid())
		{
			bInFlight = false;     // nothing was started (no valid path): the next open may try again
		}
	}

	// ---------------------------------------------------------------- layout and drawing

	/** The mockup (Tools/UI/pack_hud_mockups.py build2) is drawn on 1920 x 1080; it is scaled to fit the screen and centred.
	 *  The interface size setting still counts (the screen's own DPI scale is taken out), as far as the layout fits. */
	struct FPackFrame
	{
		FVector2f Size = FVector2f::ZeroVector;
		FVector2f Origin = FVector2f::ZeroVector;
		float S = 1.f;
		float X(float Px) const { return Origin.X + Px * S; }
		float Y(float Py) const { return Origin.Y + Py * S; }
	};

	FPackFrame PackFrameFor(const FVector2f& Size)
	{
		FPackFrame F;
		F.Size = Size;
		const float Fit = FMath::Min(Size.X / 1920.f, Size.Y / 1080.f);
		const UUserInterfaceSettings* UISettings = GetDefault<UUserInterfaceSettings>();
		const float Interface = UISettings ? FMath::Clamp(UISettings->ApplicationScale, 0.5f, 2.f) : 1.f;
		F.S = FMath::Max(0.25f, FMath::Min(Fit * Interface, FMath::Min(Size.X / 1700.f, Size.Y / 900.f)));
		F.Origin = (Size - FVector2f(1920.f, 1080.f) * F.S) * 0.5f;
		return F;
	}

	/** The voices on the pack screen, so each lettering can give them different faces. */
	enum class EPackType : uint8 { Head, Body, Caps, Number };

	int32 PackFontMode() { return FMath::Clamp(CVarPackFont.GetValueOnGameThread(), 0, 3); }

	/** One face as a font, made once and kept for the session (the HUD's way, YggUI::MakeFace). A face that does not
	 *  load is tried once, not on every frame. */
	UFont* PackFace(int32 Which)
	{
		static const TCHAR* const Paths[4] = {
			TEXT("/Game/UI/Fonts/FF_Alegreya.FF_Alegreya"), TEXT("/Game/UI/Fonts/FF_Caveat.FF_Caveat"),
			TEXT("/Game/UI/Fonts/FF_Cinzel.FF_Cinzel"), TEXT("/Game/UI/Fonts/FF_CormorantGaramond.FF_CormorantGaramond") };
		static TStrongObjectPtr<UFont> Made[4];
		static bool bTried[4] = { false, false, false, false };
		if (Which < 0 || Which > 3) { return nullptr; }
		if (!bTried[Which])
		{
			bTried[Which] = true;
			if (UFont* F = YggUI::MakeFace(GetTransientPackage(), Paths[Which])) { Made[Which].Reset(F); }
		}
		return Made[Which].Get();
	}

	/** The mockup's pixel size (Slate sizes are points: px * 0.75), in quarter points so sizes stay few. Face = the
	 *  Roboto weight (lettering 0); the other letterings pick a face by voice, scaled so each reads about as large as
	 *  Roboto did (their x-heights differ). */
	FSlateFontInfo PackFont(EPackType Type, const TCHAR* Face, float Px, float S, int32 Track = 0)
	{
		int32 Which = -1;
		float K = 1.f;
		switch (PackFontMode())
		{
		case 1: Which = 0; K = 1.08f; break;
		case 2: if (Type == EPackType::Body) { Which = 1; K = 1.32f; } else { Which = 0; K = 1.08f; } break;
		case 3: if (Type == EPackType::Head || Type == EPackType::Caps) { Which = 2; K = 0.9f; } else { Which = 3; K = 1.16f; } break;
		default: break;
		}
		UFont* Font = PackFace(Which);
		if (!Font) { K = 1.f; }   // a face that did not load falls back to Roboto at Roboto's size (review 2026-11-14)
		const float Size = FMath::Max(5.f, FMath::RoundToFloat(Px * K * 0.75f * S * 4.f) * 0.25f);
		FSlateFontInfo F = Font ? FSlateFontInfo(Font, Size, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle(FName(Face), Size);
		F.LetterSpacing = Track;
		return F;
	}

	/** The description in lines as wide as the page; a blank line stays blank; more than MaxLines ends in an ellipsis. */
	void PackWrap(const FString& Text, const FSlateFontInfo& Font, float MaxW, int32 MaxLines, TArray<FString>& OutLines)
	{
		OutLines.Reset();
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		TArray<FString> Paragraphs;
		Text.ParseIntoArray(Paragraphs, TEXT("\n"), false);
		for (const FString& Paragraph : Paragraphs)
		{
			TArray<FString> Words;
			Paragraph.ParseIntoArray(Words, TEXT(" "), true);
			FString Line;
			for (const FString& Word : Words)
			{
				const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
				if (!Line.IsEmpty() && Measure->Measure(Try, Font).X > MaxW)
				{
					OutLines.Add(Line);
					Line = Word;
				}
				else
				{
					Line = Try;
				}
			}
			OutLines.Add(Line);
		}
		if (MaxLines > 0 && OutLines.Num() > MaxLines)
		{
			OutLines.SetNum(MaxLines);
			OutLines.Last() = TEXT("…");
		}
	}

	/** Paints in the HUD's way: white on a tight blue-black shadow. Layer: row light, +1 shadows, +2 the white. */
	struct FPackPen
	{
		const FGeometry& G;
		FSlateWindowElementList& Out;
		int32 Layer;
		float Alpha;

		FLinearColor A(const FLinearColor& C) const { return FLinearColor(C.R, C.G, C.B, C.A * Alpha); }
		FLinearColor Ink(float Opacity) const { return FLinearColor(PackInk.R, PackInk.G, PackInk.B, Opacity * Alpha); }
		static TSharedRef<FSlateFontMeasure> Measure() { return FSlateApplication::Get().GetRenderer()->GetFontMeasureService(); }
		float Width(const FString& T, const FSlateFontInfo& F) const { return T.IsEmpty() ? 0.f : float(Measure()->Measure(T, F).X); }
		// from the top of a text box to its baseline: the mockup places every text on its baseline
		float Base(const FSlateFontInfo& F) const
		{
			const TSharedRef<FSlateFontMeasure> M = Measure();
			return float(M->GetMaxCharacterHeight(F)) + float(M->GetBaseline(F));
		}
		/** Text on a baseline; returns where it ends. */
		float Text(const FString& T, float X, float BaseY, const FSlateFontInfo& F, const FLinearColor& C) const
		{
			if (T.IsEmpty())
			{
				return X;
			}
			const float W = Width(T, F);
			if (C.A * Alpha > 0.003f)
			{
				const FVector2f Pos(X, BaseY - Base(F));
				FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 2.f))), T, F, ESlateDrawEffect::None, Ink(C.A * 0.6f * 0.32f));
				FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(0.5f, 1.2f))), T, F, ESlateDrawEffect::None, Ink(C.A * 0.6f * 0.55f));
				FSlateDrawElement::MakeText(Out, Layer + 2, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)), T, F, ESlateDrawEffect::None, A(C));
			}
			return X + W;
		}
		float TextRight(const FString& T, float Right, float BaseY, const FSlateFontInfo& F, const FLinearColor& C) const
		{
			return Text(T, Right - Width(T, F), BaseY, F, C);
		}
		/** A fine bar, with its own faint shadow a pixel under it. */
		void Rect(FVector2f Pos, FVector2f Size, const FLinearColor& C) const
		{
			if (Size.X <= 0.f || Size.Y <= 0.f || C.A * Alpha <= 0.003f)
			{
				return;
			}
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos + FVector2f(0.f, 1.f))), White, ESlateDrawEffect::None, Ink(0.35f * C.A));
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, A(C));
		}
		/** The chosen row's faint light, under everything else. */
		void Back(FVector2f Pos, FVector2f Size, const FLinearColor& C) const
		{
			FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, A(C));
		}
		void Brush(const FSlateBrush& B, FVector2f Pos, FVector2f Size, const FLinearColor& C, float Shadow) const
		{
			if (Shadow > 0.f)
			{
				FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos + FVector2f(0.5f, 1.f))), &B, ESlateDrawEffect::None, Ink(Shadow * C.A));
			}
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), &B, ESlateDrawEffect::None, A(C));
		}
		void Lines(TArray<FVector2f>&& Pts, const FLinearColor& C, float T) const
		{
			if (Pts.Num() < 2)
			{
				return;
			}
			TArray<FVector2f> Under = Pts;
			for (FVector2f& P : Under)
			{
				P += FVector2f(0.5f, 1.f);
			}
			FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), MoveTemp(Under), ESlateDrawEffect::None, Ink(0.45f * C.A), true, T + 1.f);
			FSlateDrawElement::MakeLines(Out, Layer + 2, G.ToPaintGeometry(), MoveTemp(Pts), ESlateDrawEffect::None, A(C), true, T);
		}
		void Ring(FVector2f At, float R, const FLinearColor& C, float T) const
		{
			TArray<FVector2f> P;
			P.Reserve(25);
			for (int32 i = 0; i <= 24; ++i)
			{
				const float An = UE_TWO_PI * i / 24.f;
				P.Add(At + FVector2f(FMath::Cos(An), FMath::Sin(An)) * R);
			}
			Lines(MoveTemp(P), C, T);
		}
		/** A small filled dot: a ring as thick as it is wide. */
		void Bead(FVector2f At, float R, const FLinearColor& C) const { Ring(At, R * 0.5f, C, R); }
	};

	/** The kit's line icons in the HUD's stroke (Tools/UI/pack_hud_mockups.py kit_icon), drawn on a 256 box. The mockup's
	 *  outlines sit inside their boxes, so the stroke's middle is half a stroke (5) in. */
	void PackKitIcon(const FPackPen& P, EPackKit Kit, FVector2f At, float H, const FLinearColor& C)
	{
		const float K = H / 256.f;
		const float T = FMath::Max(1.1f, 11.f * K);
		const float Thin = FMath::Max(0.9f, 8.f * K);
		const auto Poly = [&](std::initializer_list<FVector2f> Pts, float Thick)
		{
			TArray<FVector2f> L;
			L.Reserve(int32(Pts.size()));
			for (const FVector2f& Pt : Pts)
			{
				L.Add(At + Pt * K);
			}
			P.Lines(MoveTemp(L), C, Thick);
		};
		const auto Round = [&](float X0, float Y0, float X1, float Y1, float R)
		{
			X0 += 5.f; Y0 += 5.f; X1 -= 5.f; Y1 -= 5.f;
			R = FMath::Max(0.f, R - 5.f);
			const FVector2f Corner[4] = { FVector2f(X1 - R, Y0 + R), FVector2f(X1 - R, Y1 - R), FVector2f(X0 + R, Y1 - R), FVector2f(X0 + R, Y0 + R) };
			TArray<FVector2f> L;
			L.Reserve(29);
			for (int32 c = 0; c < 4; ++c)
			{
				for (int32 s = 0; s <= 6; ++s)
				{
					const float An = FMath::DegreesToRadians(-90.f + 90.f * c + 15.f * s);
					L.Add(At + (Corner[c] + FVector2f(FMath::Cos(An), FMath::Sin(An)) * R) * K);
				}
			}
			const FVector2f First = L[0];   // a copy: TArray refuses to add one of its own elements
			L.Add(First);
			P.Lines(MoveTemp(L), C, T);
		};
		const auto Oval = [&](float CX, float CY, float RX, float RY, float Thick)
		{
			TArray<FVector2f> L;
			L.Reserve(33);
			for (int32 s = 0; s <= 32; ++s)
			{
				const float An = UE_TWO_PI * s / 32.f;
				L.Add(At + FVector2f(CX + FMath::Cos(An) * RX, CY + FMath::Sin(An) * RY) * K);
			}
			P.Lines(MoveTemp(L), C, Thick);
		};
		switch (Kit)
		{
		case EPackKit::Ration:
			Round(52.f, 92.f, 204.f, 200.f, 18.f);
			Poly({ { 57.f, 122.f }, { 199.f, 122.f } }, T);
			Oval(167.f, 79.f, 13.5f, 5.5f, Thin);
			break;
		case EPackKit::Water:
			Round(88.f, 74.f, 168.f, 226.f, 24.f);
			Round(106.f, 36.f, 150.f, 74.f, 8.f);
			Poly({ { 93.f, 122.f }, { 163.f, 122.f } }, T);
			break;
		case EPackKit::Oxygen:
			Round(96.f, 76.f, 160.f, 228.f, 30.f);
			Poly({ { 128.f, 81.f }, { 128.f, 44.f } }, T);
			Poly({ { 104.f, 44.f }, { 152.f, 44.f } }, T);
			break;
		case EPackKit::IceAxe:
			Poly({ { 88.f, 222.f }, { 156.f, 56.f } }, T);
			Poly({ { 98.f, 60.f }, { 156.f, 56.f }, { 212.f, 82.f } }, T);
			Poly({ { 156.f, 56.f }, { 174.f, 40.f } }, T);
			break;
		case EPackKit::Rope:
			Oval(128.f, 128.f, 77.f, 77.f, T);
			Oval(128.f, 128.f, 53.f, 53.f, T);
			Oval(128.f, 128.f, 29.f, 29.f, T);
			break;
		case EPackKit::Stove:
			Oval(128.f, 108.f, 43.f, 43.f, T);
			Poly({ { 128.f, 151.f }, { 128.f, 210.f } }, T);
			Poly({ { 86.f, 210.f }, { 170.f, 210.f } }, T);
			Poly({ { 94.f, 70.f }, { 162.f, 70.f } }, T);
			break;
		case EPackKit::Gas:
			Round(74.f, 100.f, 182.f, 222.f, 34.f);
			Round(114.f, 60.f, 142.f, 100.f, 6.f);
			break;
		case EPackKit::Journal:
			Round(74.f, 46.f, 186.f, 214.f, 10.f);
			Poly({ { 100.f, 51.f }, { 100.f, 209.f } }, T);
			break;
		case EPackKit::Compass:
			Oval(128.f, 128.f, 77.f, 77.f, T);
			Poly({ { 128.f, 66.f }, { 144.f, 128.f }, { 128.f, 190.f }, { 112.f, 128.f }, { 128.f, 66.f } }, Thin);
			P.Bead(At + FVector2f(128.f, 128.f) * K, FMath::Max(1.f, 7.f * K), C);
			break;
		}
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

// ---------------------------------------------------------------- pack screen

void UBackpackWidget::CloseIfOpen(APlayerController* PC)
{
	// the pack takes every click on the screen; under a death page it would swallow the clicks that miss a button
	if (!PC) { return; }
	TArray<UUserWidget*> Open;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, UBackpackWidget::StaticClass(), false);
	for (UUserWidget* W : Open)
	{
		if (UBackpackWidget* Pack = Cast<UBackpackWidget>(W)) { if (!Pack->bClosing) { Pack->Close(); } }
	}
}

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
	// the whole screen takes the mouse: hover and click find the row under the pointer (RowAt)
	SetVisibility(ESlateVisibility::Visible);

	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PackCanvas"));
	WidgetTree->RootWidget = Canvas;

	// the world behind: blurred, then the dark (the mockup: blur, then 62 % of a deep night blue)
	Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass());
	UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(Blur);
	BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BS->SetOffsets(FMargin(0.f));
	Blur->SetBlurStrength(0.f);

	Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	FSlateBrush B;
	B.DrawAs = ESlateBrushDrawType::Box;
	B.TintColor = FSlateColor(FLinearColor::White);
	Dim->SetBrush(B);
	Dim->SetColorAndOpacity(YggUI::WithA(FLinearColor::FromSRGBColor(FColor(6, 10, 18)), 0.62f));
	Dim->SetVisibility(ESlateVisibility::HitTestInvisible);
	Dim->SetRenderOpacity(0.f);
	UCanvasPanelSlot* DS = Canvas->AddChildToCanvas(Dim);
	DS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DS->SetOffsets(FMargin(0.f));
	// everything else is painted (NativePaint)
}

void UBackpackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OpenTime = 0.f;
	ShownLoadKg = 0.f;
	APawn* Pawn = GetOwningPlayerPawn();
	if (UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr)
	{
		Inv->OnChanged.AddUniqueDynamic(this, &UBackpackWidget::HandleInventoryChanged);
	}
	if (UExpeditionComponent* Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr)
	{
		Expedition->OnMessage.AddUniqueDynamic(this, &UBackpackWidget::HandleExpeditionMessage);
	}

	// the pictures and icons: one brush each, sized now and never again (the paint keeps pointers to them)
	Pictures.SetNum(PackPictureNames().Num());
	EffectIcons.SetNum(PackEffectIconNames().Num());
	TArray<FName> Art = PackPictureNames();
	Art.Append(PackEffectIconNames());
	PackRequestArt(Art);

	// what the keys do; the pack's own key as the player bound it
	KeysLine = FString::Printf(TEXT("%s  [W A S D]      %s  [Enter]      %s  %s"),
		*NSLOCTEXT("PackScreen", "Choose", "choose").ToString(), *NSLOCTEXT("PackScreen", "Use", "use").ToString(),
		*NSLOCTEXT("PackScreen", "Close", "close").ToString(), *PackKeyCase(YggSettings::KeyLabel(TEXT("Backpack"), EKeys::Tab)));

	RefreshRows(true);
	TakeArt();
	Select(TEXT("Ration"));
	if (RowLight.IsValidIndex(PageRow))
	{
		RowLight[PageRow] = 1.f;     // the first choice is simply lit, not eased in
	}
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
	if (UExpeditionComponent* Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr)
	{
		Expedition->OnMessage.RemoveDynamic(this, &UBackpackWidget::HandleExpeditionMessage);
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
		// a fresh press only: Tab held a moment too long after opening the pack would close it again on its first repeat
		if (OpenTime > 0.15f && !Event.IsRepeat())
		{
			Close();
		}
		return FReply::Handled();
	}
	// keyboard / pad: step through the list top to bottom (round at the ends), use the chosen thing
	const bool bPrev = Key == EKeys::Up || Key == EKeys::Left || Key == EKeys::W || Key == EKeys::A
		|| Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Up;
	const bool bNext = Key == EKeys::Down || Key == EKeys::Right || Key == EKeys::S || Key == EKeys::D
		|| Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Down;
	if (bPrev || bNext)
	{
		// a key held down repeats, but only one pressed here may: W or the stick still held from walking when the pack
		// opened only sends repeats, and those would spin the (wrapping) list until it is let go
		if (!Event.IsRepeat())
		{
			HeldStepKey = Key;
		}
		else if (Key != HeldStepKey)
		{
			return FReply::Handled();
		}
		const TArray<FName>& Order = PackListOrder();
		const int32 Index = FMath::Max(Order.IndexOfByKey(Selected), 0);
		Select(Order[(Index + (bPrev ? -1 : 1) + Order.Num()) % Order.Num()]);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		// one per press: held down, the key's repeats would eat the whole pack
		if (!Event.IsRepeat())
		{
			UseItem(Selected);
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

int32 UBackpackWidget::RowAt(const FGeometry& Geometry, const FVector2f& ScreenPos) const
{
	const FPackFrame F = PackFrameFor(FVector2f(Geometry.GetLocalSize()));
	const FVector2f L = FVector2f(Geometry.AbsoluteToLocal(ScreenPos));
	const float X0 = F.X(150.f);
	if (L.X < X0 || L.X > X0 + 640.f * F.S)
	{
		return -1;
	}
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		// the row and the small gap under its line, so the pointer never falls between two rows
		const float Top = F.Y(Rows[i].Top);
		if (L.Y >= Top && L.Y < Top + 56.f * F.S)
		{
			return i;
		}
	}
	return -1;
}

FReply UBackpackWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	// pointing at a row chooses it (only when the pointer comes onto another row, so the keys keep their choice)
	const int32 Row = bClosing ? -1 : RowAt(Geometry, Event.GetScreenSpacePosition());
	if (Row != HoverRow)
	{
		HoverRow = Row;
		if (Rows.IsValidIndex(Row))
		{
			Select(Rows[Row].Item);
		}
	}
	return Super::NativeOnMouseMove(Geometry, Event);
}

void UBackpackWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
	HoverRow = -1;
	Super::NativeOnMouseLeave(Event);
}

FReply UBackpackWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const int32 Row = bClosing ? -1 : RowAt(Geometry, Event.GetScreenSpacePosition());
		if (Rows.IsValidIndex(Row))
		{
			UseItem(Rows[Row].Item);
		}
	}
	// every button is handled anywhere on the screen: a click must not reach the game (the axe) under the pack, and
	// a right click that did would take the keyboard focus to the game, so Esc and the arrows stop reaching the pack
	return FReply::Handled();
}

FReply UBackpackWidget::NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	// Slate turns a quick second click into a double click (no second mouse-down): it uses nothing more and, like a
	// click, it does not reach the game (there it would count as a press)
	return FReply::Handled();
}

void UBackpackWidget::HandleInventoryChanged()
{
	RefreshRows(true);
	RefreshPage();
}

void UBackpackWidget::HandleExpeditionMessage(const FText& Message, float Seconds)
{
	// "You don't need that right now", "Used a ration": the HUD shows it under the blur; the page shows it where the eye is
	MessageText = Message.ToString();
	MessageTime = FMath::Clamp(Seconds, 1.5f, 6.f);
}

void UBackpackWidget::RefreshRows(bool bPages)
{
	APawn* Pawn = GetOwningPlayerPawn();
	const UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	const UExpeditionComponent* Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr;
	const TArray<FName>& Order = PackListOrder();

	// laid out once; afterwards only the counts and weights change
	if (Rows.Num() != Order.Num())
	{
		Rows.SetNum(Order.Num());
		RowLight.Init(0.f, Order.Num());
		int32 Breaks = 0;
		for (int32 i = 0; i < Order.Num(); ++i)
		{
			const FPackThingInfo* I = PackThingInfo(Order[i]);
			FPackRow& R = Rows[i];
			R.Item = Order[i];
			R.Kit = I ? int32(I->Kit) : 0;
			R.Group = I ? I->Group : 0;
			R.Name = I ? I->Title.ToString() : Order[i].ToString();
			R.bFirstOfGroup = i == 0 || R.Group != Rows[i - 1].Group;
			Breaks += (i > 0 && R.bFirstOfGroup) ? 1 : 0;
			// the mockup's list: the first row under its caps at 280, 56 px a row, 26 + 18 px more for each new group
			R.Top = 280.f + i * 56.f + Breaks * 44.f;
		}
	}
	if (bPages)
	{
		// the journal's pages found so far, in the order of the route
		JournalPages.Reset();
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AStoryClue> It(World); It; ++It)
			{
				if (Expedition && Expedition->HasFoundClue(It->ClueId))
				{
					JournalPages.Add(It->Title.ToString());
				}
			}
		}
	}
	const int32 Bottles = Survival ? Survival->GetCarriedBottles() : 0;
	for (FPackRow& R : Rows)
	{
		const FPackThingInfo* I = PackThingInfo(R.Item);
		float Kg = I ? I->GearKg : 0.f;
		R.Count.Reset();
		R.bDanger = false;
		switch (EPackKit(R.Kit))
		{
		case EPackKit::Ration:
		case EPackKit::Water:
		{
			const int32 N = Inv ? Inv->GetCount(R.Item) : 0;
			const FPackItemDef* Def = Inv ? Inv->ItemDefs.Find(R.Item) : nullptr;
			Kg = Def ? Def->WeightKg : Kg;
			R.Count = FString::FromInt(N);
			R.bDanger = N <= 0;
			break;
		}
		case EPackKit::Oxygen:
			Kg = Inv ? Inv->OxygenBottleKg : 3.f;
			R.Count = FString::Printf(TEXT("%d / %d"), Bottles, PackSurvivalConfig(Survival)->MaxCarriedBottles);
			R.bDanger = Bottles <= 0;
			break;
		case EPackKit::Journal:
			R.Count = FText::Format(NSLOCTEXT("PackScreen", "PagesCount", "{0} {0}|plural(one=page,other=pages)"), JournalPages.Num()).ToString();
			break;
		default:
			break;
		}
		R.Weight = FText::Format(NSLOCTEXT("PackScreen", "RowKg", "{0} kg"), PackKg(Kg)).ToString();
	}

	// the load (the bar springs towards it in the tick)
	LoadKg = Inv ? Inv->GetLoadKg() : 0.f;
	MaxKg = Inv ? FMath::Max(Inv->MaxKg, 1.f) : 26.f;
	ComfortKg = Inv ? Inv->ComfortableKg : 16.f;
	PackLine = PackKg(LoadKg).ToString();
	PackMaxLine = FText::Format(NSLOCTEXT("PackScreen", "OfMax", "/ {0} kg"), FText::AsNumber(MaxKg)).ToString();
	PackNote = (LoadKg >= MaxKg - 0.05f ? NSLOCTEXT("PackScreen", "Full", "full: nothing more fits")
		: FText::Format(NSLOCTEXT("PackScreen", "Heavier", "heavier than {0} kg: every step costs more"), FText::AsNumber(ComfortKg))).ToString();
}

void UBackpackWidget::Select(FName Item)
{
	if (Selected != Item)
	{
		Selected = Item;
		RefreshPage();
		PageAge = 0.f;
	}
}

void UBackpackWidget::RefreshPage()
{
	const int32 Row = Rows.IndexOfByPredicate([this](const FPackRow& R) { return R.Item == Selected; });
	const FPackThingInfo* I = PackThingInfo(Selected);
	if (!I || !Rows.IsValidIndex(Row))
	{
		return;
	}
	const FPackRow& R = Rows[Row];
	APawn* Pawn = GetOwningPlayerPawn();
	const UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;

	FPackPage P;
	P.Group = PackGroupCaps(I->Group);
	P.Title = R.Name;
	P.Body = I->Body.ToString();
	P.Effect = I->Effect;
	P.bDanger = R.bDanger;
	const FText NoneLeft = NSLOCTEXT("PackScreen", "NoneLeft", "None left");
	// the gear's weights are shown for each thing, but they are all in the base load the pack always carries
	const FText GearWeight = FText::Format(NSLOCTEXT("PackScreen", "GearKg", "{0} kg  ·  in the {1} kg you always carry"),
		PackKg(I->GearKg), FText::AsNumber(Inv ? Inv->BaseGearKg : 10.f));
	switch (I->Kit)
	{
	case EPackKit::Ration:
	case EPackKit::Water:
	{
		const bool bFood = I->Kit == EPackKit::Ration;
		const FPackItemDef* Def = Inv ? Inv->ItemDefs.Find(Selected) : nullptr;
		P.EffectValue = FString::Printf(TEXT("+%d"), FMath::RoundToInt(Def ? (bFood ? Def->Satiety : Def->Hydration) : 0.f));
		P.Count = R.Count;
		P.CountUnit = NSLOCTEXT("PackScreen", "Left", "left").ToString();
		P.Weight = FText::Format(NSLOCTEXT("PackScreen", "EachKg", "{0} kg each"), PackKg(Def ? Def->WeightKg : 0.f)).ToString();
		if (R.bDanger)
		{
			P.Danger = NoneLeft.ToString();
		}
		else
		{
			P.Action = FText::Format(NSLOCTEXT("PackScreen", "UseEnter", "{0}  [Enter]"),
				bFood ? NSLOCTEXT("PackScreen", "Eat", "eat one") : NSLOCTEXT("PackScreen", "Drink", "drink")).ToString();
			P.ActionHint = FText::Format(NSLOCTEXT("PackScreen", "OnTheMove", "or {0} on the move"), FText::FromString(PackKeyCase(bFood
				? YggSettings::KeyLabel(TEXT("Eat"), EKeys::One) : YggSettings::KeyLabel(TEXT("Drink"), EKeys::Two)))).ToString();
		}
		break;
	}
	case EPackKit::Oxygen:
	{
		const int32 N = Survival ? Survival->GetCarriedBottles() : 0;
		const float Flowing = Survival ? Survival->GetBottleSecondsLeft() : 0.f;
		P.EffectValue = FText::Format(NSLOCTEXT("PackScreen", "Minutes", "{0} min"),
			FMath::RoundToInt(PackSurvivalConfig(Survival)->OxygenBottleSeconds / 60.f)).ToString();
		P.Count = R.Count;
		P.CountUnit = NSLOCTEXT("PackScreen", "Spare", "spare").ToString();
		P.Weight = FText::Format(NSLOCTEXT("PackScreen", "EachKg", "{0} kg each"), PackKg(Inv ? Inv->OxygenBottleKg : 3.f)).ToString();
		if (Flowing > 0.f)
		{
			P.Action = FText::Format(NSLOCTEXT("PackScreen", "Flowing", "breathing one now  ·  {0} min left"), FMath::CeilToInt(Flowing / 60.f)).ToString();
		}
		else if (N > 0)
		{
			P.Action = NSLOCTEXT("PackScreen", "Valve", "open the valve  [Enter]").ToString();
			P.ActionHint = NSLOCTEXT("PackScreen", "ByItself", "one goes on by itself when the air runs low").ToString();
		}
		else
		{
			P.Danger = NoneLeft.ToString();
			P.DangerHint = NSLOCTEXT("PackScreen", "CampsKeep", "camps on the way up keep a few").ToString();
		}
		break;
	}
	case EPackKit::Journal:
	{
		P.Count = FString::FromInt(JournalPages.Num());
		P.CountUnit = FText::Format(NSLOCTEXT("PackScreen", "PagesUnit", "{0}|plural(one=page,other=pages)"), JournalPages.Num()).ToString();
		if (JournalPages.Num() > 0)
		{
			TArray<FString> Lines;
			for (const FString& Title : JournalPages)
			{
				Lines.Add(FString(TEXT("—  ")) + Title);
			}
			P.Body = NSLOCTEXT("PackScreen", "PagesFound", "Her pages are scattered along their route. What I have found:").ToString() + TEXT("\n\n") + FString::Join(Lines, TEXT("\n"));
		}
		else
		{
			P.Body = NSLOCTEXT("Pack", "NoPages", "Her pages are scattered along their route, somewhere above. Nothing found yet.").ToString();
		}
		P.Weight = GearWeight.ToString();
		P.Action = FText::Format(NSLOCTEXT("PackScreen", "ReadPage", "read a page where you find it  {0}"),
			FText::FromString(PackKeyCase(YggSettings::KeyLabel(TEXT("Interact"), EKeys::E)))).ToString();
		break;
	}
	case EPackKit::Compass:
		P.Weight = GearWeight.ToString();
		P.Action = FText::Format(NSLOCTEXT("PackScreen", "HoldUp", "hold it up  {0}"),
			FText::FromString(PackKeyCase(YggSettings::KeyLabel(TEXT("Compass"), EKeys::Q)))).ToString();
		break;
	default:
		P.Weight = GearWeight.ToString();
		break;
	}
	// the description is re-wrapped only when it (or the room it may take) changed
	if (Row != PageRow || P.Body != Page.Body || P.Danger.IsEmpty() != Page.Danger.IsEmpty())
	{
		++BodySerial;
	}
	PageRow = Row;
	Page = MoveTemp(P);
}

void UBackpackWidget::TakeArt()
{
	// Pictures[i] belongs to row i: both follow PackListOrder
	bool bAll = true;
	const TArray<FName>& PictureNames = PackPictureNames();
	for (int32 i = 0; i < Pictures.Num() && i < PictureNames.Num(); ++i)
	{
		if (Pictures[i].GetResourceObject())
		{
			continue;
		}
		if (UTexture2D* T = PackArt(PictureNames[i]))
		{
			HeldArt.Add(T);
			Pictures[i] = YggUI::TextureBrush(T, FVector2D(200.f, 200.f));
		}
		else
		{
			bAll = false;
		}
	}
	const TArray<FName>& IconNames = PackEffectIconNames();
	for (int32 e = 0; e < EffectIcons.Num() && e < IconNames.Num(); ++e)
	{
		if (EffectIcons[e].GetResourceObject())
		{
			continue;
		}
		if (UTexture2D* T = PackArt(IconNames[e]))
		{
			HeldArt.Add(T);
			EffectIcons[e] = YggUI::TextureBrush(T, FVector2D(128.f, 128.f));
		}
		else
		{
			bAll = false;
		}
	}
	bArtReady = bAll;
}

void UBackpackWidget::UseItem(FName Item)
{
	APawn* Pawn = GetOwningPlayerPawn();
	UInventoryComponent* Inv = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	Select(Item);
	if ((Item == TEXT("Ration") || Item == TEXT("Water")) && Inv)
	{
		Inv->Use(Item);            // OnChanged refreshes the counts and the page; "not needed" comes back as a message
	}
	else if (Item == TEXT("Oxygen") && Survival && Survival->GetCarriedBottles() > 0 && Survival->GetBottleSecondsLeft() <= 0.f)
	{
		Survival->SetCarriedBottles(Survival->GetCarriedBottles() - 1);
		Survival->UseOxygenBottle();
		RefreshRows(false);
		RefreshPage();
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
	// the world behind: blurred, then the dark; the words come a moment later
	const float Fade = PackEaseOut(T / 0.25f);
	const float BlurNow = PackBlurStrength * Fade;
	if (Blur && !FMath::IsNearlyEqual(BlurNow, LastBlur, 0.01f))
	{
		Blur->SetBlurStrength(BlurNow);
		LastBlur = BlurNow;
	}
	if (Dim)
	{
		Dim->SetRenderOpacity(Fade);
	}
	ContentAlpha = PackEaseOut((T - 0.08f) / 0.25f);

	// the chosen row's light eases over from the last one
	for (int32 i = 0; i < Rows.Num() && i < RowLight.Num(); ++i)
	{
		RowLight[i] = FMath::FInterpTo(RowLight[i], Rows[i].Item == Selected ? 1.f : 0.f, DeltaTime, 16.f);
	}

	// the load: the fine bar grows to the weight like a spring (overshoots a little, settles)
	const float Dt = FMath::Min(DeltaTime, 0.05f);
	const float Target = T > 0.3f ? LoadKg : 0.f;
	ScaleVelocity += ((Target - ShownLoadKg) * 90.f - ScaleVelocity * 9.f) * Dt;
	ShownLoadKg += ScaleVelocity * Dt;

	MessageTime = FMath::Max(0.f, MessageTime - DeltaTime);
	if (!bArtReady)
	{
		TakeArt();
	}
	// once a second: what changes without the pack hearing of it (a bottle put on by itself, its minutes left)
	RefreshClock += DeltaTime;
	if (RefreshClock > 1.f && !bClosing)
	{
		RefreshClock = 0.f;
		RefreshRows(false);
		RefreshPage();
	}
}

int32 UBackpackWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// the blur and the dark (child widgets) first, then the painted screen above them
	const int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
	if (ContentAlpha <= 0.003f || Rows.Num() == 0 || !FSlateApplication::IsInitialized())
	{
		return Layer;
	}
	const FGeometry& G = AllottedGeometry;
	const FPackFrame F = PackFrameFor(FVector2f(G.GetLocalSize()));
	const float S = F.S;
	const FPackPen P{ G, OutDrawElements, Layer, ContentAlpha };
	const float Slide = (1.f - ContentAlpha) * 14.f * S;      // the list comes in from the left, the page from the right
	const float LineH = FMath::Max(1.f, 2.f * S);
	const FSlateFontInfo CapF = PackFont(EPackType::Caps, TEXT("Medium"), 11.f, S, PackCapsTrack);
	const FSlateFontInfo SmallF = PackFont(EPackType::Body, TEXT("Regular"), 15.f, S);
	const FLinearColor Soft = YggUI::WithA(PackWhite, PackSoft);

	// ---- left: "Pack", then the rows under their group's caps: line icon, name, count, weight, a fine line under each
	static const FString TitleText = NSLOCTEXT("Pack", "Header", "Pack").ToString();
	const float LX = F.X(150.f) - Slide;
	P.Text(TitleText, LX - 3.f * S, F.Y(190.f), PackFont(EPackType::Head, TEXT("Light"), 54.f, S), YggUI::WithA(PackWhite, 0.98f));
	const FSlateFontInfo NameF = PackFont(EPackType::Head, TEXT("Regular"), 20.f, S);
	const FSlateFontInfo CountF = PackFont(EPackType::Number, TEXT("Light"), 26.f, S);
	const float RW = 640.f * S;
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		const FPackRow& R = Rows[i];
		const float Top = F.Y(R.Top);
		const float L = RowLight.IsValidIndex(i) ? RowLight[i] : 0.f;
		if (R.bFirstOfGroup)
		{
			P.Text(PackGroupCaps(R.Group), LX, Top - 18.f * S, CapF, Soft);
		}
		if (L > 0.01f)
		{
			P.Back(FVector2f(LX, Top), FVector2f(RW, 52.f * S), YggUI::WithA(PackWhite, 0.08f * L));
		}
		PackKitIcon(P, EPackKit(R.Kit), FVector2f(LX + 14.f * S, Top + 11.f * S), 30.f * S, YggUI::WithA(PackWhite, FMath::Lerp(0.82f, 1.f, L)));
		P.Text(R.Name, LX + 62.f * S, Top + 34.f * S, NameF, YggUI::WithA(PackWhite, FMath::Lerp(0.86f, 1.f, L)));
		if (!R.Count.IsEmpty())
		{
			P.TextRight(R.Count, LX + RW - 150.f * S, Top + 35.f * S, CountF, R.bDanger ? PackRed : PackWhite);
		}
		P.TextRight(R.Weight, LX + RW - 14.f * S, Top + 34.f * S, SmallF, Soft);
		P.Rect(FVector2f(LX, Top + 52.f * S), FVector2f(RW, LineH), YggUI::WithA(PackWhite, FMath::Lerp(0.12f, 0.95f, L)));
	}

	// ---- right, on top: the pack's weight - a light numeral, a fine bar to the most you can carry, a tick where it
	// starts to cost
	static const FString PackCaps = NSLOCTEXT("PackScreen", "PackCaps", "Pack").ToUpper().ToString();
	const float DX = F.X(1000.f) + Slide;
	P.Text(PackCaps, DX, F.Y(192.f), CapF, Soft);
	const float NumEnd = P.Text(PackLine, DX - 2.f * S, F.Y(238.f), PackFont(EPackType::Number, TEXT("Light"), 44.f, S), YggUI::WithA(PackWhite, 0.98f));
	P.Text(PackMaxLine, NumEnd + 6.f * S, F.Y(238.f), PackFont(EPackType::Number, TEXT("Light"), 20.f, S), Soft);
	const float BarW = 420.f * S, BarY = F.Y(250.f);
	P.Rect(FVector2f(DX, BarY), FVector2f(BarW, LineH), YggUI::WithA(PackWhite, 0.3f));
	P.Rect(FVector2f(DX, BarY), FVector2f(BarW * FMath::Clamp(ShownLoadKg / MaxKg, 0.f, 1.02f), LineH), YggUI::WithA(PackWhite, 0.95f));
	P.Rect(FVector2f(DX + BarW * FMath::Clamp(ComfortKg / MaxKg, 0.f, 1.f), BarY - 4.f * S), FVector2f(LineH, 10.f * S), YggUI::WithA(PackWhite, 0.6f));
	P.Text(PackNote, DX, F.Y(274.f), PackFont(EPackType::Body, TEXT("Regular"), 14.f, S), Soft);

	// ---- right: the chosen thing's page (it comes in again each time the choice changes) - its picture, group, name,
	// what it gives, how many are left, its weight, the words, what the keys do with it
	const float PageIn = PackEaseOut(PageAge / 0.2f);
	const FPackPen Q{ G, OutDrawElements, Layer, ContentAlpha * PageIn };
	const float PX = DX + (1.f - PageIn) * 10.f * S;
	if (Pictures.IsValidIndex(PageRow) && Pictures[PageRow].GetResourceObject())
	{
		Q.Brush(Pictures[PageRow], FVector2f(PX, F.Y(380.f)), FVector2f(200.f * S, 200.f * S), FLinearColor::White, 0.f);
	}
	float Y = 614.f;
	Q.Text(Page.Group, PX, F.Y(Y), CapF, Soft);
	Q.Text(Page.Title, PX - 2.f * S, F.Y(Y + 48.f), PackFont(EPackType::Head, TEXT("Light"), 46.f, S), YggUI::WithA(PackWhite, 0.98f));
	Y += 92.f;
	const FSlateFontInfo ReadF = PackFont(EPackType::Number, TEXT("Light"), 30.f, S);
	float RX = PX;
	if (!Page.EffectValue.IsEmpty())
	{
		const float IH = 22.f * S;
		if (EffectIcons.IsValidIndex(Page.Effect) && EffectIcons[Page.Effect].GetResourceObject())
		{
			Q.Brush(EffectIcons[Page.Effect], FVector2f(RX, F.Y(Y) - IH), FVector2f(IH, IH), YggUI::WithA(PackWhite, 0.92f), 0.45f);
		}
		RX = Q.Text(Page.EffectValue, RX + IH + 9.f * S, F.Y(Y), ReadF, PackWhite) + 34.f * S;
	}
	if (!Page.Count.IsEmpty())
	{
		const FLinearColor C = Page.bDanger ? PackRed : PackWhite;
		RX = Q.Text(Page.Count, RX, F.Y(Y), ReadF, C) + 8.f * S;
		RX = Q.Text(Page.CountUnit, RX, F.Y(Y), SmallF, YggUI::WithA(C, PackSoft)) + 34.f * S;
	}
	Q.Text(Page.Weight, RX, F.Y(Y), SmallF, Soft);
	Y += 40.f;
	const FSlateFontInfo BodyF = PackFont(EPackType::Body, TEXT("Regular"), 18.f, S);
	static int32 WrappedFontMode = -1;   // the wrap depends on the lettering too (ygg.Pack.Font can change while open)
	if (BodyLinesSerial != BodySerial || BodyLinesScale != S || WrappedFontMode != PackFontMode())
	{
		// as many lines as leave room for the action and a message above the keys
		PackWrap(Page.Body, BodyF, 600.f * S, Page.Danger.IsEmpty() ? 7 : 5, BodyLines);
		BodyLinesSerial = BodySerial;
		BodyLinesScale = S;
		WrappedFontMode = PackFontMode();
	}
	for (const FString& Line : BodyLines)
	{
		Q.Text(Line, PX, F.Y(Y), BodyF, YggUI::WithA(PackWhite, 0.88f));
		Y += 27.f;
	}
	if (!Page.Danger.IsEmpty())
	{
		// red only here: a dot, "None left", and where to find more
		Q.Bead(FVector2f(PX + 4.f * S, F.Y(Y + 18.f)), 3.5f * S, PackRed);
		const float HintX = Q.Text(Page.Danger, PX + 16.f * S, F.Y(Y + 23.f), PackFont(EPackType::Body, TEXT("Medium"), 16.f, S), PackRed) + 10.f * S;
		Q.Text(Page.DangerHint, HintX, F.Y(Y + 23.f), PackFont(EPackType::Body, TEXT("Regular"), 16.f, S), YggUI::WithA(PackWhite, 0.88f));
		Y += 40.f;
	}
	if (!Page.Action.IsEmpty())
	{
		Y += 30.f;
		const float HintX = Q.Text(Page.Action, PX, F.Y(Y), PackFont(EPackType::Body, TEXT("Regular"), 17.f, S), YggUI::WithA(PackWhite, 0.95f)) + 18.f * S;
		Q.Text(Page.ActionHint, HintX, F.Y(Y), SmallF, Soft);
	}
	if (MessageTime > 0.f && !MessageText.IsEmpty())
	{
		P.Text(MessageText, PX, F.Y(Y + 44.f), PackFont(EPackType::Body, TEXT("Regular"), 16.f, S), YggUI::WithA(PackWhite, 0.88f * FMath::Clamp(MessageTime / 0.4f, 0.f, 1.f)));
	}

	// ---- what the keys do, bottom right of the screen itself
	P.TextRight(KeysLine, F.Size.X - 56.f * S, F.Size.Y - 52.f * S, SmallF, Soft);
	return Layer + 3;
}
