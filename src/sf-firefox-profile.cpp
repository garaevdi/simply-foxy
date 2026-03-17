#include "sf-firefox-profile.hpp"

#include "config.h"
#include "log.h"

using namespace peel;

namespace Sf
{
PEEL_CLASS_IMPL (FirefoxProfile, "SfFirefoxProfile", peel::GObject::Object);

inline void
FirefoxProfile::Class::init ()
{
}

inline void
FirefoxProfile::init (Class *)
{
  connect_notify (
    FirefoxProfile::prop_file (),
    [] (Object *obj, peel::GObject::ParamSpec *pspec)
    {
      RefPtr<FirefoxProfile> profile = (FirefoxProfile *)obj;
      String path = GLib::strconcat (APP_PATH, "/", profile->get_profile_basename (), "/");
      debug ("Creating relocatable schema with path: %s", path.c_str ());
      profile->config = Gio::Settings::create_with_path (APP_ID ".profile", path);

      profile->set_theme_sha (profile->config->get_string ("theme-sha"));
      profile->set_layout (profile->config->get_enum ("button-layout"));
      profile->set_rounded_corners (profile->config->get_boolean ("rounded-corners"));
    }
  );
}

void
FirefoxProfile::write_config ()
{
  config->set_string ("theme-sha", get_theme_sha ());
  config->set_enum ("button-layout", get_layout ());
  config->set_boolean ("rounded-corners", get_rounded_corners ());

  notify (prop_has_theme ());
}
} // namespace Sf
