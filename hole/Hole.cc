#include <elle/log.hh>

#include <hole/Hole.hh>
#include <hole/Holeable.hh>
#include <hole/implementations/local/Implementation.hh>
#include <hole/implementations/remote/Implementation.hh>
#include <hole/implementations/slug/Implementation.hh>

#include <nucleus/proton/Address.hh>
#include <nucleus/proton/Network.hh>
#include <nucleus/proton/ImmutableBlock.hh>
#include <nucleus/proton/MutableBlock.hh>

#include <Infinit.hh>

#include <elle/idiom/Close.hh>
# include <boost/format.hpp>
#include <elle/idiom/Open.hh>

ELLE_LOG_COMPONENT("infinit.hole.Hole");

namespace hole
{
  /*----------------.
  | Global instance |
  `----------------*/

  Hole&
  Hole::instance()
  {
    assert(Hole::_instance);
    return *Hole::_instance;
  }

  Hole* Hole::_instance(nullptr);

  /*-------------.
  | Construction |
  `-------------*/

  Hole::Hole()
  {
    assert(!Hole::_instance);

    nucleus::proton::Network network;

    // Disable the meta logging.
    if (elle::radix::Meta::Disable() == elle::Status::Error)
      throw reactor::Exception(elle::concurrency::scheduler(),
                      "unable to disable the meta logging");

    // Retrieve the descriptor.
    {
      if (lune::Descriptor::exists(Infinit::Network) == false)
        throw reactor::Exception(elle::concurrency::scheduler(),
                        "this network does not seem to exist");
      _descriptor.load(Infinit::Network);
      _descriptor.validate(Infinit::Authority);
    }

    // Retrieve the set, if present.
    if (lune::Set::exists(Infinit::Network) == true)
      _set.load(Infinit::Network);

    // Retrieve the passport.
    {
      if (lune::Passport::exists() == false)
        throw reactor::Exception(elle::concurrency::scheduler(),
                        "the device passport does not seem to exist");
      _passport.load();
      if (_passport.Validate(Infinit::Authority) == elle::Status::Error)
        throw reactor::Exception(elle::concurrency::scheduler(),
                        "unable to validate the passport");
    }

    // Enable the meta logging.
    if (elle::radix::Meta::Enable() == elle::Status::Error)
      throw reactor::Exception(elle::concurrency::scheduler(),
                      "unable to enable the meta logging");

    // Create the network instance.
    if (network.Create(Infinit::Network) == elle::Status::Error)
      throw reactor::Exception(elle::concurrency::scheduler(),
                      "unable to create the network instance");

    // create the holeable depending on the model.
    switch (this->descriptor().meta().model().type)
      {
        case Model::TypeLocal:
        {
          this->_implementation =
            new implementations::local::Implementation(network);
          break;
        }
        case Model::TypeRemote:
        {
          this->_implementation =
            new implementations::remote::Implementation(network);
          break;
        }
        case Model::TypeSlug:
        {
          this->_implementation =
            new implementations::slug::Implementation(network);
          break;
        }
        case Model::TypeCirkle:
        {
          /* XXX
          // allocate the instance.
          this->_implementation =
          new implementations::cirkle::Implementation(network);
          */
          elle::abort("Cirkle implementation is disabled for now");
          break;
        }
        default:
        {
          static boost::format fmt("unknown or not-yet-supported model '%u'");
          throw reactor::Exception(elle::concurrency::scheduler(),
                                   str(fmt % Hole::instance().descriptor().meta().model().type));
        }
      }

    Hole::_instance = this;
  }

  Hole::~Hole()
  {
    assert(Hole::_instance);
    this->_implementation->Leave();
    delete this->_implementation;
    Hole::_instance = nullptr;
  }

  /*-------------.
  | Joint, leave |
  `-------------*/

  void
  Hole::join()
  {
    // join the network
    this->_implementation->Join();
  }

  void
  Hole::leave()
  {
    // XXX
  }

  /*------.
  | Ready |
  `------*/

  void
  Hole::ready()
  {
    ELLE_LOG_SCOPE("ready");
    if (this->_state != State::online)
      {
        this->_ready();
        this->_state = Hole::State::online;
      }
  }

  void
  Hole::readyHook(boost::function<void ()> const& f)
  {
    Hole::_ready.connect(f);
  }

  nucleus::proton::Address
  Hole::origin()
  {
    // return the address.
    return Hole::instance().descriptor().meta().root();
  }

  void
  Hole::push(const nucleus::proton::Address& address,
             const nucleus::proton::Block& block)
  {
    ELLE_TRACE_SCOPE("%s(%s, %s)", __FUNCTION__, address, block);
    // XXX check the block's footprint which should not exceed Extent

    // Forward the request depending on the nature of the block which
    // the address indicates.
    switch (address.family)
      {
      case nucleus::proton::FamilyContentHashBlock:
        {
          const nucleus::proton::ImmutableBlock*        ib;
          ib = static_cast<const nucleus::proton::ImmutableBlock*>(&block);
          assert(dynamic_cast<const nucleus::proton::ImmutableBlock*>(
                   &block) != nullptr);
          this->_implementation->Put(address, *ib);
          break;
        }
      case nucleus::proton::FamilyPublicKeyBlock:
      case nucleus::proton::FamilyOwnerKeyBlock:
      case nucleus::proton::FamilyImprintBlock:
        {
          const nucleus::proton::MutableBlock*          mb;
          mb = static_cast<const nucleus::proton::MutableBlock*>(&block);
          assert(dynamic_cast<const nucleus::proton::MutableBlock*>(
                   &block) != nullptr);
          this->_implementation->Put(address, *mb);
          break;
        }
      default:
        {
          throw reactor::Exception(elle::concurrency::scheduler(),
                                   elle::sprintf("unknown block family '%u'",
                                                 address.family));
        }
      }
  }

  std::unique_ptr<nucleus::proton::Block>
  Hole::pull(const nucleus::proton::Address& address,
             const nucleus::proton::Revision& revision)
  {
    ELLE_TRACE_SCOPE("%s(%s, %s)", __FUNCTION__, address, revision);

    // Forward the request depending on the nature of the block which
    // the addres indicates.
    switch (address.family)
      {
        case nucleus::proton::FamilyContentHashBlock:
          return this->_implementation->Get(address);
        case nucleus::proton::FamilyPublicKeyBlock:
        case nucleus::proton::FamilyOwnerKeyBlock:
        case nucleus::proton::FamilyImprintBlock:
          return this->_implementation->Get(address, revision);
        default:
          throw reactor::Exception(elle::concurrency::scheduler(),
                                   elle::sprintf("unknown block family '%u'", address.family));
      }
  }

  void
  Hole::wipe(nucleus::proton::Address const& address)
  {
    ELLE_TRACE_SCOPE("%s(%s)", __FUNCTION__, address);
    this->_implementation->Kill(address);
  }

  /*-----------.
  | Attributes |
  `-----------*/

  nucleus::proton::Network const&
  Hole::network() const
  {
    return _implementation->network;
  }
}
