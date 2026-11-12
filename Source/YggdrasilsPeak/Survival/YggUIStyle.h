// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// The HUD's visual language for the menus (2026-11-11 user: the boxed menu did not belong with the HUD):
// no cards or boxes, a soft veil over the world, ivory text with a soft halo, Alegreya serif for names, letter-spaced
// small capitals for labels, light numerals with small units, hairlines that end in a bead.
// 2026-11-13 (v2, "industrial level"): a frame of hairlines (header with the game's name and where you are, footer
// with key caps), a film grain and a vignette over every menu, a selection band of soft light with a gold accent
// bar for the chosen row, numbered options, the World Tree emblem (YggEmblem.h).

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
#include "Styling/CoreStyle.h"

namespace YggUI
{
	inline const FLinearColor Ivory(0.93f, 0.91f, 0.84f, 0.96f);
	inline const FLinearColor Dim(0.93f, 0.91f, 0.84f, 0.66f);
	inline const FLinearColor Faint(0.93f, 0.91f, 0.84f, 0.5f);
	inline const FLinearColor Hair(0.93f, 0.91f, 0.84f, 0.32f);
	inline const FLinearColor Rule(0.93f, 0.91f, 0.84f, 0.12f);     // the quiet lines between rows
	inline const FLinearColor Gold(0.96f, 0.84f, 0.58f, 0.98f);    // the chosen one: the bead, an active tab
	inline const FLinearColor Ember(0.88f, 0.46f, 0.34f, 0.96f);   // the death screen's accent
	inline const FLinearColor Ink(0.004f, 0.006f, 0.012f, 1.f);      // the veil

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

	/** The menus' veil: dark on the left where the words are, thinning to the right so the world stays seen. */
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

	/** A soft round glow (also the snow flakes of the start menu). */
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

	/** The selection band: soft light, strongest at the left, gone by the right end, soft at top and bottom. */
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

