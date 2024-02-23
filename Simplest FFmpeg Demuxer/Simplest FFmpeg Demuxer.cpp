// Simplest FFmpeg Demuxer.cpp : �������̨Ӧ�ó������ڵ㡣
//

/**
* ��򵥵Ļ��� FFmpeg ������Ƶ������
* Simplest FFmpeg Demuxer
*
* Դ����
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* �޸ģ�
* ���ĳ� Liu Wenchen
* 812288728@qq.com
* ���ӿƼ���ѧ/������Ϣ
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* ��������Խ���װ��ʽ�е���Ƶ�������ݺ���Ƶ�������ݷ��������
* �ڸ������У� �� MPEG2TS ���ļ�����õ� H.264 ��Ƶ�����ļ��� AAC ��Ƶ�����ļ���
*
* This software split a media file (in Container such as
* MKV, FLV, AVI...) to video and audio bitstream.
* In this example, it demux a MPEG2TS file to H.264 bitstream and AAC bitstream.
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// �������'fopen': This function or variable may be unsafe.Consider using fopen_s instead.
#pragma warning(disable:4996)

// ��������޷��������ⲿ���� __imp__fprintf���÷����ں��� _ShowError �б�����
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// ��������޷��������ⲿ���� __imp____iob_func���÷����ں��� _ShowError �б�����
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavformat/avformat.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif

/*
FIX: H.264 in some container formats (FLV, MP4, MKV etc.)
need "h264_mp4toannexb" bitstream filter (BSF).
1. Add SPS,PPS in front of IDR frame
2. Add start code ("0,0,0,1") in front of NALU
H.264 in some containers (such as MPEG2TS) doesn't need this BSF.
*/

// 1: Use H.264 Bitstream Filter 
#define USE_H264BSF 0

int main(int argc, char* argv[])
{
	AVOutputFormat *ofmt_audio = NULL, *ofmt_video = NULL;
	// Input AVFormatContext
	AVFormatContext *ifmt_ctx = NULL;
	// Output AVFormatContext
	AVFormatContext *ofmt_ctx_audio = NULL, *ofmt_ctx_video = NULL;
	AVPacket pkt;

	int ret;
	int videoindex = -1, audioindex = -1;
	int frame_index = 0;

	// Input file URL
	const char *in_filename = "cuc_ieschool.ts";
	// Output video file URL
	const char *out_video_filename = "cuc_ieschool.h264";
	// Output audio file URL
	const char *out_audio_filename = "cuc_ieschool.aac";

	av_register_all();

	// ����
	ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0);
	if (ret < 0)
	{
		printf("Could not open input file.\n");
		goto end;
	}

	ret = avformat_find_stream_info(ifmt_ctx, 0);
	if (ret < 0)
	{
		printf("Failed to retrieve input stream information.\n");
		goto end;
	}

	// ���
	avformat_alloc_output_context2(&ofmt_ctx_video, NULL, NULL, out_video_filename);
	if (ofmt_ctx_video == NULL)
	{
		printf("Could not create output video context.\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt_video = ofmt_ctx_video->oformat;

	avformat_alloc_output_context2(&ofmt_ctx_audio, NULL, NULL, out_audio_filename);
	if (ofmt_ctx_audio == NULL)
	{
		printf("Could not create output audio context.\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt_audio = ofmt_ctx_audio->oformat;

	// Print some input information
	printf("\n============== Input Video =============\n");
	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	for (size_t i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		// Create output AVStream according to input AVStream
		AVFormatContext *ofmt_ctx;
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = NULL;

		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			out_stream = avformat_new_stream(ofmt_ctx_video, in_stream->codec->codec);
			ofmt_ctx = ofmt_ctx_video;
		}
		else if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioindex = i;
			out_stream = avformat_new_stream(ofmt_ctx_audio, in_stream->codec->codec);
			ofmt_ctx = ofmt_ctx_audio;
		}
		else
			break;

		if (out_stream == NULL)
		{
			printf("Failed allocating output stream.\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		// Copy the settings of AVCodecContext
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ret < 0)
		{
			printf("Failed to copy context from input to output stream codec context.\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;

		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		{
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	// // Print some output information
	printf("\n============== Output Video ============\n");
	av_dump_format(ofmt_ctx_video, 0, out_video_filename, 1);
	printf("\n============== Output Audio ============\n");
	av_dump_format(ofmt_ctx_audio, 0, out_audio_filename, 1);

	// Open output file
	if (!(ofmt_video->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmt_ctx_video->pb, out_video_filename, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("Could not open output file '%s'.\n", out_video_filename);
			goto end;
		}
	}

	if (!(ofmt_audio->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmt_ctx_audio->pb, out_audio_filename, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("Could not open output file '%s'.\n", out_audio_filename);
			goto end;
		}
	}

	// Write file header
	ret = avformat_write_header(ofmt_ctx_video, NULL);
	if (ret < 0)
	{
		printf("Error occurred when opening video output file.\n");
		goto end;
	}
	ret = avformat_write_header(ofmt_ctx_audio, NULL);
	if (ret < 0)
	{
		printf("Error occurred when opening audio output file.\n");
		goto end;
	}

#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif

	while (1)
	{
		AVFormatContext *ofmt_ctx;
		AVStream *in_stream, *out_stream;
		// ��ȡһ�� AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
		{
			break;
		}

		in_stream = ifmt_ctx->streams[pkt.stream_index];

		if (pkt.stream_index == videoindex)
		{
			out_stream = ofmt_ctx_video->streams[0];
			ofmt_ctx = ofmt_ctx_video;
			printf("Write a video packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);

#if USE_H264BSF
			av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
		}
		else if (pkt.stream_index == audioindex)
		{
			out_stream = ofmt_ctx_audio->streams[0];
			ofmt_ctx = ofmt_ctx_audio;
			printf("Write an audio packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);
		}
		else
			continue;

		// ת�� PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base,
			out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base,
			out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = 0;

		// Write
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0)
		{
			printf("Error muxing packet.\n");
			break;
		}

		printf("Write %8d frames to output file.\n", frame_index);
		av_free_packet(&pkt);
		frame_index++;
	}


#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);
#endif

	av_write_trailer(ofmt_ctx_audio);
	av_write_trailer(ofmt_ctx_video);


end:
	// Close input
	avformat_close_input(&ifmt_ctx);
	// Close output
	if (ofmt_ctx_audio && !(ofmt_audio->flags & AVFMT_NOFILE))
	{
		avio_close(ofmt_ctx_audio->pb);
	}
	if (ofmt_ctx_video && !(ofmt_video->flags & AVFMT_NOFILE))
	{
		avio_close(ofmt_ctx_video->pb);
	}

	avformat_free_context(ofmt_ctx_audio);
	avformat_free_context(ofmt_ctx_video);

	if (ret < 0 && ret != AVERROR_EOF)
	{
		printf("Error occurred.\n");
		return -1;
	}


	system("pause");
	return 0;
}