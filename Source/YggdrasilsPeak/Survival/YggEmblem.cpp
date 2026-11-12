// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggEmblem.h"

#include "Survival/YggUIStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	const FSlateBrush& EmDotBrush()
	{
		static FSlateBrush B;
		static bool bInit = false;
		if (!bInit)
		{
			bInit = true;
			B.DrawAs = ESlateBrushDrawType::RoundedBox;
			B.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			B.TintColor = FSlateColor(FLinearColor::White);
		}
		return B;
	}
	const FSlateBrush& EmHaloBrush()
	{
		static FSlateBrush B;
		if (!B.GetResourceObject()) { B = YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f)); }
		return B;
	}

	FLinearColor EmA(const FLinearColor& C, float Alpha) { return FLinearColor(C.R, C.G, C.B, C.A * Alpha); }

	/** Draws the first K (0..1) of a polyline, by length. */
	void EmPartial(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const TArray<FVector2f>& P, float K, const FLinearColor& C, float Th)
	{
		if (P.Num() < 2 || K <= 0.f) { return; }
		float Total = 0.f;
		for (int32 i = 1; i < P.Num(); ++i) { Total += FVector2f::Distance(P[i - 1], P[i]); }
		float Left = Total * FMath::Clamp(K, 0.f, 1.f);
		TArray<FVector2f> Q;
		Q.Add(P[0]);
		for (int32 i = 1; i < P.Num() && Left > 0.f; ++i)
		{
			const float L = FVector2f::Distance(P[i - 1], P[i]);
			if (L <= Left) { Q.Add(P[i]); Left -= L; }
			else { Q.Add(P[i - 1] + (P[i] - P[i - 1]) * (Left / FMath::Max(L, 0.001f))); Left = 0.f; }
		}
		if (Q.Num() >= 2) { FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Q, ESlateDrawEffect::None, C, true, Th); }
	}

	/** A quadratic curve from A through the control point to B, as a polyline. */
	TArray<FVector2f> EmCurve(const FVector2f& P0, const FVector2f& P1, const FVector2f& P2, int32 N = 14)
	{
		TArray<FVector2f> R;
		for (int32 i = 0; i <= N; ++i)
		{
			const float t = float(i) / N;
			R.Add(P0 * FMath::Square(1.f - t) + P1 * (2.f * t * (1.f - t)) + P2 * (t * t));
		}
		return R;
	}

	void EmDot(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& P, float D, const FLinearColor& C, float Glow)
	{
		if (Glow > 0.01f)
		{
			const float H = D * 4.f;
			FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(H, H), FSlateLayoutTransform(P - FVector2f(H * 0.5f))), &EmHaloBrush(), ESlateDrawEffect::None, EmA(C, Glow * 0.5f));
		}
		FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(D, D), FSlateLayoutTransform(P - FVector2f(D * 0.5f))), &EmDotBrush(), ESlateDrawEffect::None, C);
	}

	class SYggEmblem : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SYggEmblem) {}
			SLATE_ARGUMENT(float, Diameter)
			SLATE_ARGUMENT(float, Spin)
		SLATE_END_ARGS()

		void Construct(const FArguments& Args)
		{
			Diameter = Args._Diameter;
			Spin = Args._Spin;
			Start = FSlateApplication::Get().GetCurrentTime();
		}
		virtual bool ComputeVolatility() const override { return true; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Diameter, Diameter); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2f Size(G.GetLocalSize());
			YggEmblem::Paint(Out, Layer, G, Size * 0.5f, FMath::Min(Size.X, Size.Y), float(FSlateApplication::Get().GetCurrentTime() - Start), Style.GetColorAndOpacityTint().A, Spin);
			return Layer + 3;
		}

	private:
		float Diameter = 120.f;
		float Spin = 0.f;
		double Start = 0.0;
	};
}

