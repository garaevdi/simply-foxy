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
#include <peel/signal.h>

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

  static peel::Signal<ThemeManager, void ()> theme_installed_sig;
  static peel::Signal<ThemeManager, void ()> update_available_sig;

  bool busy;
  bool broken;
  peel::String message;
  peel::String downloaded_sha;
  peel::RefPtr<peel::Gtk::Settings> gtk_settings;

  template <typename F>
  static void
  define_properties (F &f)
  {
    // clang-format off
    f.prop (prop_busy (), false)
      .get (&ThemeManager::get_busy)
      .set (&ThemeManager::set_busy);
    f.prop (prop_broken (), false)
      .get (&ThemeManager::get_broken)
      .set (&ThemeManager::set_broken);
    f.prop (prop_message (), nullptr)
      .get (&ThemeManager::get_message)
      .set (&ThemeManager::set_message);
    f.prop (prop_downloaded_sha (), nullptr)
      .get (&ThemeManager::get_downloaded_sha)
      .set (&ThemeManager::set_downloaded_sha);
    f.prop (prop_gtk_settings ())
      .get (&ThemeManager::get_gtk_settings)
      .set (&ThemeManager::set_gtk_settings);
    // clang-format on
  }

  inline void
  init (Class *);

  peel::coro::Future<peel::String>
  get_latest_hash (peel::UniquePtr<peel::GLib::Error> *error);

  peel::coro::Future<peel::RefPtr<peel::Gio::File>>
  download_archive (peel::UniquePtr<peel::GLib::Error> *error);

  peel::coro::Future<bool>
  extract_archive (peel::RefPtr<peel::Gio::File> file, peel::UniquePtr<peel::GLib::Error> *error);

  peel::coro::Future<void>
  cleanup_data_dir (peel::UniquePtr<peel::GLib::Error> *error);

  peel::coro::Future<peel::RefPtr<peel::Gio::File>>
  find_theme_dir (peel::UniquePtr<peel::GLib::Error> *error);

  peel::coro::Future<void>
  actually_install_theme (
    peel::RefPtr<FirefoxProfile> profile, peel::UniquePtr<peel::GLib::Error> *error
  );

  peel::coro::Future<void>
  actually_uninstall_theme (
    peel::RefPtr<FirefoxProfile> profile, peel::UniquePtr<peel::GLib::Error> *error
  );

  void
  set_busy (bool state)
  {
    if (busy == state)
      return;

    busy = state;
    notify (prop_busy ());
  }

  void
  set_broken (bool new_value)
  {
    if (broken == new_value)
      return;

    broken = new_value;
    notify (prop_broken ());
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

  void
  set_message (const char *new_message)
  {
    if (message)
      if (peel::GLib::str_equal (message.c_str (), new_message))
        return;

    message = new_message;
    notify (prop_message ());
  }

public:
  PEEL_SIGNAL_CONNECT_METHOD (theme_installed, theme_installed_sig);
  PEEL_SIGNAL_CONNECT_METHOD (update_available, update_available_sig);

  bool
  get_busy ()
  {
    return busy;
  }

  bool
  get_broken ()
  {
    return broken;
  }

  const char *
  get_message ()
  {
    return message;
  }

  const char *
  get_downloaded_sha ()
  {
    return downloaded_sha;
  }

  void
  set_gtk_settings (peel::RefPtr<peel::Gtk::Settings> new_gtk_settings)
  {
    gtk_settings = new_gtk_settings;
    notify (prop_gtk_settings ());
  }

  peel::Gtk::Settings *
  get_gtk_settings ()
  {
    return gtk_settings;
  }

  PEEL_PROPERTY (bool, busy, "busy");
  PEEL_PROPERTY (bool, broken, "broken");
  PEEL_PROPERTY (peel::String, message, "message");
  PEEL_PROPERTY (peel::String, downloaded_sha, "downloaded-sha");
  PEEL_PROPERTY (peel::Gtk::Settings, gtk_settings, "gtk-settings");

  peel::coro::SimpleTask
  pull_repo ();

  peel::coro::SimpleTask
  install_theme (peel::RefPtr<FirefoxProfile> profile);

  peel::coro::SimpleTask
  uninstall_theme (peel::RefPtr<FirefoxProfile> profile);

  static peel::RefPtr<ThemeManager>
  create ()
  {
    return peel::GObject::Object::create<ThemeManager> ();
  }
};
} // namespace Sf
