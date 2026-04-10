#include "sf-theme-manager.hpp"

#include "config.h"

#include <glib/gi18n.h>
#include <string>

using namespace peel;

namespace Sf
{
GLib::Quark
theme_manager_error_quark ()
{
  return "sf-theme-manager-error-quark";
}

// clang-format off
Signal<ThemeManager, void ()> ThemeManager::theme_installed_sig;
Signal<ThemeManager, void ()> ThemeManager::theme_uninstalled_sig;
Signal<ThemeManager, void ()> ThemeManager::update_available_sig;
Signal<ThemeManager, void (const char *, const char *, const char *)> ThemeManager::show_warning_sig;
// clang-format on

PEEL_CLASS_IMPL (ThemeManager, "SfThemeManager", peel::GObject::Object);

inline void
ThemeManager::Class::init ()
{
}

inline void
ThemeManager::init (Class *)
{
  // clang-format off
  theme_installed_sig = Signal<ThemeManager, void ()>::create ("theme-installed");
  theme_uninstalled_sig = Signal<ThemeManager, void ()>::create ("theme-uninstalled");
  update_available_sig = Signal<ThemeManager, void ()>::create ("update-available");
  show_warning_sig = Signal<ThemeManager, void (const char *, const char *, const char *)>::create ("show-warning");
  // clang-format on

  session = Soup::Session::create ();
  session->set_timeout (10);
  busy = false;
  data_dir
    = Gio::File::create_for_path (GLib::strconcat (GLib::get_user_data_dir (), "/", APP_NAME));
  if (!data_dir->query_exists (nullptr))
  {
    UniquePtr<GLib::Error> error;
    data_dir->make_directory (nullptr, &error);
    if (error) [[unlikely]]
    {
      critical (_ ("Couldn't create data folder: %s"), error->message);
      set_broken (true);
      return;
    }
  }
  extract_dir = data_dir->get_child ("themes");
  config = Gio::Settings::create_with_path (APP_ID ".theme_manager", APP_PATH "/theme_manager/");
  config->bind ("downloaded-sha", this, "downloaded-sha", Gio::Settings::BindFlags::DEFAULT);
}

coro::Future<String>
ThemeManager::get_latest_hash (UniquePtr<GLib::Error> *error, RefPtr<Gio::Cancellable> cancellable)
{
  debug ("%s", _ ("Trying to get latest commit hash"));
  set_message (_ ("Checking for updates..."));
  RefPtr<Soup::Message> message = Soup::Message::create (
    "GET", "https://api.github.com/repos/Zonnev/elementaryos-firefox-theme/commits?per_page=1"
  );
  RefPtr<Soup::MessageHeaders> headers = message->get_request_headers ();
  headers->append ("Accept", "application/vnd.github+json");
  headers->append ("User-Agent", APP_NAME "/" VERSION);
  headers->append ("X-GitHub-Api-Version", "2026-03-10");

  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> internal_error;

  session->send_async (message, G_PRIORITY_LOW, cancellable, async_result.callback ());
  RefPtr<Gio::InputStream> stream = session->send_finish (co_await async_result, &internal_error);
  if (internal_error)
  {
    GLib::propagate_error (error, std::move (internal_error));
    co_return nullptr;
  }

  if (message->get_status () != Soup::Status::OK)
  {
    GLib::set_error (
      error, SF_THEME_MANAGER_ERROR, (int)ThemeManagerError::WRONG_RETURN_STATUS_CODE,
      _ ("Wrong status code : %d %s"), message->get_status (), message->get_reason_phrase ()
    );
    co_return nullptr;
  }

  RefPtr<Json::Parser> pareser = Json::Parser::create ();
  pareser->load_from_stream_async (stream, cancellable, async_result.callback ());
  if (!(pareser->load_from_stream_finish (co_await async_result, error)))
    co_return nullptr;

  RefPtr<Json::Node> root = pareser->get_root ();
  RefPtr<Json::Node> commit = root->get_array ()->get_element (0);
  String hash = commit->get_object ()->get_string_member ("sha");

  debug (_ ("Got latest commit hash: %s"), hash.c_str ());
  co_return hash;
}

coro::Future<RefPtr<Gio::File>>
ThemeManager::download_archive (UniquePtr<GLib::Error> *error, RefPtr<Gio::Cancellable> cancellable)
{
  set_message (_ ("Downloading theme..."));
  // clang-format off
  RefPtr<Soup::Message> message = Soup::Message::create (
    "GET", "https://api.github.com/repos/Zonnev/elementaryos-firefox-theme/zipball/elementaryos-firefox-theme"
  );
  // clang-format on
  RefPtr<Soup::MessageHeaders> headers = message->get_request_headers ();
  headers->append ("Accept", "application/vnd.github+json");
  headers->append ("User-Agent", APP_NAME "/" VERSION);
  headers->append ("X-GitHub-Api-Version", "2026-03-10");

  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> internal_error;

  debug ("%s", _ ("Sending GET request to github..."));
  session->send_and_read_async (message, G_PRIORITY_DEFAULT, cancellable, async_result.callback ());
  RefPtr<GLib::Bytes> bytes
    = session->send_and_read_finish (co_await async_result, &internal_error);
  if (internal_error)
  {
    GLib::propagate_error (error, std::move (internal_error));
    co_return nullptr;
  }

  if (message->get_status () != Soup::Status::OK)
  {
    GLib::set_error (
      error, SF_THEME_MANAGER_ERROR, (int)ThemeManagerError::WRONG_RETURN_STATUS_CODE,
      _ ("Wrong status code : %d %s"), message->get_status (), message->get_reason_phrase ()
    );
    co_return nullptr;
  }

  debug (_ ("Read %zu bytes"), bytes->get_size ());

  RefPtr<Gio::File> archive = data_dir->get_child ("elementary-firefox-theme.zip");
  if (archive->query_exists (nullptr))
  {
    debug ("%s", _ ("Old archive already exists, trying to overwrite it..."));
    archive->replace_contents_bytes_async (
      bytes, nullptr, false, Gio::File::CreateFlags::NONE, cancellable, async_result.callback ()
    );
    archive->replace_contents_finish (co_await async_result, nullptr, &internal_error);
    if (internal_error)
    {
      GLib::propagate_error (error, std::move (internal_error));
      co_return nullptr;
    }

    debug ("%s", _ ("Archive overwritten"));
  }
  else
  {
    debug ("%s", _ ("Creating new file for archive..."));
    archive->create_async (
      Gio::File::CreateFlags::NONE, G_PRIORITY_DEFAULT, cancellable, async_result.callback ()
    );
    RefPtr<Gio::FileOutputStream> stream
      = archive->create_finish (co_await async_result, &internal_error);
    if (internal_error)
    {
      GLib::propagate_error (error, std::move (internal_error));
      co_return nullptr;
    }

    debug ("%s", _ ("File created, trying to write data into it..."));

    stream->write_bytes_async (bytes, G_PRIORITY_DEFAULT, cancellable, async_result.callback ());
    stream->write_bytes_finish (co_await async_result, &internal_error);
    if (internal_error)
    {
      GLib::propagate_error (error, std::move (internal_error));
      co_return nullptr;
    }

    debug ("%s", _ ("Data written, archive ready!"));
  }
  co_return archive;
}

coro::Future<bool>
ThemeManager::extract_archive (
  RefPtr<Gio::File> archive, UniquePtr<GLib::Error> *error, RefPtr<Gio::Cancellable> cancellable
)
{
  set_message (_ ("Unpacking theme..."));

  coro::AsyncResult async_result;

  const char *cmd[]{ "unzip", "-d", extract_dir->peek_path (), "-o", archive->peek_path (), nullptr };
  debug (_ ("Unzippign archive with the following cmd: %s"), GLib::strjoinv (" ", cmd).c_str ());
  RefPtr<Gio::Subprocess> subprocess
    = Gio::Subprocess::createv (cmd, Gio::Subprocess::Flags::STDOUT_SILENCE, error);
  if (!subprocess) [[unlikely]]
    co_return false;

  subprocess->wait_check_async (cancellable, async_result.callback ());
  bool success = subprocess->wait_check_finish (co_await async_result, error);

  co_return success;
}

coro::Future<void>
ThemeManager::cleanup_data_dir (UniquePtr<GLib::Error> *error, RefPtr<Gio::Cancellable> cancellable)
{
  set_message (_ ("Cleaning old files..."));
  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> internal_error;

  debug ("%s", _ ("Starting data directory cleanup..."));
  if (extract_dir->query_exists (nullptr))
  {
    extract_dir->trash_async (G_PRIORITY_LOW, cancellable, async_result.callback ());
    extract_dir->trash_finish (co_await async_result, &internal_error);
  }
  if (internal_error)
    GLib::propagate_error (error, std::move (internal_error));

  debug ("%s", _ ("Cleanup finished"));
  co_return;
}

coro::Future<RefPtr<Gio::File>>
ThemeManager::find_theme_dir (UniquePtr<GLib::Error> *error, RefPtr<Gio::Cancellable> cancellable)
{
  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> internal_error;

  if (!(extract_dir->query_exists (nullptr)))
  {
    GLib::set_error_literal (
      error, SF_THEME_MANAGER_ERROR, (int)ThemeManagerError::NO_EXTRACT_DIRECTORY,
      _ ("No extract directory")
    );
    co_return nullptr;
  }
  extract_dir->enumerate_children_async (
    G_FILE_ATTRIBUTE_STANDARD_NAME, Gio::File::QueryInfoFlags::NONE, G_PRIORITY_LOW,
    cancellable, async_result.callback ()
  );
  RefPtr<Gio::FileEnumerator> enumerator
    = extract_dir->enumerate_children_finish (co_await async_result, &internal_error);
  if (internal_error)
  {
    GLib::propagate_error (error, std::move (internal_error));
    co_return nullptr;
  }

  enumerator->next_files_async (10, G_PRIORITY_LOW, cancellable, async_result.callback ());
  UniquePtr<GLib::List> files
    = enumerator->next_files_finish (co_await async_result, &internal_error);
  if (internal_error)
  {
    GLib::propagate_error (error, std::move (internal_error));
    co_return nullptr;
  }

  RefPtr<Gio::File> possible_file = nullptr;
  if (files)
  {
    GLib::List::foreach (
      files,
      [&possible_file, enumerator] (void *data)
      {
        RefPtr<Gio::FileInfo> info = (Gio::FileInfo *)data;
        RefPtr<Gio::File> file = enumerator->get_child (info);
        std::string name = info->get_name ();
        if (name.find ("Zonnev-elementaryos-firefox-theme-") != std::string::npos)
          possible_file = file;
      }
    );
  }

  enumerator->close_async (G_PRIORITY_LOW, cancellable, async_result.callback ());
  enumerator->close_finish (co_await async_result, &internal_error);

  if (!possible_file)
  {
    GLib::set_error_literal (
      error, SF_THEME_MANAGER_ERROR, (int)ThemeManagerError::NO_THEME_DIRECTORY,
      _ ("No theme directory")
    );
  }

  co_return possible_file;
}

coro::Future<void>
ThemeManager::actually_install_theme (
  RefPtr<FirefoxProfile> profile, UniquePtr<GLib::Error> *error,
  RefPtr<Gio::Cancellable> cancellable
)
{
  set_message (_ ("Installing theme..."));
  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> internal_error;

  RefPtr<Gio::File> usercontent = theme_dir->get_child ("userContent.css");
  RefPtr<Gio::File> base = theme_dir->get_child ("base.css");
  RefPtr<Gio::File> flatpak = theme_dir->get_child ("flatpak.css");

  RefPtr<Gio::File> userchrome = nullptr;

  peel::String button_layout
    = gtk_settings->get_property (Gtk::Settings::prop_gtk_decoration_layout ());

  if (!button_layout)
    profile->set_layout (ButtonLayout::UNKNOWN);
  else if (button_layout == "close:maximize")
    profile->set_layout (ButtonLayout::ELEMENTARY);
  else if (button_layout == "maximize:close")
    profile->set_layout (ButtonLayout::ELEMENTARY_REVERSED);
  else if (button_layout == ":close")
    profile->set_layout (ButtonLayout::CLOSE_ONLY_RIGHT);
  else if (button_layout == "close:")
    profile->set_layout (ButtonLayout::CLOSE_ONLY_LEFT);
  else if (button_layout == "close,minimize:maximize")
    profile->set_layout (ButtonLayout::ADD_MINIMIZE_LEFT);
  else if (button_layout == "close:minimize,maximize")
    profile->set_layout (ButtonLayout::ADD_MINIMIZE_RIGHT);
  else if (button_layout == "close:minimize")
    profile->set_layout (ButtonLayout::REPLACE_MAXIMIZE);
  else if (button_layout == ":minimize,maximize,close")
    profile->set_layout (ButtonLayout::WINDOWS);
  else if (button_layout == "close,minimize,maximize:")
    profile->set_layout (ButtonLayout::MACOS);
  else if (button_layout == "close,maximize,minimized:")
    profile->set_layout (ButtonLayout::UBUNTU);
  else
    profile->set_layout (ButtonLayout::UNKNOWN);
  userchrome = theme_dir->get_child (profile->get_theme_name ())
                 .release_ref ()
                 ->get_child ("userChrome.css");

  if (!(usercontent->query_exists (nullptr) && base->query_exists (nullptr)
        && flatpak->query_exists (nullptr) && userchrome->query_exists (nullptr)))
  {
    critical ("%s", _ ("Couldn't find theme files, try to redownload the theme"));
    debug (_ ("Theme dir is: %s"), theme_dir->get_path ().c_str ());
    co_return;
  }

  RefPtr<Gio::File> chrome = profile->get_file ()->get_child ("chrome");
  if (!chrome->query_exists (nullptr))
  {
    chrome->make_directory_async (G_PRIORITY_LOW, nullptr, async_result.callback ());
    chrome->make_directory_finish (co_await async_result, &internal_error);
    if (internal_error)
    {
      GLib::propagate_error (error, std::move (internal_error));
      co_return;
    }
  }

  userchrome->copy (
    chrome->get_child ("userChrome.css").release_ref (), Gio::File::CopyFlags::OVERWRITE,
    cancellable, nullptr, &internal_error
  );
  usercontent->copy (
    chrome->get_child ("userContent.css").release_ref (), Gio::File::CopyFlags::OVERWRITE,
    cancellable, nullptr, &internal_error
  );
  base->copy (
    chrome->get_child ("base.css").release_ref (), Gio::File::CopyFlags::OVERWRITE, cancellable,
    nullptr, &internal_error
  );
  if (profile->get_sandboxed ())
    flatpak->copy (
      chrome->get_child ("flatpak.css").release_ref (), Gio::File::CopyFlags::OVERWRITE,
      cancellable, nullptr, &internal_error
    );

  RefPtr<Gio::File> user_js = profile->get_file ()->get_child ("user.js");
  if (!user_js->query_exists (nullptr))
  {
    user_js->create_async (
      Gio::File::CreateFlags::NONE, G_PRIORITY_LOW, cancellable, async_result.callback ()
    );
    user_js->create_finish (co_await async_result, &internal_error);
  }
  if (internal_error) [[unlikely]]
  {
    GLib::propagate_error (error, std::move (internal_error));
    co_return;
  }

  // clang-format off
  String conf = GLib::strconcat(
    "user_pref(\"toolkit.legacyUserProfileCustomizations.stylesheets\", true);\n",
    GLib::strconcat ("user_pref(\"widget.gtk.rounded-bottom-corners.enabled\", ",profile->get_rounded_corners () ? "true" : "false", ");\n"),
    GLib::strconcat ("user_pref(\"browser.tabs.drawInTitlebar\", ", profile->get_native_titlebar () || profile->get_layout () == ButtonLayout::UNKNOWN ? "false" : "true", ");\n"),
    GLib::strconcat ("user_pref(\"browser.tabs.inTitlebar\", ", profile->get_native_titlebar () || profile->get_layout () == ButtonLayout::UNKNOWN ? "0" : "1", ");\n")
  );
  // clang-format on

  ArrayRef<unsigned char> buffer{
    reinterpret_cast<unsigned char *> (const_cast<char *> (conf.c_str ())), strlen (conf)
  };
  user_js->replace_contents_async (
    buffer, nullptr, false, Gio::File::CreateFlags::REPLACE_DESTINATION, cancellable,
    async_result.callback ()
  );
  user_js->replace_contents_finish (co_await async_result, nullptr, &internal_error);

  profile->set_theme_sha (downloaded_sha);
  profile->write_config ();

  co_return;
}

coro::Future<void>
ThemeManager::actually_uninstall_theme (
  RefPtr<FirefoxProfile> profile, UniquePtr<GLib::Error> *error,
  RefPtr<Gio::Cancellable> cancellable
)
{
  set_message (_ ("Uninstalling theme..."));
  if (!profile->get_has_theme ())
    co_return;

  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> internal_error;

  RefPtr<Gio::File> chrome = profile->get_file ()->get_child ("chrome");
  chrome->trash_async (G_PRIORITY_LOW, cancellable, async_result.callback ());
  chrome->trash_finish (co_await async_result, &internal_error);
  if (internal_error) [[unlikely]]
  {
    GLib::propagate_error (error, std::move (internal_error));
    co_return;
  }

  RefPtr<Gio::File> user_js = profile->get_file ()->get_child ("user.js");
  if (user_js->query_exists (nullptr))
  {
    user_js->delete_async (G_PRIORITY_LOW, cancellable, async_result.callback ());
    user_js->delete_finish (co_await async_result, &internal_error);
  }

  profile->set_theme_sha ("");
  profile->write_config ();

  co_return;
}

coro::SimpleTask
ThemeManager::pull_repo (RefPtr<Gio::Cancellable> cancellable)
{
  if (busy)
    co_return;

  set_busy (true);

  UniquePtr<GLib::Error> error;
  String hash = co_await get_latest_hash (&error, cancellable);
  if (error)
    warning (_ ("Couldn't get theme's latest commit hash: %s"), error->message);

  theme_dir = co_await find_theme_dir (&error, cancellable);

  bool download_needed = false;

  if (hash)
    if (downloaded_sha != hash || downloaded_sha == "")
    {
      download_needed = true;
      update_available_sig.emit (this);
    }

  if (!theme_dir)
    download_needed = true;

  if (download_needed)
  {
    RefPtr<Gio::File> archive = co_await download_archive (&error, cancellable);
    if (error || !archive) [[unlikely]]
      critical ("%s", error->message);

    set_downloaded_sha (hash);

    co_await cleanup_data_dir (&error, cancellable);

    co_await extract_archive (archive, &error, cancellable);
    if (error) [[unlikely]]
      critical ("%s", error->message);

    // Try to find theme_dir one more time
    theme_dir = co_await find_theme_dir (&error, cancellable);
  }

  if (error) [[unlikely]]
    critical ("%s", error->message);

  if (!theme_dir)
  {
    set_broken (true);
    show_warning_sig.emit (
      this, _ ("Couldn't download theme"),
      _ ("Check your internet connection and permissions of your file system"), "dialog-error"
    );
  }
  else
    set_broken (false);

  set_busy (false);
}

coro::SimpleTask
ThemeManager::install_theme (RefPtr<FirefoxProfile> profile, RefPtr<Gio::Cancellable> cancellable)
{
  if (busy)
    co_return;

  set_busy (true);
  UniquePtr<GLib::Error> error;
  co_await actually_install_theme (profile, &error, cancellable);
  if (error)
  {
    warning ("%s", error->message);
    show_warning_sig.emit (
      this, _ ("Couldn't install theme"),
      _ ("Check permissions of your file system and try to redownload the theme"), "dialog-error"
    );
  }
  else
    theme_installed_sig.emit (this);

  set_busy (false);
}

coro::SimpleTask
ThemeManager::uninstall_theme (RefPtr<FirefoxProfile> profile, RefPtr<Gio::Cancellable> cancellable)
{
  if (busy)
    co_return;

  set_busy (true);
  UniquePtr<GLib::Error> error;
  co_await actually_uninstall_theme (profile, &error, cancellable);
  if (error)
  {
    warning ("%s", error->message);
    show_warning_sig.emit (
      this, _ ("Couldn't uninstall theme"), _ ("Check permissions of your file system"),
      "dialog-error"
    );
  }
  else
    theme_uninstalled_sig.emit (this);

  set_busy (false);
}
} // namespace Sf
