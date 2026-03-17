#pragma once

#include "log.h"
#include "sf-firefox-profile.hpp"

#include <peel/GLib/functions.h>
#include <peel/GObject/GObject.h>
#include <peel/Gio/Gio.h>
#include <peel/Json/Json.h>
#include <peel/Soup/Soup.h>
#include <peel/class.h>
#include <peel/coro/AsyncResult.h>
#include <peel/coro/Future.h>
#include <peel/coro/SimpleTask.h>

namespace Sf
{
class ThemeManager final : public peel::GObject::Object
{
  PEEL_SIMPLE_CLASS (ThemeManager, peel::GObject::Object);

  peel::RefPtr<peel::Soup::Session> session;
  peel::RefPtr<peel::Gio::File> data_dir;
  peel::RefPtr<peel::Gio::File> extract_dir;
  peel::RefPtr<peel::Gio::File> theme_dir;
  peel::RefPtr<peel::Gio::Settings> config;

  bool busy;
  peel::String downloaded_sha;

  template <typename F>
  static void
  define_properties (F &f)
  {
    // clang-format off
    f.prop (prop_busy (), false)
      .get (&ThemeManager::get_busy)
      .set (&ThemeManager::set_busy);
    f.prop (prop_downloaded_sha (), nullptr)
      .get (&ThemeManager::get_downloaded_sha)
      .set (&ThemeManager::set_downloaded_sha);
    // clang-format on
  }

  inline void
  init (Class *);

  peel::coro::Future<void>
  get_latest_hash ();

  peel::coro::Future<void>
  download_archive ();

  void
  extract_archive (peel::RefPtr<peel::Gio::File>);

  peel::coro::Future<void>
  cleanup_data_dir ();

  peel::coro::Future<bool>
  find_theme_dir ();

  void
  set_busy (bool state)
  {
    if (busy == state)
      return;

    busy = state;
    debug ("Is working: %s", state ? "true" : "false");
    notify (prop_busy ());
  }

  void
  set_downloaded_sha (const char *new_sha)
  {
    if (downloaded_sha)
      if (peel::GLib::str_equal (downloaded_sha.c_str (), new_sha))
        return;

    downloaded_sha = new_sha;
    notify (prop_downloaded_sha ());
  }

public:
  bool
  get_busy ()
  {
    return busy;
  }

  const char *
  get_downloaded_sha ()
  {
    return downloaded_sha;
  }

  PEEL_PROPERTY (bool, busy, "busy");
  PEEL_PROPERTY (peel::String, downloaded_sha, "downloaded-sha");

  peel::coro::SimpleTask
  pull_repo ();

  peel::coro::SimpleTask
  install_theme (peel::RefPtr<FirefoxProfile>);

  peel::coro::SimpleTask
  uninstall_theme (peel::RefPtr<FirefoxProfile>);

  static peel::RefPtr<ThemeManager>
  create ()
  {
    return peel::GObject::Object::create<ThemeManager> ();
  }
};
} // namespace Sf
