//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       infinit
//
// author        julien quintard   [tue feb 23 13:44:55 2010]
//

#ifndef ELLE_NETWORK_LOCALSOCKET_HXX
#define ELLE_NETWORK_LOCALSOCKET_HXX

//
// ---------- includes --------------------------------------------------------
//

#include <elle/standalone/Maid.hh>
#include <elle/standalone/Report.hh>

#include <elle/utility/BufferStream.hh>

#include <elle/serialize/BufferArchive.hh>
#include <elle/serialize/BaseArchive.hxx>

#include <elle/network/Packet.hh>
#include <elle/network/Header.hh>
#include <elle/network/Data.hh>
#include <elle/network/Parcel.hh>

#include <elle/Manifest.hh>

namespace elle
{
  namespace network
  {

//
// ---------- methods ---------------------------------------------------------
//

    ///
    /// this method sends a packet.
    ///
    template <typename I>
    Status              LocalSocket::Send(const I               inputs,
                                          const Event&          event)
    {
      Packet            packet;
      Data              data;
      Header            header;

      // serialize the inputs.
      if (inputs.Serialize(data) == Status::Error)
        escape("unable to serialize the inputs");

      // create the header now that we know that final archive's size.
      if (header.Create(event,
                        inputs.tag,
                        data.Size()) == Status::Error)
        escape("unable to create the header");

      // serialize the the header and data.
      elle::utility::OutputBufferStream os(packet);
      elle::serialize::OutputBufferArchive ar(os);

      ar << header;
      ar << data;

      // write the socket.
      if (this->Write(packet) == Status::Error)
        escape("unable to write the packet");

      return Status::Ok;
    }

    ///
    /// this method receives a packet through blocking.
    ///
    template <typename O>
    Status              LocalSocket::Receive(const Event&       event,
                                             O                  outputs)
    {
      std::shared_ptr<Parcel> parcel;

      // block the current fiber until the given event is received.
      if (Fiber::Wait(event, parcel) == Status::Error)
        escape("an error occured while waiting for a specific event");

      // check the tag.
      if (parcel->header->tag != outputs.tag)
        {
          //
          // in this case, the blocked fiber received a message whose
          // tag does not match the expected one.
          //
          Tag           tag = parcel->header->tag;

          // first, test if the message received is an error, if so, append
          // the report to the local one.
          if (tag == TagError)
            {
              Report    report;

              // extract the error message.
              if (report.Extract(*parcel->data) == Status::Error)
                escape("unable to extract the error message");

              // report the remote error.
              transpose(report);
            }
          else
            {
              // otherwise, try to ship the parcel since a handler may be
              // able to treat it.
              if (Socket::Ship(parcel) == Status::Error)
                log("an error occured while shipping the parcel");
            }

          // in any case, return an error from the Receive() method.
          escape("received a packet with an unexpected tag '%u'", tag);
        }

      // extract the arguments.
      if (outputs.Extract(*parcel->data) == Status::Error)
        escape("unable to extract the arguments");

      return Status::Ok;
    }

    ///
    /// this method sends a message and waits for a response.
    ///
    template <typename I,
              typename O>
    Status              LocalSocket::Call(const I               inputs,
                                          O                     outputs)
    {
      Event             event;

      // generate an event to link the request with the response.
      if (event.Generate() == Status::Error)
        escape("unable to generate the event");

      // send the inputs.
      if (this->Send(inputs, event) == Status::Error)
        escape("unable to send the inputs");

      // wait for the reply.
      if (this->Receive(event, outputs) == Status::Error)
        escape("unable to receive the outputs");

      return Status::Ok;
    }

    ///
    /// this method replies to the message which has just been received i.e
    /// whose tag is specified in the current session.
    ///
    template <typename I>
    Status              LocalSocket::Reply(const I              inputs,
                                           Session*             session)
    {
      // retrieve the current session, if necessary.
      if (session == NULL)
        {
          if (Session::Instance(session) == Status::Error)
            escape("unable to retrieve the session instance");
        }

      // send a message as a response by using the event of
      // the received message i.e the current session.
      if (this->Send(inputs, session->event) == Status::Error)
        escape("unable to send the reply");

      return Status::Ok;
    }

  }
}

#endif