	/** Film grain, tiles (white specks; drawn at a few percent). */
	inline UTexture2D* GrainTexture()
	{
		static TWeakObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid())
		{
			FRandomStream R(1913);
			TArray<float> V;
			V.SetNumUninitialized(128 * 128);
			for (float& X : V) { X = FMath::Pow(R.FRand(), 2.4f); }
			Tex = MakeAlphaTexture(128, 128, [&V](float u, float v)
			{
				return V[FMath::Clamp(int32(v * 128.f), 0, 127) * 128 + FMath::Clamp(int32(u * 128.f), 0, 127)];
			}, true);
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

	inline FSlateBrush TextureBrush(UTexture2D* T, const FVector2D& Size)
	{
		FSlateBrush B;
		B.SetResourceObject(T);
		B.DrawAs = ESlateBrushDrawType::Image;
		B.ImageSize = Size;
		B.TintColor = FSlateColor(FLinearColor::White);
		return B;
	}

	/** A hairline track with a bead for a thumb. */
	inline USlider* Slider(UWidgetTree* Tree, float W)
	{
		USlider* S = Tree->ConstructWidget<USlider>(USlider::StaticClass());
		FSliderStyle St = S->GetWidgetStyle();
		FSlateBrush Bar;
		Bar.DrawAs = ESlateBrushDrawType::Box;
		Bar.TintColor = FSlateColor(FLinearColor::White);
		Bar.ImageSize = FVector2D(W, 1.f);
		St.NormalBarImage = St.HoveredBarImage = St.DisabledBarImage = Bar;
		St.BarThickness = 1.f;
		FSlateBrush Thumb;
		Thumb.DrawAs = ESlateBrushDrawType::RoundedBox;
		Thumb.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		Thumb.TintColor = FSlateColor(FLinearColor::White);
		Thumb.ImageSize = FVector2D(11.f, 11.f);
		St.NormalThumbImage = St.HoveredThumbImage = St.DisabledThumbImage = Thumb;
		S->SetWidgetStyle(St);
		S->SetSliderBarColor(Hair);
		S->SetSliderHandleColor(Ivory);
		S->SetIndentHandle(false);
		return S;
	}

	inline UFont* MakeSerif(UObject* Outer)
	{
		UFontFace* Face = LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Fonts/FF_Alegreya.FF_Alegreya"));
		if (!Face) { return nullptr; }
		UFont* F = NewObject<UFont>(Outer);
		F->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry& Entry = F->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = TEXT("Regular");
		Entry.Font = FFontData(Face);
		return F;
	}

	/** Text with the HUD's soft halo. Style: "Light" / "Regular" (Roboto), or a serif font. Track in 1/1000 em. */
	inline UTextBlock* Text(UWidgetTree* Tree, const FString& S, int32 Size, const FLinearColor& C, int32 Track = 0, const TCHAR* Style = TEXT("Light"), UFont* Serif = nullptr)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo F = Serif ? FSlateFontInfo(Serif, Size, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle(FName(Style), Size);
		F.LetterSpacing = Track;
		T->SetFont(F);
		T->SetText(FText::FromString(S));
		T->SetColorAndOpacity(FSlateColor(C));
		T->SetShadowOffset(FVector2D(0.f, 1.f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.02f, 0.55f));
		return T;
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

	inline UImage* Bead(UWidgetTree* Tree, float D, const FLinearColor& C = Ivory)
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

	/** A full-screen layer of a texture (veil, vignette, glow). */
	inline UImage* Layer(UWidgetTree* Tree, UCanvasPanel* Canvas, UTexture2D* Tex, const FLinearColor& C, const FMargin& Offsets = FMargin(0.f))
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

	/** The film grain over everything: a tiled layer a little larger than the screen, jittered each frame (Jitter). */
	inline UImage* Grain(UWidgetTree* Tree, UCanvasPanel* Canvas, float Alpha = 0.045f)
	{
		UImage* I = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush B = TextureBrush(GrainTexture(), FVector2D(128.f, 128.f));
		B.Tiling = ESlateBrushTileType::Both;
		I->SetBrush(B);
		I->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Alpha));
		I->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(I);
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(-128.f, -128.f, -128.f, -128.f));
		return I;
	}
	inline void JitterGrain(UImage* G, float& Clock, float Dt)
	{
		Clock += Dt;
		if (G && Clock > 1.f / 24.f)
		{
			Clock = 0.f;
			G->SetRenderTranslation(FVector2D(FMath::FRandRange(-64.f, 64.f), FMath::FRandRange(-64.f, 64.f)));
		}
	}

	/** A key cap: the key's name in a thin rounded outline. */
	inline UBorder* KeyCap(UWidgetTree* Tree, const FString& Key, int32 Size = 9)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		FSlateBrush Br;
		Br.DrawAs = ESlateBrushDrawType::RoundedBox;
		Br.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.04f));
		Br.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Br.OutlineSettings.CornerRadii = FVector4(3.f, 3.f, 3.f, 3.f);
		Br.OutlineSettings.Color = FSlateColor(FLinearColor(0.93f, 0.91f, 0.84f, 0.42f));
		Br.OutlineSettings.Width = 1.f;
		B->SetBrush(Br);
		B->SetPadding(FMargin(7.f, 3.f, 7.f, 3.f));
		B->SetHorizontalAlignment(HAlign_Center);
		B->SetVerticalAlignment(VAlign_Center);
		B->SetContent(Text(Tree, Key.ToUpper(), Size, Ivory, 120, TEXT("Regular")));
		return B;
	}

	/** Key caps with what they do: { {"Esc", "Back"}, ... } */
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
				S->SetPadding(FMargin(i == 0 && k == 0 ? 0.f : (k == 0 ? 26.f : 4.f), 0.f, 0.f, 0.f));
			}
			UHorizontalBoxSlot* L = Row->AddChildToHorizontalBox(Text(Tree, Items[i].Value.ToUpper(), 9, Dim, 280, TEXT("Regular")));
			L->SetVerticalAlignment(VAlign_Center);
			L->SetPadding(FMargin(9.f, 1.f, 0.f, 0.f));
		}
		return Row;
	}

	/** The menus' frame: a header hairline with the game's name and a place on the right, a footer hairline with a
	 *  quiet line on the left; the key caps go into OutPrompts (bottom right, filled by the caller). */
	struct FChrome
	{
		UTextBlock* Crumb = nullptr;
		UTextBlock* Right = nullptr;
		UTextBlock* Foot = nullptr;
		UCanvasPanelSlot* PromptSlot = nullptr;
		TArray<UWidget*> All;
	};
	inline FChrome Chrome(UWidgetTree* Tree, UCanvasPanel* Canvas, float X, bool bHeader)
	{
		FChrome C;
		auto Place = [Canvas](UWidget* W, const FAnchors& A, const FVector2D& Align, const FVector2D& Pos)
		{
			UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
			S->SetAnchors(A);
			S->SetAlignment(Align);
			S->SetPosition(Pos);
			S->SetAutoSize(true);
			return S;
		};
		auto HairRule = [Tree, Canvas, X](float Top, bool bFromBottom)
		{
			UImage* L = Line(Tree, 8.f, FLinearColor(0.93f, 0.91f, 0.84f, 0.16f));
			L->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* S = Canvas->AddChildToCanvas(L);
			S->SetAnchors(bFromBottom ? FAnchors(X, 1.f, 1.f - X, 1.f) : FAnchors(X, 0.f, 1.f - X, 0.f));
			S->SetOffsets(FMargin(0.f, Top, 0.f, 1.f));
			return L;
		};
		if (bHeader)
		{
			C.Crumb = Text(Tree, TEXT("YGGDRASIL'S PEAK"), 9, Dim, 420, TEXT("Regular"));
			Place(C.Crumb, FAnchors(X, 0.f), FVector2D(0.f, 1.f), FVector2D(0.f, 64.f));
			C.Right = Text(Tree, TEXT(""), 9, Faint, 360, TEXT("Regular"));
			Place(C.Right, FAnchors(1.f - X, 0.f), FVector2D(1.f, 1.f), FVector2D(0.f, 64.f));
			C.All.Add(HairRule(76.f, false));
			C.All.Add(C.Crumb);
			C.All.Add(C.Right);
		}
		C.All.Add(HairRule(-78.f, true));
		C.Foot = Text(Tree, TEXT(""), 9, Faint, 360, TEXT("Regular"));
		Place(C.Foot, FAnchors(X, 1.f), FVector2D(0.f, 0.5f), FVector2D(0.f, -50.f));
		C.All.Add(C.Foot);
		return C;
	}

	/** hairline - bead - hairline, centred */
	inline UHorizontalBox* Divider(UWidgetTree* Tree, float HalfW)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (UWidget* W : { static_cast<UWidget*>(Line(Tree, HalfW)), static_cast<UWidget*>(Bead(Tree, 5.f)), static_cast<UWidget*>(Line(Tree, HalfW)) })
		{
			UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(W);
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(6.f, 0.f));
		}
		return Row;
	}

	/** A text-only option: a bead marks the chosen / hovered one. */
	inline UButton* Option(UWidgetTree* Tree, const FString& Label, int32 Size, UTextBlock*& OutText, UImage*& OutBead)
	{
		UButton* B = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		FButtonStyle St = B->GetStyle();
		for (FSlateBrush* Br : { &St.Normal, &St.Hovered, &St.Pressed, &St.Disabled })
		{
			Br->DrawAs = ESlateBrushDrawType::NoDrawType;
		}
		St.NormalPadding = FMargin(0.f, 2.f);
		St.PressedPadding = FMargin(0.f, 2.f);
		B->SetStyle(St);
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		OutBead = Bead(Tree, 5.f);
		OutBead->SetRenderOpacity(0.f);
		UHorizontalBoxSlot* BS = Row->AddChildToHorizontalBox(OutBead);
		BS->SetVerticalAlignment(VAlign_Center);
		BS->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		OutText = Text(Tree, Label.ToUpper(), Size, Dim, 260, TEXT("Regular"));
		Row->AddChildToHorizontalBox(OutText)->SetVerticalAlignment(VAlign_Center);
		B->SetContent(Row);
		return B;
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
