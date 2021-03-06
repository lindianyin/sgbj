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
    MSG_USER_HEART,// 心跳包.
    MSG_USER_LOGON // 用户登陆包.    
} MSGTYPE;

#define headSize        packHeadSize      // 数据包头大小.
#define maxBodySize        1024           // 数据包最大大小.

#endif //_HEAP_FILE_DEFS_