void YggEmblem::Paint(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& C, float Diameter, float T, float Alpha, float Spin)
{
	const float R = Diameter * 0.5f;
	auto P = [&C, R](float x, float y) { return C + FVector2f(x, y) * R; };
	const float Th = FMath::Clamp(Diameter / 110.f, 1.f, 2.2f);
	// it draws itself on: the ring, then the mountain, then the tree
	const float KRing = YggUI::EaseOut(T / 1.4f);
	const float KMount = YggUI::EaseOut((T - 0.5f) / 1.2f);
	const float KTree = YggUI::EaseOut((T - 1.1f) / 1.3f);
	const float KDots = YggUI::Ease((T - 0.3f) / 1.2f);

	// the rings: outer hairline, a fainter inner one
	for (int32 Ring = 0; Ring < 2; ++Ring)
	{
		const float Rr = Ring == 0 ? 0.96f : 0.84f;
		const int32 N = 120;
		TArray<FVector2f> Pts;
		const int32 Upto = FMath::RoundToInt(N * FMath::Clamp(KRing * (Ring == 0 ? 1.f : 1.15f) - (Ring == 0 ? 0.f : 0.15f), 0.f, 1.f));
		for (int32 i = 0; i <= Upto; ++i)
		{
			const float Ang = -HALF_PI + TWO_PI * i / N * (Ring == 0 ? 1.f : -1.f);
			Pts.Add(P(Rr * FMath::Cos(Ang), Rr * FMath::Sin(Ang)));
		}
		if (Pts.Num() >= 2) { FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, EmA(Ring == 0 ? YggUI::Hair : YggUI::Rule, Alpha * (Ring == 0 ? 1.4f : 1.6f)), true, 1.f); }
	}

	// the nine realms on the ring; a gold light wanders from one to the next (or runs round while loading)
	for (int32 k = 0; k < 9; ++k)
	{
		const float Ang = -HALF_PI + TWO_PI * k / 9.f;
		const float Wander = Spin > 0.f ? FMath::Fmod(T * 3.2f, 9.f) : 4.f + 4.f * FMath::Sin(T * 0.35f);
		float Dd = FMath::Abs(Wander - k);
		if (Spin > 0.f) { Dd = FMath::Min(Dd, 9.f - Dd); }
		const float L = FMath::Clamp(1.f - Dd * 0.75f, 0.f, 1.f);
		const float Show = FMath::Clamp(KDots * 9.f - k, 0.f, 1.f);
		const float D = FMath::Max(2.5f, Diameter * (k == 0 ? 0.05f : 0.036f));
		EmDot(Out, Layer + 1, G, P(0.96f * FMath::Cos(Ang), 0.96f * FMath::Sin(Ang)), D, EmA(FMath::Lerp(YggUI::Faint, YggUI::Gold, FMath::Max(L, k == 0 ? 0.6f : 0.f)), Alpha * Show), L * Show);
	}

	// the mountain: a ridge with a shoulder and the summit, on a quiet ground line
	const TArray<FVector2f> Ridge = { P(-0.66f, 0.42f), P(-0.36f, 0.04f), P(-0.24f, 0.14f), P(0.02f, -0.30f), P(0.20f, -0.06f), P(0.30f, -0.14f), P(0.66f, 0.42f) };
	EmPartial(Out, Layer + 1, G, Ridge, KMount, EmA(YggUI::Ivory, Alpha * 0.92f), Th * 1.15f);
	EmPartial(Out, Layer + 1, G, { P(-0.74f, 0.42f), P(0.74f, 0.42f) }, KMount, EmA(YggUI::Hair, Alpha), 1.f);
	// a snow line under the summit
	EmPartial(Out, Layer + 1, G, { P(-0.12f, -0.10f), P(-0.02f, -0.02f), P(0.06f, -0.12f), P(0.14f, -0.02f) }, KMount * 1.2f - 0.2f, EmA(YggUI::Dim, Alpha * 0.8f), Th * 0.8f);

	// the World Tree: a trunk from the summit, branches that curve up and out like the rune Algiz, roots into the rock
	const FVector2f Top = P(0.02f, -0.30f);
	const FLinearColor TreeC = EmA(YggUI::Gold, Alpha * 0.95f);
	EmPartial(Out, Layer + 2, G, { Top, P(0.02f, -0.74f) }, KTree, TreeC, Th);
	const float KB = YggUI::Ease((T - 1.5f) / 1.1f);
	EmPartial(Out, Layer + 2, G, EmCurve(P(0.02f, -0.42f), P(-0.10f, -0.50f), P(-0.26f, -0.68f)), KB, TreeC, Th);
	EmPartial(Out, Layer + 2, G, EmCurve(P(0.02f, -0.42f), P(0.14f, -0.50f), P(0.30f, -0.68f)), KB, TreeC, Th);
	EmPartial(Out, Layer + 2, G, EmCurve(P(0.02f, -0.56f), P(-0.06f, -0.62f), P(-0.13f, -0.75f)), KB * 1.2f - 0.2f, TreeC, Th * 0.85f);
	EmPartial(Out, Layer + 2, G, EmCurve(P(0.02f, -0.56f), P(0.10f, -0.62f), P(0.17f, -0.75f)), KB * 1.2f - 0.2f, TreeC, Th * 0.85f);
	const float KR = YggUI::Ease((T - 1.3f) / 1.4f);
	EmPartial(Out, Layer + 1, G, EmCurve(Top, P(-0.04f, -0.10f), P(-0.16f, 0.30f)), KR, EmA(YggUI::Gold, Alpha * 0.45f), Th * 0.8f);
	EmPartial(Out, Layer + 1, G, EmCurve(Top, P(0.10f, -0.08f), P(0.18f, 0.34f)), KR, EmA(YggUI::Gold, Alpha * 0.45f), Th * 0.8f);
	EmPartial(Out, Layer + 1, G, EmCurve(Top, P(0.02f, 0.f), P(0.0f, 0.38f)), KR, EmA(YggUI::Gold, Alpha * 0.35f), Th * 0.7f);
	// the crown's light
	const float Glow = YggUI::Ease((T - 2.2f) / 1.2f) * (0.75f + 0.25f * FMath::Sin(T * 1.3f));
	EmDot(Out, Layer + 2, G, P(0.02f, -0.78f), FMath::Max(3.f, Diameter * 0.045f), EmA(YggUI::Gold, Alpha * Glow), Glow);
}

TSharedRef<SWidget> UYggEmblem::RebuildWidget()
{
	TSharedRef<SWidget> W = SNew(SYggEmblem).Diameter(Diameter).Spin(Spin);
	Slate = W;
	return W;
}

void UYggEmblem::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Slate.Reset();
}
