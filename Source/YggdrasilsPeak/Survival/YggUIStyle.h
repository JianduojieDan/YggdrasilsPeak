// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// The UI's visual language, v3 (2026-11-13, Docs/Development/UI_Redesign_2026-11-13.md). The user found v2 "AI generated"
// (hairline frames, tracked capitals, eyebrows, beads, numbered options, grain). v3 takes everything from the game's
// light: selection is a word lit by sun through a cloud gap (cold blue-grey words in shadow, the chosen one warm snow-white
// with a soft apricot glow), backgrounds are drifting cloud shadow (M_UI_CloudVeil), one voice (Alegreya, sentence case,
// old-style figures), few words. Roboto only inside key caps.

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/FontFace.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/CoreStyle.h"
#include "UObject/StrongObjectPtr.h"
#include "Survival/YggAssets.h"

namespace YggUI
{
	inline FLinearColor Hex(uint8 R, uint8 G, uint8 B, float A = 1.f) { FLinearColor C = FLinearColor::FromSRGBColor(FColor(R, G, B)); C.A = A; return C; }
	inline FLinearColor WithA(const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, A); }

	// ---- palette (sRGB hex in the comments)
	inline const FLinearColor Snow = Hex(238, 232, 220);      // #EEE8DC lit words, titles
	inline const FLinearColor Mist = Hex(169, 180, 194);      // #A9B4C2 words in shadow, quiet lines
	inline const FLinearColor Shade = Hex(111, 123, 140);     // #6F7B8C hints, the far camps
	inline const FLinearColor InkC = Hex(10, 16, 25);         // #0A1019 veils: deep night blue, never black
	inline const FLinearColor Sun = Hex(244, 184, 132);       // #F4B884 the lit word's glow, the one number that matters
	inline const FLinearColor EmberC = Hex(224, 122, 102);    // #E07A66 danger, sparingly
	inline const FLinearColor Ice = Hex(156, 199, 232);       // #9CC7E8 cold

	// v2 names, still used by older screens: they now speak v3's palette
	inline const FLinearColor Ivory = Snow;
	inline const FLinearColor Dim = Mist;
	inline const FLinearColor Faint = Shade;
	inline const FLinearColor Hair = WithA(Mist, 0.34f);
	inline const FLinearColor Rule = WithA(Mist, 0.13f);
	inline const FLinearColor Gold = Sun;
	inline const FLinearColor Ember = EmberC;
	inline const FLinearColor Ink = InkC;

	/** A transient white texture whose alpha is A(u, v), u and v 0..1 (made once, kept for the session). */
	inline UTexture2D* MakeAlphaTexture(int32 W, int32 H, TFunctionRef<float(float, float)> A, bool bWrap = false)
	{
		UTexture2D* T = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		if (!T) { return nullptr; }
		T->SRGB = false;
		T->Filter = TF_Bilinear;
		T->AddressX = bWrap ? TA_Wrap : TA_Clamp;
		T->AddressY = bWrap ? TA_Wrap : TA_Clamp;
		FColor* Px = static_cast<FColor*>(T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
		for (int32 y = 0; y < H; ++y)
		{
			for (int32 x = 0; x < W; ++x)
			{
				const float a = FMath::Clamp(A((x + 0.5f) / W, (y + 0.5f) / H), 0.f, 1.f);
				Px[y * W + x] = FColor(255, 255, 255, uint8(FMath::RoundToInt(a * 255.f)));
			}
		}
		T->GetPlatformData()->Mips[0].BulkData.Unlock();
		T->UpdateResource();
		T->AddToRoot();
		return T;
	}

	/** Dark on the left where the words are, thinning to the right (the fallback when M_UI_CloudVeil is missing). */
	inline UTexture2D* VeilTexture()
	{
		static TWeakObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid())
		{
			Tex = MakeAlphaTexture(256, 64, [](float u, float v)
			{
				const float Side = FMath::Lerp(0.9f, 0.2f, FMath::SmoothStep(0.f, 0.78f, u));
				const float Vig = 0.18f * FMath::Square(2.f * v - 1.f);
				return Side + Vig;
			});
		}
		return Tex.Get();
	}

	/** A soft round glow; stretched, the light behind a chosen word. */
	inline UTexture2D* GlowTexture()
	{
		static TWeakObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid())
		{
			Tex = MakeAlphaTexture(64, 64, [](float u, float v)
			{
				const float d = FMath::Sqrt(FMath::Square(u - 0.5f) + FMath::Square(v - 0.5f)) * 2.f;
				return FMath::Pow(FMath::Clamp(1.f - d, 0.f, 1.f), 2.2f);
			});
		}
		return Tex.Get();
	}

	/** Soft light, strongest at the left, gone by the right end, soft at top and bottom (settings rows). */
	inline UTexture2D* BandTexture()
	{
		static TWeakObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid())
		{
			Tex = MakeAlphaTexture(256, 32, [](float u, float v)
			{
				const float E = FMath::SmoothStep(0.f, 0.32f, v) * (1.f - FMath::SmoothStep(0.68f, 1.f, v));
				return FMath::Pow(1.f - u, 1.7f) * (0.2f + 0.8f * E);
			});
		}
		return Tex.Get();
	}

	/** Darker corners, a clear middle. */
	inline UTexture2D* VignetteTexture()
	{
		static TWeakObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid())
		{
			Tex = MakeAlphaTexture(128, 128, [](float u, float v)
			{
				const float d = FMath::Sqrt(FMath::Square(u - 0.5f) * 1.2f + FMath::Square(v - 0.5f)) * 2.f;
				return FMath::Pow(FMath::SmoothStep(0.45f, 1.25f, d), 1.4f);
			});
		}
		return Tex.Get();
	}

	/** Opaque at the top, clear at the bottom (masks for the rolling credits). */
	inline UTexture2D* FadeTexture()
	{
		static TWeakObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid())
		{
			Tex = MakeAlphaTexture(4, 64, [](float u, float v) { return 1.f - FMath::SmoothStep(0.f, 1.f, v); });
		}
		return Tex.Get();
	}

	inline FSlateBrush TextureBrush(UObject* T, const FVector2D& Size)
	{
		FSlateBrush B;
		B.SetResourceObject(T);
		B.DrawAs = ESlateBrushDrawType::Image;
		B.ImageSize = Size;
		B.TintColor = FSlateColor(FLinearColor::White);
		return B;
	}

	inline UFont* MakeFace(UObject* Outer, const TCHAR* FacePath)
	{
		// kept loaded: each menu is built again when it opens, and a face that garbage collection had dropped in between
		// was read from the disk on the game thread (2026-11-14, YggAssets.h)
		UFontFace* Face = YggAssets::Kept<UFontFace>(FacePath);
		if (!Face) { return nullptr; }
		UFont* F = NewObject<UFont>(Outer);
		F->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry& Entry = F->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = TEXT("Regular");
		Entry.Font = FFontData(Face);
		return F;
	}
	inline UFont* MakeSerif(UObject* Outer) { return MakeFace(Outer, TEXT("/Game/UI/Fonts/FF_Alegreya.FF_Alegreya")); }
	/** The hand-written face (Caveat): only the Lost death screen writes with it now. */
	inline UFont* MakeHand(UObject* Outer) { return MakeFace(Outer, TEXT("/Game/UI/Fonts/FF_Caveat.FF_Caveat")); }

	/** The one serif for the whole UI, made once and kept (HUD, menus, pack and settings share it). */
	inline UFont* SerifFont()
	{
		static TStrongObjectPtr<UFont> F;
		if (!F.IsValid()) { if (UFont* N = MakeSerif(GetTransientPackage())) { F.Reset(N); } }
		return F.Get();
	}
	inline FSlateFontInfo SerifInfo(int32 Size)
	{
		UFont* F = SerifFont();
		return F ? FSlateFontInfo(F, Size, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
	}

	/** Text with a soft shadow under it (readable on bright snow). Style: "Light" / "Regular" (Roboto), or a serif font.
	 *  Track in 1/1000 em (v3 keeps it at 0 for words; only key caps are tracked). */
	inline UTextBlock* Text(UWidgetTree* Tree, const FString& S, int32 Size, const FLinearColor& C, int32 Track = 0, const TCHAR* Style = TEXT("Light"), UFont* Serif = nullptr)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo F = Serif ? FSlateFontInfo(Serif, Size, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle(FName(Style), Size);
		F.LetterSpacing = Track;
		T->SetFont(F);
		T->SetText(FText::FromString(S));
		T->SetColorAndOpacity(FSlateColor(C));
		T->SetShadowOffset(FVector2D(0.f, 1.f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.005f, 0.02f, 0.5f));
		return T;
	}

	/** v3's words: the serif, sentence case, no tracking. */
	inline UTextBlock* Words(UWidgetTree* Tree, const FString& S, int32 Size, const FLinearColor& C)
	{
		return Text(Tree, S, Size, C, 0, TEXT("Regular"), SerifFont());
	}

	inline UImage* Line(UWidgetTree* Tree, float W, const FLinearColor& C = Hair, float H = 1.2f)
	{
		// an explicit brush size: an empty UImage draws its default 32 px box (the hairlines showed as grey squares)
		UImage* I = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::Box;
		B.TintColor = FSlateColor(FLinearColor::White);
		B.ImageSize = FVector2D(W, H);
		B.Margin = FMargin(0.f);
		I->SetBrush(B);
		I->SetColorAndOpacity(C);
		I->SetDesiredSizeOverride(FVector2D(W, H));
		return I;
	}

	inline UImage* Bead(UWidgetTree* Tree, float D, const FLinearColor& C = Snow)
	{
		UImage* I = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		B.TintColor = FSlateColor(FLinearColor::White);
		B.ImageSize = FVector2D(D, D);
		I->SetBrush(B);
		I->SetColorAndOpacity(C);
		I->SetDesiredSizeOverride(FVector2D(D, D));
		return I;
	}

	/** The soft light behind a chosen word: a stretched glow. It takes no room in the layout (an 8 px box drawn scaled
	 *  up to Size around its centre), so rows and tabs keep their own spacing; move it with SetRenderTranslation. */
	inline UImage* Glow(UWidgetTree* Tree, const FLinearColor& C, const FVector2D& Size)
	{
		UImage* I = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		I->SetBrush(TextureBrush(GlowTexture(), FVector2D(8.f, 8.f)));
		I->SetColorAndOpacity(C);
		I->SetDesiredSizeOverride(FVector2D(8.f, 8.f));
		I->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		I->SetRenderScale(Size / 8.f);
		I->SetVisibility(ESlateVisibility::HitTestInvisible);
		return I;
	}

	/** A full-screen layer of a texture or material (veil, vignette, glow). */
	inline UImage* Layer(UWidgetTree* Tree, UCanvasPanel* Canvas, UObject* Tex, const FLinearColor& C, const FMargin& Offsets = FMargin(0.f))
	{
		UImage* I = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		I->SetBrush(TextureBrush(Tex, FVector2D(64.f, 64.f)));
		I->SetColorAndOpacity(C);
		I->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(I);
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(Offsets);
		return I;
	}

	/** Drifting cloud shadow over the whole screen (M_UI_CloudVeil): Left = how dark where the words stand, Right = the
	 *  far side. Call TickVeil every frame. Falls back to the plain veil texture when the material is missing. */
	inline UImage* CloudVeil(UWidgetTree* Tree, UCanvasPanel* Canvas, UObject* Outer, float Left, float Right, UMaterialInstanceDynamic*& OutMID)
	{
		OutMID = nullptr;
		if (UMaterialInterface* M = YggAssets::Kept<UMaterialInterface>(TEXT("/Game/UI/Materials/M_UI_CloudVeil.M_UI_CloudVeil"), LOAD_NoWarn | LOAD_Quiet))
		{
			OutMID = UMaterialInstanceDynamic::Create(M, Outer);
			OutMID->SetScalarParameterValue(TEXT("Left"), Left);
			OutMID->SetScalarParameterValue(TEXT("Right"), Right);
			// shadow, not fog: the thin parts of the cloud carry only a breath of cold light
			OutMID->SetVectorParameterValue(TEXT("Ink"), InkC);
			OutMID->SetVectorParameterValue(TEXT("Light"), Hex(34, 44, 60));
			OutMID->SetScalarParameterValue(TEXT("Drift"), 0.14f);
			return Layer(Tree, Canvas, OutMID, FLinearColor::White);
		}
		return Layer(Tree, Canvas, VeilTexture(), WithA(InkC, Left));
	}
	inline void TickVeil(UMaterialInstanceDynamic* MID, float Time, const FVector2D& View)
	{
		if (!MID) { return; }
		MID->SetScalarParameterValue(TEXT("T"), Time);
		MID->SetScalarParameterValue(TEXT("Aspect"), View.Y > 1.f ? float(View.X / View.Y) : 1.7778f);
	}

	/** Loads now, and keeps, what the screens opened during play read later: both faces, the cloud veil and the death
	 *  page's fate material (YggMenuBase::UpdateFateWorld loads it by path: kept here, its LoadObject finds it in memory).
	 *  Called when the HUD is built, with the level (2026-11-14: each pause menu and death page read them from the disk on
	 *  the game thread - a freeze whenever the drive stalled). */
	inline void KeepMenuAssets()
	{
		SerifFont();
		YggAssets::Kept<UFontFace>(TEXT("/Game/UI/Fonts/FF_Caveat.FF_Caveat"));
		YggAssets::Kept<UMaterialInterface>(TEXT("/Game/UI/Materials/M_UI_CloudVeil.M_UI_CloudVeil"), LOAD_NoWarn | LOAD_Quiet);
		YggAssets::Kept<UMaterialInterface>(TEXT("/Game/UI/Materials/M_PP_Fate.M_PP_Fate"), LOAD_NoWarn | LOAD_Quiet);
	}

	/** A hairline track with a bead for a thumb. */
	inline USlider* Slider(UWidgetTree* Tree, float W)
	{
		USlider* S = Tree->ConstructWidget<USlider>(USlider::StaticClass());
		FSliderStyle St = S->GetWidgetStyle();
		FSlateBrush Bar;
		Bar.DrawAs = ESlateBrushDrawType::Box;
		Bar.TintColor = FSlateColor(FLinearColor::White);
		Bar.ImageSize = FVector2D(W, 2.f);
		St.NormalBarImage = St.HoveredBarImage = St.DisabledBarImage = Bar;
		St.BarThickness = 2.f;
		FSlateBrush Thumb;
		Thumb.DrawAs = ESlateBrushDrawType::RoundedBox;
		Thumb.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		Thumb.TintColor = FSlateColor(FLinearColor::White);
		Thumb.ImageSize = FVector2D(12.f, 12.f);
		St.NormalThumbImage = St.HoveredThumbImage = St.DisabledThumbImage = Thumb;
		S->SetWidgetStyle(St);
		S->SetSliderBarColor(WithA(Mist, 0.28f));
		S->SetSliderHandleColor(Snow);
		S->SetIndentHandle(false);
		return S;
	}

	/** A key cap: the key's name, small, in a soft rounded outline. */
	inline UBorder* KeyCap(UWidgetTree* Tree, const FString& Key, int32 Size = 9)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		FSlateBrush Br;
		Br.DrawAs = ESlateBrushDrawType::RoundedBox;
		Br.TintColor = FSlateColor(WithA(Mist, 0.06f));
		Br.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Br.OutlineSettings.CornerRadii = FVector4(4.f, 4.f, 4.f, 4.f);
		Br.OutlineSettings.Color = FSlateColor(WithA(Mist, 0.45f));
		Br.OutlineSettings.Width = 1.f;
		B->SetBrush(Br);
		B->SetPadding(FMargin(7.f, 3.f, 7.f, 3.f));
		B->SetHorizontalAlignment(HAlign_Center);
		B->SetVerticalAlignment(VAlign_Center);
		B->SetContent(Text(Tree, Key.ToUpper(), Size, Snow, 60, TEXT("Medium")));
		return B;
	}

	/** Key caps with what they do, the words in the serif: { {"Esc", "Back"}, ... } */
	inline UHorizontalBox* Prompts(UWidgetTree* Tree, const TArray<TPair<FString, FString>>& Items)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			TArray<FString> Keys;
			Items[i].Key.ParseIntoArray(Keys, TEXT(" "));
			for (int32 k = 0; k < Keys.Num(); ++k)
			{
				UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(KeyCap(Tree, Keys[k]));
				S->SetVerticalAlignment(VAlign_Center);
				S->SetPadding(FMargin(i == 0 && k == 0 ? 0.f : (k == 0 ? 30.f : 4.f), 0.f, 0.f, 0.f));
			}
			UHorizontalBoxSlot* L = Row->AddChildToHorizontalBox(Words(Tree, Items[i].Value, 15, Mist));
			L->SetVerticalAlignment(VAlign_Center);
			L->SetPadding(FMargin(9.f, 0.f, 0.f, 1.f));
		}
		return Row;
	}

	/** A button that draws nothing itself (its content is the look). */
	inline UButton* Bare(UWidgetTree* Tree)
	{
		UButton* B = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		FButtonStyle St = B->GetStyle();
		for (FSlateBrush* Br : { &St.Normal, &St.Hovered, &St.Pressed, &St.Disabled }) { Br->DrawAs = ESlateBrushDrawType::NoDrawType; }
		St.NormalPadding = FMargin(0.f);
		St.PressedPadding = FMargin(0.f);
		B->SetStyle(St);
		return B;
	}

	inline float Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	inline float EaseOut(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - FMath::Pow(1.f - T, 3.f); }
}
