/**********************************************************
 * Author        : RaKiRaKiRa
 * Email         : 763600693@qq.com
 * Create time   : 2019-06-18 16:07
 * Last modified : 2019-07-18 01:01
 * Filename      : Epoller.h
 * Description   : 
 **********************************************************/

#ifndef EPOLLER_H
#define EPOLLER_H

#include "IPoller.h"
#include <sys/epoll.h>

class Epoller :public IPoller
{
public:
  //通过data() 实现对epoll_wait返回值的存储
  typedef std::vector<struct epoll_event> eventList;

  Epoller(EventLoop* loop);
  
  ~Epoller();
  
  //IO复用,将活跃Channel放入activeChannels
  void poll(int timeout, ChannelList* activeChannels);
  
  //将对应channel从channelByFd——删除并通过epoll_ctl从监听中取消
  void removeChannel(Channel* channel);

  //维护和更新eventfd_   和channel_
  void updateChannel(Channel* channel);
  

private:
  //封装epoll_ctl
  void update(int op, Channel* channel);

  void fill_activeChannels(int activeNum, ChannelList* activeChannels) const;

  const int eventList_InitialSize = 16;


  int epollfd_;

  //存储epoll_wait返回事件
  eventList events_;

};

#endif
