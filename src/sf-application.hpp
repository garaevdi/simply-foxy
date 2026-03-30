#pragma once

#include "config.h"

#include <peel/Adw/Adw.h>
#include <peel/Gio/Gio.h>
#include <peel/Granite/Granite.h>
#include <peel/Gtk/Gtk.h>
#include <peel/class.h>

namespace Sf
{
class Application final : public peel::Gtk::Application
{
  PEEL_SIMPLE_CLASS (Application, peel::Gtk::Application);
  friend class peel::Gio::Application;

  inline void
  vfunc_activate ();

public:
  static peel::RefPtr<Application>
  create ()
  {
    return peel::Object::create<Application> (
      prop_application_id (), APP_ID, prop_flags (), peel::Gio::Application::Flags::DEFAULT_FLAGS
    );
  }
};
} // namespace Sf
