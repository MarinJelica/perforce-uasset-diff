// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExposeDiffCommand.h"

#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "DiffUtils.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "FExposeDiffCommandModule"

#if WITH_EDITOR

namespace ExposeDiffCommand
{
	// Copied from DiffUtils::LoadAssetFromExternalPath()
	static UObject* LoadAssetFromExternalPath(FString Path)
	{
		
		// copy to the temp directory so it can be loaded properly
		FString File = FPaths::GetBaseFilename(Path) + TEXT("-");
		for (const ANSICHAR Char : "#(){}[].")
		{
			File.ReplaceCharInline(Char, '-');
		}

		const FString Extension = TEXT(".") + FPaths::GetExtension(Path);
		const FString SourcePath = Path;
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*FPaths::DiffDir());
		Path = FPaths::CreateTempFilename(*FPaths::DiffDir(), *File, *Extension);
		Path = FPaths::ConvertRelativePathToFull(Path);

		if (!FPlatformFileManager::Get().GetPlatformFile().CopyFile(*Path, *SourcePath))
		{
			UE_LOG(LogExec, Warning, TEXT("Failed to Copy %s"), *SourcePath);
			return nullptr;
		}

		//const UPackage* TempPackage = LoadPackage(nullptr, *Path, LOAD_None);
		const UPackage* TempPackage = LoadPackage(nullptr, *Path, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
		
		if (TempPackage)
		{
			if (UObject* Object = TempPackage->FindAssetInPackage())
			{
				return Object;
			}
		}

		UE_LOG(LogExec, Warning, TEXT("Failed to load: %s"), *Path);
		return nullptr;
	}

	// See UEDiffUtils_Private::RunDiffCommand
	static FAutoConsoleCommand DiffUAssetCommand(
		TEXT("diff.uasset"),
		TEXT("Open the diff view on two uassets to check their changes against one another.")
		TEXT("Use: \"diff.uasset 'AbsoluteAssetPath1' 'AbsoluteAssetPath2'\""),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
			{
				if (Args.Num() != 2)
				{
					UE_LOG(LogExec, Warning, TEXT("Invalid arguments provided to diff command. Need 2, got %d"), Args.Num());
					return;
				}

				UObject* LHS = LoadAssetFromExternalPath(Args[0]);
				UObject* RHS = LoadAssetFromExternalPath(Args[1]);
				if (LHS && RHS)
				{
					IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
					AssetTools.DiffAssets(LHS, RHS, {}, {});
				}
			}),
		ECVF_Default);
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FExposeDiffCommandModule, ExposeDiffCommand)