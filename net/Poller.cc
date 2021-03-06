/**********************************************************
 * Author        : RaKiRaKiRa
 * Email         : 763600693@qq.com
 * Create time   : 2019-06-19 16:13
 * Last modified : 2019-07-18 01:00
 * Filename      : Poller.cc
 * Description   : 
 **********************************************************/

#include "Poller.h"

Poller::Poller(EventLoop *loop) : IPoller(loop)
{
}

//IO复用,将活跃Channel放入activeChannels
void Poller::poll(int timeout, ChannelList *activeChannels)
{
  //int poll(struct pollfd fds[], nfds_t nfds, int timeout);
  int activeNum = ::poll(pollfds_.data(), pollfds_.size(), timeout);
  int savedErr = errno;

  //若有活跃事件触发,放入activeChannels
  if(activeNum > 0)
  {
    LOG_TRACE << activeNum <<" events happened";
    fill_activeChannels(activeNum, activeChannels);
  }
  //若无事件且不报错
  else if(activeNum == 0)
  {
    LOG_TRACE << " nothing happened";
  }
  //若有错误发生
  else if(savedErr != EINTR)
  {
    errno = savedErr;
    LOG_ERROR <<"Poller::poll()"; 
  }

}

// 活跃事件对应Channel修改revent并加入activeChannels
void Poller::fill_activeChannels(int activeNum, ChannelList *activeChannels) const
{
  /*
   *struct pollfd {
   *  int fd;        // 文件描述符
   *  short events;  // 等待的需要测试事件 
   *  short revents; // 实际发生了的事件，也就是返回结果
   *};
  */
  for(pollfdList::const_iterator it = pollfds_.cbegin(); it < pollfds_.cend() && activeNum > 0; ++it)
  {
    if(it -> revents)
    {
      --activeNum;//减为0时退出循环,加速轮询
      ChannelMap::const_iterator ch = channelsByFd_.find(it -> fd);//寻找触发事件的fd对应的Channel
      assert(ch != channelsByFd_.end());//不可能找不到
      Channel* channel = ch -> second;
      assert(channel -> fd() == it -> fd);//找错则报错
      //修改Channel的revent并加入activeChannels
      channel -> setRevent(it -> revents);
      activeChannels -> push_back(channel);
    }
  }
}

//维护和更新pollfd_   和channel_
void Poller::updateChannel(Channel* channel)
{
  assertInLoop();
  //Channel初始index为-1,再加入监听后再改为其在pollfds_的下标
  if(channel -> index() < 0)//未加入监听,则加入监听 pollfds_
  {
    /*
     *struct pollfd {
     *  int fd;        // 文件描述符
     *  short events;  // 等待的需要测试事件 
     *  short revents; // 实际发生了的事件，也就是返回结果
     *};
    */
    struct pollfd pfd;
    pfd.fd      = channel -> fd();
    pfd.events  = channel -> event();
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    channel -> setIndex(idx);
    channelsByFd_[pfd.fd] = channel;
  }
  else//已加入监听,进行更新
  {
    assert(channelsByFd_.find(channel -> fd()) != channelsByFd_.end());
    assert(channelsByFd_[channel -> fd()] == channel);

    int idx = channel -> index();
    struct pollfd& pfd = pollfds_[idx];//引用

    //当channel无监听事件时,为了增加性能,使对应pollfds_中的fd为负数,不进行监听,故有此看似多此一举的操作
    pfd.fd      = channel -> fd();
    if(channel -> isNoneEvent())
    {
      pfd.fd = -channel->fd() - 1;
    }
    pfd.events  = static_cast<short>(channel -> event());
    pfd.revents = 0;
  }
}

void Poller::removeChannel(Channel* channel)
{
  LOG_TRACE <<"fd = " << channel -> fd();
  assert(channelsByFd_.find(channel->fd()) != channelsByFd_.end() && channelsByFd_[channel->fd()] == channel);

  int idx = channel -> index();//获得其在pollfds_中的下标

  //从channelsByFd_删除
  channelsByFd_.erase(channel -> fd());

  //从pollfds_中删除，移至最后在pop
  if(static_cast<size_t>(idx) == pollfds_.size() - 1)
  {
    pollfds_.pop_back();
  }
  else
  {
    int endFd = pollfds_.back().fd;
    iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
    if(endFd < 0)//无关注事件，获取其真实fd
    {
      endFd = -endFd - 1;
    }
    channelsByFd_[endFd] -> setIndex(idx);
    pollfds_.pop_back();
  }
}
