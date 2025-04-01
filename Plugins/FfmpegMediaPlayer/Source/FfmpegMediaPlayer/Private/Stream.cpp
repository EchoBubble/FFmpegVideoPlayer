// Fill out your copyright notice in the Description page of Project Settings.


#include "Stream.h"

UStream::UStream()
{
}

UStream::~UStream()
{
	UE_LOG(LogTemp, Log, TEXT(">>>>>>>>>>>>>>~stream>>>>>>>>>>>>>>"));
	stop();
}

int UStream::checki(int ret, FString msg)
{
	if (ret != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("///////////%d %s"),ret, *msg);
		OnReadyTwo.Broadcast(-1,msg);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("***********%d %s"),ret, *msg);
	}
	return ret;
}

void* UStream::checkp(void* object, FString msg)
{
	if (object == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("/////////// %s"), *msg);
		OnReadyTwo.Broadcast(-1,msg);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("***********%s  is not nullptr"), *msg);
	}
	return object;
}

void UStream::stop()
{
	IsRun = 0;

	FPlatformProcess::Sleep(0.5);
	if (vframe != nullptr)
	{
		av_frame_unref(vframe);
	}
	if (vcodecc != nullptr)
	{
		avcodec_close(vcodecc),vcodecc=nullptr;
	}
	if (vswsc != nullptr)
	{
		sws_freeContext(vswsc),vswsc = nullptr;
	}
	if (avpacket != nullptr)
	{
		av_packet_free(&avpacket),avpacket = nullptr;
	}
	if (avformatcontext != nullptr)
	{
		avformat_close_input(&avformatcontext),avformat_free_context(avformatcontext),avformatcontext = nullptr;
	}
	
}

void UStream::ready()
{
	IsRun = 1;
	AsyncTask(ENamedThreads::AnyThread, [&]()
	{
		readyThread();
	});

}

void UStream::readyThread()
{
	UE_LOG(LogTemp, Log, TEXT("--------URL----------%s"),*URL);

	int ret;

	avformat_network_init();

	avformatcontext = avformat_alloc_context();

	ret = avformat_open_input(&avformatcontext,TCHAR_TO_ANSI(*URL),nullptr,nullptr);
	if (checki(ret,TEXT("Opening Input Media Streams & Getting Headers")) != 0)return;

	ret = avformat_find_stream_info(avformatcontext,nullptr);
	if (checki(ret,TEXT("Get audio/video details ")) != 0)return;

	for (size_t i = 0; i < avformatcontext->nb_streams; i++)
	{
		AVStream *stream = avformatcontext->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)//视频流
		{
			const AVCodec* dec = avcodec_find_decoder(stream->codecpar->codec_id);//查找解码信息结构体
			if (checkp((void*)dec,TEXT("Vedio AVCodec Struct ")) == NULL)return;

			vcodecc = avcodec_alloc_context3(dec);//分配解析器上下文
			if (checkp((void*)dec,TEXT("Assigning a parser context ")) == NULL)return;

			ret = avcodec_parameters_to_context(vcodecc,stream->codecpar);//解码信息赋给codeContext的各个成员
			if (checki(ret,TEXT("解码信息赋给codeContext的各个成员 ")) != 0)return;

			ret = avcodec_open2(vcodecc,dec,0);//打开解码器
			if (checki(ret,TEXT("打开解码器 ")) != 0)return;
			
			width = stream->codecpar->width;
			height = stream->codecpar->height;

			UE_LOG(LogTemp, Log, TEXT("--------视频宽高----------%d %d"),width,height);

			vstream = stream;
			videoIndex = i;

		}
	}
	if (videoIndex == -1)
	{
		UE_LOG(LogTemp, Log, TEXT("/////////没有找到视频////////"));
		return;
	}
	avpacket = av_packet_alloc();//分配一个包头
	if (checkp((void*)avpacket,TEXT("分配一个包头 ")) == NULL)return;

	vframe = av_frame_alloc();

	IsRun = 1;//准备状态

	Async(EAsyncExecution::TaskGraph, [&]()
	{
		//在处理纹理之前添加FTaskTagScope标签
		FTaskTagScope TaskTagScope(ETaskTag::EParallelRenderingThread);
		texture = UTexture2D::CreateTransient(width,height,PF_B8G8R8A8);
		texture->UpdateResource();
	});
	FPlatformProcess::Sleep(0.5);
	FTaskTagScope TaskTagScope(ETaskTag::EParallelGameThread);
	OnReadyTwo.Broadcast(0,"success");
	
}

void UStream::play()
{
	IsRun = 2;
	AsyncTask(ENamedThreads::AnyThread, [&]()
	{
		playThread();
	});
}

void UStream::playThread()
{
	while (IsRun == 2)
	{
		int ret = av_read_frame(avformatcontext,avpacket);
		if (ret < 0 || IsRun !=2)
		{
			UE_LOG(LogTemp, Log, TEXT("--------av_read_frame err----------%d"),ret);
			return;
		}
		if(avpacket->stream_index == videoIndex)
		{
			ret = avcodec_send_packet(vcodecc,avpacket);//packet发送给解码器
			return;
		}

		while (ret>=0 || IsRun != 2)
		{
			ret = avcodec_receive_frame(vcodecc,vframe);//接收解码后的视频帧
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				//UE_LOG(LogTemp, Log, TEXT("-------- AVERROR(EAGAIN) AVERROR_EOF----------%d"),ret);
				break;
			}
			else if (ret < 0 || IsRun != 2)
			{
				UE_LOG(LogTemp, Log, TEXT("--------avcodec_receive_frame err----------%d"),ret);
				return;
			}
			if (vswsc == nullptr)
			{
				vswsc = sws_getContext(width,height,(AVPixelFormat)vframe->format,width,height,
					AV_PIX_FMT_BGRA,SWS_FAST_BILINEAR,NULL,NULL,NULL);
			}
			uint8_t* rgb = new uint8[width * height * 4];
			uint8_t* data[AV_NUM_DATA_POINTERS] = {0};
			int lines[AV_NUM_DATA_POINTERS] = {0};

			data[0] = (uint8_t*)rgb;
			lines[0] = width * 4;

			int retsws = sws_scale(vswsc,(const uint8_t**)vframe->data,vframe->linesize,0,height,data,lines);

			av_frame_unref(vframe);

			FTaskTagScope TaskTagScope(ETaskTag::EParallelGameThread);
			auto regin = FUpdateTextureRegion2D(0,0,0,0,width,height);
			auto pitch = GPixelFormats[texture->GetPixelFormat()].BlockBytes * width;

			int width_ = width;
			int height_ = height;

			FTextureResource* TextureResource = texture->GetResource();

			ENQUEUE_RENDER_COMMAND(UpdateTextureDataCommand)([TextureResource,pitch,width_,height_, rgb]
				(FRHICommandListImmediate& RHICmdList)
				{
					RHIUpdateTexture2D(TextureResource->GetTexture2DRHI(),
						0,
						FUpdateTextureRegion2D(0,0,0,0,width_,height_),
						pitch,rgb
						);
						delete rgb;
				 
				}
			);
		}
	}
}


