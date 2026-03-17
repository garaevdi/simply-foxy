#include "sf-application-window.hpp"

#include "config.h"
#include "sf-firefox-profile.hpp"

using namespace peel;

namespace Sf
{
PEEL_CLASS_IMPL (ApplicationWindow, "SfApplicationWindow", Gtk::ApplicationWindow)

inline void
ApplicationWindow::Class::init ()
{
  override_vfunc_dispose<ApplicationWindow> ();
  set_template_from_resource (APP_PATH "/ui/sf-application-window.ui");
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, refresh_btn);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, profile_dd);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, overlaybar);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, layout_dd);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, corners_sw);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, uninstall_btn);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, install_btn);
}

inline void
ApplicationWindow::init (Class *)
{
  theme_manager = ThemeManager::create ();
  firefox_manager = FirefoxManager::create ();

  init_template ();

  // clang-format off
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_busy (),
    overlaybar, Granite::OverlayBar::prop_active ()
  );
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_busy (),
    overlaybar, Granite::OverlayBar::prop_visible ()
  );
  // clang-format on

  refresh_btn->connect_clicked (
    [this] (Gtk::Button *)
    {
      this->firefox_manager->update_profiles ();
      this->theme_manager->pull_repo ();
    }
  );

  profile_dd->set_model (firefox_manager->get_profiles ());
  RefPtr<Gtk::Expression> expression
    = Gtk::PropertyExpression::create (Type::of<FirefoxProfile> (), nullptr, "app-name");
  profile_dd->set_expression (expression);

  binded = false;
  binding_group = peel::GObject::BindingGroup::create ();
  profile_dd->connect_notify (
    Gtk::DropDown::prop_selected_item (),
    [this] (peel::GObject::Object *obj, peel::GObject::ParamSpec *pspec)
    {
      // clang-format off
      binding_group->set_source (
        profile_dd->get_selected_item ()
      );
      // clang-format on
      if (!binded)
      {
        binding_group->bind (
          "has-theme", uninstall_btn, "sensitive", peel::GObject::BindingFlags::DEFAULT
        );
        binding_group->bind (
          "layout", layout_dd, "selected", peel::GObject::BindingFlags::BIDIRECTIONAL
        );
        binding_group->bind (
          "rounded-corners", corners_sw, "active", peel::GObject::BindingFlags::BIDIRECTIONAL
        );
        binded = true;
      }
    }
  );

  firefox_manager->update_profiles ();
  theme_manager->pull_repo ();
}

inline void
ApplicationWindow::vfunc_dispose ()
{
  dispose_template (Type::of<ApplicationWindow> ());
  parent_vfunc_dispose<ApplicationWindow> ();
}
} // namespace Sf
