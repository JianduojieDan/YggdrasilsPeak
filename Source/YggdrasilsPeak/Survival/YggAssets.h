// Assets loaded once and kept for the whole run (2026-11-14). The second monitored play session froze for 2-5 s at a
// time whenever the drive stalled; every LoadObject during play is a FlushAsyncLoading - the game thread waits for the
// disk - and the log showed 17 of them in five minutes: the pause menu's font and cloud veil each time it opened (garbage
// collection had dropped them since the last time), the wheel's sounds on its first spin, the HUD's icons, the first
// step's snow clump... Anything used later in play is asked for here at BeginPlay / NativeConstruct, so the load happens
// with the level, and asking again later finds it in memory.

#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

namespace YggAssets
{
	/** The asset at Path ("/Game/Dir/Name.Name"), loaded the first time it is asked for and then kept until the game
	 *  closes: garbage collection never drops it, so asking again never touches the disk. A missing asset is remembered as
	 *  missing (no second search). Game thread only. LoadFlags as for LoadObject (LOAD_NoWarn | LOAD_Quiet: no warning
	 *  when missing). In the editor (Play in Editor) it is a plain LoadObject: nothing is held, so assets can still be
	 *  deleted, renamed or swapped there. */
	template<typename T>
	T* Kept(const TCHAR* Path, uint32 LoadFlags = LOAD_None)
	{
		check(IsInGameThread());
		if (GIsEditor)
		{
			return LoadObject<T>(nullptr, Path, FStringView(), LoadFlags);
		}
		static TMap<FName, TStrongObjectPtr<T>> Loaded;            // one table per asset type
		const FName Key(Path);
		if (const TStrongObjectPtr<T>* Found = Loaded.Find(Key))
		{
			return Found->Get();
		}
		T* Asset = LoadObject<T>(nullptr, Path, FStringView(), LoadFlags);
		Loaded.Add(Key, TStrongObjectPtr<T>(Asset));
		return Asset;
	}
}
