/*
 * DiskSim Storage Subsystem Simulation Environment (Version 4.0)
 * Revision Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty" statements
 * are included with all reproductions and derivative works and associated
 * documentation. This software may also be redistributed without charge
 * provided that the copyright and "No Warranty" statements are included
 * in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE
 * OR DOCUMENTATION.
 *
 */

/*
 * A sample skeleton for a system simulator that calls DiskSim as
 * a slave.
 *
 * Contributed by Eran Gabber of Lucent Technologies - Bell Laboratories
 *
 * Usage:
 *	syssim <parameters file> <output file> <max. block number>
 * Example:
 *	syssim parv.seagate out 2676846
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#include "syssim_driver.h"
#include "disksim_interface.h"
#include "disksim_rand48.h"


#define	BLOCK	8	// xfer 8 blocks
#define	SECTOR	512

typedef	struct	{
  int n;
  double sum;
  double sqr;
} Stat;


static SysTime now = 0;		/* current time */
static SysTime next_event = -1;	/* next event */
static int completed = 0;	/* last request was completed */
static Stat st;


void
panic(const char *s)
{
  perror(s);
  exit(1);
}


void
add_statistics(Stat *s, double x)
{
  s->n++;
  s->sum += x;
  s->sqr += x*x;
}


void
print_statistics(Stat *s, const char *title)
{
  double avg, std;

  avg = s->sum/s->n;
  std = sqrt((s->sqr - 2*avg*s->sum + s->n*avg*avg) / s->n);
  printf("%s: n=%d average=%f std. deviation=%f\n", title, s->n, avg, std);
}

void
avg_statistics_pro(Stat *s, const char *title, int seq)
{
  double avg, std;
  // if (seq == 0)
  // {
  //   s->sum += s->n*5;
  // }
  avg = s->sum/s->n;
  std = sqrt((s->sqr - 2*avg*s->sum + s->n*avg*avg) / s->n);
  //改成纳秒，和ssdsim同步
  printf("%d\n", (int)(avg * 1000000));
}

void
avg_statistics(Stat *s, const char *title)
{
  double avg, std;

  avg = s->sum/s->n;
  std = sqrt((s->sqr - 2*avg*s->sum + s->n*avg*avg) / s->n);
  //改成纳秒，和ssdsim同步
  printf("%d\n", (int)(avg * 1000000));
}

/*
 * Schedule next callback at time t.
 * Note that there is only *one* outstanding callback at any given time.
 * The callback is for the earliest event.
 */
void
syssim_schedule_callback(disksim_interface_callback_t fn, 
			 SysTime t, 
			 void *ctx)
{
  next_event = t;
}


/*
 * de-scehdule a callback.
 */
void
syssim_deschedule_callback(double t, void *ctx)
{
  next_event = -1;
}


void
syssim_report_completion(SysTime t, struct disksim_request *r, void *ctx)
{
  completed = 1;
  now = t;
  add_statistics(&st, t - r->start);
}

/**
 * @brief 执行次数、读写、随机/顺序、配置文件
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int
main(int argc, char *argv[])
{
  int i, is_read;
  int nsectors = 2676846, times, blkno = 1, is_sequential;
  struct disksim_request r;
  struct disksim_interface *disksim;

  if (argc != 5 || (times = atoi(argv[1])) <= 0) {
    fprintf(stderr, "usage: %s <tiems> <#is_sequential>\n",
	    argv[0]);
    exit(1);
  }
  //是否是读,==1是读,否则写
  is_read = atoi(argv[2]);
  //是否是随机的,==1是顺序,否则随机
  is_sequential = atoi(argv[3]);

  disksim = disksim_interface_initialize(argv[4], 
					 "syssim.outv",
					 syssim_report_completion,
					 syssim_schedule_callback,
					 syssim_deschedule_callback,
					 0,
					 0,
					 0);
  DISKSIM_srand48(1);
  /**
 * 值得注意的两点是请求的 blkno 和 bytecount。
 * 不管使用 ssdsim 还是 hddsim, disksim 内部操作的块都是以512B为单位，
 * 所以请求的 blkno 的单位为 512B，对于 hdd 而言，大小可以任意，而对ssd而言其必须为4k的整数倍，即该值必须是8的整数倍。
 * bytecount 的单位是字节，对于 ssd 而言同样应该是 4k 的倍数，即该值应该为4096的整数倍。
 */
  for (i=0; i < times; i++) {
    r.start = now;
    r.flags = is_read == 1 ? DISKSIM_READ : DISKSIM_WRITE;
    // printf("flags: %d\n", r.flags);
    r.devno = 0;
    if (i == 0)
    {
      blkno = DISKSIM_lrand48()%nsectors;
      r.blkno = blkno;
    } else {
      r.blkno = is_sequential == 1 ? blkno += 2 : DISKSIM_lrand48()%nsectors;
    }
    // printf("blkno: %d\n", r.blkno);
    r.bytecount = BLOCK * 2;
    completed = 0;
    disksim_interface_request_arrive(disksim, now, &r);

    /* Process events until this I/O is completed */
    while(next_event >= 0) {
      now = next_event;
      next_event = -1;
      disksim_interface_internal_event(disksim, now, 0);
    }

    if (!completed) {
      fprintf(stderr,
	      "%s: internal error. Last event not completed %d\n",
	      argv[0], i);
      exit(1);
    }
  }

  disksim_interface_shutdown(disksim, now);

  avg_statistics(&st, "response time");
  print_statistics(&st, "response time");

  exit(0);
}
