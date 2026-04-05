#pragma once

#include <peel/GLib/functions.h>
#include <peel/GObject/GObject.h>
#include <peel/Gio/Gio.h>
#include <peel/Gtk/Gtk.h>
#include <peel/class.h>
#include <peel/enum.h>

namespace Sf
{
enum class ButtonLayout
{
  ELEMENTARY = 0,
  ELEMENTARY_REVERSED = 1,
  CLOSE_ONLY_RIGHT = 2,
  CLOSE_ONLY_LEFT = 3,
  ADD_MINIMIZE_LEFT = 4,
  ADD_MINIMIZE_RIGHT = 5,
  REPLACE_MAXIMIZE = 6,
  WINDOWS = 7,
  MACOS = 8,
  UBUNTU = 9,
  UNKNOWN = 10
};
}

PEEL_ENUM (Sf::ButtonLayout)

namespace Sf
{
class FirefoxProfile final : public peel::GObject::Object
{
  PEEL_SIMPLE_CLASS (FirefoxProfile, peel::GObject::Object);

  peel::RefPtr<peel::Gio::Settings> config;

  peel::String app_name;
  bool sandboxed;
  peel::RefPtr<peel::Gio::File> file;
  peel::String theme_sha;
  ButtonLayout layout;
  bool native_titlebar;
  bool rounded_corners;

  template <typename F>
  static void
  define_properties (F &f)
  {
    // clang-format off
    f.prop (prop_app_name (), nullptr)
      .get (&FirefoxProfile::get_app_name)
      .set (&FirefoxProfile::set_app_name);
    f.prop (prop_sandboxed (), false)
      .get (&FirefoxProfile::get_sandboxed)
      .set (&FirefoxProfile::set_sandboxed);
    f.prop (prop_profile_path (), nullptr)
      .get (&FirefoxProfile::get_profile_path);
    f.prop (prop_profile_basename (), nullptr)
      .get (&FirefoxProfile::get_profile_basename);
    f.prop (prop_icon_name (), nullptr)
      .get (&FirefoxProfile::get_icon_name);
    f.prop (prop_file ())
      .get (&FirefoxProfile::get_file)
      .set (&FirefoxProfile::set_file);
    f.prop (prop_has_theme (), false)
      .get (&FirefoxProfile::get_has_theme);
    f.prop (prop_theme_sha (), nullptr)
      .get (&FirefoxProfile::get_theme_sha)
      .set (&FirefoxProfile::set_theme_sha);
    f.prop (prop_layout (), ButtonLayout::ELEMENTARY)
      .get (&FirefoxProfile::get_layout)
      .set (&FirefoxProfile::set_layout);
    f.prop (prop_native_titlebar (), false)
      .get (&FirefoxProfile::get_native_titlebar)
      .set (&FirefoxProfile::set_native_titlebar);
    f.prop (prop_rounded_corners (), false)
      .get (&FirefoxProfile::get_rounded_corners)
      .set (&FirefoxProfile::set_rounded_corners);
    // clang-format on
  }

  void
  set_app_name (const char *new_name)
  {
    if (app_name)
      if (peel::GLib::str_equal (app_name.c_str (), new_name))
        return;

    app_name = new_name;
    notify (prop_app_name ());
  }

  void
  set_sandboxed (bool new_value)
  {
    if (sandboxed == new_value)
      return;

    sandboxed = !sandboxed;
    notify (prop_sandboxed ());
  }

  void
  set_file (peel::Gio::File *new_file)
  {
    if (file)
      return;

    file = new_file;
    notify (prop_file ());
  }

  inline void
  init (Class *);

public:
  const char *
  get_app_name ()
  {
    return app_name;
  }

  bool
  get_sandboxed ()
  {
    return sandboxed;
  }

  const char *
  get_profile_path ()
  {
    if (!file)
    {
      return "";
    }
    return file->get_path ().release_string ();
  }

  const char *
  get_profile_basename ()
  {
    if (!file)
    {
      return "";
    }
    return file->get_basename ().release_string ();
  }

