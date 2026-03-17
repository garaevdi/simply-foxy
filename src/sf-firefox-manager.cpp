#include "sf-firefox-manager.hpp"

#include "config.h"
#include "sf-firefox-profile.hpp"

#include <string>

using namespace peel;

namespace Sf
{
PEEL_CLASS_IMPL (FirefoxManager, "SfFirefoxManager", peel::GObject::Object)

inline void
FirefoxManager::Class::init ()
{
}

int
str_equal (const void *v1, const void *v2)
{
  const char *string1 = (char *)v1;
  const char *string2 = (char *)v2;
  return strcmp (string1, string2);
}

inline void
FirefoxManager::init (Class *)
{
  profiles = Gio::ListStore::create (Type::of<FirefoxProfile> ());

  String home = GLib::getenv ("HOME");

  // clang-format off
  locations = {
    { GLib::strconcat (home, "/.mozilla/firefox"), "Firefox", false },
    { GLib::strconcat (home, "/.config/mozilla/firefox"), "Firefox", false },
    { GLib::strconcat (home, "/.var/app/org.mozilla.firefox/.mozilla/firefox"), "Firefox (flatpak)", true },
    { GLib::strconcat (home, "/.var/app/org.mozilla.firefox/config/mozilla/firefox/"), "Firefox (flatpak)", true },
    { GLib::strconcat (home, "/snap/firefox/common/.mozilla/firefox"), "Firefox (snap)", true },
    { GLib::strconcat (home, "/.librewolf"), "Librewolf", false },
    { GLib::strconcat (home, "/.var/app/io.gitlab.librewolf-community/.librewolf"), "Librewolf (flatpak)", true },
  };
  // clang-format on
}

// Super function that idk how to split up :(
void
FirefoxManager::update_profiles ()
{
  profiles->remove_all ();
  for (Firefox fox : locations)
  {
    String path = fox.location;
    String name = fox.name;
    bool sandboxed = fox.sandboxed;
    RefPtr<Gio::File> file = Gio::File::create_for_path (path);
    if (!file->query_exists (nullptr))
    {
      GLib::log (APP_ID, peel::GLib::LogLevelFlags::LEVEL_DEBUG, "No such file %s", path.c_str ());
      continue;
    }
    file->enumerate_children_async (
      G_FILE_ATTRIBUTE_STANDARD_NAME, Gio::File::QueryInfoFlags::NONE, G_PRIORITY_DEFAULT, nullptr,
      // clang-format off
      [name, sandboxed, profiles = this->profiles]
      (peel::GObject::Object *source, Gio::AsyncResult *res)
      // clang-format on
      {
        String profile_name = name;
        UniquePtr<GLib::Error> err;
        RefPtr<Gio::FileEnumerator> enumerator
          = source->cast<Gio::File> ()->enumerate_children_finish (res, &err);
        if (err)
        {
          GLib::log (
            APP_ID, peel::GLib::LogLevelFlags::LEVEL_CRITICAL, "Couldn't load profiles: %s",
            err->message
          );
          return;
        }

        while (true)
        {
          UniquePtr<GLib::Error> err;
          RefPtr<Gio::FileInfo> info = enumerator->next_file (nullptr, &err);
          if (!info)
          {
            break;
          }

          RefPtr<Gio::File> file = enumerator->get_child (info);
          if (err)
          {
            GLib::log (
              APP_ID, peel::GLib::LogLevelFlags::LEVEL_CRITICAL, "Couldn't get fileinfo: %s",
              err->message
            );
            continue;
          }

          std::string name = info->get_name ();
          if (name.find ("default-") != std::string::npos)
          {
            if (name.find ("esr") != std::string::npos)
            {
              profile_name = GLib::strconcat (profile_name, " ESR");
            }
            else if (name.find ("nightly") != std::string::npos)
            {
              profile_name = GLib::strconcat (profile_name, " Nightly");
            }
            RefPtr<FirefoxProfile> profile = FirefoxProfile::create (profile_name, file, sandboxed);
            profiles->append (profile);
          }
        }
      }
    );
  }
  profiles->sort (
    [] (const void *a, const void *b)
    {
      RefPtr<FirefoxProfile> left = (FirefoxProfile *)a;
      RefPtr<FirefoxProfile> right = (FirefoxProfile *)b;
      UniquePtr<GLib::Error> err;
      RefPtr<Gio::FileInfo> left_info = left->get_file ()->query_info (
        G_FILE_ATTRIBUTE_STANDARD_NAME, Gio::File::QueryInfoFlags::NONE, nullptr, &err
      );
      RefPtr<Gio::FileInfo> right_info = right->get_file ()->query_info (
        G_FILE_ATTRIBUTE_STANDARD_NAME, Gio::File::QueryInfoFlags::NONE, nullptr, &err
      );
      RefPtr<GLib::DateTime> left_edit_dt = left_info->get_modification_date_time ();
      RefPtr<GLib::DateTime> right_edit_dt = right_info->get_modification_date_time ();
      return left_edit_dt->compare (right_edit_dt);
    }
  );
}
} // namespace Sf
