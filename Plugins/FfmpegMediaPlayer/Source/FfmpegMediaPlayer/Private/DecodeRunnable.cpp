// Fill out your copyright notice in the Description page of Project Settings.


#include "DecodeRunnable.h"
#include "FfmpegVideoPlayer.h"  // 包含视频播放组件头文件

uint32 FDecodeRunnable::Run()
{
	// 在单独线程中调用视频播放组件的解码循环
	if (Owner)
	{
		Owner->DecodeVideoLoop();
	}
	return 0;
}