  const char *
  get_icon_name ()
  {
    if (app_name == "")
    {
      return "";
    }
    peel::Strv split_string = peel::GLib::strsplit (app_name, " ", 0);
    // clang-format off
    return peel::GLib::strconcat (peel::GLib::ascii_strdown (split_string[0], sizeof (split_string[0])), "-symbolic").release_string ();
    // clang-format on
  }

  bool
  get_has_theme ()
  {
    if (!file)
      return false;

    peel::RefPtr<peel::Gio::File> chrome = file->get_child ("chrome");
    return chrome->query_exists (nullptr);
  }

  peel::Gio::File *
  get_file ()
  {
    return file;
  }

  void
  set_theme_sha (const char *new_sha)
  {
    if (theme_sha)
      if (peel::GLib::str_equal (theme_sha, new_sha))
        return;

    theme_sha = new_sha;
    notify (prop_theme_sha ());
  }

  const char *
  get_theme_sha ()
  {
    return theme_sha;
  }

  void
  set_layout (ButtonLayout new_layout)
  {
    if (layout == new_layout)
      return;

    layout = new_layout;
    notify (prop_layout ());
  }

  ButtonLayout
  get_layout ()
  {
    return layout;
  }

  const char *
  get_theme_name ()
  {
    if (native_titlebar)
      return "Titlebar Enabled";

    switch (layout)
    {
    case Sf::ButtonLayout::ELEMENTARY:
      return "Elementary";
    case Sf::ButtonLayout::ELEMENTARY_REVERSED:
      return "Elementary Reversed";
    case Sf::ButtonLayout::CLOSE_ONLY_LEFT:
      return "Close Only Left";
    case Sf::ButtonLayout::CLOSE_ONLY_RIGHT:
      return "Close Only Right";
    case Sf::ButtonLayout::ADD_MINIMIZE_LEFT:
      return "Minimize Left";
    case Sf::ButtonLayout::ADD_MINIMIZE_RIGHT:
      return "Minimize Right";
    case Sf::ButtonLayout::REPLACE_MAXIMIZE:
      return "Replace Maximize to Minimize";
    case Sf::ButtonLayout::WINDOWS:
      return "Windows";
    case Sf::ButtonLayout::MACOS:
      return "macOS";
    case Sf::ButtonLayout::UBUNTU:
      return "Ubuntu";
    case Sf::ButtonLayout::UNKNOWN:
      return "Titlebar Enabled";
    }
  }

  void
  set_native_titlebar (bool new_value)
  {
    if (native_titlebar == new_value)
      return;

    native_titlebar = !native_titlebar;
    notify (prop_native_titlebar ());
  }

  bool
  get_native_titlebar ()
  {
    return native_titlebar;
  }

  void
  set_rounded_corners (bool new_corners)
  {
    if (rounded_corners == new_corners)
      return;

    rounded_corners = !rounded_corners;
    notify (prop_rounded_corners ());
  }

  bool
  get_rounded_corners ()
  {
    return rounded_corners;
  }

  PEEL_PROPERTY (peel::String, app_name, "app-name");
  PEEL_PROPERTY (bool, sandboxed, "sandboxed");
  PEEL_PROPERTY (peel::String, profile_path, "profile-path");
  PEEL_PROPERTY (peel::String, profile_basename, "profile-basename");
  PEEL_PROPERTY (peel::String, icon_name, "icon-name");
  PEEL_PROPERTY (bool, has_theme, "has-theme");
  PEEL_PROPERTY (peel::Gio::File, file, "file");
  PEEL_PROPERTY (peel::String, theme_sha, "theme-sha");
  PEEL_PROPERTY (ButtonLayout, layout, "layout");
  PEEL_PROPERTY (peel::String, theme_name, "theme_name");
  PEEL_PROPERTY (bool, native_titlebar, "native-titlebar");
  PEEL_PROPERTY (bool, rounded_corners, "rounded-corners");

  void
  write_config ();

  static peel::RefPtr<FirefoxProfile>
  create (const char *name, peel::RefPtr<peel::Gio::File> file, bool sandboxed)
  {
    return peel::GObject::Object::create<FirefoxProfile> (
      prop_app_name (), name, prop_file (), std::move (file), prop_sandboxed (), sandboxed
    );
  }
};
} // namespace Sf
