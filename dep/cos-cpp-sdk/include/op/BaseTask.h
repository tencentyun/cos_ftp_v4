/*************************************************************************
  Copyright (C) 2016 Tencent Inc.
  All rights reserved.

  > File Name: BaseTask.h
  > Author: chengwu
  > Mail: chengwu@tencent.com
  > Created Time: Wed 28 Sep 2016 07:46:00 PM CST
  > Description: 
 ************************************************************************/

#ifndef _BASETASK_H
#define _BASETASK_H
#pragma once
class BaseTask {
    public:
        virtual void run()=0;
};
#endif // #ifndef _BASETASK_H
