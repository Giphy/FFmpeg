/*
 * WEBP
 * Copyright (c) 2019 David Hargat
 * Copyright (c) 2019 Jacob Graff
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * WEBP demuxer.
 */

#include "avformat.h"
#include "libavutil/bprint.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/opt.h"
#include "internal.h"

typedef struct WEBPDemuxContext
{
} WEBPDemuxContext;

static int webp_read_header(AVFormatContext *s)
{
    AVIOContext *pb = s->pb;
    AVStream *st;
    unsigned int header_riff, header_webp;

    header_riff = avio_rl32(pb);

    avio_rl32(pb); /* header_size */

    header_webp = avio_rl32(pb);

    if (header_riff != MKTAG('R', 'I', 'F', 'F'))
        return AVERROR(EINVAL);
    if (header_webp != MKTAG('W', 'E', 'B', 'P'))
        return AVERROR(EINVAL);

    st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);

    avpriv_set_pts_info(st, 64, 1, 1000); // 1 millisecond units
    st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codecpar->codec_id = AV_CODEC_ID_WEBP;
    st->start_time = 0;
    st->duration = 1000;

    return 0;
}

static int webp_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    AVIOContext *pb = s->pb;
    int64_t frame_start = avio_tell(pb);
    int64_t packet_size;
    unsigned int chunk_type, frame_size, subchunk_type;
    int ret;

    chunk_type = avio_rl32(pb);
    frame_size = avio_rl32(pb);

    if (frame_size > INT32_MAX)
    {
        av_log(s, AV_LOG_ERROR, "chunk too large %u > %u\n", frame_size, UINT32_MAX);
        return AVERROR(EINVAL);
    }

    // skip over ALPH chunks.
#if 0
    if (chunk_type == MKTAG('A', 'N', 'M', 'F'))
    {
        avio_skip(pb, 16);
        subchunk_type = avio_rl32(pb);
        if (subchunk_type == MKTAG('A', 'L', 'P', 'H'))
        {

            av_log(s, AV_LOG_ERROR, "GIPHY detected ALPH\n");

            if (avio_skip(pb, frame_size - (16 + 4)) < 0)
            {
                av_log(s, AV_LOG_ERROR, "GIPHY seek failed\n");
                return AVERROR(EIO);
            }

            av_log(s, AV_LOG_ERROR, "GIPHY skipping ALPH\n");

            return 0;
        }
    }
#endif

    if (avio_seek(pb, frame_start, SEEK_SET) != frame_start)
        return AVERROR(EIO);

    packet_size = 4 + 4 + (int64_t)frame_size;

    ret = av_get_packet(pb, pkt, packet_size);
    if (ret < 0)
        return ret;

    pkt->pts = pkt->dts = AV_NOPTS_VALUE;
    pkt->duration = 150;

    return 0;
}

static int webp_probe(AVProbeData *p)
{
    const uint8_t *b = p->buf;
    if (AV_RB32(b) == 0x52494646 &&
        AV_RB32(b + 8) == 0x57454250)
        return AVPROBE_SCORE_MAX - 1;
    return 0;
}

static const AVOption options[] = {
    {NULL},
};

static const AVClass demuxer_class = {
    .class_name = "WEBP demuxer",
    .item_name = av_default_item_name,
    .option = options,
    .version = LIBAVUTIL_VERSION_INT,
    .category = AV_CLASS_CATEGORY_DEMUXER,
};

AVInputFormat ff_webp_demuxer = {
    .name = "webp",
    .long_name = NULL_IF_CONFIG_SMALL("WebP"),
    .priv_data_size = sizeof(WEBPDemuxContext),
    .read_probe = webp_probe,
    .read_header = webp_read_header,
    .read_packet = webp_read_packet,
    .priv_class = &demuxer_class,
    .flags = AVFMT_GENERIC_INDEX,
};
