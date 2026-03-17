#pragma once

#include "sf-firefox-manager.hpp"

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
  peel::Gtk::DropDown *layout_dd;
  peel::Gtk::Switch *corners_sw;
  peel::Gtk::Button *uninstall_btn;
  peel::RefPtr<FirefoxManager> firefox_manager;
  template <typename F>
  static void
  define_properties (F &f)
  {
    // clang-format off
    f.prop (prop_firefox_manager ())
      .get (&ApplicationWindow::get_firefox_manager)
      .set (&ApplicationWindow::set_firefox_manager);
    // clang-format on
  }

  inline void
  init (Class *);

  inline void
  vfunc_dispose ();

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

public:
  FirefoxManager *
  get_firefox_manager ()
  {
    return firefox_manager;
  }

  PEEL_PROPERTY (FirefoxManager, firefox_manager, "firefox-manager");

  static ApplicationWindow *
  create (peel::Gtk::Application *app)
  {
    return Object::create<ApplicationWindow> (prop_application (), app);
  }
};
} // namespace Sf
