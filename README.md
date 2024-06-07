# phonon-native
Phonon(KDE) Backend using only Qt and its Multimedia components.

## Definition
This experimental backend is written in C++20, using only components from Phonon and Qt.
Its recommended to use the (default) FFmpeg backend of QtMultimedia for best experience.
Feature support is limited by QtMultimedia. For playing advanced formats (like DVD, Streams, old/exotic formats) use a different backend.
The main purpose for this backend was the tighter integration with Qt and removing the requirement for an external player like vlc or mpv.

### Missing Phonon Features
- DVD and DVD Menu playback
- Media Streams
- Audio/Video Tracks
- Subtitles
- Chapters
- User-defined Aspect Ratios
- Audio/Video Filters

## Requirements
- cmake >= 3.28
- Phonon >= 4.12
- Qt6

## Build and Install
Run this commands as root (or with sudo):

```
  # mkdir build
  # cd build
  # cmake -DCMAKE_INSTALL_PREFIX=/usr <source directory>
  # make
  # make install
```
