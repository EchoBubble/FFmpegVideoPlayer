#include "FFmpegVideoPlayer.h"
#include "HAL/RunnableThread.h"
#include "DecodeRunnable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformProcess.h"
#include "Rendering/Texture2DResource.h"


UFfmpegVideoPlayer::UFfmpegVideoPlayer()
{
    PrimaryComponentTick.bCanEverTick = true;   // Enable ticking to update texture
    PrimaryComponentTick.TickGroup = TG_PrePhysics;  // Tick early if needed

    // Initialize pointers
    FormatCtx = nullptr;
    CodecCtx = nullptr;
    Codec = nullptr;
    Frame = nullptr;
    Packet = nullptr;
    SwsCtx = nullptr;
    VideoTexture = nullptr;
    VideoUpdateRegion = nullptr;
    DecodeThread = nullptr;
    VideoStreamIndex = -1;
    VideoWidth = VideoHeight = 0;
    bStopThread = false;
}

void UFfmpegVideoPlayer::BeginPlay()
{
    Super::BeginPlay();
    // (Optional: Initialize FFmpeg global state here if not already done)
    // e.g., av_log_set_level(AV_LOG_WARNING);
}

bool UFfmpegVideoPlayer::StartVideoPlayback(const FString& VideoFilePath)
{
    // 将路径转换为绝对路径，并转换为 UTF-8 格式的 std::string
    FString AbsolutePath = FPaths::ConvertRelativePathToFull(VideoFilePath);
    std::string FilePath(TCHAR_TO_UTF8(*AbsolutePath));
    // Convert UE string to standard C string (UTF-8)
    //std::string FilePath(TCHAR_TO_UTF8(*VideoFilePath));

    // Open video file
    FormatCtx = avformat_alloc_context();
    if (avformat_open_input(&FormatCtx, FilePath.c_str(), NULL, NULL) != 0) {
        UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not open video file %s"), *AbsolutePath);
        return false;
    }
    if (avformat_find_stream_info(FormatCtx, NULL) < 0) {
        UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not find stream info"));
        return false;
    }
    // Find the first video stream
    AVStream* VideoStream = nullptr;
    for (unsigned i = 0; i < FormatCtx->nb_streams; ++i) {
        if (FormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            VideoStreamIndex = i;
            VideoStream = FormatCtx->streams[i];
            break;
        }
    }
    if (!VideoStream) {
        UE_LOG(LogTemp, Error, TEXT("FFmpeg: No video stream found in file"));
        return false;
    }
    VideoTimeBase = av_q2d(VideoStream->time_base);
    FrameRate = VideoStream->r_frame_rate; // could also use avg_frame_rate

    // Find decoder for the video stream
    AVCodecParameters* CodecPar = VideoStream->codecpar;
    Codec = avcodec_find_decoder(CodecPar->codec_id);
    if (!Codec) {
        UE_LOG(LogTemp, Error, TEXT("FFmpeg: Unsupported codec (ID %d)"), CodecPar->codec_id);
        return false;
    }
    // Allocate codec context
    CodecCtx = avcodec_alloc_context3(Codec);
    avcodec_parameters_to_context(CodecCtx, CodecPar);
    // 👉 插入 GPU 解码上下文配置（仅当使用 NVIDIA 硬解码器）：
    if (Codec && Codec->name && (FCStringAnsi::Strstr(Codec->name, "cuvid") != nullptr)) {
        AVBufferRef* hw_device_ctx = nullptr;
        if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0) < 0) {
            UE_LOG(LogTemp, Error, TEXT("FFmpeg: 无法创建 CUDA 设备上下文"));
            return false;
        }
        CodecCtx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

        // 分配硬件帧上下文
        AVBufferRef* hw_frames_ref = av_hwframe_ctx_alloc(CodecCtx->hw_device_ctx);
        if (!hw_frames_ref) {
            UE_LOG(LogTemp, Error, TEXT("FFmpeg: 无法分配硬件帧上下文"));
            return false;
        }

        AVHWFramesContext* hw_frames_ctx = (AVHWFramesContext*)hw_frames_ref->data;
        hw_frames_ctx->format = AV_PIX_FMT_CUDA;      // GPU 上的格式
        hw_frames_ctx->sw_format = AV_PIX_FMT_NV12;   // 转到 CPU 后的格式（用于 sws_scale）
        hw_frames_ctx->width = CodecCtx->width;
        hw_frames_ctx->height = CodecCtx->height;
        hw_frames_ctx->initial_pool_size = 20;

        if (av_hwframe_ctx_init(hw_frames_ref) < 0) {
            UE_LOG(LogTemp, Error, TEXT("FFmpeg: 初始化硬件帧上下文失败"));
            av_buffer_unref(&hw_frames_ref);
            return false;
        }

        CodecCtx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
        av_buffer_unref(&hw_frames_ref);
    }
    // Suggest multi-threading
    CodecCtx->thread_count = FMath::Max(4, FPlatformMisc::NumberOfCores()); // use 4 or number of cores
    // Open codec
    if (avcodec_open2(CodecCtx, Codec, NULL) < 0) {
        UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not open codec"));
        return false;
    }

    // Get video dimensions and format
    VideoWidth = CodecCtx->width;
    VideoHeight = CodecCtx->height;
    enum AVPixelFormat DecodedFormat = CodecCtx->pix_fmt;

    // Allocate frame and packet
    Frame = av_frame_alloc();
    Packet = av_packet_alloc();

    // Allocate scaling context to convert to BGRA8888
    SwsCtx = sws_getContext(VideoWidth, VideoHeight, DecodedFormat,
                             VideoWidth, VideoHeight, AV_PIX_FMT_BGRA,
                             SWS_BILINEAR, NULL, NULL, NULL);
    if (!SwsCtx) {
        UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not initialize SwsContext"));
        return false;
    }

    // Allocate frame buffer for BGRA data
    int destLinesize[4];
    // Determine required buffer size and allocate
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGRA, VideoWidth, VideoHeight, 1);
    FrameBuffer.SetNumUninitialized(numBytes);
    uint8* destData[4];
    av_image_fill_arrays(destData, destLinesize, FrameBuffer.GetData(), AV_PIX_FMT_BGRA, VideoWidth, VideoHeight, 1);

    // Create the UTexture2D to hold video frames
    if (!VideoTexture) {
        VideoTexture = UTexture2D::CreateTransient(VideoWidth, VideoHeight, PF_B8G8R8A8);
        if (!VideoTexture) {
            UE_LOG(LogTemp, Error, TEXT("Unreal: Failed to create video texture"));
            return false;
        }
        VideoTexture->NeverStream = true;
        VideoTexture->SRGB = true;  // assuming video is in sRGB color space
        VideoTexture->UpdateResource(); // finalize texture creation
    }
    // Prepare an update region covering the whole texture
    VideoUpdateRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, VideoWidth, VideoHeight);

    // Start the decoding thread
    bStopThread = false;
    FDecodeRunnable* Runnable = new FDecodeRunnable(this);
    DecodeThread = FRunnableThread::Create(Runnable,
        *FString::Printf(TEXT("VideoDecodeThread_%s"), *AbsolutePath),
        0,
        TPri_AboveNormal);

    return true;
}

