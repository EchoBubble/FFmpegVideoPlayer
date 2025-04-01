// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

// 前向声明视频播放组件
class UFfmpegVideoPlayer;
/**
 * 
 */

class FDecodeRunnable : public FRunnable
{
public:
	// 构造时传入视频播放组件指针
	FDecodeRunnable(UFfmpegVideoPlayer* InOwner)
		: Owner(InOwner)
	{
	}

	// 线程入口函数
	virtual uint32 Run() override;

	// 停止线程时调用（此处依赖组件内部的 bStopThread 控制）
	virtual void Stop() override { /* 可在此扩展停止逻辑，但本例由组件内部控制 */ }

	// 可选的初始化接口
	virtual bool Init() override { return true; }

private:
	// 持有视频播放组件指针
	UFfmpegVideoPlayer* Owner;
	
	
};
