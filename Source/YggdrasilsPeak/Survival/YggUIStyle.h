// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// The HUD's visual language for the menus (2026-11-11 user: the boxed menu did not belong with the HUD):
// no cards or boxes, a soft veil over the world, ivory text with a soft halo, Alegreya serif for names, letter-spaced
// small capitals for labels, light numerals with small units, hairlines that end in a bead.

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Styling/CoreStyle.h"

namespace YggUI
{
	inline const FLinearColor Ivory(0.93f, 0.91f, 0.84f, 0.96f);
	inline const FLinearColor Dim(0.93f, 0.91f, 0.84f, 0.66f);
	inline const FLinearColor Faint(0.93f, 0.91f, 0.84f, 0.5f);
	inline const FLinearColor Hair(0.93f, 0.91f, 0.84f, 0.32f);

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

	inline UImage* Line(UWidgetTree* Tree, float W, const FLinearColor& C = Hair, float H = 1.f)
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
}
