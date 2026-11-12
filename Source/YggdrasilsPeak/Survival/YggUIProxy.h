// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YggUIProxy.generated.h"

/** Lets code-built menus bind a lambda to a UMG button click or slider change (dynamic delegates need a UFUNCTION). */
UCLASS(Transient)
class YGGDRASILSPEAK_API UYggUIProxy : public UObject
{
	GENERATED_BODY()

public:
	TFunction<void()> OnClick;
	TFunction<void(float)> OnValue;

	UFUNCTION() void Click() { if (OnClick) { OnClick(); } }
	UFUNCTION() void Value(float V) { if (OnValue) { OnValue(V); } }
};
