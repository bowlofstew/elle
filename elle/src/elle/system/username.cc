#include <elle/system/username.hh>

#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <elle/assert.hh>
#include <elle/os/environ.hh>

namespace elle
{
  namespace system
  {
    std::string
    username()
    {
#ifdef INFINIT_WINDOWS
      ELLE_ABORT("implement the windows version, minions!");
#else
      {
        struct passwd* pw = ::getpwuid(::getuid());
        if (pw != nullptr && pw->pw_name != nullptr)
          return pw->pw_name;
      }
      {
        auto login = getlogin();
        if (login)
          return login;
      }
      ELLE_ABORT("could not determine username");
#endif
    }
  }
}