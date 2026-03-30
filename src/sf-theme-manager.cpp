#include "sf-theme-manager.hpp"

#include "config.h"

#include <string>

using namespace peel;

namespace Sf
{

Signal<ThemeManager, void ()> ThemeManager::theme_installed_sig;
Signal<ThemeManager, void ()> ThemeManager::update_available_sig;

PEEL_CLASS_IMPL (ThemeManager, "SfThemeManager", peel::GObject::Object);

inline void
ThemeManager::Class::init ()
{
}

inline void
ThemeManager::init (Class *)
{
  theme_installed_sig = Signal<ThemeManager, void ()>::create ("theme-installed");
  update_available_sig = Signal<ThemeManager, void ()>::create ("update-available");

  session = Soup::Session::create ();
  session->set_timeout (10);
  busy = false;
  data_dir
    = Gio::File::create_for_path (GLib::strconcat (GLib::get_user_data_dir (), "/", APP_NAME));
  if (!data_dir->query_exists (nullptr))
  {
    UniquePtr<GLib::Error> err;
    data_dir->make_directory (nullptr, &err);
    if (err) [[unlikely]]
    {
      critical ("Couldn't create data folder: %s", err->message);
      return;
    }
  }
  extract_dir = data_dir->get_child ("themes");
  config = Gio::Settings::create_with_path (APP_ID ".theme_manager", APP_PATH "/theme_manager/");
  config->bind ("downloaded-sha", this, "downloaded-sha", Gio::Settings::BindFlags::DEFAULT);
}

coro::Future<void>
ThemeManager::get_latest_hash ()
{
  set_message ("Checking for updates...");
  RefPtr<Soup::Message> message = Soup::Message::create (
    "GET", "https://api.github.com/repos/Zonnev/elementaryos-firefox-theme/commits?per_page=1"
  );
  RefPtr<Soup::MessageHeaders> headers = message->get_request_headers ();
  headers->append ("Accept", "application/vnd.github+json");
  headers->append ("User-Agent", APP_NAME "/" VERSION);
  headers->append ("X-GitHub-Api-Version", "2026-03-10");

  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> error;

  session->send_async (message, G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
  RefPtr<Gio::InputStream> stream = session->send_finish (co_await async_result, &error);
  if (error)
  {
    critical ("Couldn't get commits information: %s", error->message);
    co_return;
  }
  if (message->get_status () != Soup::Status::OK)
  {
    critical ("Wrong status code: %d %s", message->get_status (), message->get_reason_phrase ());
    co_return;
  }

  RefPtr<Json::Parser> pareser = Json::Parser::create ();
  pareser->load_from_stream_async (stream, nullptr, async_result.callback ());
  if (!(pareser->load_from_stream_finish (co_await async_result, &error)))
  {
    critical ("Couldn't parser recieved JSON: ", error->message);
    co_return;
  }

  RefPtr<Json::Node> root = pareser->get_root ();
  RefPtr<Json::Node> commit = root->get_array ()->get_element (0);
  String hash = commit->get_object ()->get_string_member ("sha");

  debug ("Got latest commit hash: %s", hash.c_str ());

  if (downloaded_sha != hash || !(co_await find_theme_dir ()))
  {
    if (downloaded_sha != hash || downloaded_sha == "")
      update_available_sig.emit (this);
    debug ("Downloading theme at %s", hash.c_str ());
    co_await download_archive ();
    set_downloaded_sha (hash);
    if (!(co_await find_theme_dir ()))
      critical ("%s", "Still couldn't find theme");
  }

  co_return;
}

coro::Future<void>
ThemeManager::download_archive ()
{
  set_message ("Downloading theme...");
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
  UniquePtr<GLib::Error> error;

  debug ("%s", "Sending GET request to github...");
  session->send_and_read_async (message, G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
  RefPtr<GLib::Bytes> bytes = session->send_and_read_finish (co_await async_result, &error);
  if (error)
  {
    critical ("Couldn't download archive: %s", error->message);
    co_return;
  }
  if (message->get_status () != Soup::Status::OK)
  {
    critical ("Wrong status code: %d %s", message->get_status (), message->get_reason_phrase ());
    co_return;
  }

  debug ("Read %zu bytes", bytes->get_size ());

  RefPtr<Gio::File> archive = data_dir->get_child ("elementary-firefox-theme.zip");
  if (archive->query_exists (nullptr))
  {
    debug ("%s", "Old archive already exists, trying to overwrite it...");
    archive->replace_contents_bytes_async (
      bytes, nullptr, false, Gio::File::CreateFlags::NONE, nullptr, async_result.callback ()
    );
    archive->replace_contents_finish (co_await async_result, nullptr, &error);
    if (error)
    {
      critical ("Couldn't replace old archive: %s", error->message);
      co_return;
    }
    debug ("%s", "Archive overwritten");
  }
  else
  {
    debug ("%s", "Creating new file for archive...");
    archive->create_async (
      Gio::File::CreateFlags::NONE, G_PRIORITY_DEFAULT, nullptr, async_result.callback ()
    );
    RefPtr<Gio::FileOutputStream> stream = archive->create_finish (co_await async_result, &error);
    if (error)
    {
      critical ("Couldn't create archive file: %s", error->message);
      co_return;
    }

    debug ("%s", "File created, trying to write data into it...");

    stream->write_bytes_async (bytes, G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
    stream->write_bytes_finish (co_await async_result, &error);
    if (error)
    {
      critical ("Couldn't write archive file: %s", error->message);
      co_return;
    }
    debug ("%s", "Data written, archive ready!");
  }

  extract_archive (archive);

  co_return;
}

coro::Future<void>
ThemeManager::extract_archive (RefPtr<Gio::File> archive)
{
  set_message ("Unpacking theme...");
  co_await cleanup_data_dir ();

  UniquePtr<GLib::Error> error;
  String cmd
    = GLib::strconcat ("unzip -d ", extract_dir->get_path (), " -o ", archive->get_path ());
  debug ("Unzippign archive with the following cmd: %s", cmd.c_str ());
  GLib::spawn_command_line_sync (cmd, nullptr, nullptr, nullptr, &error);
  if (error)
    critical ("Couldn't unzip archive: %s", error->message);
  else
    debug ("%s", "Unzipped archive");
}

coro::Future<void>
ThemeManager::cleanup_data_dir ()
{
  set_message ("Cleaning old files...");
  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> error;

  debug ("%s", "Starting data directory cleanup...");
  if (extract_dir->query_exists (nullptr))
  {
    extract_dir->trash_async (G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
    extract_dir->trash_finish (co_await async_result, &error);
  }
  debug ("%s", "Cleanup finished");
  co_return;
}

coro::Future<bool>
ThemeManager::find_theme_dir ()
{
  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> error;

  if (!(extract_dir->query_exists (nullptr)))
  {
    critical ("%s", "No extract dir, aborting");
    co_return false;
  }
  extract_dir->enumerate_children_async (
    G_FILE_ATTRIBUTE_STANDARD_NAME, Gio::File::QueryInfoFlags::NONE, G_PRIORITY_DEFAULT, nullptr,
    async_result.callback ()
  );
  RefPtr<Gio::FileEnumerator> enumerator
    = extract_dir->enumerate_children_finish (co_await async_result, &error);
  if (error)
  {
    critical ("Coulnd't get data childern: %s", error->message);
    co_return false;
  }
  enumerator->next_files_async (10, G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
  UniquePtr<GLib::List> files = enumerator->next_files_finish (co_await async_result, &error);
  if (error)
  {
    critical ("Couldn't get list of files to find theme folder: %s", error->message);
    co_return false;
  }
  if (files)
  {
    GLib::List::foreach (
      files,
      [this, enumerator] (void *data)
      {
        RefPtr<Gio::FileInfo> info = (Gio::FileInfo *)data;
        RefPtr<Gio::File> file = enumerator->get_child (info);
        std::string name = info->get_name ();
        if (name.find ("Zonnev-elementaryos-firefox-theme-") != std::string::npos)
          theme_dir = file;
      }
    );
  }

  enumerator->close_async (G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
  enumerator->close_finish (co_await async_result, &error);
  if (!theme_dir)
    co_return false;

  debug ("%s", "Found theme dir");
  co_return true;
}

coro::Future<void>
ThemeManager::actually_install_theme (RefPtr<FirefoxProfile> profile)
{
  set_message ("Installing theme...");
  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> error;

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
    critical ("%s", "Couldn't find theme files, try to redownload the theme");
    debug ("Theme dir is: %s", theme_dir->get_path ().c_str ());
    co_return;
  }

  RefPtr<Gio::File> chrome = profile->get_file ()->get_child ("chrome");
  if (!chrome->query_exists (nullptr))
  {
    chrome->make_directory_async (G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
    chrome->make_directory_finish (co_await async_result, &error);
    if (error)
    {
      critical ("Couldn't create \"chrome\" directory: %s", error->message);
      co_return;
    }
  }

  userchrome->copy (
    chrome->get_child ("userChrome.css").release_ref (), Gio::File::CopyFlags::OVERWRITE, nullptr,
    nullptr, &error
  );
  usercontent->copy (
    chrome->get_child ("userContent.css").release_ref (), Gio::File::CopyFlags::OVERWRITE, nullptr,
    nullptr, &error
  );
  base->copy (
    chrome->get_child ("base.css").release_ref (), Gio::File::CopyFlags::OVERWRITE, nullptr,
    nullptr, &error
  );
  if (profile->get_sandboxed ())
    flatpak->copy (
      chrome->get_child ("flatpak.css").release_ref (), Gio::File::CopyFlags::OVERWRITE, nullptr,
      nullptr, &error
    );

  RefPtr<Gio::File> user_js = profile->get_file ()->get_child ("user.js");
  if (!user_js->query_exists (nullptr))
  {
    user_js->create_async (
      Gio::File::CreateFlags::NONE, G_PRIORITY_DEFAULT, nullptr, async_result.callback ()
    );
    user_js->create_finish (co_await async_result, &error);
  }
  if (error) [[unlikely]]
  {
    critical ("Couldn't create userjs file: %s", error->message);
    co_return;
  }

  // clang-format off
  String conf = GLib::strconcat(
    "user_pref(\"toolkit.legacyUserProfileCustomizations.stylesheets\", true);\n",
    GLib::strconcat ("user_pref(\"widget.gtk.rounded-bottom-corners.enabled\", ",profile->get_rounded_corners () ? "true" : "false", ");\n")
  );
  // clang-format on

  ArrayRef<unsigned char> buffer{
    reinterpret_cast<unsigned char *> (const_cast<char *> (conf.c_str ())), strlen (conf)
  };
  user_js->replace_contents_async (
    buffer, nullptr, false, Gio::File::CreateFlags::REPLACE_DESTINATION, nullptr,
    async_result.callback ()
  );
  user_js->replace_contents_finish (co_await async_result, nullptr, &error);
  if (error) [[unlikely]]
  {
    critical ("Couldn't write to userjs file: %s", error->message);
  }

  profile->set_theme_sha (downloaded_sha);
  profile->write_config ();

  co_return;
}

coro::Future<void>
ThemeManager::actually_uninstall_theme (RefPtr<FirefoxProfile> profile)
{
  set_message ("Uninstalling theme...");
  if (!profile->get_has_theme ())
    co_return;

  coro::AsyncResult async_result;
  UniquePtr<GLib::Error> error;
  RefPtr<Gio::File> chrome = profile->get_file ()->get_child ("chrome");
  chrome->trash_async (G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
  chrome->trash_finish (co_await async_result, &error);
  if (error)
  {
    critical ("Couldn't delete chrome folder: %s", error->message);
    co_return;
  }

  RefPtr<Gio::File> user_js = profile->get_file ()->get_child ("user.js");
  if (user_js->query_exists (nullptr))
  {
    user_js->delete_async (G_PRIORITY_DEFAULT, nullptr, async_result.callback ());
    user_js->delete_finish (co_await async_result, &error);
    if (error)
      critical ("Couldn't delete user.js file: %s", error->message);
  }

  profile->set_theme_sha ("");
  profile->write_config ();

  co_return;
}

coro::SimpleTask
ThemeManager::pull_repo ()
{
  if (busy)
    co_return;

  set_busy (true);
  co_await get_latest_hash ();
  set_busy (false);
}

coro::SimpleTask
ThemeManager::install_theme (RefPtr<FirefoxProfile> profile)
{
  if (busy)
    co_return;

  set_busy (true);
  co_await actually_install_theme (profile);
  set_busy (false);
}

coro::SimpleTask
ThemeManager::uninstall_theme (RefPtr<FirefoxProfile> profile)
{
  if (busy)
    co_return;

  set_busy (true);
  co_await actually_uninstall_theme (profile);
  set_busy (false);
}
} // namespace Sf
