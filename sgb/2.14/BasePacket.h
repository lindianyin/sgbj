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
    boost::uint32_t type;          // �������.
    boost::uint32_t packsize;      // �����С,������ͷ��С,���ݰ���С���㷽��: datasize = packsize - packheadsize.
    boost::uint32_t chksum;        // У���,����ʡ��.
    boost::uint32_t encrypt;       // ���ܷ�ʽ,����ʡ��.
    boost::uint8_t  data[1];       // ���ݿ�ʼ�ֽ�.
}* packHeadPtr, packHead;

#define packHeadSize 16            // ���������ݿ�ʼ�ֽ�.

//////////////////////////////////////////////////////////////////////////


#endif // BASEPACKET_H__

