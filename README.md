# Simply Foxy

![Screenshot](data/screenshot.png?raw=true)

Simple gui installer for [elementary OS Firefox Theme](https://github.com/Zonnev/elementaryos-firefox-theme) with some additional options on top. Written in C++ using [peel](https://gitlab.gnome.org/bugaevc/peel).

# Building from source

## Flatpak

The only way to build this on elementary OS is to use `flatpak-builder`. Also you would need to install daily versions of `io.elementary.Platform` and `io.elementary.Sdk`. Then simply run the following command in the repository directory
```
flatpak-builder --force-clean --install --user .flatpak io.github.garaevdi.simplyfoxy.yml
```
