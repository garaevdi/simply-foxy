#pragma once

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

  inline void
  init (Class *);

  inline void
  vfunc_dispose ();

public:
  static ApplicationWindow *
  create (peel::Gtk::Application *app)
  {
    return Object::create<ApplicationWindow> (prop_application (), app);
  }
};
} // namespace Sf
