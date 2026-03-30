#include "sf-firefox-profile.hpp"

#include "config.h"
#include "log.h"

#include <glib/gi18n.h>

using namespace peel;

// clang-format off
PEEL_ENUM_IMPL (Sf::ButtonLayout, "SfButtonLayout",
  PEEL_ENUM_VALUE (Sf::ButtonLayout::ELEMENTARY, "Elementary"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::ELEMENTARY_REVERSED, "Elementary Reversed"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::CLOSE_ONLY_RIGHT, "Close Only Right"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::CLOSE_ONLY_LEFT, "Close Only Left"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::ADD_MINIMIZE_LEFT, "Minimize Left"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::ADD_MINIMIZE_RIGHT, "Minimize Right"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::REPLACE_MAXIMIZE, "Replace Maximize to Minimize"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::WINDOWS, "Windows"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::MACOS, "macOS"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::UBUNTU, "Ubuntu"),
  PEEL_ENUM_VALUE (Sf::ButtonLayout::UNKNOWN, "Titlebar Enabled")
);
// clang-format on

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
      debug (_("Creating relocatable schema with path: %s"), path.c_str ());
      profile->config = Gio::Settings::create_with_path (APP_ID ".profile", path);

      profile->set_theme_sha (profile->config->get_string ("theme-sha"));
      profile->set_layout ((ButtonLayout)profile->config->get_int ("button-layout"));
      profile->set_rounded_corners (profile->config->get_boolean ("rounded-corners"));
    }
  );
}

void
FirefoxProfile::write_config ()
{
  config->set_string ("theme-sha", get_theme_sha ());
  config->set_int ("button-layout", static_cast<int> (get_layout ()));
  config->set_boolean ("rounded-corners", get_rounded_corners ());

  notify (prop_has_theme ());
}
} // namespace Sf
