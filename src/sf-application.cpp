#include "sf-application.hpp"

#include "sf-application-window.hpp"

#include <glib/gi18n.h>
#include <locale.h>

using namespace peel;

namespace Sf
{
PEEL_CLASS_IMPL (Application, "SfApplication", Gtk::Application)

inline void
Application::Class::init ()
{
  Granite::init ();
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
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_DOMAIN, DATADIR "/locale");
  bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
  textdomain(GETTEXT_DOMAIN);
  RefPtr<Sf::Application> app = Sf::Application::create ();
  return app->run (argc, argv);
}