void UFfmpegVideoPlayer::DecodeVideoLoop()
{
    // This runs in a separate thread
    int ret = 0;
    // Initial timestamp reference
    double startTime = FPlatformTime::Seconds();
    while (!bStopThread)
    {
        ret = av_read_frame(FormatCtx, Packet);
        if (ret < 0) {
            // End of file or error
            break;
        }
        if (Packet->stream_index == VideoStreamIndex) {
            // Send packet to decoder
            if (avcodec_send_packet(CodecCtx, Packet) < 0) {
                // Error sending packet (could log)
            }
            // Receive available frames
            while (!bStopThread) {
                ret = avcodec_receive_frame(CodecCtx, Frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break; // need more data or end of stream
                }
                if (ret < 0) {
                    // Decoding error
                    UE_LOG(LogTemp, Warning, TEXT("FFmpeg: Error during decoding frame"));
                    break;
                }
                // We have a decoded frame (in YUV format). Convert it to BGRA.
                // === 判断帧是否在 GPU 内存中 ===
                AVFrame* sw_frame = nullptr;
                if (CodecCtx->hw_device_ctx)
                {
                    AVHWFramesContext* hwFramesCtx = nullptr;
                    if (CodecCtx->hw_frames_ctx)
                    {
                        hwFramesCtx = (AVHWFramesContext*)CodecCtx->hw_frames_ctx->data;
                    }

                    if (hwFramesCtx && Frame->format == hwFramesCtx->format)
                    {
                        sw_frame = av_frame_alloc();
                        if (av_hwframe_transfer_data(sw_frame, Frame, 0) < 0) {
                            UE_LOG(LogTemp, Error, TEXT("FFmpeg: GPU 帧转 CPU 帧失败"));
                            av_frame_free(&sw_frame);
                            return;
                        }
                    }
                }

                // 如果不是 GPU 帧，直接用原始帧
                if (!sw_frame) {
                    sw_frame = Frame;
                }

                // 初始化 SwsCtx（第一次进来才会执行）
                if (!SwsCtx)
                {
                    SwsCtx = sws_getContext(VideoWidth, VideoHeight, (AVPixelFormat)sw_frame->format,
                                            VideoWidth, VideoHeight, AV_PIX_FMT_BGRA,
                                            SWS_BILINEAR, NULL, NULL, NULL);

                    if (!SwsCtx) {
                        UE_LOG(LogTemp, Error, TEXT("FFmpeg 初始化 SwsContext 失败"));
                        return;
                    }
                }

                // ✅ 开始转换为 BGRA
                uint8* DestData[4] = { FrameBuffer.GetData(), nullptr, nullptr, nullptr };
                int DestLinesize[4] = { VideoWidth * 4, 0, 0, 0 };

                sws_scale(SwsCtx,
                    sw_frame->data, sw_frame->linesize,
                    0, VideoHeight,
                    DestData, DestLinesize);

                if (sw_frame != Frame)
                {
                    av_frame_free(&sw_frame);
                }
                // FrameBuffer now has the BGRA data for this frame.

                // Frame timing: wait if needed to sync to FPS (rough synchronization)
                if (Frame->pts != AV_NOPTS_VALUE) {
                    double framePtsSec = Frame->pts * VideoTimeBase;  // PTS in seconds
                    double currentTime = FPlatformTime::Seconds() - startTime;
                    double delta = framePtsSec - currentTime;
                    if (delta > 0) {
                        // Sleep for the remaining time until this frame should be displayed
                        FPlatformProcess::Sleep(static_cast<float>(delta));
                    }
                }

                // Mark that a new frame is ready (could use a thread-safe flag or event)
                // In this simple approach, we update the texture on the game thread Tick by checking for new data.
                // Here we might set an atomic flag like bFrameReady = true, or just rely on the texture update being done in Tick.

                // For simplicity, we will trigger the texture update here in a thread-unsafe way (not ideal for actual code):
                // We enqueue a render command to update the texture with the new FrameBuffer.
                // (Better approach: copy FrameBuffer to a second buffer and let Tick handle it, but omitted for brevity.)
                ENQUEUE_RENDER_COMMAND(UpdateVideoTexture)(
                    [this](FRHICommandListImmediate& RHICmdList)
                    {
                        if (VideoTexture && VideoUpdateRegion && FrameBuffer.Num() > 0) {
                            // Lock the texture and update pixel data
                            FTexture2DResource* Resource = (FTexture2DResource*)VideoTexture->Resource;
                            if(Resource) {
                                RHIUpdateTexture2D(Resource->GetTexture2DRHI(), 0, *VideoUpdateRegion,
                                                   VideoWidth * 4, FrameBuffer.GetData());
                            }
                        }
                    }
                );

                // After queuing the update, break out to process next packet (or to allow next tick).
                break;
            }
        }
        // Unref packet regardless of type
        av_packet_unref(Packet);
    }

    // Flush decoder for any remaining frames
    avcodec_send_packet(CodecCtx, NULL);
    while (avcodec_receive_frame(CodecCtx, Frame) == 0) {
        // (For brevity, not handling flush frames here. In practice, you would convert and display them same as above.)
    }

    // Decoding loop ended. We signal that playback is done (could notify game thread if needed).
}

