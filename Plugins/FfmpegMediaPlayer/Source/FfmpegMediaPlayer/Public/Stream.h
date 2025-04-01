// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

}

#include "Engine/Texture2D.h"
#include "Async/Async.h"
#include "RHICommandList.h"
#include "Rendering/Texture2DResource.h"
#include "RHI.h"
#include "RHIResources.h"

#include "Stream.generated.h"





/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReadyTwo,int,code,FString,msg);
UCLASS(Blueprintable, BlueprintType)
class FFMPEGMEDIAPLAYER_API UStream : public UObject
{
	GENERATED_BODY()

protected:
	
	AVFormatContext* avformatcontext = nullptr;
	AVPacket* avpacket = nullptr;
	
	//---------------video-----------
	AVStream* vstream;
	AVCodecContext* vcodecc;
	AVFrame* vframe;

	SwsContext* vswsc;
	//-------------------------------------

	int videoIndex;
	int IsRun = 0;//Pstop 1ready 2play

	UStream();
	~UStream();

	int checki(int ret, FString msg);
	void* checkp(void* object,FString msg);

	void readyThread();
	void playThread();

public:
	UPROPERTY(BlueprintAssignable)
	FOnReadyTwo OnReadyTwo;

	UFUNCTION(BlueprintCallable,Category="FfmpegMediaPlayer|video")
	void ready();
	UFUNCTION(BlueprintCallable,Category="FfmpegMediaPlayer|video")
	void play();
	UFUNCTION(BlueprintCallable,Category="FfmpegMediaPlayer|video")
	void stop();

	UPROPERTY(BlueprintReadWrite,Category="FfmpegMediaPlayer|video")
	FString URL;
	UPROPERTY(BlueprintReadWrite,Category="FfmpegMediaPlayer|video")
	UTexture2D* texture;
	UPROPERTY(BlueprintReadWrite,Category="FfmpegMediaPlayer|video")
	int width = 0;
	UPROPERTY(BlueprintReadWrite,Category="FfmpegMediaPlayer|video")
	int height = 0;
};
