# gtklock (Forked)
GTK-based lockscreen for Wayland.

Design inpired by [Syndrizzle](https://github.com/Syndrizzle)'s [hotfiles](https://github.com/syndrizzle/hotfiles/tree/bspwm)

![screenshot](screenshots/lock.png)

### Building from source
```
$ meson setup builddir
$ ninja -C builddir
# meson install -C builddir
```

### Using with swayidle
```
swayidle -w timeout 60 'gtklock -d'
```

# gtklock (Original)
GTK-based lockscreen for Wayland.

![screenshot](https://user-images.githubusercontent.com/21199271/169707623-2ac5f02b-b6ed-461a-b9a3-5d96440843a2.png)
## About
gtklock is a lockscreen based on [gtkgreet](https://git.sr.ht/~kennylevinsen/gtkgreet).
It uses the ext-session-lock Wayland protocol.
Works on sway and other wlroots-based compositors.

ℹ️ __For documentation, check out the [man page](https://man.voidlinux.org/gtklock) and [wiki](https://github.com/jovanlanik/gtklock/wiki).__


## Installing gtklock from a repository
gtklock is available on these repositories:

[![Packaging status](https://repology.org/badge/vertical-allrepos/gtklock.svg)](https://repology.org/project/gtklock/versions)

- Arch: `# yay -S gtklock` (using [Yay](https://github.com/Jguer/yay))
- Gentoo: `# emerge --ask gui-apps/gtklock` (in [GURU repository](https://wiki.gentoo.org/wiki/Project:GURU))
- Void: `# xbps-install gtklock`

❤️ __Please submit an installation command for your distro!__
## Building from source
```
$ meson setup builddir
$ ninja -C builddir
# meson install -C builddir
```
### Dependencies
- Meson (build-time)
- pkg-config (build-time)
- scdoc (optional, build-time)
- PAM
- gtk+3.0
- [gtk-session-lock](https://github.com/Cu3PO42/gtk-session-lock)
### Installing build dependencies
- Arch: `# yay -S gcc meson pkgconf scdoc pam wayland gtk3 gtk-session-lock`
- Fedora: `# dnf install gcc meson pkgconf scdoc pam-devel wayland-devel gtk3-devel`, install gtk-session-lock manually
- Void: `# xbps-install gcc meson pkgconf scdoc pam-devel wayland-devel gtk+3-devel gtk-session-lock-devel`

❤️ __Please submit an dependency installation command for your distro!__
