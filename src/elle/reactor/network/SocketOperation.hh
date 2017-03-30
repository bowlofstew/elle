#pragma once

#include <boost/asio.hpp>

#include <elle/reactor/network/Error.hh>
#include <elle/reactor/network/socket.hh>
#include <elle/reactor/Operation.hh>

namespace elle
{
  namespace reactor
  {
    namespace network
    {
      template <typename AsioSocket>
      class SocketOperation: public Operation
      {
      public:
        SocketOperation(AsioSocket& socket);
      protected:
        void
        _abort() override;

        void
        _wakeup(const boost::system::error_code& error);

        virtual
        void
        _handle_error(boost::system::error_code const& error);

        ELLE_ATTRIBUTE_R(AsioSocket&, socket);
        ELLE_ATTRIBUTE_R(bool, canceled);
      };


      template <typename AsioSocket>
      class DataOperation
        : public SocketOperation<AsioSocket>
      {
      public:
        using Super = SocketOperation<AsioSocket>;
        DataOperation(AsioSocket& socket);

      protected:
        virtual
        void
        _handle_error(boost::system::error_code const& error) override;
      };
    }
  }
}

#ifdef INFINIT_LINUX
namespace elle
{
  namespace reactor
  {
    namespace network
    {
      /// Register a callback that will be invoked if an exception is caught
      /// in reactor_epoll_wait() from thread 'thread'.
      /// If a callback is set the exception will be intercepted, otherwise it
      /// will propagate.
      /// Call with an empty std::function to remove the callback.
      void
      epoll_interrupt_callback(std::function<void()> cb,
                               reactor::Thread* thread);
    }
  }
}

/// epoll adapters using async ios
extern "C"
{
  /// timeout is in ms.
  int
  reactor_epoll_wait(int epfd, struct epoll_event *events,
                     int maxevents, int timeout);
  /// timeout is in ms.
  int
  reactor_epoll_pwait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout,
                      const sigset_t *sigmask);
}
#endif
