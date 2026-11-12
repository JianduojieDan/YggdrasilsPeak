// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggFateFx.h"

#include "Survival/YggUIStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	FLinearColor FxA(const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, C.A * A); }
	const FLinearColor FxIce(0.82f, 0.93f, 1.f, 1.f);
	const FLinearColor FxSnow(0.94f, 0.96f, 1.f, 1.f);

	const FSlateBrush& FxBrush(int32 Which)
	{
		// 0 glow, 1 vignette, 2 rising (clear top, opaque bottom)
		static FSlateBrush B[4];
		if (!B[0].GetResourceObject())
		{
			B[0] = YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f));
			B[1] = YggUI::TextureBrush(YggUI::VignetteTexture(), FVector2D(128.f, 128.f));
			B[2] = YggUI::TextureBrush(YggUI::MakeAlphaTexture(4, 64, [](float u, float v) { return FMath::SmoothStep(0.f, 1.f, v); }), FVector2D(4.f, 64.f));
			B[3] = YggUI::TextureBrush(YggUI::FadeTexture(), FVector2D(4.f, 64.f));   // opaque top, clear bottom
		}
		return B[FMath::Clamp(Which, 0, 3)];
	}

	void FxLine(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const TArray<FVector2f>& P, const FLinearColor& C, float Th)
	{
		if (P.Num() >= 2) { FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), P, ESlateDrawEffect::None, C, true, Th); }
	}

	/** The first K (0..1) of a polyline, by points. */
	void FxPartial(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const TArray<FVector2f>& P, float K, const FLinearColor& C, float Th)
	{
		if (P.Num() < 2 || K <= 0.f) { return; }
		const float F = FMath::Clamp(K, 0.f, 1.f) * (P.Num() - 1);
		const int32 N = FMath::FloorToInt(F);
		TArray<FVector2f> Q(P.GetData(), N + 1);
		if (N + 1 < P.Num()) { Q.Add(FMath::Lerp(P[N], P[N + 1], F - N)); }
		FxLine(Out, L, G, Q, C, Th);
	}

	void FxBox(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, int32 Brush, const FLinearColor& C)
	{
		FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), &FxBrush(Brush), ESlateDrawEffect::None, C);
	}

	/** A six-armed glint of frost. */
	void FxStar(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const FVector2f& P, float R, const FLinearColor& C)
	{
		for (int32 k = 0; k < 3; ++k)
		{
			const float A = PI / 3.f * k + 0.3f;
			const FVector2f D(FMath::Cos(A) * R, FMath::Sin(A) * R);
			FxLine(Out, L, G, { P - D, P + D }, C, 1.f);
		}
	}

	struct FFxPoly { TArray<FVector2f> P; float Delay = 0.f; float Dur = 1.f; float Alpha = 1.f; };

	class SYggFateFx : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SYggFateFx) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, TSharedPtr<FYggFateState> In) { State = In; }
		virtual bool ComputeVolatility() const override { return true; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			if (!State.IsValid() || State->Fate == YggFate::None) { return Layer; }
			const float T = float(FSlateApplication::Get().GetCurrentTime() - State->Start);
			const float A = Style.GetColorAndOpacityTint().A;
			if (State->bScreen) { PaintScreen(Out, Layer, G, T, A); }
			else { PaintTitle(Out, Layer, G, T, A); }
			return Layer + 4;
		}

	private:
		TSharedPtr<FYggFateState> State;
		mutable FVector2f BuiltFor = FVector2f::ZeroVector;
		mutable uint8 BuiltFate = 255;
		mutable FVector2f BuiltAnchor = FVector2f(-9.f, -9.f);
		mutable TArray<FFxPoly> Polys;     // frost or cracks
		mutable TArray<FVector4f> Motes;   // x, y (0..1), size px, speed

		void Build(const FVector2f& V, const FVector2f& Anchor) const
		{
			Polys.Reset();
			Motes.Reset();
			FRandomStream R(int32(State->Fate) * 977 + 3);
			if (State->Fate == YggFate::Frozen)
			{
				// frost from the edges: corners first, then along every side; each stem a jagged line inward with twigs
				const float Perim = 2.f * (V.X + V.Y);
				const int32 Seeds = FMath::Clamp(int32(Perim / 22.f), 60, 420);
				for (int32 s = 0; s < Seeds; ++s)
				{
					float D = R.FRand() * Perim;
					FVector2f P, In;
					if (D < V.X) { P = FVector2f(D, 0.f); In = FVector2f(0.f, 1.f); }
					else if ((D -= V.X) < V.Y) { P = FVector2f(V.X, D); In = FVector2f(-1.f, 0.f); }
					else if ((D -= V.Y) < V.X) { P = FVector2f(V.X - D, V.Y); In = FVector2f(0.f, -1.f); }
					else { D -= V.X; P = FVector2f(0.f, V.Y - D); In = FVector2f(1.f, 0.f); }
					const float Corner = FMath::Min(FMath::Min(P.X, V.X - P.X), FMath::Min(P.Y, V.Y - P.Y) + 0.f);
					const float ToCorner = FMath::Min(FMath::Min(FVector2f::Distance(P, FVector2f(0.f, 0.f)), FVector2f::Distance(P, FVector2f(V.X, 0.f))),
						FMath::Min(FVector2f::Distance(P, FVector2f(0.f, V.Y)), FVector2f::Distance(P, V)));
					const float Near = 1.f - FMath::Clamp(ToCorner / (0.5f * V.Y), 0.f, 1.f);   // 1 at a corner
					const float Len = FMath::Lerp(30.f, 90.f, R.FRand()) * (1.f + 1.6f * Near) * (V.Y / 1080.f);
					const float Delay = 0.3f + 2.6f * (1.f - Near) + R.FRand() * 0.6f + 0.f * Corner;
					float Ang = FMath::Atan2(In.Y, In.X) + R.FRandRange(-0.55f, 0.55f);
					FFxPoly Stem;
					Stem.P.Add(P);
					Stem.Delay = Delay; Stem.Dur = 1.6f; Stem.Alpha = 0.36f;
					const int32 Steps = FMath::Max(3, int32(Len / 8.f));
					FVector2f Q = P;
					for (int32 k = 1; k <= Steps; ++k)
					{
						Ang += R.FRandRange(-0.22f, 0.22f);
						Q += FVector2f(FMath::Cos(Ang), FMath::Sin(Ang)) * (Len / Steps);
						Stem.P.Add(Q);
						if (k % 2 == 0 && k < Steps)
						{
							// a twig at about sixty degrees, alternating sides
							FFxPoly Tw;
							const float TA = Ang + ((k / 2) % 2 == 0 ? 1.f : -1.f) * R.FRandRange(0.9f, 1.15f);
							const float TL = (Len / Steps) * (Steps - k) * R.FRandRange(0.25f, 0.5f);
							Tw.P = { Q, Q + FVector2f(FMath::Cos(TA), FMath::Sin(TA)) * TL * 0.5f, Q + FVector2f(FMath::Cos(TA + 0.15f), FMath::Sin(TA + 0.15f)) * TL };
							Tw.Delay = Delay + Stem.Dur * float(k) / Steps; Tw.Dur = 0.7f; Tw.Alpha = 0.24f;
							Polys.Add(Tw);
						}
					}
					Polys.Add(Stem);
				}
			}
			else if (State->Fate == YggFate::Fallen)
			{
				// cracks from where the word landed
				const FVector2f O = Anchor.X >= 0.f ? Anchor : FVector2f(V.X * 0.45f, V.Y * 0.4f);
				const int32 Count = 7;
				for (int32 c = 0; c < Count; ++c)
				{
					float Ang = TWO_PI * c / Count + R.FRandRange(-0.35f, 0.35f);
					const float Len = R.FRandRange(220.f, 620.f) * (V.Y / 1080.f);
					FFxPoly Cr;
					Cr.P.Add(O);
					FVector2f Q = O;
					const int32 Steps = FMath::Max(6, int32(Len / 26.f));
					for (int32 k = 1; k <= Steps; ++k)
					{
						Ang += R.FRandRange(-0.42f, 0.42f);
						Q += FVector2f(FMath::Cos(Ang), FMath::Sin(Ang)) * (Len / Steps) * R.FRandRange(0.6f, 1.3f);
						Cr.P.Add(Q);
						if (k == Steps / 2 || (k == Steps / 3 && R.FRand() < 0.5f))
						{
							FFxPoly Br;
							Br.P.Add(Q);
							float BA = Ang + (R.FRand() < 0.5f ? 1.f : -1.f) * R.FRandRange(0.5f, 1.0f);
							FVector2f B = Q;
							for (int32 j = 0; j < 4; ++j) { BA += R.FRandRange(-0.4f, 0.4f); B += FVector2f(FMath::Cos(BA), FMath::Sin(BA)) * R.FRandRange(14.f, 34.f); Br.P.Add(B); }
							Br.Delay = 1.05f + 0.2f * float(k) / Steps; Br.Dur = 0.2f; Br.Alpha = 0.7f;
							Polys.Add(Br);
						}
					}
					Cr.Delay = 0.95f; Cr.Dur = 0.32f; Cr.Alpha = 1.f;
					Polys.Add(Cr);
				}
			}
			// drifting things: snow (buried, starved), dust (parched), fog (lost)
			const int32 Count = State->Fate == YggFate::Buried ? 260 : State->Fate == YggFate::Starved ? 50 : State->Fate == YggFate::Parched ? 70 : State->Fate == YggFate::Lost ? 8 : 0;
			for (int32 i = 0; i < Count; ++i)
			{
				const float Depth = FMath::Pow(R.FRand(), 2.f);
				if (State->Fate == YggFate::Lost) { Motes.Add(FVector4f(R.FRand(), R.FRandRange(0.2f, 0.9f), R.FRandRange(600.f, 1200.f), R.FRandRange(0.004f, 0.012f))); }
				else if (State->Fate == YggFate::Parched) { Motes.Add(FVector4f(R.FRand(), R.FRand(), FMath::Lerp(2.f, 6.f, Depth), FMath::Lerp(0.004f, 0.016f, Depth))); }
				else { Motes.Add(FVector4f(R.FRand(), R.FRand(), FMath::Lerp(2.f, State->Fate == YggFate::Buried ? 13.f : 8.f, Depth), FMath::Lerp(0.02f, State->Fate == YggFate::Buried ? 0.16f : 0.05f, Depth))); }
			}
		}

		void PaintScreen(FSlateWindowElementList& Out, int32 L, const FGeometry& G, float T, float A) const
		{
			const FVector2f V(G.GetLocalSize());
			const FVector2f Anchor = State->AnchorAbs.X >= 0.f ? FVector2f(G.AbsoluteToLocal(FVector2D(State->AnchorAbs))) : FVector2f(-1.f, -1.f);
			if (BuiltFor != V || BuiltFate != State->Fate || (State->Fate == YggFate::Fallen && T < 0.9f && FVector2f::Distance(Anchor, BuiltAnchor) > 2.f))
			{
				Build(V, Anchor);
				BuiltFor = V; BuiltFate = State->Fate; BuiltAnchor = Anchor;
			}
			switch (State->Fate)
			{
			case YggFate::Frozen:
			{
				FxBox(Out, L, G, FVector2f(0.f), V, 1, FLinearColor(0.7f, 0.86f, 1.f, 0.42f * A * YggUI::Ease(T / 4.f)));
				for (const FFxPoly& P : Polys)
				{
					const float K = (T - P.Delay) / P.Dur;
					if (K > 0.f) { FxPartial(Out, L + 1, G, P.P, YggUI::EaseOut(K), FxA(FxIce, P.Alpha * A), 1.f); }
				}
				break;
			}
			case YggFate::Fallen:
				for (const FFxPoly& P : Polys)
				{
					const float K = (T - P.Delay) / P.Dur;
					if (K <= 0.f) { continue; }
					// a dark split with a lit edge beside it
					FxPartial(Out, L + 1, G, P.P, K, FLinearColor(0.f, 0.f, 0.01f, 0.75f * P.Alpha * A), 1.8f);
					TArray<FVector2f> Lit = P.P;
					for (FVector2f& Q : Lit) { Q += FVector2f(1.2f, 1.2f); }
					FxPartial(Out, L + 2, G, Lit, K, FLinearColor(0.93f, 0.91f, 0.84f, 0.22f * P.Alpha * A), 1.f);
				}
				break;
			case YggFate::Buried:
				FxBox(Out, L, G, FVector2f(0.f, V.Y * 0.35f), FVector2f(V.X, V.Y * 0.65f), 2, FxA(FxSnow, 0.42f * A * YggUI::Ease(T / 5.f)));
				break;
			case YggFate::Parched:
				FxBox(Out, L, G, FVector2f(0.f), V, 1, FLinearColor(0.42f, 0.2f, 0.05f, 0.6f * A * YggUI::Ease(T / 3.f)));
				break;
			case YggFate::Starved:
				FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(0.08f, 0.08f, 0.08f, 0.35f * A * YggUI::Ease(T / 4.f)));
				break;
			case YggFate::Breathless:
			{
				// the view closes in on every letting-go of breath
				const float B = YggFate::Breath(T - 0.6f);
				const float S = FMath::Lerp(1.0f, 1.55f, B);
				const FVector2f Size = V * S;
				FxBox(Out, L, G, (V - Size) * 0.5f, Size, 1, FLinearColor(0.f, 0.f, 0.02f, A * YggUI::Ease(T / 2.f) * 0.95f));
				break;
			}
			default:
				break;
			}
			for (const FVector4f& M : Motes)
			{
				FVector2f P;
				FLinearColor C;
				if (State->Fate == YggFate::Lost)
				{
					P = FVector2f(FMath::Frac(M.X + M.W * T) * (V.X + M.Z) - M.Z * 0.5f, M.Y * V.Y);
					C = FLinearColor(0.85f, 0.88f, 0.92f, 0.07f * A * YggUI::Ease(T / 3.f));
				}
				else if (State->Fate == YggFate::Parched)
				{
					P = FVector2f(FMath::Frac(M.X + M.W * T) * V.X, (M.Y + 0.01f * FMath::Sin(T * 0.8f + M.X * 20.f)) * V.Y);
					C = FLinearColor(1.f, 0.78f, 0.48f, 0.35f * A);
				}
				else
				{
					P = FVector2f(FMath::Frac(M.X + 0.012f * FMath::Sin(T * 0.7f + M.Y * 9.f) + 0.006f * T) * V.X, FMath::Frac(M.Y + M.W * T) * V.Y);
					C = FxA(FxSnow, (State->Fate == YggFate::Buried ? 0.6f : 0.35f) * A);
				}
				FxBox(Out, L + 2, G, P - FVector2f(M.Z * 0.5f), FVector2f(M.Z), 0, C);
			}
		}

		void PaintTitle(FSlateWindowElementList& Out, int32 L, const FGeometry& G, float T, float A) const
		{
			const TArray<FBox2f>& Ls = State->Letters;
			if (Ls.Num() == 0) { return; }
			FBox2f All(ForceInit);
			for (const FBox2f& B : Ls) { All += B; }
			const float H = All.GetSize().Y;
			switch (State->Fate)
			{
			case YggFate::Frozen:
				for (int32 i = 0; i < Ls.Num(); ++i)
				{
					const FBox2f& B = Ls[i];
					if (B.GetSize().X < 6.f) { continue; }   // a space
					FRandomStream R(i * 131 + 7);
					const float Base = B.Min.Y + B.GetSize().Y * 0.78f;
					const float Grow = YggUI::EaseOut((T - 1.0f - 0.12f * i) / 2.6f);
					// icicles under the letter: a few lines side by side, the middle longest, a glint at the tip
					for (int32 n = 0; n < 2; ++n)
					{
						const float X = B.Min.X + B.GetSize().X * R.FRandRange(0.22f, 0.78f);
						const float Len = H * R.FRandRange(0.08f, 0.3f) * Grow;
						const float W = FMath::Max(2.f, H * 0.028f);
						if (Len < 1.f) { continue; }
						for (int32 j = 0; j < 4; ++j)
						{
							const float U = (j - 1.5f) / 1.5f;
							const float LJ = Len * (1.f - 0.55f * FMath::Abs(U));
							FxLine(Out, L, G, { FVector2f(X + U * W, Base - 2.f), FVector2f(X + U * W * 0.4f, Base + LJ) }, FxA(FxIce, 0.75f * A), 1.f);
						}
						const float Tw = 0.5f + 0.5f * FMath::Sin(T * 3.1f + i * 1.7f + n);
						FxBox(Out, L + 1, G, FVector2f(X - 3.f, Base + Len - 3.f), FVector2f(6.f), 0, FxA(FLinearColor::White, 0.6f * Tw * Grow * A));
					}
					// frost glints on the letter's shoulders
					const float Glint = YggUI::Ease((T - 0.8f - 0.1f * i) / 1.2f);
					const FVector2f Sp(B.Min.X + B.GetSize().X * R.FRandRange(0.2f, 0.8f), B.Min.Y + B.GetSize().Y * R.FRandRange(0.28f, 0.45f));
					FxStar(Out, L + 1, G, Sp, H * 0.045f * Glint, FxA(FLinearColor::White, (0.35f + 0.4f * FMath::Abs(FMath::Sin(T * 1.7f + i))) * Glint * A));
				}
				break;
			case YggFate::Buried:
			{
				// a drift of snow rising over the letters' feet, its top edge soft and uneven
				const float K = YggUI::EaseOut((T - 1.2f) / 3.5f);
				if (K <= 0.f) { break; }
				// a soft mound: tapering at both ends, a gently uneven crest with a blue shadow under it, its foot fading out
				// (a uniform block of lines read as a white box over the word)
				const float Base = All.Min.Y + H * 0.8f;
				const float W = All.GetSize().X;
				for (float X = All.Min.X - 30.f; X <= All.Max.X + 30.f; X += 2.f)
				{
					const float U = (X - All.Min.X + 30.f) / (W + 60.f);
					const float Taper = FMath::SmoothStep(0.f, 0.18f, U) * (1.f - FMath::SmoothStep(0.82f, 1.f, U));
					const float Ht = H * 0.24f * K * Taper * (0.8f + 0.14f * FMath::Sin(X * 0.023f + 1.f) + 0.06f * FMath::Sin(X * 0.09f));
					if (Ht < 0.5f) { continue; }
					const float Top = Base - Ht;
					FxLine(Out, L, G, { FVector2f(X, Top), FVector2f(X, Base + H * 0.03f) }, FxA(FxSnow, 0.93f * A), 2.4f);
					FxBox(Out, L, G, FVector2f(X - 1.f, Base + H * 0.03f - 1.5f), FVector2f(2.f, H * 0.12f + 1.5f), 3, FxA(FxSnow, 0.8f * A));
					FxLine(Out, L + 1, G, { FVector2f(X - 1.f, Top + 3.f), FVector2f(X + 1.f, Top + 3.f) }, FLinearColor(0.62f, 0.74f, 0.92f, 0.35f * A), 1.f);
					if (FMath::Fmod(X - All.Min.X + 30.f, 8.f) < 2.f) { FxBox(Out, L + 1, G, FVector2f(X - 6.f, Top - 6.f), FVector2f(12.f), 0, FxA(FxSnow, 0.55f * A)); }
				}
				break;
			}
			case YggFate::Parched:
				for (int32 i = 0; i < Ls.Num(); ++i)
				{
					const FBox2f& B = Ls[i];
					if (B.GetSize().X < 6.f) { continue; }
					FRandomStream R(i * 71 + 5);
					const float K = YggUI::EaseOut((T - 1.4f - 0.15f * i) / 1.2f);
					for (int32 c = 0; c < 2; ++c)
					{
						TArray<FVector2f> P;
						FVector2f Q(B.Min.X + B.GetSize().X * R.FRandRange(0.3f, 0.7f), B.Min.Y + B.GetSize().Y * R.FRandRange(0.3f, 0.45f));
						float Ang = R.FRandRange(0.9f, 2.2f);
						P.Add(Q);
						for (int32 k = 0; k < 5; ++k) { Ang += R.FRandRange(-0.6f, 0.6f); Q += FVector2f(FMath::Cos(Ang), FMath::Sin(Ang)) * H * 0.06f; P.Add(Q); }
						FxPartial(Out, L, G, P, K, FLinearColor(0.22f, 0.11f, 0.04f, 0.85f * A), 1.4f);
					}
				}
				break;
			default:
				break;
			}
		}
	};
}

void UYggFateFx::Restart(uint8 InFate, bool bInScreen)
{
	State->Fate = InFate;
	State->bScreen = bInScreen;
	State->Start = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : 0.0;
	State->Letters.Reset();
	State->AnchorAbs = FVector2f(-1.f, -1.f);
}

TSharedRef<SWidget> UYggFateFx::RebuildWidget()
{
	TSharedRef<SWidget> W = SNew(SYggFateFx, State);
	Slate = W;
	return W;
}

void UYggFateFx::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Slate.Reset();
}
