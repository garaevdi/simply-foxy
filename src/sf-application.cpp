#include "sf-application.hpp"

#include "sf-application-window.hpp"

using namespace peel;

namespace Sf
{
PEEL_CLASS_IMPL (Application, "SfApplication", Gtk::Application)

inline void
Application::Class::init ()
{
  Granite::init ();
  Adw::init ();
  override_vfunc_activate<Application> ();
}

inline void
Application::vfunc_activate ()
{
  parent_vfunc_activate<Application> ();
  auto window = get_active_window ();
  if (window == NULL)
  {
    window = ApplicationWindow::create (this);
  }
  window->present ();
}
} // namespace Sf

int
main (int argc, char *argv[])
{
  RefPtr<Sf::Application> app = Sf::Application::create ();
  return app->run (argc, argv);
}
