#include "sf-application-window.hpp"

#include "config.h"

using namespace peel;

namespace Sf
{
PEEL_CLASS_IMPL (ApplicationWindow, "SfApplicationWindow", Gtk::ApplicationWindow)

inline void
ApplicationWindow::Class::init ()
{
  override_vfunc_dispose<ApplicationWindow> ();
  set_template_from_resource (APP_PATH "/ui/sf-application-window.ui");
}

inline void
ApplicationWindow::init (Class *)
{
  init_template ();
}

inline void
ApplicationWindow::vfunc_dispose ()
{
  dispose_template (Type::of<ApplicationWindow> ());
  parent_vfunc_dispose<ApplicationWindow> ();
}
} // namespace Sf
