#pragma once

#include <libavutil/rational.h>

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}
#include "FFmpegVideoPlayer.generated.h"

// 前置声明FFmpeg结构体
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVPacket;
struct AVFrame;
struct SwsContext;
struct AVBufferRef;

/**
 * 基于 FFmpeg 的视频播放器组件，支持硬件解码和蓝图控制
 */
UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class UFfmpegVideoPlayer : public UActorComponent
{
    GENERATED_BODY()

public:
    UFfmpegVideoPlayer();

    /** 开始播放指定视频文件（路径可以是绝对路径或相对于工作目录）。 */
    UFUNCTION(BlueprintCallable, Category="Video")
    bool StartVideoPlayback(const FString& VideoFilePath);

    /** 停止播放并释放资源。 */
    UFUNCTION(BlueprintCallable, Category="Video")
    void StopVideoPlayback();

    /** 获取包含视频帧的动态纹理。 */
    UFUNCTION(BlueprintCallable, Category="Video")
    UTexture2D* GetVideoTexture() const { return VideoTexture; }

    // 内部解码线程调用的函数
    void DecodeVideoLoop();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // 线程停止标志（线程安全）
    FThreadSafeBool bStopThread;

    // FFmpeg 相关结构体
    AVFormatContext* FormatCtx;
    AVCodecContext* CodecCtx;
    const AVCodec* Codec;
    AVFrame* Frame;
    AVPacket* Packet;
    SwsContext* SwsCtx;
    int VideoStreamIndex;
    double VideoTimeBase;  // 时间基（秒/单位）

    // 视频信息
    int VideoWidth;
    int VideoHeight;
    AVRational FrameRate;

    // 输出 BGRA 格式帧的缓冲区
    TArray<uint8> FrameBuffer;  // 大小为 VideoWidth * VideoHeight * 4 (BGRA)

    // 用于更新视频帧的纹理
    UPROPERTY()  // 采用 UPROPERTY 便于 GC 管理
    UTexture2D* VideoTexture;

    // 更新纹理时覆盖整个区域
    FUpdateTextureRegion2D* VideoUpdateRegion;

    // 解码线程句柄
    FRunnableThread* DecodeThread;
};