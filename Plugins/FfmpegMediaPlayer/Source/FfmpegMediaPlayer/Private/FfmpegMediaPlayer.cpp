// Copyright Epic Games, Inc. All Rights Reserved.

#include "FfmpegMediaPlayer.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "FfmpegMediaPlayerLibrary/ExampleLibrary.h"

#define LOCTEXT_NAMESPACE "FFfmpegMediaPlayerModule"

void FFfmpegMediaPlayerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("FfmpegMediaPlayer")->GetBaseDir();


	UE_LOG(LogTemp, Log, TEXT("********BaseDir********%s"), *BaseDir);

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	FString avcodec = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avcodec-61.dll"));
	FString avdevice = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avdevice-61.dll"));
	FString avfilter = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avfilter-10.dll"));
	FString avformat = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avformat-61.dll"));
	FString avutil = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avutil-59.dll"));
	FString postproc = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/postproc-58.dll"));
	FString swresample = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/swresample-5.dll"));
	FString swscale = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/swscale-8.dll"));
#elif PLATFORM_MAC
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/FfmpegMediaPlayerLibrary/Mac/Release/libExampleLibrary.dylib"));
#elif PLATFORM_LINUX
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Linux/x86_64-unknown-linux-gnu/libExampleLibrary.so"));
#endif // PLATFORM_WINDOWS

	avcodecHandle = !avcodec.IsEmpty() ? FPlatformProcess::GetDllHandle(*avcodec) : nullptr;
	avdeviceHandle = !avdevice.IsEmpty() ? FPlatformProcess::GetDllHandle(*avdevice) : nullptr;
	avfilterHandle = !avfilter.IsEmpty() ? FPlatformProcess::GetDllHandle(*avfilter) : nullptr;
	avformatHandle = !avformat.IsEmpty() ? FPlatformProcess::GetDllHandle(*avformat) : nullptr;
	avutilHandle = !avutil.IsEmpty() ? FPlatformProcess::GetDllHandle(*avutil) : nullptr;
	postprocHandle = !postproc.IsEmpty() ? FPlatformProcess::GetDllHandle(*postproc) : nullptr;
	swresampleHandle = !swresample.IsEmpty() ? FPlatformProcess::GetDllHandle(*swresample) : nullptr;
	swscaleHandle = !swscale.IsEmpty() ? FPlatformProcess::GetDllHandle(*swscale) : nullptr;

	if (avcodecHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "avcodecHandle Failed to load example third party library"));
	if (avdeviceHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "avdeviceHandle Failed to load example third party library"));
	if (avfilterHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "avfilterHandle to load example third party library"));
	if (avformatHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "avformatHandle Failed to load example third party library"));
	if (avutilHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "avutilHandle Failed to load example third party library"));
	if (postprocHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "postprocHandle Failed to load example third party library"));
	if (swresampleHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "swresampleHandle Failed to load example third party library"));
	if (swscaleHandle == nullptr)FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "swscaleHandle Failed to load example third party library"));

}

void FFfmpegMediaPlayerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Free the dll handle
	FPlatformProcess::FreeDllHandle(avcodecHandle);
	avcodecHandle = nullptr;
	
	FPlatformProcess::FreeDllHandle(avdeviceHandle);
	avdeviceHandle = nullptr;

	FPlatformProcess::FreeDllHandle(avfilterHandle);
	avfilterHandle = nullptr;

	FPlatformProcess::FreeDllHandle(avformatHandle);
	avformatHandle = nullptr;

	FPlatformProcess::FreeDllHandle(avutilHandle);
	avutilHandle = nullptr;
	
	FPlatformProcess::FreeDllHandle(postprocHandle);
	postprocHandle = nullptr;

	FPlatformProcess::FreeDllHandle(swresampleHandle);
	swresampleHandle = nullptr;

	FPlatformProcess::FreeDllHandle(swscaleHandle);
	swscaleHandle = nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFfmpegMediaPlayerModule, FfmpegMediaPlayer)
