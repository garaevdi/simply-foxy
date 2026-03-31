# Simply Foxy

![Screenshot](data/screenshot.png?raw=true)

Simple gui installer for [elementary OS Firefox Theme](https://github.com/Zonnev/elementaryos-firefox-theme) with some additional options on top. Written in C++ using [peel](https://gitlab.gnome.org/bugaevc/peel).

# Building from source

Build dependecies:

```
gtk4
libadwaita-1
libgranite-7
blueprint-compiler
libsoup-3.0
json-glib-1.0
gettext
```

Also note that it requires unzip as a runtime dependency.

Build:

```
git clone https://github.com/garaevdi/simply-foxy.git
cd simply-foxy
meson setup _build
```

Install

```
ninja -C _build install
```
