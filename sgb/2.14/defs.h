/*
 *
 * Copyright (C) 2009 jack.wgm, microcai.
 * For conditions of distribution and use, see copyright notice 
 * in (http://code.google.com/p/netsever/source/browse/trunk/COPYING)
 *
 * Author: jack.wgm
 * Email:  jack.wgm@gmail.com
 */

#ifndef _HEAP_FILE_DEFS_
#define _HEAP_FILE_DEFS_

typedef enum _tagNetMsgType
{
    MSG_USER_HEART,// ������.
    MSG_USER_LOGON // �û���½��.    
} MSGTYPE;

#define headSize        packHeadSize      // ���ݰ�ͷ��С.
#define maxBodySize        1024           // ���ݰ�����С.

#endif //_HEAP_FILE_DEFS_
