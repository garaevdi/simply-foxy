#pragma once

#include "sf-firefox-manager.hpp"
#include "sf-theme-manager.hpp"

#include <peel/Gio/Gio.h>
#include <peel/Granite/Granite.h>
#include <peel/Gtk/Gtk.h>
#include <peel/class.h>
#include <peel/widget-template.h>

namespace Sf
{
class ApplicationWindow final : public peel::Gtk::ApplicationWindow
{
  PEEL_SIMPLE_CLASS (ApplicationWindow, peel::Gtk::ApplicationWindow);

  peel::RefPtr<peel::GObject::BindingGroup> binding_group;
  bool binded;

  peel::Gtk::Button *refresh_btn;
  peel::Gtk::DropDown *profile_dd;
  peel::Granite::Toast *update_toast;
  peel::Granite::OverlayBar *overlaybar;
  peel::Granite::Box *content_box;
  peel::Gtk::Switch *corners_sw;
  peel::Gtk::Button *uninstall_btn;
  peel::Gtk::Button *install_btn;

  peel::RefPtr<FirefoxManager> firefox_manager;
  peel::RefPtr<ThemeManager> theme_manager;

  template <typename F>
  static void
  define_properties (F &f)
  {
    // clang-format off
    f.prop (prop_firefox_manager ())
      .get (&ApplicationWindow::get_firefox_manager)
      .set (&ApplicationWindow::set_firefox_manager);
    f.prop (prop_theme_manager ())
      .get (&ApplicationWindow::get_theme_manager)
      .set (&ApplicationWindow::set_theme_manager);
    // clang-format on
  }

  inline void
  init (Class *);

  inline void
  vfunc_dispose ();

  void
  install_btn_clicked_cb (peel::Gtk::Button *);

  void
  uninstall_btn_clicked_cb (peel::Gtk::Button *);

  void
  set_firefox_manager (FirefoxManager *new_manager)
  {
    if (firefox_manager)
    {
      return;
    }

    firefox_manager = new_manager;
    notify (prop_firefox_manager ());
  }

  void
  set_theme_manager (ThemeManager *new_manager)
  {
    if (theme_manager)
    {
      return;
    }

    theme_manager = new_manager;
    notify (prop_theme_manager ());
  }

public:
  FirefoxManager *
  get_firefox_manager ()
  {
    return firefox_manager;
  }

  ThemeManager *
  get_theme_manager ()
  {
    return theme_manager;
  }

  PEEL_PROPERTY (FirefoxManager, firefox_manager, "firefox-manager");
  PEEL_PROPERTY (ThemeManager, theme_manager, "theme-manager");

  static ApplicationWindow *
  create (peel::Gtk::Application *app)
  {
    return Object::create<ApplicationWindow> (prop_application (), app);
  }
};
} // namespace Sf
