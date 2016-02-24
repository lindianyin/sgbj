/*
 *
 * Copyright (C) 2009 jack.wgm, microcai.
 * For conditions of distribution and use, see copyright notice 
 * in (http://code.google.com/p/netsever/source/browse/trunk/COPYING)
 *
 * Author: jack.wgm
 * Email:  jack.wgm@gmail.com
 */

#ifndef BASEPACKET_H__
#define BASEPACKET_H__

typedef struct _tagHead
{
    boost::uint32_t type;          // 封包类型.
    boost::uint32_t packsize;      // 封包大小,包含该头大小,数据包大小计算方法: datasize = packsize - packheadsize.
    boost::uint32_t chksum;        // 校验和,可以省略.
    boost::uint32_t encrypt;       // 加密方式,可以省略.
    boost::uint8_t  data[1];       // 数据开始字节.
}* packHeadPtr, packHead;

#define packHeadSize 16            // 不包括数据开始字节.

//////////////////////////////////////////////////////////////////////////


#endif // BASEPACKET_H__

