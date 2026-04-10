#pragma once

#include <peel/GLib/GLib.h>
#include <peel/GLib/functions.h>
#include <peel/GObject/GObject.h>
#include <peel/Gio/Gio.h>
#include <peel/class.h>
#include <peel/coro/AsyncResult.h>
#include <peel/coro/Future.h>
#include <peel/coro/SimpleTask.h>
#include <vector>

namespace Sf
{
struct Firefox
{
  peel::String path;
  peel::String name;
  bool sandboxed;
};

class FirefoxManager final : public peel::GObject::Object
{
  PEEL_SIMPLE_CLASS (FirefoxManager, peel::GObject::Object);

  std::vector<Firefox> locations;

  peel::RefPtr<peel::Gio::ListStore> profiles;

  template <typename F>
  static void
  define_properties (F &f)
  {
    // clang-format off
    f.prop (prop_profiles ())
      .get (&FirefoxManager::get_profiles)
      .set (&FirefoxManager::set_profiles);
    // clang-format on
  }

  inline void
  set_profiles (peel::Gio::ListStore *new_profiles)
  {
    if (profiles)
    {
      return;
    }

    profiles = new_profiles;
    notify (prop_profiles ());
  }

  inline void
  init (Class *);

  peel::coro::Future<void>
  find_profiles (Firefox fox, peel::RefPtr<peel::Gio::Cancellable> cancellable);

public:
  peel::Gio::ListStore *
  get_profiles ()
  {
    return profiles;
  }

  PEEL_PROPERTY (peel::Gio::ListStore, profiles, "profiles");

  peel::coro::SimpleTask
  update_profiles (peel::RefPtr<peel::Gio::Cancellable> cancellable);

  static peel::RefPtr<FirefoxManager>
  create ()
  {
    return Object::create<FirefoxManager> ();
  }
};
} // namespace Sf
