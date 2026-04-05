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
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, update_toast);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, overlaybar);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, content_box);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, titlebar_sw);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, corners_sw);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, uninstall_btn);
  PEEL_WIDGET_TEMPLATE_BIND_CHILD (ApplicationWindow, install_btn);
  PEEL_WIDGET_TEMPLATE_BIND_CALLBACK (ApplicationWindow, uninstall_btn_clicked_cb);
  PEEL_WIDGET_TEMPLATE_BIND_CALLBACK (ApplicationWindow, install_btn_clicked_cb);
}

inline void
ApplicationWindow::init (Class *)
{
  theme_manager = ThemeManager::create ();
  theme_manager->set_gtk_settings (get_settings ());
  firefox_manager = FirefoxManager::create ();

  init_template ();
  RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create ();
  css_provider->load_from_resource (APP_PATH "/style.css");
  Gtk::StyleContext::add_provider_for_display (
    get_display (), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );

  // clang-format off
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_busy (),
    content_box, Gtk::Widget::prop_sensitive (),
    peel::GObject::BindingFlags::INVERT_BOOLEAN
  );
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_broken (),
    install_btn, Gtk::Widget::prop_sensitive (),
    peel::GObject::BindingFlags::INVERT_BOOLEAN
  );
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_broken (),
    uninstall_btn, Gtk::Widget::prop_sensitive (),
    peel::GObject::BindingFlags::INVERT_BOOLEAN
  );
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_busy (),
    overlaybar, Granite::OverlayBar::prop_active ()
  );
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_busy (),
    overlaybar, Granite::OverlayBar::prop_visible ()
  );
  peel::GObject::Object::bind_property (
    theme_manager, ThemeManager::prop_message (),
    overlaybar, Granite::OverlayBar::prop_label ()
  );
  // clang-format on

  connect_notify (
    ApplicationWindow::prop_display (),
    [] (peel::GObject::Object *source, peel::GObject::ParamSpec *pspec)
    {
      FloatPtr<ApplicationWindow> window = (ApplicationWindow *)source;
      window->theme_manager->set_gtk_settings (window->get_settings ());
    }
  );
  theme_manager->connect_update_available ([this] (peel::GObject::Object *source)
                                           { update_toast->send_notification (); });

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
          "rounded-corners", corners_sw, "active", peel::GObject::BindingFlags::BIDIRECTIONAL
        );
        binding_group->bind (
          "native-titlebar", titlebar_sw, "active", peel::GObject::BindingFlags::BIDIRECTIONAL
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

void
ApplicationWindow::install_btn_clicked_cb (Gtk::Button *btn)
{
  RefPtr<FirefoxProfile> profile = profile_dd->get_selected_item ()->cast<FirefoxProfile> ();
  if (profile)
    theme_manager->install_theme (profile);
}

void
ApplicationWindow::uninstall_btn_clicked_cb (Gtk::Button *btn)
{
  RefPtr<FirefoxProfile> profile = profile_dd->get_selected_item ()->cast<FirefoxProfile> ();
  if (profile)
    theme_manager->uninstall_theme (profile);
}
} // namespace Sf
