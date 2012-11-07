#ifndef INFINIT_CRYPTOGRAPHY_PRIVATEKEY_HXX
# define INFINIT_CRYPTOGRAPHY_PRIVATEKEY_HXX

# include <elle/Buffer.hh>

# include <elle/idiom/Open.hh>

namespace infinit
{
  namespace cryptography
  {

    template<typename T> elle::Status
      PrivateKey::Decrypt(Code const& in, T& out) const
      {
        elle::Buffer out_buffer;

        if (this->Decrypt(in, out_buffer) == elle::Status::Error)
          escape("Cannot decrypt code");

        try
          {
            out_buffer.reader() >> out;
          }
        catch (std::exception const& err)
          {
            escape("Cannot decrypt the object: %s", err.what());
          }

        return elle::Status::Ok;
      }

    template<typename T>  elle::Status
      PrivateKey::Encrypt(T const& in, Code& out) const
      {
        static_assert(
            !std::is_same<T, elle::Buffer>::value,
            "explicit cast to WeakBuffer needed"
        );

        elle::Buffer buf;

        try
          {
            buf.writer() << in;
          }
        catch (std::exception const& err)
          {
            escape("Cannot save object: %s", err.what());
          }

        return this->Encrypt(
            elle::WeakBuffer(buf),
            out
        );
      }

    template <typename T>
    Signature
    PrivateKey::sign(T const& plain) const
    {
      assert((!std::is_same<T, elle::Buffer>::value));

      elle::Buffer buffer;

      buffer.writer() << plain;

      return (this->sign(elle::WeakBuffer(buffer)));
    }

  }
}

//
// ---------- serialize -------------------------------------------------------
//

# include <elle/serialize/Serializer.hh>

# include <cryptography/cryptography.hh>

ELLE_SERIALIZE_SPLIT(infinit::cryptography::PrivateKey)

ELLE_SERIALIZE_SPLIT_SAVE(infinit::cryptography::PrivateKey,
                          archive,
                          value,
                          version)
{
  enforce(version == 0);

  enforce(value.key() != nullptr);

  archive << *value.key()->pkey.rsa->n;
  archive << *value.key()->pkey.rsa->e;
  archive << *value.key()->pkey.rsa->d;
  archive << *value.key()->pkey.rsa->p;
  archive << *value.key()->pkey.rsa->q;
  archive << *value.key()->pkey.rsa->dmp1;
  archive << *value.key()->pkey.rsa->dmq1;
  archive << *value.key()->pkey.rsa->iqmp;
}

ELLE_SERIALIZE_SPLIT_LOAD(infinit::cryptography::PrivateKey,
                          archive,
                          value,
                          version)
{
  enforce(version == 0);
  struct ScopeGuard
    {
      bool              track;
      elle::Large*            n;
      elle::Large*            e;
      elle::Large*            d;
      elle::Large*            p;
      elle::Large*            q;
      elle::Large*            dmp1;
      elle::Large*            dmq1;
      elle::Large*            iqmp;
      ScopeGuard()
        : track(true)
        , n(::BN_new())
        , e(::BN_new())
        , d(::BN_new())
        , p(::BN_new())
        , q(::BN_new())
        , dmp1(::BN_new())
        , dmq1(::BN_new())
        , iqmp(::BN_new())
      {
        if (!n || !e || !d || !p || !q || !dmp1 || !dmq1 || !iqmp)
          throw std::bad_alloc();
      }

      ~ScopeGuard()
      {
        if (!this->track)
          return;
        ::BN_clear_free(n);
        ::BN_clear_free(e);
        ::BN_clear_free(d);
        ::BN_clear_free(p);
        ::BN_clear_free(q);
        ::BN_clear_free(dmp1);
        ::BN_clear_free(dmq1);
        ::BN_clear_free(iqmp);
      }
    } guard;

  archive >> *guard.n
          >> *guard.e
          >> *guard.d
          >> *guard.p
          >> *guard.q
          >> *guard.dmp1
          >> *guard.dmq1
          >> *guard.iqmp
    ;

  // XXX because the private key is not always cleaned
  // TODO redesign cryptography classes !
  value.~PrivateKey();
  new (&value) infinit::cryptography::PrivateKey();

  enforce(value.key() == nullptr);

  auto res = value.Create(
      guard.n,
      guard.e,
      guard.d,
      guard.p,
      guard.q,
      guard.dmp1,
      guard.dmq1,
      guard.iqmp
  );

  if (res == elle::Status::Error)
    throw std::runtime_error("Could not create the private key !");

  guard.track = false;
}

#endif
