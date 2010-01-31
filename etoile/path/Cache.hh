//
// ---------- header ----------------------------------------------------------
//
// project       etoile
//
// license       infinit (c)
//
// file          /home/mycure/infinit/etoile/path/Cache.hh
//
// created       julien quintard   [fri aug  7 19:39:51 2009]
// updated       julien quintard   [sat jan 30 03:25:16 2010]
//

#ifndef ETOILE_PATH_CACHE_HH
#define ETOILE_PATH_CACHE_HH

//
// ---------- includes --------------------------------------------------------
//

#include <elle/Elle.hh>

#include <etoile/configuration/Configuration.hh>

#include <etoile/path/Item.hh>
#include <etoile/path/Route.hh>
#include <etoile/path/Venue.hh>

#include <list>

namespace etoile
{
  using namespace configuration;

  namespace path
  {

//
// ---------- classes ---------------------------------------------------------
//

    ///
    /// this cache relies on the LRU algorithm by keeping two data structures:
    /// a map for looking up and a queue for removing the least recently used
    /// item quickly.
    ///
    /// to avoid using too much memory for these data structures, both point
    /// to the same data: items.
    ///
    /// noteworthy is that, since this cache is used for paths and that paths
    /// follow a pattern where /music/meshuggah is a subset of /music, the
    /// data structure for storing the paths is hierachical.
    ///
    /// indeed, the Item class keeps a directory name such as 'meshuggah' along
    /// with a map of all the child entries.
    ///
    /// this design has been chosen to speed up the resolution process. indeed,
    /// this cache is used when a path must be resolved into an object address.
    /// the objective of the cache is to find the longest part of a given path.
    ///
    /// for example, given /music/meshuggah/nothing/, the objective is to
    /// find the corresponding address of this directory object. instead of
    /// trying /music/meshuggah/nothing/, then /music/meshuggah/, then
    /// /music/ etc. the designed cache is capable of returning the longest
    /// match within a single pass because items are hierarchically organised.
    ///
    class Cache
    {
    public:
      //
      // constants
      //
      static Natural32&		Capacity;

      //
      // types
      //
      typedef std::list<String>		Container;
      typedef Container::iterator	Iterator;
      typedef Container::const_iterator	Scoutor;

      //
      // static methods
      //
      static Status	Initialize();
      static Status	Clean();

      static Status	Update(const Route&,
			       const Venue&);
      static Status	Resolve(const Route&,
				Venue&);
      static Status	Evict();

      static Status	Dump(const Natural32 = 0);

      //
      // static attributes
      //
      static Item		Data;
      static Container		Queue;

      static Natural32		Size;
    };

  }
}

#endif
