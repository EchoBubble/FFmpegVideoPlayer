// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class FfmpegMediaPlayerLibrary : ModuleRules
{
	public FfmpegMediaPlayerLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicSystemIncludePaths.Add("$(ModuleDir)/Public");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "avcodec.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "avdevice.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "avfilter.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "avformat.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "avutil.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "postproc.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "swresample.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "swscale.lib"));


			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("avcodec-61.dll");
			PublicDelayLoadDLLs.Add("avdevice-61.dll");
			PublicDelayLoadDLLs.Add("avfilter-10.dll");
			PublicDelayLoadDLLs.Add("avformat-61.dll");
			PublicDelayLoadDLLs.Add("avutil-59.dll");
			PublicDelayLoadDLLs.Add("postproc-58.dll");
			PublicDelayLoadDLLs.Add("swresample-5.dll");
			PublicDelayLoadDLLs.Add("swscale-8.dll");
			PublicDelayLoadDLLs.Add("nvcuvid.dll");  // 如果你使用 cuvid 解码器


			// Ensure that the DLL is staged along with the executable
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avcodec-61.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avdevice-61.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avfilter-10.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avformat-61.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/avutil-59.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/postproc-58.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/swresample-5.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/FfmpegMediaPlayerLibrary/Win64/swscale-8.dll");

			//Add inclue source
			//PublicIncludePaths.Add("$(PluginDir)/FfmpegMediaPlayer/Source/ThirdParty/include");
			PublicIncludePaths.Add("$(PluginDir)/Binaries/ThirdParty/include");
			
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
			RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/FfmpegMediaPlayerLibrary/Mac/Release/libExampleLibrary.dylib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string ExampleSoPath = Path.Combine("$(PluginDir)", "Binaries", "ThirdParty", "FfmpegMediaPlayerLibrary", "Linux", "x86_64-unknown-linux-gnu", "libExampleLibrary.so");
			PublicAdditionalLibraries.Add(ExampleSoPath);
			PublicDelayLoadDLLs.Add(ExampleSoPath);
			RuntimeDependencies.Add(ExampleSoPath);
		}
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core" , "AudioMixer", "Engine" });
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{ "CoreUObject", 
			"Engine" ,
			"Slate",
			"SlateCore" ,
			"Core",
			"CoreUObject",
			"Engine",
			"MediaUtils",
			"RenderCore",
			"Projects",
			"MediaAssets",
			"Projects",
			"InputCore",
			"DeveloperSettings",
			"UMG",
			"RHI"
		});
	}
}