void UFfmpegVideoPlayer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // In this example, we triggered texture update in the decode thread via ENQUEUE_RENDER_COMMAND.
    // Alternatively, we could do it here if using a flag and double buffer.
    // If using a flag (bFrameReady), we'd lock a mutex, copy FrameBuffer to texture using UpdateTextureRegions.
    // That approach would go here. Since we did it in the thread, nothing else is needed in Tick for updating texture.
}

void UFfmpegVideoPlayer::StopVideoPlayback()
{
    // Signal the decode thread to stop and wait for it
    bStopThread = true;
    if (DecodeThread) {
        DecodeThread->Kill(true);  // Wait for thread to exit
        delete DecodeThread;
        DecodeThread = nullptr;
    }

    // Free FFmpeg resources
    if (Frame) { av_frame_free(&Frame); Frame = nullptr; }
    if (Packet) { av_packet_free(&Packet); Packet = nullptr; }
    if (CodecCtx) { avcodec_free_context(&CodecCtx); CodecCtx = nullptr; }
    if (FormatCtx) { avformat_close_input(&FormatCtx); FormatCtx = nullptr; }
    if (SwsCtx) { sws_freeContext(SwsCtx); SwsCtx = nullptr; }

    // Free the update region
    if (VideoUpdateRegion) {
        delete VideoUpdateRegion;
        VideoUpdateRegion = nullptr;
    }
    // Note: We don't explicitly destroy VideoTexture here; if the component is destroyed, GC will collect it.
    // If needed, we could also manually ReleaseResource on it.
}

void UFfmpegVideoPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Ensure video stops when the component or game ends
    StopVideoPlayback();
    Super::EndPlay(EndPlayReason);
}
