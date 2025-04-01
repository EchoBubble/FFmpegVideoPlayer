// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FFfmpegMediaPlayerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Handle to the test dll we will load */
	//void*	ExampleLibraryHandle;
	void* avcodecHandle;
	void* avdeviceHandle;
	void* avfilterHandle ;
	void* avformatHandle ;
	void* avutilHandle ;
	void* postprocHandle;
	void* swresampleHandle;
	void* swscaleHandle;
};
