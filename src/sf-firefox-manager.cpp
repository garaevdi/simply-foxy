#include "sf-firefox-manager.hpp"

#include "log.h"
#include "sf-firefox-profile.hpp"

#include <glib/gi18n.h>
#include <string>

using namespace peel;

namespace Sf
{
PEEL_CLASS_IMPL (FirefoxManager, "SfFirefoxManager", peel::GObject::Object)

inline void
FirefoxManager::Class::init ()
{
}

inline void
FirefoxManager::init (Class *)
{
  profiles = Gio::ListStore::create (Type::of<FirefoxProfile> ());

  String home = GLib::getenv ("HOME");

  // clang-format off
  locations = {
    { GLib::build_filename (home, ".mozilla", "firefox"), "Firefox", false },
    { GLib::build_filename (home, ".config", "mozilla", "firefox"), "Firefox", false },
    { GLib::build_filename (home, ".var", "app", "io.gitlab.librewolf-community", ".librewolf"), "Librewolf (flatpak)", true },
    { GLib::build_filename (home, ".var", "app", "org.mozilla.firefox", ".mozilla", "firefox"), "Firefox (flatpak)", true },
    { GLib::build_filename (home, ".var", "app", "org.mozilla.firefox", "config", "mozilla", "firefox"), "Firefox (flatpak)", true },
    { GLib::build_filename (home, "snap", "firefox", "common", ".mozilla", "firefox"), "Firefox (snap)", true },
    { GLib::build_filename (home, ".librewolf"), "Librewolf", false },
  };
  // clang-format on
}

coro::Future<void>
FirefoxManager::find_profiles (Firefox fox, RefPtr<Gio::Cancellable> cancellable)
{
  String path = fox.path;
  String name = fox.name;
  bool sandboxed = fox.sandboxed;

  debug ("Trying %s", path.c_str ());
  RefPtr<Gio::File> file = Gio::File::create_for_path (path);
  if (!file->query_exists (nullptr))
  {
    debug (_ ("No profile found at %s, skipping it"), path.c_str ());
    co_return;
  }

  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> error;

  file->enumerate_children_async (
    G_FILE_ATTRIBUTE_STANDARD_NAME, Gio::File::QueryInfoFlags::NONE, G_PRIORITY_LOW, cancellable,
    async_result.callback ()
  );
  RefPtr<Gio::FileEnumerator> enumerator
    = file->enumerate_children_finish (co_await async_result, &error);
  if (error)
  {
    critical (_ ("Couldn't enumerate files in profile directory: %s"), error->message);
    co_return;
  }

  UniquePtr<GLib::List> files;
  do
  {
    enumerator->next_files_async (5, G_PRIORITY_LOW, cancellable, async_result.callback ());
    files = enumerator->next_files_finish (co_await async_result, &error);
    if (error)
    {
      warning (_ ("Couldn't get files out of enumerator: %s"), error->message);
      continue;
    }
    GLib::List::foreach (
      files,
      [enumerator, &profile_name = name, &path, &sandboxed, profiles = this->profiles] (void *data)
      {
        if (!data)
          return;

        RefPtr<Gio::FileInfo> info = (Gio::FileInfo *)data;
        RefPtr<Gio::File> file = enumerator->get_child (info);
        std::string name = info->get_name ();

        if (name.find ("default") != std::string::npos)
        {
          // Snap firefox stores user settings in a xxxxx.default folder, while other foxes store
          // user settings in a xxxxx.default-xxxxx folder
          RefPtr<Gio::File> prefsjs = file->get_child ("prefs.js");
          if (prefsjs->query_exists (nullptr))
          {
            if (name.find ("esr") != std::string::npos)
            {
              profile_name = GLib::strconcat (profile_name, " ESR");
            }
            else if (name.find ("nightly") != std::string::npos)
            {
              profile_name = GLib::strconcat (profile_name, " Nightly");
            }
            debug (_ ("Found profile %s at %s"), profile_name.c_str (), path.c_str ());
            RefPtr<FirefoxProfile> profile = FirefoxProfile::create (profile_name, file, sandboxed);
            profiles->append (profile);
          }
        }
      }
    );
  } while (files != nullptr);

  enumerator->close_async (G_PRIORITY_LOW, cancellable, async_result.callback ());
  enumerator->close_finish (co_await async_result, &error);
  if (error)
  {
    critical (_ ("Couldn't close enumerator: %s"), error->message);
    co_return;
  }
}

coro::SimpleTask
FirefoxManager::update_profiles (RefPtr<Gio::Cancellable> cancellable)
{
  profiles->remove_all ();

  std::vector<coro::Future<void>> futures;
  futures.reserve (locations.size ());

  for (Firefox fox : locations)
    futures.push_back (find_profiles (fox, cancellable));

  for (auto &future : futures)
    co_await future;

  co_return;
}
} // namespace Sf
